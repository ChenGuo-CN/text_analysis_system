# 文本分析系统 (Text Analysis System)

基于RK3588开发板的端到端文本分析系统，整合PPOCR文字识别和RKLLM大语言模型，实现图片文字的自动提取与错漏字分析。

## 功能特性

- **PPOCR文字识别**：使用PPOCR模型对图片进行文字检测与识别
- **RKLLM文本分析**：使用大语言模型分析文本中的错漏字问题
- **多线程架构**：OCR检测线程 + LLM分析线程 + 结果处理线程
- **线程安全队列**：支持上限20张图片的文本内容队列
- **JSON结果存储**：支持单张图片结果和批量汇总
- **性能统计**：毫秒级OCR和LLM推理耗时统计
- **可配置参数**：通过JSON配置文件自定义系统参数

## 系统架构

```
┌─────────────────┐     ┌─────────────────┐     ┌─────────────────┐
│   图片输入队列   │────▶│   OCR检测线程   │────▶│  OCR结果队列    │
│  (StringQueue)  │     │  (OCRThread)    │     │(OCRResultQueue) │
└─────────────────┘     └─────────────────┘     └─────────────────┘
                                                        │
                                                        ▼
┌─────────────────┐     ┌─────────────────┐     ┌─────────────────┐
│  结果处理/保存   │◀────│   LLM分析线程   │◀────│  处理结果队列   │
│(ResultHandler)  │     │  (LLMThread)    │     │(ProcessingResult│
└─────────────────┘     └─────────────────┘     │     Queue)      │
                                                └─────────────────┘
```

## NPU核心分配

- **RKLLM模型**：使用2个NPU核心（模型导出时已配置）
- **PPOCR模型**：检测和识别模型共享1个NPU核心（Core 2）

## 环境要求

- **硬件**：RK3588开发板
- **操作系统**：Debian GNU/Linux 11 (bullseye)
- **NPU驱动**：RKNPU driver v0.9.8
- **依赖库**：
  - OpenCV 4.5+
  - librknnrt.so (RKNN Runtime)
  - librkllmrt.so (RKLLM Runtime)

## 快速开始

### 1. 克隆项目

```bash
cd /home/linaro/traffic_text_analysis_system/text_analysis_system
```

### 2. 编译项目

```bash
./build.sh
```

### 3. 准备模型文件

确保以下模型文件存在：
- `model/ppocrv4_det_i8.rknn` - OCR检测模型
- `model/ppocrv4_rec_fp16.rknn` - OCR识别模型
- `model/ppocr_keys_v1.txt` - 字典文件
- `/userdata/models/Qwen3-1.7B_W8A8_RK3588_16k_2npu.rkllm` - LLM模型

### 4. 运行程序

```bash
# 处理单张图片
./text_analysis_system /path/to/image.jpg

# 处理文件夹中的所有图片
./text_analysis_system /path/to/images/

# 使用自定义配置文件
./text_analysis_system /path/to/image.jpg --config my_config.json
```

## 配置文件

配置文件位于 `config/config.json`，包含以下可配置项：

```json
{
  "model": {
    "det_model_path": "model/ppocrv4_det_i8.rknn",
    "rec_model_path": "model/ppocrv4_rec_fp16.rknn",
    "dict_path": "model/ppocr_keys_v1.txt",
    "llm_model_path": "/userdata/models/Qwen3-1.7B_W8A8_RK3588_16k_2npu.rkllm"
  },
  "llm": {
    "system_prompt": "你是一个专业的文本校对助手...",
    "max_new_tokens": 1024,
    "max_context_len": 2048,
    "temperature": 0.8,
    "top_p": 0.95,
    "top_k": 1,
    "repeat_penalty": 1.1
  },
  "ocr": {
    "threshold": 0.3,
    "box_threshold": 0.6,
    "db_unclip_ratio": 1.5
  },
  "queue": {
    "max_size": 20
  },
  "output": {
    "result_dir": "output/results",
    "save_annotated_image": true
  },
  "performance": {
    "enable_timing": true,
    "log_level": 1
  }
}
```

## 输出结果

### 单张图片结果

结果保存在 `output/results/` 目录下，格式如下：

```json
{
  "image_path": "图片路径",
  "timestamp": "2025-03-29 10:30:00",
  "ocr_result": {
    "success": true,
    "text_count": 5,
    "texts": [
      {
        "text": "识别的文本",
        "confidence": 0.95,
        "box": {
          "left_top": [100, 100],
          "right_top": [200, 100],
          "right_bottom": [200, 150],
          "left_bottom": [100, 150]
        }
      }
    ],
    "inference_time_ms": 672.5
  },
  "llm_analysis": {
    "success": true,
    "analysis_text": "分析结果文本",
    "inference_time_ms": 8150.3
  }
}
```

### 批量汇总结果

批量处理完成后会生成汇总文件：

```json
{
  "summary": {
    "total_count": 10,
    "success_count": 9,
    "failed_count": 1,
    "avg_ocr_time_ms": 650.2,
    "avg_llm_time_ms": 8200.5,
    "total_time_ms": 88500.7
  },
  "results": [
    {
      "image_path": "图片路径",
      "ocr_success": true,
      "llm_success": true,
      "ocr_time_ms": 672.5,
      "llm_time_ms": 8150.3
    }
  ]
}
```

## 目录结构

```
text_analysis_system/
├── src/                    # 源文件
│   ├── main.cpp           # 主程序
│   ├── config.cpp/h       # 配置管理
│   ├── text_queue.cpp/h   # 线程安全队列
│   ├── perf_monitor.cpp/h # 性能监控
│   ├── ocr_engine.cpp/h   # OCR引擎
│   ├── ocr_thread.cpp/h   # OCR线程
│   ├── llm_engine.cpp/h   # LLM引擎
│   ├── llm_thread.cpp/h   # LLM线程
│   └── result_handler.cpp/h # 结果处理
├── include/                # 头文件
├── config/                 # 配置文件
│   └── config.json
├── model/                  # 模型文件
├── output/                 # 输出结果
├── docs/                   # 文档
├── CMakeLists.txt         # CMake配置
├── build.sh               # 编译脚本
└── README.md              # 项目说明
```

## 性能指标

在RK3588开发板上的典型性能：

- **OCR单张图片推理时间**：约600-800ms
- **LLM首token响应时间**：约100-200ms
- **LLM生成速度**：约10-15 tokens/秒
- **端到端处理时间**：约8-10秒/张图片

## 许可证

本项目基于RKNN Model Zoo和RKLLM Toolkit开发，遵循相关许可证协议。

## 致谢

- [RKNN Model Zoo](https://github.com/rockchip-linux/rknn-model-zoo)
- [RKNN Toolkit2](https://github.com/rockchip-linux/rknn-toolkit2)
- [RKNN-LLM](https://github.com/rockchip-linux/rknn-llm)
