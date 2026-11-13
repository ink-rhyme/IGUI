// ============================================================
// UIManager.cpp - 控件管理器实现（绝对单例）
// 职责：只负责控件的产生和销毁
// ============================================================
#include "UIManager.h"
#include <functional>

UIManager& UIManager::Instance() {
    static UIManager ins;
    return ins;
}

UIManager::UIManager() {
}

UIManager::~UIManager() {
    destroyAll();
}

uint64_t UIManager::nextId() {
    return idCounter_++;
}

UIControl::Ptr UIManager::createControl(const std::string& type,
                                        const std::string& name,
                                        UIControl::Ptr parent) {
    UI_LOCK_GUARD();

    // 重名检查：如果name已存在，直接返回已有的
    auto it = nameMap_.find(name);
    if (it != nameMap_.end()) {
        return it->second;
    }

    auto ctrl = std::make_shared<UIControl>();
    ctrl->type = type;
    ctrl->name = name;
    ctrl->id = nextId();

    // parent非空则自动加入父节点childs
    if (parent) {
        ctrl->setParent(parent);
        parent->addChild(ctrl);
    }

    // 双索引存储
    nameMap_[name] = ctrl;
    idMap_[ctrl->id] = ctrl;

    return ctrl;
}

UIControl::Ptr UIManager::getControlByName(const std::string& name,
                                           bool createIfMissing,
                                           UIControl::Ptr parent) {
    UI_LOCK_GUARD();

    auto it = nameMap_.find(name);
    if (it != nameMap_.end()) {
        return it->second;
    }

    if (createIfMissing) {
        // 找不到，自动创建一个base类型控件
        auto ctrl = std::make_shared<UIControl>();
        ctrl->type = "base";
        ctrl->name = name;
        ctrl->id = nextId();

        if (parent) {
            ctrl->setParent(parent);
            parent->addChild(ctrl);
        }

        nameMap_[name] = ctrl;
        idMap_[ctrl->id] = ctrl;
        return ctrl;
    }

    return nullptr;
}

UIControl::Ptr UIManager::getControlById(uint64_t id) {
    UI_LOCK_GUARD();

    auto it = idMap_.find(id);
    if (it != idMap_.end()) {
        return it->second;
    }
    return nullptr;
}

void UIManager::destroyControl(UIControl::Ptr ctrl) {
    UI_LOCK_GUARD();

    if (!ctrl) return;

    // 从索引中移除（递归移除所有子控件）
    std::function<void(UIControl::Ptr)> removeRecursive = [&](UIControl::Ptr c) {
        nameMap_.erase(c->name);
        idMap_.erase(c->id);
        for (auto& child : c->childs) {
            removeRecursive(child);
        }
    };
    removeRecursive(ctrl);

    // 从父控件中移除
    auto parent = ctrl->parent.lock();
    if (parent) {
        parent->removeChild(ctrl);
    }

    // shared_ptr自动析构，不需要手动delete
}

void UIManager::destroyControlByName(const std::string& name) {
    auto ctrl = getControlByName(name);
    if (ctrl) {
        destroyControl(ctrl);
    }
}

void UIManager::destroyAll() {
    UI_LOCK_GUARD();

    // 清空所有索引，shared_ptr自动释放所有控件
    nameMap_.clear();
    idMap_.clear();
    idCounter_ = 1;
}

