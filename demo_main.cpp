// ============================================================
// demo_main.cpp - UI框架独立测试Demo入口
// 注意：这是单独的测试程序，不会和你原有项目的main.cpp冲突
// ============================================================
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "UIControl.h"
#include "UIManager.h"
#include "UIPageManager.h"
#include "TextRenderer.h"

int main()
{
    // 初始化GLFW
    if (!glfwInit()) return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1280, 720, "UI Framework Demo", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // 初始化GLEW
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        glfwTerminate();
        return -1;
    }

    // 初始化文本渲染器（示例用，可替换为你项目的TextRenderer）
    TextRenderer text;
    // text.init("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 16);

    // 初始化UI渲染上下文
    UIRenderContext ctx;
    ctx.init();
    ctx.resize(1280, 720);
    ctx.window = window;
    ctx.text = &text;

    // 获取全局管理器
    auto& uiMgr = UIManager::Instance();
    auto& pageMgr = UIPageManager::Instance();

    // ===== 创建示例控件树 =====
    // 根面板
    auto rootPanel = uiMgr.createControl("panel", "root_panel");
    rootPanel->x = 20;
    rootPanel->y = 20;
    rootPanel->width = 600;
    rootPanel->height = 500;
    rootPanel->bgTop = UIColor(0.15f, 0.15f, 0.2f, 1.0f);
    rootPanel->bgBot = UIColor(0.08f, 0.08f, 0.12f, 1.0f);

    // 子按钮
    auto btn1 = uiMgr.createControl("button", "btn_confirm", rootPanel);
    btn1->x = 30;
    btn1->y = 30;
    btn1->width = 150;
    btn1->height = 45;
    btn1->text = "确认按钮";
    btn1->bgTop = UIColor(0.3f, 0.4f, 0.7f, 1.0f);
    btn1->bgBot = UIColor(0.2f, 0.3f, 0.6f, 1.0f);

    auto btn2 = uiMgr.createControl("button", "btn_cancel", rootPanel);
    btn2->x = 200;
    btn2->y = 30;
    btn2->width = 150;
    btn2->height = 45;
    btn2->text = "取消按钮";
    btn2->bgTop = UIColor(0.5f, 0.2f, 0.2f, 1.0f);
    btn2->bgBot = UIColor(0.4f, 0.15f, 0.15f, 1.0f);

    // 测试自定义属性
    btn1->setProp("hover_color", UIColor(0.4f, 0.5f, 0.8f, 1.0f));
    btn1->setProp("btn_id", 1001);

    // ===== 注册页面 =====
    pageMgr.registerPage("main_page");
    pageMgr.addControlToPage("main_page", rootPanel->id);
    pageMgr.switchToPage("main_page");

    // ===== 主渲染循环 =====
    while (!glfwWindowShouldClose(window)) {
        glClearColor(0.05f, 0.05f, 0.08f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // 绘制当前页面所有控件
        pageMgr.drawCurrentPage(ctx);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // 释放资源
    uiMgr.destroyAll();
    ctx.shutdown();
    glfwTerminate();
    return 0;
}

