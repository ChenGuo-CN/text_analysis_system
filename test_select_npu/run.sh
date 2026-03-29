#!/bin/bash

# test_select_npu 运行脚本
# 功能：自动检测脚本目录，使用本地lib目录的运行时库运行程序

# 获取脚本所在目录（项目根目录）
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

# 设置库路径为本地lib目录
export LD_LIBRARY_PATH=${SCRIPT_DIR}/lib:$LD_LIBRARY_PATH

# 检查可执行文件是否存在
if [ ! -f "${SCRIPT_DIR}/build/test_select_npu" ]; then
    echo "Error: Executable not found at ${SCRIPT_DIR}/build/test_select_npu"
    echo "Please run ./build.sh first to compile the project."
    exit 1
fi

# 检查运行时库是否存在
if [ ! -f "${SCRIPT_DIR}/lib/librknnrt.so" ]; then
    echo "Warning: Runtime library not found at ${SCRIPT_DIR}/lib/librknnrt.so"
    echo "Please run ./build.sh first to copy the runtime library."
    exit 1
fi

# 检查模型文件是否存在
if [ ! -f "${SCRIPT_DIR}/model/ppocrv4_det_i8.rknn" ]; then
    echo "Error: Detection model not found at ${SCRIPT_DIR}/model/ppocrv4_det_i8.rknn"
    exit 1
fi

if [ ! -f "${SCRIPT_DIR}/model/ppocrv4_rec_fp16.rknn" ]; then
    echo "Error: Recognition model not found at ${SCRIPT_DIR}/model/ppocrv4_rec_fp16.rknn"
    exit 1
fi

# 运行程序（必须在项目根目录下运行，以便正确找到模型文件）
cd "${SCRIPT_DIR}"
./build/test_select_npu "$@"
