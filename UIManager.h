// ============================================================
// UIManager.h - 控件管理器（绝对单例，独立文件）
// 职责：
//   1. 只负责控件的产生和销毁，不负责绘制、不负责页面管理
//   2. 所有控件都由本管理器统一存储（按name/id双索引）
//   3. 绝对单例：全局只有一个实例，禁止拷贝/赋值/移动
//   4. 多线程安全由编译宏 UI_THREAD_SAFE 控制，默认关闭
// ============================================================
#pragma once
#include "UIControl.h"

#include <unordered_map>
#include <memory>
#include <mutex>
#include <string>
#include <cstdint>

// 编译时宏：开启则多线程安全（编译命令加 -DUI_THREAD_SAFE）
// 未定义时不支持多线程，不加锁，性能更高
#ifdef UI_THREAD_SAFE
#define UI_LOCK_GUARD() std::lock_guard<std::mutex> __lg(mtx_)
#else
#define UI_LOCK_GUARD() ((void)0)
#endif

class UIManager {
public:
    // ===== 绝对单例 =====
    static UIManager& Instance();

    // 禁用所有拷贝/赋值/移动，保证绝对单例
    UIManager(const UIManager&) = delete;
    UIManager& operator=(const UIManager&) = delete;
    UIManager(UIManager&&) = delete;
    UIManager& operator=(UIManager&&) = delete;

    // ===== 控件创建 =====
    // 根据类型创建控件，parent不为空则自动加入父节点childs
    // parent默认空（顶级控件）
    UIControl::Ptr createControl(const std::string& type,
                                 const std::string& name,
                                 UIControl::Ptr parent = nullptr);

    // ===== 控件查找 =====
    // 根据name查找控件；找不到时：
    //   createIfMissing = false：返回nullptr
    //   createIfMissing = true：自动创建一个base类型控件返回
    UIControl::Ptr getControlByName(const std::string& name,
                                    bool createIfMissing = false,
                                    UIControl::Ptr parent = nullptr);

    // 根据id查找控件，找不到返回nullptr
    UIControl::Ptr getControlById(uint64_t id);

    // ===== 控件销毁 =====
    // 销毁指定控件（递归销毁所有子控件）
    void destroyControl(UIControl::Ptr ctrl);
    void destroyControlByName(const std::string& name);

    // 销毁所有控件（清空整个UI树）
    void destroyAll();

    // ===== 统计 =====
    size_t getControlCount() const { return nameMap_.size(); }

private:
    UIManager();  // 私有构造，外部不能new
    ~UIManager();

    uint64_t nextId();

    // 存储所有控件：按name索引
    std::unordered_map<std::string, UIControl::Ptr> nameMap_;
    // 存储所有控件：按id索引
    std::unordered_map<uint64_t, UIControl::Ptr> idMap_;

    uint64_t idCounter_ = 1;

#ifdef UI_THREAD_SAFE
    std::mutex mtx_;
#endif
};

// 方便使用的全局访问宏
#define UI_MGR UIManager::Instance()

