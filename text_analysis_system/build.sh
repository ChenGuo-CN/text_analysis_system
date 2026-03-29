#!/bin/bash
# build.sh - 文本分析系统编译脚本

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}  文本分析系统编译脚本${NC}"
echo -e "${GREEN}========================================${NC}"

# 获取脚本所在目录
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# 创建构建目录
BUILD_DIR="build"
if [ -d "$BUILD_DIR" ]; then
    echo -e "${YELLOW}清理旧的构建目录...${NC}"
    rm -rf "$BUILD_DIR"
fi

# 创建lib目录（与build同级）
LIB_DIR="lib"
if [ -d "$LIB_DIR" ]; then
    echo -e "${YELLOW}清理旧的lib目录...${NC}"
    rm -rf "$LIB_DIR"
fi

echo -e "${YELLOW}创建构建目录...${NC}"
mkdir -p "$BUILD_DIR"

echo -e "${YELLOW}创建lib目录...${NC}"
mkdir -p "$LIB_DIR"

cd "$BUILD_DIR"

# 运行CMake
echo -e "${YELLOW}运行CMake...${NC}"
cmake .. -DCMAKE_BUILD_TYPE=Release

# 编译
echo -e "${YELLOW}开始编译...${NC}"
make -j$(nproc)

# 检查编译结果
if [ -f "text_analysis_system" ]; then
    echo -e "${GREEN}编译成功!${NC}"
    
    # 复制可执行文件到项目根目录
    cp text_analysis_system ../
    
    # 复制依赖库到lib目录
    echo -e "${YELLOW}复制依赖库...${NC}"
    
    # 查找RKNN和RKLLM库
    RKNN_LIB="../rknn_model_zoo/3rdparty/rknpu2/Linux/librknn_api/aarch64/lib/librknnrt.so"
    RKLLM_LIB="../rknn-llm/rkllm-runtime/Linux/librkllm_api/aarch64/librkllmrt.so"
    
    # 如果找不到，尝试其他路径
    if [ ! -f "$RKNN_LIB" ]; then
        RKNN_LIB="/usr/lib/librknnrt.so"
    fi
    if [ ! -f "$RKLLM_LIB" ]; then
        RKLLM_LIB="/usr/lib/librkllmrt.so"
    fi
    
    if [ -f "$RKNN_LIB" ]; then
        cp "$RKNN_LIB" ../lib/
        echo -e "${GREEN}已复制: librknnrt.so -> lib/${NC}"
    else
        echo -e "${RED}警告: 未找到 librknnrt.so${NC}"
    fi
    
    if [ -f "$RKLLM_LIB" ]; then
        cp "$RKLLM_LIB" ../lib/
        echo -e "${GREEN}已复制: librkllmrt.so -> lib/${NC}"
    else
        echo -e "${RED}警告: 未找到 librkllmrt.so${NC}"
    fi
    
    echo ""
    echo -e "${GREEN}========================================${NC}"
    echo -e "${GREEN}  编译完成!${NC}"
    echo -e "${GREEN}========================================${NC}"
    echo ""
    echo "可执行文件: ./text_analysis_system"
    echo "依赖库目录: ./lib/"
    echo "使用方法: LD_LIBRARY_PATH=./lib ./text_analysis_system <图片路径或文件夹>"
    echo ""
else
    echo -e "${RED}编译失败!${NC}"
    exit 1
fi
