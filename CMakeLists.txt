cmake_minimum_required(VERSION 3.16)
project(IGUI LANGUAGES C CXX)

# ====================== 禁止源码内构建 前置检查 ======================
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_CURRENT_BINARY_DIR)
    message(FATAL_ERROR "❌ 禁止源码内构建！请创建独立build文件夹，进入build目录执行 cmake ..")
endif()

# ====================== 选项开关 ======================
option(UI_THREAD_SAFE "Enable thread-safe for UI manager" OFF)
option(BUILD_DEMO "Build standalone UI demo program (no conflict with your own main)" ON)
option(FETCHCONTENT_QUIET OFF)

# ====================== 前置环境自动检查 ======================
message(STATUS "===== 环境前置校验开始 =====")

# 检查Git（FetchContent必须依赖git）
find_program(GIT_EXECUTABLE git)
if(NOT GIT_EXECUTABLE)
    message(FATAL_ERROR "❌ 未找到 git，请先安装 Git 并加入环境变量！")
endif()
message(STATUS "✅ Git 已找到: ${GIT_EXECUTABLE}")

# 检查C++编译器是否支持C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    if(MSVC_VERSION LESS 1920)
        message(FATAL_ERROR "❌ MSVC版本过低，需要 Visual Studio 2019 或更高版本！")
    endif()
    message(STATUS "✅ MSVC编译器版本: ${MSVC_VERSION}")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 7.0)
        message(FATAL_ERROR "❌ GCC版本过低，需要 GCC >=7.0！")
    endif()
    message(STATUS "✅ GCC编译器版本: ${CMAKE_CXX_COMPILER_VERSION}")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 5.0)
        message(FATAL_ERROR "❌ Clang版本过低，需要 Clang >=5.0！")
    endif()
    message(STATUS "✅ Clang编译器版本: ${CMAKE_CXX_COMPILER_VERSION}")
endif()

message(STATUS "===== 环境前置校验完成，开始配置依赖 =====")
# =============================================================

# ====================== FreeType 使用系统包（绕开源码编译in-source bug） ======================
find_package(Freetype REQUIRED)
if(FREETYPE_FOUND)
    message(STATUS "✅ FreeType 系统库已找到: ${FREETYPE_INCLUDE_DIRS}")
else()
    message(FATAL_ERROR "❌ 未找到 FreeType 开发库！\n"
                        "Windows: 请用vcpkg安装 freetype:x64-windows\n"
                        "Linux/WSL: 请执行 sudo apt install libfreetype6-dev")
endif()

# ====================== GLEW 使用系统库 ======================
find_package(GLEW REQUIRED)
if(GLEW_FOUND)
    message(STATUS "✅ GLEW 系统库已找到: ${GLEW_LIBRARIES}")
else()
    message(FATAL_ERROR "❌ 未找到 GLEW 开发库！\n"
                        "Windows: 请用vcpkg安装 glew:x64-windows\n"
                        "Linux/WSL: 请执行 sudo apt install libglew-dev")
endif()

# ====================== FetchContent 自动拉取 GLFW ======================
include(FetchContent)

# 依赖下载目录放到项目根目录的_deps，不放在build里
# 好处：多次编译/切换build目录时，依赖不用重新下载
set(FETCHCONTENT_BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/_deps)
message(STATUS "✅ 依赖下载目录: ${FETCHCONTENT_BASE_DIR}")

# GLFW 编译优化：关闭文档/测试/示例，减少编译时间
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_USE_WAYLAND OFF CACHE BOOL "" FORCE)

# GLFW
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.3.9
)

# 拉取并编译（只拉GLFW，GLEW/FreeType都用系统库）
FetchContent_MakeAvailable(glfw)

