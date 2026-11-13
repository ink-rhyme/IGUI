#!/bin/bash
set -e
MODE="$1"
cd /home/ink/src/IGUI

if [ "$MODE" = "full" ]; then
    echo "===== 编译模式: full ====="
    rm -rf build
    mkdir -p build bin
    cd build
    cmake ..
    make -j$(nproc)
    echo "✅ 全量编译完成！可执行文件在 ./bin/igui_demo"
elif [ "$MODE" = "fast" ]; then
    echo "===== 编译模式: fast ====="
    mkdir -p build bin
    cd build
    cmake ..
    make -j$(nproc)
    echo "✅ 增量编译完成！可执行文件在 ./bin/igui_demo"
elif [ "$MODE" = "clean" ]; then
    echo "===== 清理模式: clean ====="
    rm -rf build bin
    echo "✅ 清理完毕"
elif [ "$MODE" = "install" ]; then
    echo "===== 安装模式: install ====="
    mkdir -p ~/.lib/include/igui
    cp -v lib/libigui.a ~/.lib/
    cp -v _deps/glfw-build/src/libglfw3.a ~/.lib/
    cp -v *.h ~/.lib/include/igui/
    echo "✅ libigui.a 安装成功至 ~/.lib"
else
    echo "===== 默认编译模式: normal ====="
    mkdir -p build bin
    cd build
    cmake ..
    make -j$(nproc)
    echo "✅ 编译完成！可执行文件在 ./bin/igui_demo"
    echo "用法: ./rebuild.sh [normal|fast|full|clean|install]"
fi
