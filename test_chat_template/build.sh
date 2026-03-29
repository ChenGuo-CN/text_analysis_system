#!/bin/bash

# RKLLM Chat Template 测试程序编译脚本

set -e

echo "========================================"
echo "  RKLLM Chat Template 编译脚本"
echo "========================================"

# 获取脚本所在目录
SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
cd "${SCRIPT_DIR}"

# 创建构建目录
BUILD_DIR="build"
if [ -d "${BUILD_DIR}" ]; then
    echo "清理旧的构建目录..."
    rm -rf "${BUILD_DIR}"
fi

mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

echo ""
echo "开始配置..."

# 运行CMake
cmake .. \
    -DCMAKE_C_COMPILER=aarch64-linux-gnu-gcc \
    -DCMAKE_CXX_COMPILER=aarch64-linux-gnu-g++ \
    -DCMAKE_BUILD_TYPE=Release

echo ""
echo "开始编译..."

# 编译
make -j$(nproc)

echo ""
echo "========================================"
echo "  编译完成"
echo "========================================"
echo "可执行文件: ${SCRIPT_DIR}/${BUILD_DIR}/test_chat_template"
echo ""
echo "运行方法:"
echo "  cd ${SCRIPT_DIR}"
echo "  export LD_LIBRARY_PATH=/path/to/librkllmrt.so:\$LD_LIBRARY_PATH"
echo "  ./${BUILD_DIR}/test_chat_template <模型路径> [max_new_tokens] [max_context_len]"
echo ""
echo "示例:"
echo "  ./${BUILD_DIR}/test_chat_template /path/to/DeepSeek-R1-Distill-Qwen-1.5B_W8A8_RK3588.rkllm 2048 4096"
echo ""
