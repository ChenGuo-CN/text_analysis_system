cmake_minimum_required(VERSION 3.10)

project(test_select_npu)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 使用相对路径引用rknn_model_zoo（基于项目根目录的父目录）
set(RKNN_MODEL_ZOO_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../rknn_model_zoo)
set(RKNPU2_INCLUDE ${RKNN_MODEL_ZOO_ROOT}/3rdparty/rknpu2/include)
set(RKNPU2_SRC_LIB ${RKNN_MODEL_ZOO_ROOT}/3rdparty/rknpu2/Linux/aarch64/librknnrt.so)
set(RKNPU2_DST_LIB ${CMAKE_CURRENT_SOURCE_DIR}/lib/librknnrt.so)

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

# 链接库 - 使用本地lib目录中的库（编译后复制）
target_link_libraries(${PROJECT_NAME}
    ${OpenCV_LIBS}
    ${RKNPU2_DST_LIB}
    pthread
)

# 创建lib目录并复制运行时库（与build目录同级）
add_custom_target(copy_libs ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_SOURCE_DIR}/lib
    COMMAND ${CMAKE_COMMAND} -E copy_if_different 
        ${RKNPU2_SRC_LIB}
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/
    COMMENT "Copying runtime libraries to lib/"
)
add_dependencies(${PROJECT_NAME} copy_libs)

# 安装
install(TARGETS ${PROJECT_NAME} DESTINATION .)
