#!/bin/bash

# test_select_npu 编译脚本
# 功能：自动创建build和lib目录，编译项目并复制运行时库

set -e

# 获取脚本所在目录（项目根目录）
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

echo "========================================"
echo "  test_select_npu Build Script"
echo "========================================"
echo ""

# 检查rknn_model_zoo是否存在
if [ ! -d "../rknn_model_zoo" ]; then
    echo "Error: rknn_model_zoo directory not found in parent directory!"
    echo "Please ensure rknn_model_zoo is located at: $(dirname "$SCRIPT_DIR")/rknn_model_zoo"
    exit 1
fi

# 创建build目录
echo "[1/4] Creating build directory..."
mkdir -p build

# 进入build目录
cd build

# 运行cmake
echo "[2/4] Running cmake..."
cmake .. -DCMAKE_BUILD_TYPE=Release

# 编译
echo "[3/4] Building project..."
make -j4

# 返回项目根目录
cd ..

# 检查lib目录和库文件是否创建成功
echo "[4/4] Verifying runtime libraries..."
if [ -f "lib/librknnrt.so" ]; then
    echo "  Runtime library copied successfully: lib/librknnrt.so"
else
    echo "  Warning: Runtime library not found in lib/"
fi

echo ""
echo "========================================"
echo "  Build completed successfully!"
echo "========================================"
echo ""
echo "Directory structure:"
echo "  build/test_select_npu    - Executable"
echo "  lib/librknnrt.so         - Runtime library"
echo ""
echo "To run the program:"
echo "  ./run.sh"
echo ""