# ====================== 输出目录统一设置到项目根目录 ======================
# 可执行文件输出到项目根目录的 bin/
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)
# 动态库输出到项目根目录的 lib/
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib)
# 静态库输出到项目根目录的 lib/
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib)
# Windows Debug/Release 子目录自动处理
foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_CURRENT_SOURCE_DIR}/bin)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_CURRENT_SOURCE_DIR}/lib)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_CURRENT_SOURCE_DIR}/lib)
endforeach(OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES)
message(STATUS "✅ 可执行文件输出目录: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
message(STATUS "✅ 库文件输出目录: ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}")

# ====================== UI框架静态库（组件嵌入模式） ======================
set(UI_MODULE_FILES
    UIControl.h
    UIControl.cpp
    UIManager.h
    UIManager.cpp
    UIPageManager.h
    UIPageManager.cpp
    TextRenderer.h
    TextRenderer.cpp
)

add_library(igui STATIC ${UI_MODULE_FILES})
target_include_directories(igui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(igui PRIVATE Freetype::Freetype)

# UI线程安全宏
if(UI_THREAD_SAFE)
    target_compile_definitions(ui PUBLIC UI_THREAD_SAFE)
endif()

# ====================== 独立Demo可执行程序（测试用） ======================
if(BUILD_DEMO)
    add_executable(igui_demo demo_main.cpp)
    target_link_libraries(igui_demo PRIVATE igui)

    if(WIN32)
        target_compile_definitions(igui_demo PRIVATE _CRT_SECURE_NO_WARNINGS)
        target_link_libraries(igui_demo PRIVATE
            glfw
            GLEW::GLEW
            Freetype::Freetype
            opengl32.lib
        )
    elseif(UNIX AND NOT APPLE)
        find_package(OpenGL REQUIRED)
        target_link_libraries(igui_demo PRIVATE
            glfw
            GLEW::GLEW
            Freetype::Freetype
            OpenGL::GL
            pthread
        )
    endif()

    message(STATUS "✅ 已启用独立Demo程序 igui_demo，输出到 bin/ 目录")
endif()

# ====================== 你原有项目的可执行程序（组件嵌入模式） ======================
# 把你原本周期表项目的源码填到下面这里，取消注释并替换文件名
# add_executable(${PROJECT_NAME}
#     main.cpp
#     ElementData.cpp
#     TextRenderer.cpp
# )
# target_link_libraries(${PROJECT_NAME} PRIVATE igui)
#
# if(WIN32)
#     target_link_libraries(${PROJECT_NAME} PRIVATE
#         glfw GLEW::GLEW Freetype::Freetype opengl32.lib
#     )
# elseif(UNIX AND NOT APPLE)
#     find_package(OpenGL REQUIRED)
#     target_link_libraries(${PROJECT_NAME} PRIVATE
#         glfw GLEW::GLEW Freetype::Freetype OpenGL::GL pthread
#     )
# endif()

# ====================== 清理命令 ======================
# 1. clean-build：只清理编译中间文件和输出（bin/lib），保留下载的依赖（_deps）
add_custom_target(clean-build
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_CURRENT_SOURCE_DIR}/bin
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_CURRENT_SOURCE_DIR}/lib
    COMMAND ${CMAKE_COMMAND} -E remove ${CMAKE_BINARY_DIR}/CMakeCache.txt
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/CMakeFiles
    COMMENT "✅ 已清理编译中间文件和bin/lib输出，保留依赖源码（_deps），下次编译不用重新下载"
)

# 2. clean-all：清理所有内容，包括根目录的依赖源码（_deps）
add_custom_target(clean-all
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_CURRENT_SOURCE_DIR}/bin
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_CURRENT_SOURCE_DIR}/lib
    COMMAND ${CMAKE_COMMAND} -E remove ${CMAKE_BINARY_DIR}/CMakeCache.txt
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/CMakeFiles
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_CURRENT_SOURCE_DIR}/_deps
    COMMENT "✅ 已清理所有内容：编译缓存 + bin/lib输出 + 下载的依赖源码（_deps）"
)

message(STATUS "===== CMake 配置完成 =====")
message(STATUS "  模式1 - 组件嵌入: ui 静态库已生成，可直接链接到你的原有项目")
message(STATUS "  模式2 - 独立Demo: igui_demo 可执行程序（测试UI框架用）")
message(STATUS "  清理命令:")
message(STATUS "    make clean-build  - 只清理编译文件，保留依赖（_deps），下次编译快")
message(STATUS "    make clean-all    - 清理所有内容，包括依赖源码，下次要重新下载")

