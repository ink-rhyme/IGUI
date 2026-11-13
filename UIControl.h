// ============================================================
// UIControl.h - 万能OpenGL UI控件（独立文件）
// 设计要点：
//   1. 控件可任意嵌套（parent为weak_ptr，childs为shared_ptr，避免循环引用）
//   2. 内存全部交给UIManager统一创建/销毁，本类不裸new/delete
//   3. 自带UIRenderContext：封装项目里的drawQuad/drawQuadGrad/drawOutline
//   4. 自定义背景色/字体颜色/边框/文字/布局，外加KV动态自定义属性
//   5. 基础属性：name、parent、childs、type、id
// ============================================================
#pragma once
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <string>
#include <vector>
#include <unordered_map>
#include <variant>
#include <memory>
#include <cstdint>
#include "TextRenderer.h"

// ---------------- 颜色（RGBA，0~1） ----------------
struct UIColor {
    float r = 0.2f, g = 0.2f, b = 0.25f, a = 1.0f;
    UIColor() = default;
    UIColor(float rr, float gg, float bb, float aa = 1.0f)
        : r(rr), g(gg), b(bb), a(aa) {}
};

// ---------------- 自定义属性变体（KV动态扩展） ----------------
using UIProp = std::variant<int, float, bool, std::string, UIColor>;

// ---------------- 渲染上下文 ----------------
// 封装shader/VAO/VBO/正交投影/文本渲染器，
// 控件绘制统一从这里走，保证和项目现有磨砂玻璃风格一致。
class UIRenderContext {
public:
    UIRenderContext() = default;
    ~UIRenderContext();

    bool init();                 // 编译shader、建VAO/VBO（需当前线程有GL上下文）
    void shutdown();
    void resize(int w, int h);   // 窗口尺寸变化时调用
    void setTextRenderer(TextRenderer* t) { text = t; }

    // 基础图元（坐标为左上角原点的像素坐标，与项目一致）
    void fillRect(float x, float y, float w, float h, const UIColor& c);
    void fillRectGrad(float x, float y, float w, float h,
                      const UIColor& top, const UIColor& bot);
    void drawOutline(float x, float y, float w, float h, float t, const UIColor& c);
    void drawText(const std::string& s, float x, float y, int px, const UIColor& c);
    void drawTextCentered(const std::string& s, float cx, float cy, int px, const UIColor& c);

    GLFWwindow*   window = nullptr;
    TextRenderer* text   = nullptr;
    int winW = 0, winH = 0;

private:
    void applyOrtho();

    GLuint prog    = 0;
    GLuint vao     = 0;
    GLuint vbo     = 0;
    GLint  projLoc = -1;
};

// ============================================================
// UIControl - 万能控件基类
// ============================================================
class UIControl : public std::enable_shared_from_this<UIControl> {
public:
    using Ptr = std::shared_ptr<UIControl>;

    UIControl();
    virtual ~UIControl();

    // ===== 基础属性（需求固定项）=====
    std::string name;            // 控件名（UIManager按名查找的key）
    std::string type;            // 控件类型：panel / button / label ...
    uint64_t    id = 0;          // 全局唯一id（UIManager分配）
    std::weak_ptr<UIControl> parent;          // 父控件（弱引用，打破循环引用）
    std::vector<Ptr>         childs;          // 子控件（可空）

    // ===== 布局属性 =====
    float x = 0.0f, y = 0.0f;       // 相对父控件的左上角坐标
    float width = 100.0f, height = 40.0f;
    bool visible = true;             // false时自身及子树都不绘制
    bool enabled = true;             // false时不响应交互
    bool clip    = false;            // 是否裁剪子控件（scissor）

    // ===== 样式属性（自定义背景/字体颜色）=====
    UIColor bgTop;                   // 背景顶色（磨砂渐变）
    UIColor bgBot;                   // 背景底色
    UIColor fontColor{1.0f, 1.0f, 1.0f, 1.0f};
    UIColor borderColor{0.0f, 0.0f, 0.0f, 0.0f};
    float   borderWidth = 0.0f;      // <=0不画边框
    int     fontSize    = 14;
    std::string text;                // 控件文字

    // ===== 动态自定义KV属性 =====
    void setProp(const std::string& key, const UIProp& v);
    bool getProp(const std::string& key, UIProp& out) const;
    template <typename T>
    bool getPropT(const std::string& key, T& out) const;  // 类型安全取值

    // ===== 父子关系操作 =====
    void addChild(Ptr c);
    void removeChild(Ptr c);
    void setParent(Ptr p);

    // ===== 命中测试 / 事件 =====
    bool contains(float mx, float my) const;   // 屏幕坐标是否落在本控件矩形内
    virtual void onClick() {}                  // 子类重写：点击回调

    // ===== 绘制入口（外部调draw，内部递归画子控件）=====
    void draw(UIRenderContext& ctx);

protected:
    // 默认绘制：背景渐变 + 边框 + 居中文字。子类可重写做自定义外观。
    virtual void onDraw(UIRenderContext& ctx);

private:
    std::unordered_map<std::string, UIProp> props_;
};

// ---- 模板实现 ----
template <typename T>
bool UIControl::getPropT(const std::string& key, T& out) const {
    UIProp v;
    if (!getProp(key, v)) return false;
    if (const T* p = std::get_if<T>(&v)) { out = *p; return true; }
    return false;
}

