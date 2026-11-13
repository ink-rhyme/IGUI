// ============================================================
// UIPageManager.h - 页面管理器（独立文件）
// 职责：
//   1. 保存当前展示页面中所有控件的索引（id/name列表）
//   2. 只存索引，不持有控件本身（控件内存由UIManager管理）
//   3. 支持页面切换：清空当前页索引，加载新页面的控件列表
//   4. 负责当前页面所有控件的绘制分发
// ============================================================
#pragma once
#include "UIControl.h"
#include "UIManager.h"

#include <vector>
#include <string>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <cstdint>

// 页面定义：一个页面就是一组控件索引的集合
struct UIPage {
    std::string pageName;
    std::vector<uint64_t> controlIds;   // 本页面包含的控件id列表（索引）
};

class UIPageManager {
public:
    static UIPageManager& Instance();

    // 禁用拷贝
    UIPageManager(const UIPageManager&) = delete;
    UIPageManager& operator=(const UIPageManager&) = delete;

    // ===== 页面管理 =====
    // 注册一个新页面
    void registerPage(const std::string& pageName);

    // 给指定页面添加控件索引
    void addControlToPage(const std::string& pageName, uint64_t controlId);
    void addControlToPage(const std::string& pageName, const std::string& controlName);

    // 从指定页面移除控件索引
    void removeControlFromPage(const std::string& pageName, uint64_t controlId);

    // ===== 页面切换 =====
    // 切换到指定页面：清空当前页索引，加载目标页的控件列表
    void switchToPage(const std::string& pageName);

    // 获取当前页面名
    const std::string& getCurrentPageName() const { return currentPage_; }

    // ===== 当前页操作 =====
    // 绘制当前页面所有控件
    void drawCurrentPage(UIRenderContext& ctx);

    // 获取当前页面所有控件指针（从UIManager取）
    std::vector<UIControl::Ptr> getCurrentPageControls();

    // 当前页面控件数量
    size_t getCurrentPageControlCount() const { return currentPageIds_.size(); }

private:
    UIPageManager() = default;
    ~UIPageManager() = default;

    // 所有已注册的页面
    std::unordered_map<std::string, UIPage> pages_;

    // 当前页面名
    std::string currentPage_;

    // 当前页面的控件id索引列表（缓存，避免每次都去pages_里找）
    std::vector<uint64_t> currentPageIds_;
};

// 方便使用的全局访问宏
#define UI_PAGE UIPageManager::Instance()

