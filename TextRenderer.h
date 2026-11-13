// ============================================================
// TextRenderer.h - OpenGL FreeType文字渲染器
// ============================================================
#pragma once

#include <string>
#include <map>
#include <GL/glew.h>

struct Character {
    GLuint textureID;   // 字形纹理ID
    GLuint sizeX;       // 字形宽
    GLuint sizeY;       // 字形高
    GLuint bearingX;    // 字形左边距
    GLuint bearingY;    // 字形上边距
    GLuint advance;     // 到下一个字符的距离
};

class TextRenderer {
public:
    TextRenderer();
    ~TextRenderer();

    // 初始化，加载字体文件，fontPath为字体路径，默认用系统DejaVu
    bool init(const std::string& fontPath = "", int defaultSize = 16);
    void shutdown();

    // 设置正交投影矩阵
    void setOrtho(float width, float height);

    // 绘制文字，x/y为左下角坐标
    void draw(const std::string& text, float x, float y, int pixelSize,
              float r = 1.0f, float g = 1.0f, float b = 1.0f, float a = 1.0f);

    // 测量文字宽度
    float measure(const std::string& text, int pixelSize);

private:
    bool loaded_ = false;
    GLuint shaderProg_ = 0;
    GLuint vao_ = 0;
    GLuint vbo_ = 0;
    GLint projLoc_ = 0;
    GLint textColorLoc_ = 0;

    std::map<int, std::map<char, Character>> glyphs_; // 按字号缓存字形

    void loadGlyphSize(int pixelSize);
};

