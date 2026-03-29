# CMakeLists.txt - 文本分析系统构建配置

cmake_minimum_required(VERSION 3.10)
project(text_analysis_system)

# 设置C++标准
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置编译选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -Wall -Wextra")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -Wall")

# 查找依赖包
find_package(OpenCV REQUIRED)

# RKNN和RKLLM库路径
set(RKNN_API_PATH ${CMAKE_SOURCE_DIR}/../rknn_model_zoo/3rdparty/rknpu2)
set(RKNN_LIB_PATH ${CMAKE_SOURCE_DIR}/../rknn_model_zoo/3rdparty/rknpu2/Linux/librknn_api/aarch64/lib)
set(RKLLM_API_PATH ${CMAKE_SOURCE_DIR}/../rknn-llm/rkllm-runtime/Linux/librkllm_api/include)
set(RKLLM_LIB_PATH ${CMAKE_SOURCE_DIR}/../rknn-llm/rkllm-runtime/Linux/librkllm_api/aarch64)

# 包含目录
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${OpenCV_INCLUDE_DIRS}
    ${RKNN_API_PATH}/include
    ${RKLLM_API_PATH}
)

# 链接目录
link_directories(
    ${RKNN_LIB_PATH}
    ${RKLLM_LIB_PATH}
)

# 源文件列表
set(SOURCES
    src/main.cpp
    src/config.cpp
    src/text_queue.cpp
    src/perf_monitor.cpp
    src/ocr_engine.cpp
    src/ocr_thread.cpp
    src/llm_engine.cpp
    src/llm_thread.cpp
    src/result_handler.cpp
    # PPOCR相关源文件
    src/ppocr_system_npu2.cc
    src/postprocess.cc
    src/clipper.cc
    src/image_utils.c
    src/file_utils.c
)

# 头文件列表（用于IDE识别）
set(HEADERS
    include/config.h
    include/text_queue.h
    include/perf_monitor.h
    include/ocr_engine.h
    include/ocr_thread.h
    include/llm_engine.h
    include/llm_thread.h
    include/result_handler.h
    include/ppocr_system.h
    include/common.h
    include/image_utils.h
    include/file_utils.h
)

# 创建可执行文件
add_executable(text_analysis_system ${SOURCES} ${HEADERS})

# 链接库
target_link_libraries(text_analysis_system
    ${OpenCV_LIBS}
    rknnrt
    rkllmrt
    pthread
)

# 设置RPATH，使程序能找到动态库
set_target_properties(text_analysis_system PROPERTIES
    BUILD_WITH_INSTALL_RPATH TRUE
    INSTALL_RPATH "${RKNN_LIB_PATH};${RKLLM_LIB_PATH}"
    INSTALL_RPATH_USE_LINK_PATH TRUE
)

# 安装目标
install(TARGETS text_analysis_system
    RUNTIME DESTINATION bin
)

# 安装配置文件
install(FILES config/config.json
    DESTINATION config
)

# 打印配置信息
message(STATUS "")
message(STATUS "========================================")
message(STATUS "  文本分析系统构建配置")
message(STATUS "========================================")
message(STATUS "OpenCV版本: ${OpenCV_VERSION}")
message(STATUS "OpenCV库: ${OpenCV_LIBS}")
message(STATUS "RKNN API路径: ${RKNN_API_PATH}")
message(STATUS "RKNN LIB路径: ${RKNN_LIB_PATH}")
message(STATUS "RKLLM API路径: ${RKLLM_API_PATH}")
message(STATUS "RKLLM LIB路径: ${RKLLM_LIB_PATH}")
message(STATUS "C++标准: ${CMAKE_CXX_STANDARD}")
message(STATUS "========================================")
message(STATUS "")
