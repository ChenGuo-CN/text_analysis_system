cmake_minimum_required(VERSION 3.10)

project(ppocr_timing)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置编译选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -Wall -fpermissive")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -Wall")

# 使用相对路径引用rknn_model_zoo（基于项目根目录的父目录）
set(RKNN_MODEL_ZOO_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../rknn_model_zoo)
set(RKNPU2_INCLUDE ${RKNN_MODEL_ZOO_ROOT}/3rdparty/rknpu2/include)
set(RKNPU2_SRC_LIB ${RKNN_MODEL_ZOO_ROOT}/3rdparty/rknpu2/Linux/aarch64/librknnrt.so)
set(RKNPU2_DST_LIB ${CMAKE_CURRENT_SOURCE_DIR}/lib/librknnrt.so)
set(RGA_SRC_LIB ${RKNN_MODEL_ZOO_ROOT}/3rdparty/librga/Linux/aarch64/librga.so)
set(RGA_DST_LIB ${CMAKE_CURRENT_SOURCE_DIR}/lib/librga.so)

# 其他库路径
set(RKNN_UTILS_PATH ${RKNN_MODEL_ZOO_ROOT}/utils)
set(RKNN_RGA_PATH ${RKNN_MODEL_ZOO_ROOT}/3rdparty/librga/include)
set(RKNN_STB_PATH ${RKNN_MODEL_ZOO_ROOT}/3rdparty/stb_image)

# OpenCV
find_package(OpenCV REQUIRED)

# 包含目录
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${RKNPU2_INCLUDE}
    ${RKNN_UTILS_PATH}
    ${RKNN_RGA_PATH}
    ${RKNN_STB_PATH}
    ${OpenCV_INCLUDE_DIRS}
)

# 源文件 - 包含utils源文件（使用本地修改后的版本，避免turbojpeg依赖）
set(SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ppocr_system.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/postprocess.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/clipper.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/file_utils.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/image_utils.c
    ${RKNN_UTILS_PATH}/image_drawing.c
)

# 可执行文件
add_executable(${PROJECT_NAME} ${SOURCES})

# 设置线程库
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# 链接库 - 使用本地lib目录中的库（编译后复制）
target_link_libraries(${PROJECT_NAME}
    ${OpenCV_LIBS}
    ${RKNPU2_DST_LIB}
    ${RGA_DST_LIB}
    Threads::Threads
    m
)

# 创建lib目录并复制运行时库（与build目录同级）
add_custom_target(copy_libs ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_SOURCE_DIR}/lib
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${RKNPU2_SRC_LIB}
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${RGA_SRC_LIB}
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/
    COMMENT "Copying runtime libraries to lib/"
)
add_dependencies(${PROJECT_NAME} copy_libs)

# 安装
install(TARGETS ${PROJECT_NAME} DESTINATION .)
