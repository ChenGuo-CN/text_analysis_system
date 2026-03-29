cmake_minimum_required(VERSION 3.10)

project(test_select_npu)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置RKNN模型库路径
set(RKNN_MODEL_ZOO_ROOT /home/linaro/traffic_text_analysis_system/rknn_model_zoo)
set(RKNPU2_INCLUDE ${RKNN_MODEL_ZOO_ROOT}/3rdparty/rknpu2/include)
set(RKNPU2_LIB ${RKNN_MODEL_ZOO_ROOT}/3rdparty/rknpu2/Linux/aarch64/librknnrt.so)

# OpenCV
find_package(OpenCV REQUIRED)
message(STATUS "OpenCV_LIBS=${OpenCV_LIBS}")

# 源文件
set(SRCS
    src/main.cc
    src/ppocr_system_npu2.cc
    src/postprocess.cc
    src/clipper.cc
    src/image_utils.c
    src/file_utils.c
)

# 头文件路径
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${RKNPU2_INCLUDE}
    ${OpenCV_INCLUDE_DIRS}
)

# 可执行文件
add_executable(${PROJECT_NAME} ${SRCS})

# 链接库
target_link_libraries(${PROJECT_NAME}
    ${OpenCV_LIBS}
    ${RKNPU2_LIB}
    pthread
)

# 安装
install(TARGETS ${PROJECT_NAME} DESTINATION .)
