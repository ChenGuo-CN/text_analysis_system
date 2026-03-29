cmake_minimum_required(VERSION 3.10)

project(ppocr_timing)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置编译选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -Wall -fpermissive")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -Wall")

# 设置目标平台
set(TARGET_SOC "rk3588")

# 设置RKNN根目录
set(RKNN_ROOT "/home/linaro/traffic_text_analysis_system/rknn_model_zoo")

# 添加3rdparty和utils子目录
add_subdirectory(${RKNN_ROOT}/3rdparty 3rdparty.out)
add_subdirectory(${RKNN_ROOT}/utils utils.out)

# 查找OpenCV
find_package(OpenCV REQUIRED)

# 包含目录
include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/include
    ${LIBRKNNRT_INCLUDES}
    ${RKNN_ROOT}/utils
    ${OpenCV_INCLUDE_DIRS}
)

# 源文件
set(SOURCES
    ${CMAKE_SOURCE_DIR}/src/main.cc
    ${CMAKE_SOURCE_DIR}/src/ppocr_system.cc
    ${CMAKE_SOURCE_DIR}/src/postprocess.cc
    ${CMAKE_SOURCE_DIR}/src/clipper.cc
)

# 可执行文件
add_executable(${PROJECT_NAME} ${SOURCES})

# 设置线程库
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# 链接库
target_link_libraries(${PROJECT_NAME}
    fileutils
    imageutils
    imagedrawing
    ${OpenCV_LIBS}
    ${LIBRKNNRT}
    Threads::Threads
    m
)

# 设置运行时库路径
set_target_properties(${PROJECT_NAME} PROPERTIES
    BUILD_WITH_INSTALL_RPATH TRUE
    INSTALL_RPATH "${RKNN_ROOT}/install/rk3588_linux_aarch64/rknn_PPOCR-System_demo/lib"
    INSTALL_RPATH_USE_LINK_PATH TRUE
)
