cmake_minimum_required(VERSION 3.10)
project(test_chat_template)

# 设置C++标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置编译选项
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")

# 查找RKLLM库和头文件
# 优先使用项目自带的头文件
set(RKLLM_INCLUDE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/include")

# 查找库文件
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../rknn-llm/rkllm-runtime/Linux/librkllm_api/aarch64/librkllmrt.so")
    set(RKLLM_LIB_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../rknn-llm/rkllm-runtime/Linux/librkllm_api/aarch64/librkllmrt.so")
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../rknn-llm/rkllm-runtime/arm64/librkllmrt.so")
    set(RKLLM_LIB_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../rknn-llm/rkllm-runtime/arm64/librkllmrt.so")
elseif(EXISTS "/usr/lib/librkllmrt.so")
    set(RKLLM_LIB_PATH "/usr/lib/librkllmrt.so")
else()
    message(FATAL_ERROR "无法找到librkllmrt.so库文件")
endif()

message(STATUS "RKLLM库路径: ${RKLLM_LIB_PATH}")
message(STATUS "RKLLM头文件路径: ${RKLLM_INCLUDE_PATH}")

# 添加可执行文件
add_executable(test_chat_template
    src/main.cpp
)

# 添加头文件搜索路径
target_include_directories(test_chat_template PRIVATE
    ${RKLLM_INCLUDE_PATH}
)

# 链接RKLLM库
target_link_libraries(test_chat_template
    ${RKLLM_LIB_PATH}
    pthread
)

# 设置运行时库搜索路径（相对路径，指向与build同级的lib目录）
set_target_properties(test_chat_template PROPERTIES
    BUILD_WITH_INSTALL_RPATH TRUE
    INSTALL_RPATH "../lib"
)

# 安装目标
install(TARGETS test_chat_template
    RUNTIME DESTINATION bin
)
