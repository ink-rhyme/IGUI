// ============================================================
// UIControl.cpp - 万能OpenGL UI控件实现
// ============================================================
#include "UIControl.h"
#include "TextRenderer.h"

#include <cstring>
#include <cstdio>

// ---------------- 着色器（与main.cpp风格一致，磨砂玻璃噪点） ----------------
static const char* kQuadVS = R"(#version 330 core
layout(location = 0) in vec2 pos;
layout(location = 1) in vec4 color;
uniform mat4 proj;
out vec4 vColor;
void main() { gl_Position = proj * vec4(pos, 0.0, 1.0); vColor = color; }
)";

static const char* kQuadFS = R"(#version 330 core
in vec4 vColor;
out vec4 fragColor;
float hash(vec2 p) { return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453); }
void main() {
    float n = hash(gl_FragCoord.xy * 0.7) * 0.065 - 0.0325;
    vec4 c = vColor;
    c.rgb += n * min(c.a * 1.6, 1.0);
    fragColor = c;
}
)";

static void ortho(float w, float h, float m[16]) {
    std::memset(m, 0, 16 * sizeof(float));
    m[0] = 2.0f / w; m[5] = -2.0f / h; m[10] = -1.0f;
    m[12] = -1.0f; m[13] = 1.0f; m[15] = 1.0f;
}

static GLuint mkShader(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok = 0; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024]; glGetShaderInfoLog(s, sizeof(log), nullptr, log);
        fprintf(stderr, "UIControl shader error: %s\n", log);
    }
    return s;
}

// ============================================================
// UIRenderContext 实现
// ============================================================
UIRenderContext::~UIRenderContext() {
    shutdown();
}

bool UIRenderContext::init() {
    if (prog) return true;  // 已初始化

    GLuint vs = mkShader(GL_VERTEX_SHADER, kQuadVS);
    GLuint fs = mkShader(GL_FRAGMENT_SHADER, kQuadFS);
    prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    glDeleteShader(vs);
    glDeleteShader(fs);

    projLoc = glGetUniformLocation(prog, "proj");

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 6, nullptr, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(2 * sizeof(float)));

    return prog != 0 && vao != 0 && vbo != 0;
}

void UIRenderContext::shutdown() {
    if (vbo)  { glDeleteBuffers(1, &vbo); vbo = 0; }
    if (vao)  { glDeleteVertexArrays(1, &vao); vao = 0; }
    if (prog) { glDeleteProgram(prog); prog = 0; }
}

void UIRenderContext::resize(int w, int h) {
    winW = w;
    winH = h;
}

void UIRenderContext::applyOrtho() {
    if (projLoc < 0) projLoc = glGetUniformLocation(prog, "proj");
    float m[16];
    ortho((float)winW, (float)winH, m);
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, m);
}

void UIRenderContext::fillRect(float x, float y, float w, float h, const UIColor& c) {
    float buf[6][6] = {
        {x,     y,     c.r, c.g, c.b, c.a},
        {x + w, y,     c.r, c.g, c.b, c.a},
        {x + w, y + h, c.r, c.g, c.b, c.a},
        {x,     y,     c.r, c.g, c.b, c.a},
        {x + w, y + h, c.r, c.g, c.b, c.a},
        {x,     y + h, c.r, c.g, c.b, c.a},
    };
    glUseProgram(prog);
    applyOrtho();
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(buf), buf);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void UIRenderContext::fillRectGrad(float x, float y, float w, float h,
                                   const UIColor& top, const UIColor& bot) {
    float buf[6][6] = {
        {x,     y,     top.r, top.g, top.b, top.a},
        {x + w, y,     top.r, top.g, top.b, top.a},
        {x + w, y + h, bot.r, bot.g, bot.b, bot.a},
        {x,     y,     top.r, top.g, top.b, top.a},
        {x + w, y + h, bot.r, bot.g, bot.b, bot.a},
        {x,     y + h, bot.r, bot.g, bot.b, bot.a},
    };
    glUseProgram(prog);
    applyOrtho();
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(buf), buf);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void UIRenderContext::drawOutline(float x, float y, float w, float h, float t, const UIColor& c) {
    fillRect(x, y, w, t, c);
    fillRect(x, y + h - t, w, t, c);
    fillRect(x, y, t, h, c);
    fillRect(x + w - t, y, t, h, c);
}

