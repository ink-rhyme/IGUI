// ============================================================
// UIPageManager.cpp - 页面管理器实现
// 职责：保存当前展示页面中所有控件的索引
// ============================================================
#include "UIPageManager.h"

UIPageManager& UIPageManager::Instance() {
    static UIPageManager ins;
    return ins;
}

void UIPageManager::registerPage(const std::string& pageName) {
    if (pages_.find(pageName) == pages_.end()) {
        UIPage page;
        page.pageName = pageName;
        pages_[pageName] = page;
    }
}

void UIPageManager::addControlToPage(const std::string& pageName, uint64_t controlId) {
    auto it = pages_.find(pageName);
    if (it == pages_.end()) return;

    // 去重添加
    for (auto id : it->second.controlIds) {
        if (id == controlId) return;
    }
    it->second.controlIds.push_back(controlId);
}

void UIPageManager::addControlToPage(const std::string& pageName, const std::string& controlName) {
    auto ctrl = UIManager::Instance().getControlByName(controlName);
    if (ctrl) {
        addControlToPage(pageName, ctrl->id);
    }
}

void UIPageManager::removeControlFromPage(const std::string& pageName, uint64_t controlId) {
    auto it = pages_.find(pageName);
    if (it == pages_.end()) return;

    auto& ids = it->second.controlIds;
    for (auto vit = ids.begin(); vit != ids.end(); ++vit) {
        if (*vit == controlId) {
            ids.erase(vit);
            break;
        }
    }
}

void UIPageManager::switchToPage(const std::string& pageName) {
    auto it = pages_.find(pageName);
    if (it == pages_.end()) return;

    currentPage_ = pageName;
    currentPageIds_ = it->second.controlIds;
}

void UIPageManager::drawCurrentPage(UIRenderContext& ctx) {
    auto& mgr = UIManager::Instance();
    for (auto id : currentPageIds_) {
        auto ctrl = mgr.getControlById(id);
        if (ctrl && ctrl->visible) {
            ctrl->draw(ctx);
        }
    }
}

std::vector<UIControl::Ptr> UIPageManager::getCurrentPageControls() {
    std::vector<UIControl::Ptr> result;
    auto& mgr = UIManager::Instance();
    for (auto id : currentPageIds_) {
        auto ctrl = mgr.getControlById(id);
        if (ctrl) {
            result.push_back(ctrl);
        }
    }
    return result;
}

