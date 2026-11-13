// ============================================================
// TextRenderer.cpp - OpenGL FreeType文字渲染实现
// ============================================================
#include "TextRenderer.h"
#include <ft2build.h>
#include FT_FREETYPE_H

#include <iostream>
#include <filesystem>
#include <vector>

namespace fs = std::filesystem;

static const char* kTextVS = R"(
#version 330 core
layout(location = 0) in vec4 vertex;
out vec2 texCoord;
uniform mat4 proj;
void main() {
    gl_Position = proj * vec4(vertex.xy, 0.0, 1.0);
    texCoord = vertex.zw;
}
)";

static const char* kTextFS = R"(
#version 330 core
in vec2 texCoord;
out vec4 fragColor;
uniform sampler2D text;
uniform vec3 textColor;
void main() {
    float alpha = texture(text, texCoord).r;
    fragColor = vec4(textColor, alpha);
}
)";

static GLuint mkShader(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok = 0; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024]; glGetShaderInfoLog(s, sizeof(log), nullptr, log);
        std::cerr << "TextRenderer shader error: " << log << std::endl;
    }
    return s;
}

TextRenderer::TextRenderer() {}

TextRenderer::~TextRenderer() {
    shutdown();
}

bool TextRenderer::init(const std::string& fontPath, int defaultSize) {
    if (loaded_) return true;

    // 编译着色器
    GLuint vs = mkShader(GL_VERTEX_SHADER, kTextVS);
    GLuint fs = mkShader(GL_FRAGMENT_SHADER, kTextFS);
    shaderProg_ = glCreateProgram();
    glAttachShader(shaderProg_, vs);
    glAttachShader(shaderProg_, fs);
    glLinkProgram(shaderProg_);
    glDeleteShader(vs);
    glDeleteShader(fs);

    projLoc_ = glGetUniformLocation(shaderProg_, "proj");
    textColorLoc_ = glGetUniformLocation(shaderProg_, "textColor");

    // VBO设置
    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // FreeType初始化
    FT_Library ft;
    if (FT_Init_FreeType(&ft)) {
        std::cerr << "FreeType init failed" << std::endl;
        return false;
    }

    // 自动找系统默认字体
    std::string path = fontPath;
    if (path.empty()) {
        std::vector<std::string> candidates = {
            "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
            "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
            "C:/Windows/Fonts/arial.ttf"
        };
        for (auto& c : candidates) {
            if (fs::exists(c)) {
                path = c;
                break;
            }
        }
    }

    FT_Face face;
    if (FT_New_Face(ft, path.c_str(), 0, &face)) {
        std::cerr << "Failed to load font: " << path << std::endl;
        FT_Done_FreeType(ft);
        return false;
    }

    FT_Set_Pixel_Sizes(face, 0, defaultSize);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    // 预加载默认字号的ASCII字符
    for (unsigned char c = 0; c < 128; c++) {
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) continue;

        Character ch;
        glGenTextures(1, &ch.textureID);
        glBindTexture(GL_TEXTURE_2D, ch.textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED,
                     face->glyph->bitmap.width,
                     face->glyph->bitmap.rows,
                     0, GL_RED, GL_UNSIGNED_BYTE,
                     face->glyph->bitmap.buffer);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        ch.sizeX = face->glyph->bitmap.width;
        ch.sizeY = face->glyph->bitmap.rows;
        ch.bearingX = face->glyph->bitmap_left;
        ch.bearingY = face->glyph->bitmap_top;
        ch.advance = face->glyph->advance.x >> 6;

        glyphs_[defaultSize][c] = ch;
    }

    FT_Done_Face(face);
    FT_Done_FreeType(ft);

    loaded_ = true;
    return true;
}

void TextRenderer::shutdown() {
    for (auto& [size, chars] : glyphs_) {
        for (auto& [c, ch] : chars) {
            glDeleteTextures(1, &ch.textureID);
        }
    }
    glyphs_.clear();
    if (vbo_) glDeleteBuffers(1, &vbo_);
    if (vao_) glDeleteVertexArrays(1, &vao_);
    if (shaderProg_) glDeleteProgram(shaderProg_);
    loaded_ = false;
}

void TextRenderer::setOrtho(float width, float height) {
    float m[16] = {0};
    m[0] = 2.0f / width;
    m[5] = -2.0f / height;
    m[10] = -1.0f;
    m[12] = -1.0f;
    m[13] = 1.0f;
    m[15] = 1.0f;
    glUseProgram(shaderProg_);
    glUniformMatrix4fv(projLoc_, 1, GL_FALSE, m);
}

void TextRenderer::draw(const std::string& text, float x, float y, int pixelSize,
                        float r, float g, float b, float a) {
    if (!loaded_) return;

    // 检查是否缓存了这个字号，没有就动态加载（简化版：直接用默认字号缩放）
    if (glyphs_.find(pixelSize) == glyphs_.end()) {
        // 简化处理：直接用已有字号，不动态加载新字号
        pixelSize = glyphs_.begin()->first;
    }

    glUseProgram(shaderProg_);
    glUniform3f(textColorLoc_, r, g, b);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(vao_);

    float scale = (float)pixelSize / 16.0f; // 按16号为基准缩放
    float curX = x;
    float curY = y;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    for (char c : text) {
        auto& ch = glyphs_[pixelSize][c];
        if (!ch.textureID) continue;

        float xpos = curX + ch.bearingX * scale;
        float ypos = curY - (ch.sizeY - ch.bearingY) * scale;
        float w = ch.sizeX * scale;
        float h = ch.sizeY * scale;

        float vertices[6][4] = {
            {xpos,     ypos + h, 0.0f, 0.0f},
            {xpos,     ypos,     0.0f, 1.0f},
            {xpos + w, ypos,     1.0f, 1.0f},
            {xpos,     ypos + h, 0.0f, 0.0f},
            {xpos + w, ypos,     1.0f, 1.0f},
            {xpos + w, ypos + h, 1.0f, 0.0f}
        };

        glBindTexture(GL_TEXTURE_2D, ch.textureID);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        curX += ch.advance * scale;
    }

    glBindVertexArray(0);
    glDisable(GL_BLEND);
}

float TextRenderer::measure(const std::string& text, int pixelSize) {
    if (!loaded_) return 0;
    if (glyphs_.find(pixelSize) == glyphs_.end()) {
        pixelSize = glyphs_.begin()->first;
    }
    float scale = (float)pixelSize / 16.0f;
    float width = 0;
    for (char c : text) {
        width += glyphs_[pixelSize][c].advance * scale;
    }
    return width;
}

