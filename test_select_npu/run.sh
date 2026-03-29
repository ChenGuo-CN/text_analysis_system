#!/bin/bash

# 设置库路径
export LD_LIBRARY_PATH=/home/linaro/traffic_text_analysis_system/rknn_model_zoo/3rdparty/rknpu2/Linux/aarch64:$LD_LIBRARY_PATH

# 运行程序
./build/test_select_npu /home/linaro/traffic_text_analysis_system/datasets/test.jpg
