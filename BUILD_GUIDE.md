# PeriodicTableOpenGL 编译指南（Windows + Linux/WSL）
项目：OpenGL元素周期表 + 自研UI控件框架
C++17 + OpenGL + GLFW + GLEW + FreeType

> 本项目使用 CMake：
> - GLFW/GLEW 用 FetchContent 自动下载编译
> - FreeType 使用系统库，彻底绕开源码编译的 in-source build bug
> CMake配置阶段会**自动校验前置工具**，缺少Git/编译器版本过低会直接报错退出。

---

## 双模式说明（两种都支持）
| 模式 | 说明 | 产物 |
|---|---|---|
| **模式1 - 组件嵌入** | UI框架编译为静态库 `ui_framework`，直接链接到你原有周期表项目，原有main/逻辑完全不动 | `libui_framework.a` / `ui_framework.lib` |
| **模式2 - 独立Demo** | 自带测试入口 `demo_main.cpp`，可单独运行测试UI框架效果，和你原有项目的main不冲突 | `ui_demo` / `ui_demo.exe` |

---

## 前置必备工具（CMake自动检测）
1. Git（必须，FetchContent拉取GLFW/GLEW源码用）
2. CMake >=3.16
3. C++编译器
   - Windows: Visual Studio 2019/2022（MSVC >=19.20）
   - Linux/WSL: gcc >=7.0 或者 clang >=5.0

> 运行 `cmake ..` 会自动执行环境校验，控制台输出 ✅ 成功 / ❌ 错误信息

---

# 一、Windows 平台编译

## 1. 安装基础工具
1. 安装 Visual Studio 2022（勾选「使用C++的桌面开发」工作负载）
2. 安装 Git for Windows
3. 安装 CMake，全部勾选「添加到系统PATH」
4. 安装 vcpkg，并用vcpkg安装 FreeType：
   ```powershell
   vcpkg install freetype:x64-windows
   ```

## 2. 编译步骤
打开 `x64 Native Tools Command Prompt for VS 2022`，进入项目源码目录：

```powershell
# 创建构建目录
mkdir build
cd build

# CMake配置（自动检测环境 + 拉取GLFW/GLEW依赖）
cmake .. -DCMAKE_TOOLCHAIN_FILE="你的vcpkg路径/scripts/buildsystems/vcpkg.cmake"

# 可选：开启UI多线程安全
# cmake .. -DUI_THREAD_SAFE=ON

# 可选：关闭Demo，只编译UI静态库（嵌入项目用）
# cmake .. -DBUILD_DEMO=OFF

# 编译 Release 版本
cmake --build . --config Release
```

## 3. 运行Demo程序
```
build\bin\Release\ui_demo.exe
```

---

# 二、Linux / WSL(Ubuntu) 平台编译

## 1. 安装基础编译环境（一次性装全）
```bash
sudo apt update
sudo apt install build-essential cmake git
# X11开发库（编译GLFW用）
sudo apt install libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev
# FreeType开发库（系统库，不再用源码编译，绕开in-source bug）
sudo apt install libfreetype6-dev
# OpenGL开发库
sudo apt install libgl1-mesa-dev
```

## 2. 编译步骤
```bash
# 进入项目源码目录
cd /home/ink/src/Periodic

# 一键清理所有缓存（如果之前编译报错，先执行这个）
mkdir -p build && cd build && cmake .. && make clean-all

# 重新cmake配置
cmake ..

# 开启多线程安全版本：
# cmake .. -DUI_THREAD_SAFE=ON

# 关闭Demo，只编译UI静态库：
# cmake .. -DBUILD_DEMO=OFF

# 编译（多线程加速）
make -j$(nproc)
```

## 3. WSL 额外配置（OpenGL窗口显示）
WSL需要配置VcXsrv才能弹出OpenGL窗口：

1. Windows安装并启动 VcXsrv（XLaunch），勾选「Disable access control」
2. WSL终端执行环境变量：
```bash
export DISPLAY=$(cat /etc/resolv.conf | grep nameserver | awk '{print $2}'):0
export LIBGL_ALWAYS_INDIRECT=0
```

## 4. 运行Demo程序
```bash
./bin/ui_demo
```

---

# 三、分级清理命令
依赖源码现在下载在**项目根目录的_deps/**，不在build目录里，所以清理分两个等级：

## 1. 日常清理（快）- 只清编译文件，保留依赖
```bash
# 进入build目录后执行
make clean-build
```
删除内容：
- `build/bin/` 编译输出
- `build/CMakeCache.txt` 和 `build/CMakeFiles/` 缓存

**保留**：根目录的 `_deps/` 依赖源码，下次编译不用重新下载，速度快。

---

## 2. 彻底清理（慢）- 连依赖一起删
```bash
# 进入build目录后执行
make clean-all
```
删除内容：
- 上面clean-build删的所有内容
- **根目录的 `_deps/` 所有下载的依赖源码**

下次编译会重新下载GLFW/GLEW源码，适合依赖缓存损坏、版本更新时用。

---

# 四、如何嵌入到你原有周期表项目
1. 把 `UIControl.h/cpp`、`UIManager.h/cpp`、`UIPageManager.h/cpp` 这6个文件复制到你项目里
2. 在CMakeLists.txt末尾取消注释「原有项目可执行程序」部分，填上你自己的源码文件名
3. 在你原来的main.cpp里 `#include "UIControl.h"`，用 `UI_MGR.createControl()` 创建控件
4. 渲染循环里调用 `UI_PAGE.drawCurrentPage(ctx)` 绘制UI即可

---

# 五、常用编译参数说明

| 参数 | 说明 |
|---|---|
| `-DUI_THREAD_SAFE=ON` | 开启控件管理器多线程安全锁，多线程操作UI时启用；单线程渲染不建议开启，有性能损耗 |
| `-DBUILD_DEMO=OFF` | 关闭独立Demo，只编译UI静态库，嵌入项目时用 |
| `-DCMAKE_BUILD_TYPE=Release` | Release发布版本（默认） |
| `-DCMAKE_BUILD_TYPE=Debug` | Debug调试版本，带符号信息 |

---

# 六、常见问题

## Windows 平台
1. **❌ 未找到 git**：安装Git并加入系统环境变量，重启终端重试
2. **LNK2019 链接错误**：确认VS平台是x64，不是Win32
3. **找不到FreeType**：用vcpkg安装 freetype:x64-windows

## Linux / WSL 平台
1. **❌ RandR headers not found**：执行 `sudo apt install libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev`
2. **❌ 找不到 FreeType**：执行 `sudo apt install libfreetype6-dev`
3. **❌ in-source builds not permitted**：现在已经改用系统FreeType，不会再出现这个问题；如果还有，执行 `make clean-all` 清理旧缓存
4. **窗口黑屏 / 创建GL上下文失败**：检查VcXsrv是否启动，DISPLAY环境变量是否正确
5. **依赖下载超时**：国内网络可替换FetchContent的GLFW/GLEW仓库地址为Gitee镜像

## 通用问题
1. **首次编译很慢**：FetchContent需要下载+编译GLFW/GLEW源码，属于正常现象，第二次编译会复用缓存
2. **和原有项目main冲突**：Demo入口是`demo_main.cpp`，不是`main.cpp`，不会和你原有项目冲突