void UIRenderContext::drawText(const std::string& s, float x, float y, int px, const UIColor& c) {
    if (text) text->draw(s, x, y, px, c.r, c.g, c.b, c.a);
}

void UIRenderContext::drawTextCentered(const std::string& s, float cx, float cy, int px, const UIColor& c) {
    if (!text || s.empty()) return;
    float tw = text->measure(s, px);
    drawText(s, cx - tw * 0.5f, cy - px * 0.5f, px, c);
}

// ============================================================
// UIControl 实现
// ============================================================
UIControl::UIControl()
    : id(0),
      bgTop(0.155f, 0.185f, 0.260f, 0.92f),
      bgBot(0.110f, 0.135f, 0.195f, 0.90f) {
}

UIControl::~UIControl() {
    // shared_ptr自动释放子节点，不需要手动delete
    childs.clear();
    props_.clear();
}

void UIControl::setProp(const std::string& key, const UIProp& v) {
    props_[key] = v;
}

bool UIControl::getProp(const std::string& key, UIProp& out) const {
    auto it = props_.find(key);
    if (it != props_.end()) {
        out = it->second;
        return true;
    }
    return false;
}

void UIControl::addChild(Ptr c) {
    if (!c) return;
    c->setParent(shared_from_this());
    childs.push_back(c);
}

void UIControl::removeChild(Ptr c) {
    if (!c) return;
    for (auto it = childs.begin(); it != childs.end(); ++it) {
        if (*it == c) {
            c->parent.reset();
            childs.erase(it);
            break;
        }
    }
}

void UIControl::setParent(Ptr p) {
    auto oldParent = parent.lock();
    if (oldParent && oldParent != p) {
        oldParent->removeChild(shared_from_this());
    }
    parent = p;
}

bool UIControl::contains(float mx, float my) const {
    // 计算屏幕坐标（递归累加父控件偏移）
    float sx = x, sy = y;
    auto p = parent.lock();
    while (p) {
        sx += p->x;
        sy += p->y;
        p = p->parent.lock();
    }
    return mx >= sx && mx <= sx + width &&
           my >= sy && my <= sy + height;
}

void UIControl::draw(UIRenderContext& ctx) {
    if (!visible) return;

    // 计算当前控件的屏幕坐标（递归累加父控件偏移）
    float sx = x, sy = y;
    auto p = parent.lock();
    while (p) {
        sx += p->x;
        sy += p->y;
        p = p->parent.lock();
    }

    // 临时修改ctx坐标，绘制自身
    // 这里直接用屏幕坐标绘制，子控件递归时会自己算偏移
    onDraw(ctx);

    // 递归绘制所有子控件
    for (auto& c : childs) {
        if (c) c->draw(ctx);
    }
}

void UIControl::onDraw(UIRenderContext& ctx) {
    // 计算屏幕坐标
    float sx = x, sy = y;
    auto p = parent.lock();
    while (p) {
        sx += p->x;
        sy += p->y;
        p = p->parent.lock();
    }

    // 绘制背景渐变
    ctx.fillRectGrad(sx, sy, width, height, bgTop, bgBot);

    // 绘制边框
    if (borderWidth > 0) {
        ctx.drawOutline(sx, sy, width, height, borderWidth, borderColor);
    }

    // 绘制文字（居中）
    if (!text.empty()) {
        ctx.drawTextCentered(text, sx + width * 0.5f, sy + height * 0.5f, fontSize, fontColor);
    }
}

