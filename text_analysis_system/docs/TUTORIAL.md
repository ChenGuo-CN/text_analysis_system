# 文本分析系统使用教程

本教程详细介绍如何在RK3588开发板上搭建环境、编译和运行文本分析系统。

## 目录

1. [环境准备](#环境准备)
2. [项目结构](#项目结构)
3. [编译流程](#编译流程)
4. [配置说明](#配置说明)
5. [运行测试](#运行测试)
6. [常见问题](#常见问题)

## 环境准备

### 硬件要求

- **开发板**：ATK-DLRK3588 或兼容RK3588开发板
- **内存**：至少4GB RAM
- **存储**：至少10GB可用空间
- **NPU**：RK3588内置NPU（3个核心）

### 软件环境

确认系统信息：

```bash
# 查看系统版本
cat /etc/os-release
# 预期输出：PRETTY_NAME="Debian GNU/Linux 11 (bullseye)"

# 查看NPU驱动版本
cat /sys/kernel/debug/rknpu/version
# 预期输出：RKNPU driver: v0.9.8

# 查看RKNN运行时版本
strings /usr/lib/librknnrt.so | grep "librknnrt version"
# 预期输出：librknnrt version: 2.3.0

# 查看RKLLM运行时版本
strings /usr/lib/librkllmrt.so | grep "rkllm version"
# 预期输出：rkllm version: 1.x.x
```

### 安装依赖

```bash
# 更新软件包列表
sudo apt update

# 安装编译工具
sudo apt install -y cmake build-essential

# 安装OpenCV开发库
sudo apt install -y libopencv-dev

# 确认OpenCV安装
pkg-config --modversion opencv4
# 预期输出：4.5.1 或更高版本
```

### 准备模型文件

#### 1. OCR模型

将以下模型文件放置在 `model/` 目录：

```bash
mkdir -p model

# 复制OCR检测模型（需提前准备）
cp /path/to/ppocrv4_det_i8.rknn model/

# 复制OCR识别模型（需提前准备）
cp /path/to/ppocrv4_rec_fp16.rknn model/

# 复制字典文件
cp /path/to/ppocr_keys_v1.txt model/
```

#### 2. LLM模型

确保LLM模型存在于指定路径：

```bash
# 检查模型文件
ls -lh /userdata/models/Qwen3-1.7B_W8A8_RK3588_16k_2npu.rkllm
```

如果模型不存在，需要从PC端转换并上传到开发板。

## 项目结构

```
text_analysis_system/
├── src/                    # 源文件目录
│   ├── main.cpp           # 主程序入口
│   ├── config.cpp/h       # 配置管理模块
│   ├── text_queue.cpp/h   # 线程安全队列
│   ├── perf_monitor.cpp/h # 性能监控模块
│   ├── ocr_engine.cpp/h   # OCR引擎
│   ├── ocr_thread.cpp/h   # OCR工作线程
│   ├── llm_engine.cpp/h   # LLM引擎
│   ├── llm_thread.cpp/h   # LLM工作线程
│   ├── result_handler.cpp/h # 结果处理模块
│   ├── ppocr_system_npu2.cc # PPOCR系统实现
│   ├── postprocess.cc     # OCR后处理
│   ├── clipper.cc         # Clipper库（多边形处理）
│   ├── image_utils.c      # 图像工具
│   └── file_utils.c       # 文件工具
├── include/                # 头文件目录
│   ├── config.h
│   ├── text_queue.h
│   ├── perf_monitor.h
│   ├── ocr_engine.h
│   ├── ocr_thread.h
│   ├── llm_engine.h
│   ├── llm_thread.h
│   ├── result_handler.h
│   ├── ppocr_system.h
│   ├── common.h
│   ├── image_utils.h
│   ├── file_utils.h
│   ├── clipper.h
│   └── dict.h
├── config/                 # 配置文件目录
│   └── config.json        # 默认配置文件
├── model/                  # 模型文件目录
├── output/                 # 输出结果目录
├── docs/                   # 文档目录
├── CMakeLists.txt         # CMake构建配置
├── build.sh               # 编译脚本
└── README.md              # 项目说明
```

## 编译流程

### 方法一：使用编译脚本（推荐）

```bash
# 进入项目目录
cd /home/linaro/traffic_text_analysis_system/text_analysis_system

# 运行编译脚本
./build.sh
```

编译成功后，会生成 `text_analysis_system` 可执行文件。

### 方法二：手动编译

```bash
# 进入项目目录
cd /home/linaro/traffic_text_analysis_system/text_analysis_system

# 创建构建目录
mkdir -p build
cd build

# 运行CMake
cmake .. -DCMAKE_BUILD_TYPE=Release

# 编译
make -j$(nproc)

# 复制可执行文件到项目根目录
cp text_analysis_system ../
```

### 编译输出

编译成功后，项目根目录下会有：

```
text_analysis_system/
├── text_analysis_system    # 可执行文件
├── build/                  # 构建目录
│   ├── text_analysis_system
│   ├── librknnrt.so       # RKNN运行时库
│   └── librkllmrt.so      # RKLLM运行时库
└── ...
```

## 配置说明

### 配置文件位置

默认配置文件：`config/config.json`

### 配置项详解

#### 模型路径配置

```json
"model": {
    "det_model_path": "model/ppocrv4_det_i8.rknn",
    "rec_model_path": "model/ppocrv4_rec_fp16.rknn",
    "dict_path": "model/ppocr_keys_v1.txt",
    "llm_model_path": "/userdata/models/Qwen3-1.7B_W8A8_RK3588_16k_2npu.rkllm"
}
```

- `det_model_path`：OCR检测模型路径
- `rec_model_path`：OCR识别模型路径
- `dict_path`：OCR字典文件路径
- `llm_model_path`：LLM模型路径

#### LLM参数配置

```json
"llm": {
    "system_prompt": "你是一个专业的文本校对助手...",
    "max_new_tokens": 1024,
    "max_context_len": 2048,
    "temperature": 0.8,
    "top_p": 0.95,
    "top_k": 1,
    "repeat_penalty": 1.1
}
```

- `system_prompt`：系统提示词，可自定义修改以改变LLM行为
- `max_new_tokens`：最大生成token数
- `max_context_len`：最大上下文长度
- `temperature`：温度参数（0-2），越高越随机
- `top_p`：核采样参数（0-1）
- `top_k`：Top-K采样参数
- `repeat_penalty`：重复惩罚系数

#### OCR参数配置

```json
"ocr": {
    "threshold": 0.3,
    "box_threshold": 0.6,
    "db_unclip_ratio": 1.5,
    "use_dilate": false,
    "db_score_mode": "slow",
    "db_box_type": "poly"
}
```

- `threshold`：像素分数阈值
- `box_threshold`：文本框分数阈值
- `db_unclip_ratio`：DBNet解压缩比例
- `use_dilate`：是否进行膨胀操作
- `db_score_mode`：分数计算模式（slow/fast）
- `db_box_type`：文本框类型（poly/quad）

#### 队列配置

```json
"queue": {
    "max_size": 20
}
```

- `max_size`：队列最大容量（建议10-50）

#### 输出配置

```json
"output": {
    "result_dir": "output/results",
    "save_annotated_image": true
}
```

- `result_dir`：结果输出目录
- `save_annotated_image`：是否保存带标注的图像

#### 性能配置

```json
"performance": {
    "enable_timing": true,
    "log_level": 1
}
```

- `enable_timing`：是否启用耗时统计
- `log_level`：日志级别（0-3）

## 运行测试

### 基本用法

```bash
# 处理单张图片
./text_analysis_system /path/to/image.jpg

# 处理文件夹中的所有图片
./text_analysis_system /path/to/images/

# 使用自定义配置文件
./text_analysis_system /path/to/image.jpg --config my_config.json

# 显示帮助
./text_analysis_system --help
```

### 运行示例

```bash
# 示例1：处理单张图片
./text_analysis_system test_images/sample1.jpg

# 示例2：处理整个文件夹
./text_analysis_system test_images/

# 示例3：使用自定义配置
./text_analysis_system test_images/ --config config/custom.json
```

### 预期输出

```
========================================
  文本分析系统
========================================

[Main] 加载配置文件: config/config.json
[Config] 配置文件加载成功: config/config.json
========================================
  系统配置信息
========================================

[模型配置]
  检测模型: model/ppocrv4_det_i8.rknn
  识别模型: model/ppocrv4_rec_fp16.rknn
  字典文件: model/ppocr_keys_v1.txt
  LLM模型: /userdata/models/Qwen3-1.7B_W8A8_RK3588_16k_2npu.rkllm

[LLM配置]
  System Prompt: 你是一个专业的文本校对助手...
  Max New Tokens: 1024
  ...

[Main] 初始化OCR引擎...
[OCREngine] 正在初始化OCR引擎...
[OCREngine] 初始化检测模型: model/ppocrv4_det_i8.rknn
[OCREngine] 检测模型初始化成功 (NPU Core 2)
[OCREngine] 初始化识别模型: model/ppocrv4_rec_fp16.rknn
[OCREngine] 识别模型初始化成功 (NPU Core 2)
[OCREngine] OCR引擎初始化完成

[Main] 初始化LLM引擎...
[LLMEngine] 正在初始化LLM引擎...
[LLMEngine] LLM引擎初始化完成

[Main] 系统初始化完成

[Main] 启动工作线程...
[OCRThread] OCR线程已启动
[LLMThread] LLM线程已启动
[Main] 工作线程已启动

[Main] 开始处理 1 张图片...

[Main] 添加图片到队列: test_images/sample1.jpg

[Main] 等待处理完成...
[OCRThread] 处理图片: test_images/sample1.jpg
[OCRThread] OCR识别成功: test_images/sample1.jpg, 识别到 5 个文本
[LLMThread] 分析图片: test_images/sample1.jpg
[LLMThread] LLM分析成功: test_images/sample1.jpg
[ResultHandler] 结果已保存: output/results/sample1_20250329_103000.json
[Main] 已完成 1/1

[Main] 处理完成，共处理 1 张图片

========================================
  文本分析系统运行完成
========================================
```

### 查看结果

```bash
# 查看生成的结果文件
ls -lh output/results/

# 查看JSON结果
cat output/results/sample1_20250329_103000.json | python3 -m json.tool
```

## 常见问题

### Q1: 编译时找不到头文件

**问题**：
```
fatal error: rknn_api.h: 没有那个文件或目录
```

**解决**：
检查CMakeLists.txt中的RKNN API路径是否正确：
```cmake
set(RKNN_API_PATH ${CMAKE_SOURCE_DIR}/../rknn_model_zoo/3rdparty/rknpu2)
```

### Q2: 运行时找不到动态库

**问题**：
```
error while loading shared libraries: librknnrt.so: cannot open shared object file
```

**解决**：
方法1：设置LD_LIBRARY_PATH
```bash
export LD_LIBRARY_PATH=/usr/lib:$LD_LIBRARY_PATH
./text_analysis_system image.jpg
```

方法2：将库文件复制到系统目录
```bash
sudo cp build/librknnrt.so /usr/lib/
sudo cp build/librkllmrt.so /usr/lib/
sudo ldconfig
```

### Q3: 模型加载失败

**问题**：
```
[OCREngine] 检测模型初始化失败!
```

**解决**：
1. 检查模型文件是否存在
2. 检查模型文件路径是否正确
3. 检查模型文件权限

```bash
# 检查模型文件
ls -lh model/

# 检查文件权限
file model/ppocrv4_det_i8.rknn
```

### Q4: OCR识别结果为空

**问题**：
OCR识别成功但没有检测到文本

**解决**：
1. 调整OCR阈值参数
2. 检查图片质量
3. 检查图片格式是否支持

```json
{
  "ocr": {
    "threshold": 0.2,
    "box_threshold": 0.5
  }
}
```

### Q5: LLM分析超时

**问题**：
LLM分析时间过长或超时

**解决**：
1. 减少max_new_tokens
2. 优化system_prompt
3. 检查NPU核心是否正常工作

```json
{
  "llm": {
    "max_new_tokens": 512
  }
}
```

### Q6: 内存不足

**问题**：
程序运行时内存不足

**解决**：
1. 减小队列大小
2. 减少同时处理的图片数量
3. 关闭其他占用内存的程序

```json
{
  "queue": {
    "max_size": 10
  }
}
```

## 性能优化建议

1. **队列大小**：根据内存情况设置合适的队列大小（10-50）
2. **批量处理**：建议一次处理10-50张图片
3. **模型预热**：首次推理较慢，建议先运行一次测试
4. **NPU频率**：可以设置NPU频率以获得更好性能

```bash
# 设置NPU频率为最高
sudo bash -c 'echo performance > /sys/class/devfreq/fdab0000.npu/governor'
```

## 联系与支持

如有问题，请参考：
- [RKNN Model Zoo文档](https://github.com/rockchip-linux/rknn-model-zoo)
- [RKNN Toolkit2文档](https://github.com/rockchip-linux/rknn-toolkit2)
- [RKNN-LLM文档](https://github.com/rockchip-linux/rknn-llm)
