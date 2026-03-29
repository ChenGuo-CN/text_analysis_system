#!/bin/bash

# RKLLM Chat Template 测试程序编译脚本

set -e

echo "========================================"
echo "  RKLLM Chat Template 编译脚本"
echo "========================================"

# 获取脚本所在目录
SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
cd "${SCRIPT_DIR}"

# 创建lib目录（与build同级）
LIB_DIR="${SCRIPT_DIR}/lib"
echo "[1/5] 创建lib目录..."
mkdir -p "${LIB_DIR}"

# 查找并复制运行时库到lib目录
echo "[2/5] 复制运行时库..."
RKLLM_LIB_PATH=""

# 尝试多个可能的路径
if [ -f "${SCRIPT_DIR}/../rknn-llm/rkllm-runtime/Linux/librkllm_api/aarch64/librkllmrt.so" ]; then
    RKLLM_LIB_PATH="${SCRIPT_DIR}/../rknn-llm/rkllm-runtime/Linux/librkllm_api/aarch64/librkllmrt.so"
elif [ -f "${SCRIPT_DIR}/../rknn-llm/rkllm-runtime/arm64/librkllmrt.so" ]; then
    RKLLM_LIB_PATH="${SCRIPT_DIR}/../rknn-llm/rkllm-runtime/arm64/librkllmrt.so"
elif [ -f "/usr/lib/librkllmrt.so" ]; then
    RKLLM_LIB_PATH="/usr/lib/librkllmrt.so"
fi

if [ -n "${RKLLM_LIB_PATH}" ]; then
    cp "${RKLLM_LIB_PATH}" "${LIB_DIR}/"
    echo "  [成功] 已复制 librkllmrt.so 到 lib/"
    echo "  库路径: ${RKLLM_LIB_PATH}"
else
    echo "  [警告] 未找到 librkllmrt.so，请手动复制到 lib/ 目录"
fi

# 创建构建目录
BUILD_DIR="build"
echo "[3/5] 创建build目录..."
if [ -d "${BUILD_DIR}" ]; then
    echo "  清理旧的构建目录..."
    rm -rf "${BUILD_DIR}"
fi
mkdir -p "${BUILD_DIR}"

# 运行CMake
echo "[4/5] 配置CMake..."
cd "${BUILD_DIR}"
cmake .. \
    -DCMAKE_C_COMPILER=aarch64-linux-gnu-gcc \
    -DCMAKE_CXX_COMPILER=aarch64-linux-gnu-g++ \
    -DCMAKE_BUILD_TYPE=Release

# 编译
echo "[5/5] 编译程序..."
make -j$(nproc)

echo ""
echo "========================================"
echo "  编译完成"
echo "========================================"
echo "目录结构:"
echo "  test_chat_template/"
echo "  ├── build/"
echo "  │   └── test_chat_template    # 可执行文件"
echo "  ├── lib/                      # 运行时库"
echo "  │   └── librkllmrt.so"
echo "  └── src/"
echo "      └── main.cpp"
echo ""
echo "运行方法:"
echo "  cd ${SCRIPT_DIR}"
echo "  export LD_LIBRARY_PATH=./lib:\$LD_LIBRARY_PATH"
echo "  ./build/test_chat_template <模型路径> [max_new_tokens] [max_context_len]"
echo ""
echo "示例:"
echo "  ./build/test_chat_template /userdata/models/Qwen2.5-1.5B-Instruct_W8A8_RK3588_16k.rkllm 1024 2048"
echo ""
