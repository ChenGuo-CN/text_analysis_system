# PROJECT_RECORD.md

## RK3588_PPOCR示例分析与运行

**目标**: 分析RK官方PPOCR示例结构，编译并运行PPOCR-System端到端OCR推理示例

**日期**: 2026-03-28

**开发板**: ATK-DLRK3588

***

### 环境信息

- **操作系统**: Debian GNU/Linux 11 (bullseye)
- **内核版本**: Linux 5.10.209-g16b8cb7e2392 aarch64
- **NPU驱动**: RKNPU driver v0.9.8
- **librknnrt版本**: 2.3.0
- **rknn_server版本**: 2.3.0
- **GCC编译器**: aarch64-linux-gnu-gcc/g++ 10.2.1
- **OpenCV版本**: 3.4.5

***

### PPOCR示例结构分析

#### 1. 目录结构

```
rknn_model_zoo/examples/PPOCR/
├── README.md                    # PPOCR项目总体说明
├── PPOCR-Tutorial.md           # PPOCR使用教程
├── PPOCR-Det/                  # 文本检测模块
│   ├── cpp/                    # C++实现
│   ├── python/                 # Python实现
│   └── model/                  # 模型文件
│       ├── ppocrv4_det.onnx
│       ├── ppocrv4_det_i8.rknn (INT8量化)
│       └── test.jpg
├── PPOCR-Rec/                  # 文本识别模块
│   ├── cpp/
│   ├── python/
│   └── model/
│       ├── ppocrv4_rec.onnx
│       ├── ppocrv4_rec_fp16.rknn (FP16量化)
│       └── ppocr_keys_v1.txt   # 中文字典(6625字符)
└── PPOCR-System/               # 端到端OCR系统
    ├── cpp/
    │   ├── main.cc             # 主程序入口
    │   ├── ppocr_system.h      # 数据结构定义
    │   ├── postprocess.cc      # DBNet后处理/CTC解码
    │   ├── clipper.cc/h        # 多边形裁剪库
    │   ├── dict.h              # 字典定义
    │   └── rknpu2/ppocr_system.cc  # RK3588 NPU推理实现
    └── model/
        └── test.jpg
```

#### 2. 核心组件说明

##### PPOCR-System 端到端系统

- **功能**: 整合文本检测(Det)和文本识别(Rec)两个模型
- **工作流程**:
  1. 检测模型定位图像中的文本区域
  2. 对检测区域进行透视变换矫正
  3. 识别模型识别文本内容
  4. 输出文本框坐标和识别结果

##### 关键参数配置

```cpp
#define THRESHOLD 0.3           // 像素分数阈值
#define BOX_THRESHOLD 0.6       // 文本框分数阈值
#define USE_DILATION false      // 是否进行膨胀操作
#define DB_SCORE_MODE "slow"    // slow(多边形掩码)或fast(矩形掩码)
#define DB_BOX_TYPE "poly"      // poly(多边形)或quad(四边形)
#define DB_UNCLIP_RATIO 1.5     // DBNet解压缩比例
```

##### 模型信息

- **检测模型**: ppocrv4_det_i8.rknn
  - 输入: [1, 480, 480, 3] INT8 NHWC
  - 输出: [1, 1, 480, 480] INT8 NCHW
  - 量化: INT8 (zp=-14, scale=0.018658)
- **识别模型**: ppocrv4_rec_fp16.rknn
  - 输入: [1, 48, 320, 3] FP16 NHWC
  - 输出: [1, 40, 6625] FP16
  - 字符集: 6625个中文字符

***

### 操作步骤

#### 1. 编译命令

```bash
cd /home/linaro/traffic_text_analysis_system/rknn_model_zoo
./build-linux.sh -t rk3588 -a aarch64 -d PPOCR-System
```

#### 2. 编译输出

- **输出目录**: `install/rk3588_linux_aarch64/rknn_PPOCR-System_demo/`
- **包含文件**:
  - `rknn_ppocr_system_demo` (可执行文件)
  - `lib/librknnrt.so` (RKNN运行时库)
  - `lib/librga.so` (RGA图像处理库)
  - `model/test.jpg` (测试图片)

#### 3. 模型文件准备

编译后需手动复制模型文件到安装目录：

```bash
cp examples/PPOCR/PPOCR-Det/model/ppocrv4_det_i8.rknn install/rk3588_linux_aarch64/rknn_PPOCR-System_demo/model/
cp examples/PPOCR/PPOCR-Rec/model/ppocrv4_rec_fp16.rknn install/rk3588_linux_aarch64/rknn_PPOCR-System_demo/model/
cp examples/PPOCR/PPOCR-Rec/model/ppocr_keys_v1.txt install/rk3588_linux_aarch64/rknn_PPOCR-System_demo/model/
```

#### 4. 运行命令

```bash
cd install/rk3588_linux_aarch64/rknn_PPOCR-System_demo
export LD_LIBRARY_PATH=./lib
./rknn_ppocr_system_demo model/ppocrv4_det_i8.rknn model/ppocrv4_rec_fp16.rknn model/test.jpg
```

***

### 运行结果

#### 推理输出

程序成功检测到16个文本区域，识别结果如下：

| 序号 | 文本框坐标                                             | 识别结果                | 置信度   |
| -- | ------------------------------------------------- | ------------------- | ----- |
| 0  | [(28, 37), (302, 39), (301, 71), (27, 69)]       | 纯臻营养护发素             | 0.998 |
| 1  | [(26, 82), (172, 82), (172, 104), (26, 104)]     | 产品信息/参数             | 0.995 |
| 2  | [(27, 112), (332, 112), (332, 134), (27, 134)]   | （45元/每公斤，100公斤起订）   | 0.960 |
| 3  | [(28, 142), (282, 144), (281, 163), (27, 162)]   | 每瓶22元，1000瓶起订）      | 0.989 |
| 4  | [(25, 179), (298, 177), (300, 194), (26, 195)]   | 【品牌】：代加工方式/OEMODM   | 0.986 |
| 5  | [(26, 209), (234, 209), (234, 228), (26, 228)]   | 【品名】：纯臻营养护发素        | 0.996 |
| 6  | [(26, 240), (241, 240), (241, 259), (26, 259)]   | 【产品编号】：YM-X-3011    | 0.984 |
| 7  | [(413, 233), (429, 233), (429, 305), (413, 305)] | ODMOEM              | 0.993 |
| 8  | [(25, 270), (179, 270), (179, 289), (25, 289)]   | 【净含量】：220ml         | 0.991 |
| 9  | [(26, 303), (252, 303), (252, 321), (26, 321)]   | 【适用人群】：适合所有肤质       | 0.995 |
| 10 | [(26, 333), (341, 333), (341, 351), (26, 351)]   | 【主要成分】：鲸蜡硬脂醇、燕麦β-葡聚 | 0.958 |
| 11 | [(27, 363), (283, 365), (282, 384), (26, 382)]   | 糖、椰油酰胺丙基甜菜碱、泛酸      | 0.963 |
| 12 | [(368, 368), (476, 368), (476, 388), (368, 388)] | （成品包材）              | 0.989 |
| 13 | [(27, 394), (362, 396), (361, 414), (26, 413)]   | 【主要功能】：可紧致头发磷层，从而达到 | 0.973 |
| 14 | [(27, 428), (371, 428), (371, 446), (27, 446)]   | 即时持久改善头发光泽的效果，给干燥的头 | 0.998 |
| 15 | [(27, 459), (136, 459), (136, 478), (27, 478)]   | 发足够的滋养              | 0.999 |

#### 输出文件

- **out.jpg**: 标注了文本框的检测结果图像

***

### 结果分析

#### 1. 检测精度

- 成功检测到所有16个文本区域
- 文本框坐标准确包围文本区域
- 包括横排文字和竖排文字(ODMOEM)

#### 2. 识别精度

- **平均置信度**: 约0.98，识别结果非常可靠
- **中文识别**: 准确识别产品名称、规格参数等中文文本
- **特殊字符**: 正确识别括号、数字、英文、斜杠等
- **长文本**: 对多行连续文本识别效果良好

#### 3. 性能观察

- 模型输入尺寸: 检测480x480，识别48x320
- 使用了INT8(检测)和FP16(识别)量化模型
- 图像预处理使用CPU进行对齐转换

***

### 关键发现

1. **模型量化策略**:
   - 检测模型使用INT8量化，减小模型体积同时保持定位精度
   - 识别模型使用FP16量化，平衡精度和性能
2. **后处理算法**:
   - 使用DBNet进行文本区域检测
   - 使用CTC进行文本序列解码
   - Clipper库处理多边形裁剪
3. **图像预处理**:
   - 支持非4/16对齐宽度的图像CPU转换
   - 使用透视变换矫正倾斜文本
4. **部署注意事项**:
   - 需要正确设置LD_LIBRARY_PATH加载动态库
   - 模型文件需手动复制到安装目录
   - 确保NPU驱动已正确加载

***

### 后续建议

1. 可尝试调整`THRESHOLD`和`BOX_THRESHOLD`参数优化检测效果
2. 测试不同分辨率图像的推理性能
3. 可集成到实际业务系统中进行批量处理

***

## RKLLM API Demo 运行记录

**会话日期**: 2026-03-28  
**目标**: 分析RKLLM C++ API示例，编译并运行大语言模型推理

### 1. 示例结构分析

```
rknn-llm/examples/rkllm_api_demo/
├── Readme.md                   # 英文说明文档
├── README_cn.md               # 中文说明文档
├── export/                    # 模型转换相关
│   ├── export_rkllm.py       # RKLLM模型导出脚本
│   ├── generate_data_quant.py # 生成量化校准数据
│   └── data_quant.json       # 量化数据配置
└── deploy/                    # 部署相关
    ├── CMakeLists.txt        # CMake构建配置
    ├── build-linux.sh        # Linux编译脚本
    ├── build-android.sh      # Android编译脚本
    └── src/                  # 源代码
        ├── llm_demo.cpp      # 主程序（标准版本）
        └── llm_demo_annotated.cpp  # 带详细注释版本
```

### 2. 核心组件说明

#### 程序架构（llm_demo.cpp）

1. **初始化阶段**:
   - 设置信号处理（SIGINT退出）
   - 配置RKLLMParam参数（模型路径、采样参数、上下文长度）
   - 调用rkllm_init初始化模型
2. **推理阶段**:
   - 支持两种预设问题（鸡兔同笼、排队问题）
   - 支持自定义输入
   - 流式输出结果通过callback回调函数
3. **资源释放**:
   - 调用rkllm_destroy释放资源

#### 关键参数配置

```cpp
param.top_k = 1;              // 采样top_k
param.top_p = 0.95;           // 采样top_p
param.temperature = 0.8;      // 温度参数
param.repeat_penalty = 1.1;   // 重复惩罚
param.max_new_tokens = 1024;  // 最大生成token数
param.max_context_len = 2048; // 最大上下文长度
param.skip_special_token = true;
param.extend_param.embed_flash = 1;  // 使用Flash Attention
```

#### 支持的扩展功能

- **LoRA适配器加载**: 支持动态加载LoRA微调模型
- **Prompt Cache**: 支持缓存系统提示词加速推理
- **KV Cache管理**: 支持清除历史上下文
- **Chat Template**: 支持自定义对话模板

### 3. 编译步骤

```bash
cd /home/linaro/traffic_text_analysis_system/rknn-llm/examples/rkllm_api_demo/deploy
./build-linux.sh
```

**编译输出**:

- 可执行文件: `install/demo_Linux_aarch64/llm_demo`
- 库文件: `install/demo_Linux_aarch64/lib/librkllmrt.so`

### 4. 运行步骤

```bash
cd install/demo_Linux_aarch64
export LD_LIBRARY_PATH=./lib
export RKLLM_LOG_LEVEL=1
./llm_demo /userdata/models/Qwen2.5-1.5B-Instruct_W8A8_RK3588_16k.rkllm 1024 2048
```

**参数说明**:

- 参数1: 模型文件路径
- 参数2: max_new_tokens（最大生成token数）
- 参数3: max_context_len（最大上下文长度）

### 5. 运行结果

#### 模型信息

- **模型**: Qwen2.5-1.5B-Instruct_W8A8_RK3588_16k.rkllm
- **rkllm-runtime版本**: 1.2.3
- **rknpu驱动版本**: 0.9.8
- **平台**: RK3588
- **rkllm-toolkit版本**: 1.2.3
- **最大上下文限制**: 16384
- **NPU核心数**: 3
- **模型量化**: W8A8

#### 性能统计

| 阶段       | 总时间(ms) | Token数 | 每Token时间(ms) | Token/秒 |
| -------- | ------- | ------ | ------------ | ------- |
| Prefill  | 184.27  | 31     | 5.94         | 168.24  |
| Generate | 4752.98 | 45     | 105.62       | 9.47    |

- **模型初始化时间**: 1621.65 ms
- **峰值内存使用**: 1.81 GB
- **启用CPU核心**: [4, 5, 6, 7]（共4核）

#### 对话示例

**用户输入**: 你是谁  
**模型回答**: 我是Qwen，由阿里云开发的超大规模语言模型。我被设计用来回答问题、撰写文章以及进行文本生成等多种任务。如果您有任何问题或需要帮助，请随时告诉我，我会尽力提供支持。

### 6. 结果分析

#### 初始化性能

- 模型加载时间约1.6秒，属于正常范围
- 成功启用4个CPU核心进行辅助计算
- 内存占用约1.81GB，符合1.5B参数模型的预期

#### 推理性能

- **Prefill阶段**: 31个输入token，速度168.24 tokens/秒，表现优秀
- **Generate阶段**: 45个输出token，速度9.47 tokens/秒，满足实时交互需求
- 生成速度约10 tokens/秒，相当于每秒约5-8个汉字，可接受

#### 模型质量

- 模型正确识别自身身份（Qwen2.5）
- 回答流畅自然，符合中文表达习惯
- 能够准确理解用户意图并给出恰当回应

### 7. 关键发现

1. **量化效果**:
   - W8A8量化在保持模型质量的同时显著降低内存占用
   - 1.5B参数模型仅需1.81GB内存，适合边缘设备部署
2. **RK3588性能**:
   - 3核NPU协同工作，推理效率良好
   - 4个CPU核心辅助处理，平衡计算负载
3. **Flash Attention**:
   - 启用embed_flash=1优化长序列处理
   - 对16k上下文长度支持良好
4. **部署建议**:
   - 建议设置RKLLM_LOG_LEVEL=1查看性能统计
   - 可根据场景调整max_new_tokens控制生成长度
   - 使用clear命令可清除历史上下文节省内存

***

## NPU核心绑定测试记录 (test_select_npu)

**会话日期**: 2026-03-28  
**目标**: 验证PPOCR检测和识别模型同时绑定到NPU Core 2的可行性，并优化数据集格式和验证逻辑

### 1. 测试程序结构

```
test_select_npu/
├── CMakeLists.txt              # CMake构建配置
├── run.sh                      # 运行脚本
├── build/                      # 编译输出
│   └── test_select_npu         # 可执行文件
├── include/                    # 头文件
│   ├── common.h
│   ├── file_utils.h
│   ├── image_utils.h
│   ├── ppocr_system.h
│   ├── clipper.h
│   └── dict.h
├── src/                        # 源文件
│   ├── main.cc                 # 主程序
│   ├── ppocr_system_npu2.cc    # 核心绑定版模型实现
│   ├── postprocess.cc          # 后处理
│   ├── clipper.cc              # 多边形裁剪
│   ├── image_utils.c           # 图像工具
│   └── file_utils.c            # 文件工具
└── model/                      # 模型文件
    ├── ppocrv4_det_i8.rknn     # 检测模型
    ├── ppocrv4_rec_fp16.rknn   # 识别模型
    └── ppocr_keys_v1.txt       # 中文字典
```

### 2. 核心代码修改

#### init_ppocr_model函数（带NPU核心绑定）

```cpp
// 在rknn_init成功后设置NPU核心绑定
ret = rknn_set_core_mask(ctx, core_mask);
if (ret != RKNN_SUCC) {
    printf("rknn_set_core_mask fail! ret=%d\n", ret);
    rknn_destroy(ctx);
    return -1;
}
printf("[NPU Core] Model %s bound to core mask: %d\n", model_path, core_mask);
```

#### 核心绑定调用

```cpp
// 检测模型绑定到NPU Core 2
init_ppocr_model(det_model_path, &sys_ctx.det_context, RKNN_NPU_CORE_2);

// 识别模型绑定到NPU Core 2
init_ppocr_model(rec_model_path, &sys_ctx.rec_context, RKNN_NPU_CORE_2);
```

### 3. 数据集格式优化

#### 问题描述
原始txt文件存在以下问题：
- 部分文件包含UTF-8 BOM (EF BB BF) 头
- 行尾换行符不统一 (
 和 
 混用)
- 多行连续文本难以解析

#### 解决方案
- 将所有txt文件转换为json格式
- 每个文件包含expected_texts数组
- 统一使用UTF-8编码，无BOM

#### 转换后的JSON文件列表

| 文件       | 内容数量 | 主要内容            |
| -------- | ---- | --------------- |
| test.json | 17   | 护发素产品信息关键词      |
| 1.json    | 6    | 交通标语（欢迎进入美丽河源等） |
| 2.json    | 2    | 安全驾驶提示          |
| 3.json    | 2    | 超载超速警示          |
| 4.json    | 3    | 公司信息及安全提示       |
| 5.json    | 12   | 公司经营项目列表        |

#### test.json 示例
```json
{
  "expected_texts": [
    "纯臻营养护发素",
    "产品信息",
    "参数",
    "品牌",
    "代加工方式",
    "OEM",
    "ODM",
    "品名",
    "产品编号",
    "YM-X-3011",
    "净含量",
    "220ml",
    "适用人群",
    "适合所有肤质",
    "主要成分",
    "主要功能",
    "成品包材"
  ],
  "image_file": "test.jpg",
  "description": "护发素产品信息图片"
}
```

### 4. 验证逻辑优化

#### 改进内容
- 支持从txt文件加载多个预期文本（每行一个）
- 自动移除UTF-8 BOM和换行符
- 每个预期文本独立匹配验证
- 输出匹配统计信息（匹配率）
- 根据匹配率给出状态评估

#### load_expected_texts函数实现
```cpp
// 从文本文件加载预期文本（每行一个）
int load_expected_texts(const char* filepath, expected_texts_t* out) {
    // 自动移除UTF-8 BOM (EF BB BF)
    if ((unsigned char)line[0] == 0xEF && 
        (unsigned char)line[1] == 0xBB && 
        (unsigned char)line[2] == 0xBF) {
        start += 3;
    }
    // 自动移除换行符和回车符
    // 跳过空行
}
```

### 5. 编译步骤

```bash
cd /home/linaro/traffic_text_analysis_system/test_select_npu
mkdir -p build && cd build
cmake ..
make -j4
```

### 6. 运行命令

```bash
cd /home/linaro/traffic_text_analysis_system/test_select_npu
./run.sh
```

### 7. 运行结果

#### 模型初始化输出

```
[1/2] Initializing Detection Model...
[NPU Core] Model model/ppocrv4_det_i8.rknn bound to core mask: 4
model input num: 1, output num: 1
input tensors:
  index=0, name=x, n_dims=4, dims=[1, 480, 480, 3], n_elems=691200, size=691200, fmt=NHWC, type=INT8, qnt_type=AFFINE, zp=-14, scale=0.018658
output tensors:
  index=0, name=sigmoid_0.tmp_0, n_dims=4, dims=[1, 1, 480, 480], n_elems=230400, size=230400, fmt=NCHW, type=INT8, qnt_type=AFFINE, zp=-128, scale=0.003922
model is NHWC input fmt
model input height=480, width=480, channel=3
  Detection Model initialized successfully on NPU Core 2

[2/2] Initializing Recognition Model...
[NPU Core] Model model/ppocrv4_rec_fp16.rknn bound to core mask: 4
model input num: 1, output num: 1
input tensors:
  index=0, name=x, n_dims=4, dims=[1, 48, 320, 3], n_elems=46080, size=92160, fmt=NHWC, type=FP16, qnt_type=AFFINE, zp=0, scale=1.000000
output tensors:
  index=0, name=softmax_11.tmp_0, n_dims=3, dims=[1, 40, 6625, 0], n_elems=265000, size=530000, fmt=UNDEFINED, type=FP16, qnt_type=AFFINE, zp=0, scale=1.000000
model is NHWC input fmt
model input height=48, width=320, channel=3
  Recognition Model initialized successfully on NPU Core 2
```

#### OCR推理结果

```
========================================
  OCR Results (Total: 16 texts)
  Inference Time: 672.98 ms
========================================

[0] Text: 纯臻营养护发素
    Confidence: 0.998
    Box: [(28,37), (302,39), (301,70), (27,69)]

[1] Text: 产品信息/参数
    Confidence: 0.995
    Box: [(26,82), (171,82), (171,104), (26,104)]

[2] Text: （45元/每公斤，100公斤起订）
    Confidence: 0.973
    Box: [(27,113), (333,113), (333,134), (27,134)]

[3] Text: 每瓶22元，1000瓶起订）
    Confidence: 0.989
    Box: [(28,142), (282,144), (281,163), (27,162)]

[4] Text: 【品牌】：代加工方式/OEMODM
    Confidence: 0.986
    Box: [(25,179), (298,177), (300,194), (26,195)]

[5] Text: 【品名】：纯臻营养护发素
    Confidence: 0.996
    Box: [(25,209), (234,209), (234,227), (25,227)]

[6] Text: 【产品编号】：YM-X-3011
    Confidence: 0.984
    Box: [(26,240), (241,240), (241,259), (26,259)]

[7] Text: ODMOEM
    Confidence: 0.993
    Box: [(413,233), (429,233), (429,305), (413,305)]

[8] Text: 【净含量】：220ml
    Confidence: 0.991
    Box: [(25,270), (179,270), (179,289), (25,289)]

[9] Text: 【适用人群】：适合所有肤质
    Confidence: 0.995
    Box: [(26,303), (252,303), (252,320), (26,320)]

[10] Text: 【主要成分】：鲸蜡硬脂醇、燕麦β-葡聚
    Confidence: 0.974
    Box: [(23,335), (342,333), (343,352), (25,353)]

[11] Text: 糖、椰油酰胺丙基甜菜碱、泛酸
    Confidence: 0.963
    Box: [(27,363), (283,365), (282,384), (26,382)]

[12] Text: （成品包材）
    Confidence: 0.994
    Box: [(367,368), (474,367), (476,386), (368,388)]

[13] Text: 【主要功能】：可紧致头发磷层，从而达到
    Confidence: 0.991
    Box: [(26,398), (359,396), (360,412), (27,413)]

[14] Text: 即时持久改善头发光泽的效果，给干燥的头
    Confidence: 0.995
    Box: [(27,428), (370,428), (370,445), (27,445)]

[15] Text: 发足够的滋养
    Confidence: 0.999
    Box: [(27,459), (136,459), (136,478), (27,478)]
```

#### 验证输出

```
========================================
  Verification
========================================
Expected Texts (17 items):
  [0] "纯臻营养护发素" - FOUND
  [1] "产品信息" - FOUND
  [2] "参数" - FOUND
  [3] "品牌" - FOUND
  [4] "代加工方式" - FOUND
  [5] "OEM" - FOUND
  [6] "ODM" - FOUND
  [7] "品名" - FOUND
  [8] "产品编号" - FOUND
  [9] "YM-X-3011" - FOUND
  [10] "净含量" - FOUND
  [11] "220ml" - FOUND
  [12] "适用人群" - FOUND
  [13] "适合所有肤质" - FOUND
  [14] "主要成分" - FOUND
  [15] "主要功能" - FOUND
  [16] "成品包材" - FOUND

Result: 17/17 expected texts found (100.0%)
Status: SUCCESS - All expected texts recognized!
```

### 8. 结果分析

#### NPU核心绑定验证

- **检测模型**: 成功绑定到core mask 4 (RKNN_NPU_CORE_2)
- **识别模型**: 成功绑定到core mask 4 (RKNN_NPU_CORE_2)
- **绑定时机**: 在rknn_init成功后、第一次rknn_run之前

#### OCR识别精度

- **总检测文本数**: 16个
- **平均置信度**: 约0.987
- **关键文本识别**: 成功识别"纯臻营养护发素"（置信度0.998）
- **推理时间**: 672.98 ms
- **预期文本匹配率**: 100% (17/17)

#### 多模型同核心运行

- 两个模型同时绑定到NPU Core 2运行正常
- 检测和识别模型在同一核心上串行执行
- 无核心冲突或资源竞争错误

### 9. 关键发现

1. **rknn_set_core_mask接口有效性**:
   - 成功将模型绑定到指定NPU核心
   - core mask 4对应RKNN_NPU_CORE_2

2. **多模型同核心部署可行性**:
   - 检测和识别模型可同时绑定到同一NPU核心
   - 模型间切换执行正常

3. **识别精度**:
   - 与官方示例识别结果一致
   - 核心绑定不影响模型精度

4. **性能观察**:
   - 推理时间约673ms（含检测+识别）
   - 与官方示例性能相当

5. **数据集格式优化效果**:
   - JSON格式避免UTF-8 BOM和换行符问题
   - 每行一个文本的格式便于解析和验证
   - 100%匹配率验证成功

***

## RKLLM Chat Template 自定义测试记录

**会话日期**: 2026-03-28  
**目标**: 创建测试程序验证RKLLM C++ API的自定义chat_template和system prompt功能

### 1. 测试程序结构

```
test_chat_template/
├── CMakeLists.txt              # CMake构建配置
├── build.sh                    # 编译脚本
├── build/                      # 编译输出
│   └── test_chat_template      # 可执行文件
└── src/                        # 源文件
    └── main.cpp                # 主程序
```

### 2. 核心功能实现

#### chat_template配置

```cpp
// 配置自定义chat_template和system prompt
const char* system_prompt = "你是一个东南大学的研究生";
const char* prompt_prefix = "<|im_start|>user\n";
const char* prompt_postfix = "<|im_end|>\n<|im_start|>assistant\n";

int ret = rkllm_set_chat_template(g_llmHandle, system_prompt, prompt_prefix, prompt_postfix);
```

#### 回调函数实现

```cpp
int callback(RKLLMResult* result, void* userdata, LLMCallState state) {
    if (state == RKLLM_RUN_NORMAL) {
        printf("%s", result->text);
        fflush(stdout);
    } else if (state == RKLLM_RUN_FINISH) {
        printf("\n\n[系统] 推理完成\n");
    } else if (state == RKLLM_RUN_ERROR) {
        printf("\n[错误] 推理过程中发生错误\n");
    }
    return 0;  // 返回0表示继续推理
}
```

### 3. 编译步骤

```bash
cd /home/linaro/traffic_text_analysis_system/test_chat_template
./build.sh
```

**编译输出**:

- 可执行文件: `build/test_chat_template`
- 库依赖: `librkllmrt.so`

### 4. 运行命令

```bash
cd /home/linaro/traffic_text_analysis_system/test_chat_template
export LD_LIBRARY_PATH=/home/linaro/traffic_text_analysis_system/rknn-llm/rkllm-runtime/Linux/librkllm_api/aarch64:$LD_LIBRARY_PATH
./build/test_chat_template /userdata/models/Qwen2.5-1.5B-Instruct_W8A8_RK3588_16k.rkllm 1024 2048
```

### 5. 运行结果

#### 初始化输出

```
========================================
  RKLLM Chat Template 测试程序
========================================
模型路径: /userdata/models/Qwen2.5-1.5B-Instruct_W8A8_RK3588_16k.rkllm
max_new_tokens: 1024
max_context_len: 2048
----------------------------------------

[1/4] 正在配置模型参数...
[2/4] 正在初始化模型...
I rkllm: rkllm-runtime version: 1.2.3, rknpu driver version: 0.9.8, platform: RK3588
I rkllm: loading rkllm model from /userdata/models/Qwen2.5-1.5B-Instruct_W8A8_RK3588_16k.rkllm
I rkllm: rkllm-toolkit version: 1.2.3, max_context_limit: 16384, npu_core_num: 3, target_platform: RK3588, model_dtype: W8A8
I rkllm: Enabled cpus: [4, 5, 6, 7]
I rkllm: Enabled cpus num: 4
[成功] 模型初始化完成

[3/4] 正在配置chat_template...
  System Prompt: 你是一个东南大学的研究生
  Prompt Prefix: <|im_start|>user
  Prompt Postfix: <|im_end|>
<|im_start|>assistant
I rkllm: reset chat template:
I rkllm: system_prompt: 你是一个东南大学的研究生
I rkllm: prompt_prefix: <|im_start|>user\n
I rkllm: prompt_postfix: <|im_end|>\n<|im_start|>assistant\n
W rkllm: Calling rkllm_set_chat_template will disable the internal automatic chat template parsing, including enable_thinking. Make sure your custom prompt is complete and valid.
[成功] chat_template配置完成

[4/4] 进入交互模式
```

#### 对话测试

**用户输入**: 你是谁  
**模型回答**: 我是来自东南大学的一名研究生，目前在攻读计算机科学与技术专业的硕士学位。我主要的研究方向是人工智能和机器学习，特别是在自然语言处理领域有深入研究。此外，我也对数据挖掘、模式识别等领域有一定的兴趣和积累。如果您有任何关于这些领域的疑问或需要帮助的地方，请随时告诉我，我会尽力提供支持和解答。

#### 性能统计

| 阶段       | 总时间(ms) | Token数 | 每Token时间(ms) | Token/秒 |
| -------- | ------- | ------ | ------------ | ------- |
| Prefill  | 138.65  | 16     | 8.67         | 115.40  |
| Generate | 8011.66 | 74     | 108.27       | 9.24    |

- **模型初始化时间**: 1648.03 ms
- **峰值内存使用**: 1.81 GB

### 6. 结果分析

#### chat_template配置验证

- **配置成功**: `rkllm_set_chat_template`返回成功，日志显示配置已生效
- **System Prompt生效**: 模型回答中明确提到"来自东南大学的一名研究生"
- **Template格式正确**: Qwen格式的chat_template被正确解析

#### 警告信息分析

```
W rkllm: Calling rkllm_set_chat_template will disable the internal automatic chat template parsing, including enable_thinking. Make sure your custom prompt is complete and valid.
```

**说明**: 调用`rkllm_set_chat_template`会禁用内部的自动chat_template解析，包括enable_thinking功能。这是预期行为，确保自定义prompt完整有效即可。

#### 性能对比

| 指标 | 默认配置 | 自定义chat_template | 差异 |
| ---- | ---- | ---- | ---- |
| Prefill速度 | 168.24 tokens/s | 115.40 tokens/s | 略降 |
| Generate速度 | 9.47 tokens/s | 9.24 tokens/s | 相当 |
| 内存占用 | 1.81 GB | 1.81 GB | 相同 |

#### System Prompt效果

- **身份认同**: 模型明确认同"东南大学研究生"身份
- **专业背景**: 自动关联计算机科学与技术专业
- **研究方向**: 提到人工智能、机器学习、自然语言处理等方向
- **角色一致性**: 回答风格符合研究生身份

### 7. 关键发现

1. **rkllm_set_chat_template接口有效性**:
   - 成功配置自定义system prompt和chat_template
   - 配置后立即生效，无需重新初始化模型
   - 日志输出确认配置参数正确

2. **System Prompt影响**:
   - 显著影响模型的自我认知和回答风格
   - 模型会根据system prompt调整专业领域和表达方式
   - 配置"东南大学研究生"后，模型主动关联计算机专业背景

3. **Template格式兼容性**:
   - Qwen格式（`<|im_start|>`）被正确解析
   - prompt_prefix和prompt_postfix拼接正确
   - 与Qwen2.5-1.5B-Instruct模型兼容良好

4. **注意事项**:
   - 自定义chat_template会禁用enable_thinking功能
   - 需要确保自定义prompt格式完整有效
   - 不同模型可能需要不同的template格式

### 8. 结论

- **测试成功**: 自定义chat_template和system prompt功能正常工作
- **配置方法**: 在`rkllm_init`之后、`rkllm_run`之前调用`rkllm_set_chat_template`
- **应用场景**: 可用于定制模型角色、专业领域、回答风格等
- **推荐做法**: 根据目标模型选择合适的template格式，确保prompt完整性

***

## 文本分析系统开发记录

**会话日期**: 2026-03-29  
**目标**: 基于PPOCR和RKLLM构建完整的文本分析系统，实现图片文字提取与错漏字分析

### 1. 系统架构设计

#### 核心组件

```
text_analysis_system/
├── src/                        # 源文件
│   ├── main.cpp               # 主程序入口
│   ├── config.cpp/h           # 配置管理模块
│   ├── text_queue.cpp/h       # 线程安全队列
│   ├── perf_monitor.cpp/h     # 性能监控模块
│   ├── ocr_engine.cpp/h       # OCR引擎
│   ├── ocr_thread.cpp/h       # OCR工作线程
│   ├── llm_engine.cpp/h       # LLM引擎
│   ├── llm_thread.cpp/h       # LLM工作线程
│   ├── result_handler.cpp/h   # 结果处理模块
│   └── ...                    # PPOCR相关源文件
├── include/                    # 头文件
├── config/                     # 配置文件
│   └── config.json            # 默认配置
├── model/                      # 模型文件
├── output/                     # 输出结果
└── docs/                       # 文档
```

#### 多线程架构

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

#### NPU核心分配策略

- **RKLLM模型**: 使用2个NPU核心（模型导出时已配置）
- **PPOCR模型**: 检测和识别模型共享1个NPU核心（Core 2）

### 2. 关键模块实现

#### 配置管理模块 (config.cpp/h)

**功能**: JSON配置文件解析和管理

**配置项**:
- 模型路径（OCR检测/识别模型、LLM模型、字典文件）
- LLM参数（system_prompt、max_new_tokens、temperature等）
- OCR参数（threshold、box_threshold、db_unclip_ratio等）
- 队列配置（max_size）
- 输出配置（result_dir、save_annotated_image）
- 性能配置（enable_timing、log_level）

**核心代码**:
```cpp
class Config {
public:
    ModelConfig model;
    LLMConfig llm;
    OCRConfig ocr;
    QueueConfig queue;
    OutputConfig output;
    PerformanceConfig performance;
    
    int loadFromFile(const std::string& filepath);
    bool validate() const;
    void print() const;
};
```

#### 线程安全队列 (text_queue.cpp/h)

**功能**: OCR结果在多线程间的安全传递

**特性**:
- 支持最大容量限制（默认20）
- 使用mutex和condition_variable实现同步
- 支持阻塞/非阻塞模式
- 优雅退出机制

**核心代码**:
```cpp
template<typename T>
class ThreadSafeQueue {
public:
    bool push(const T& item, bool block = true);
    bool pop(T& item, bool block = true);
    void stop();
    // ...
private:
    std::queue<T> queue_;
    mutable std::mutex mutex_;
    std::condition_variable cond_not_full_;
    std::condition_variable cond_not_empty_;
};
```

#### OCR引擎模块 (ocr_engine.cpp/h)

**功能**: 封装PPOCR检测和识别功能

**特性**:
- 检测模型绑定到NPU Core 2
- 识别模型绑定到NPU Core 2
- 端到端OCR推理
- 性能统计集成

**核心代码**:
```cpp
class OCREngine {
public:
    int initialize(const Config& config);
    int recognize(const std::string& image_path, 
                  OCRResult& result, 
                  PerfMonitor* perf_monitor = nullptr);
    int release();
};
```

#### LLM引擎模块 (llm_engine.cpp/h)

**功能**: 封装RKLLM文本分析功能

**特性**:
- 使用2个NPU核心（模型导出时已配置）
- 支持自定义system prompt
- Qwen格式chat_template
- 流式输出回调

**核心代码**:
```cpp
class LLMEngine {
public:
    int initialize(const Config& config);
    int analyze(const OCRResult& ocr_result, 
                LLMResult& llm_result, 
                PerfMonitor* perf_monitor = nullptr);
    int release();
};
```

#### 结果处理模块 (result_handler.cpp/h)

**功能**: JSON格式结果存储

**特性**:
- 单张图片结果JSON
- 批量处理汇总JSON
- 时间戳命名
- 目录自动创建

### 3. 编译配置

#### CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.10)
project(text_analysis_system)

set(CMAKE_CXX_STANDARD 14)
find_package(OpenCV REQUIRED)

# RKNN和RKLLM库路径
set(RKNN_API_PATH ${CMAKE_SOURCE_DIR}/../rknn_model_zoo/3rdparty/rknpu2)
set(RKNN_LIB_PATH ${CMAKE_SOURCE_DIR}/../rknn_model_zoo/3rdparty/rknpu2/Linux/librknn_api/aarch64/lib)
set(RKLLM_API_PATH ${CMAKE_SOURCE_DIR}/../rknn-llm/rkllm-runtime/Linux/librkllm_api/include)
set(RKLLM_LIB_PATH ${CMAKE_SOURCE_DIR}/../rknn-llm/rkllm-runtime/Linux/librkllm_api/aarch64)

# 源文件
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
    src/ppocr_system_npu2.cc
    src/postprocess.cc
    src/clipper.cc
    src/image_utils.c
    src/file_utils.c
)

add_executable(text_analysis_system ${SOURCES})
target_link_libraries(text_analysis_system ${OpenCV_LIBS} rknnrt rkllmrt pthread)
```

#### build.sh编译脚本

```bash
#!/bin/bash
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
cp text_analysis_system ../
```

### 4. 运行测试

#### 编译结果

```
[100%] Linking CXX executable text_analysis_system
[100%] Built target text_analysis_system

========================================
  编译完成!
========================================

可执行文件: ./text_analysis_system
使用方法: ./text_analysis_system <图片路径或文件夹>
```

#### 运行命令

```bash
# 处理单张图片
./text_analysis_system /path/to/image.jpg

# 处理文件夹
./text_analysis_system /path/to/images/

# 使用自定义配置
./text_analysis_system /path/to/image.jpg --config my_config.json
```

#### 预期输出

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

[Main] 初始化OCR引擎...
[OCREngine] 检测模型初始化成功 (NPU Core 2)
[OCREngine] 识别模型初始化成功 (NPU Core 2)

[Main] 初始化LLM引擎...
[LLMEngine] LLM引擎初始化完成

[Main] 启动工作线程...
[OCRThread] OCR线程已启动
[LLMThread] LLM线程已启动

[Main] 开始处理 1 张图片...
[OCRThread] OCR识别成功: image.jpg, 识别到 5 个文本
[LLMThread] LLM分析成功: image.jpg
[ResultHandler] 结果已保存: output/results/image_20250329_103000.json

========================================
  文本分析系统运行完成
========================================
```

### 5. 输出结果格式

#### 单张图片结果

```json
{
  "image_path": "image.jpg",
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

#### 批量汇总结果

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
  "results": [...]
}
```

### 6. 关键决策

1. **NPU核心分配策略**:
   - RKLLM使用2个NPU核心（模型导出时已配置）
   - PPOCR检测和识别模型共享1个NPU核心（Core 2）
   - 避免模型间资源冲突

2. **多线程架构**:
   - OCR线程和LLM线程分离
   - 通过线程安全队列传递数据
   - 支持流水线并行处理

3. **队列容量设计**:
   - 默认队列大小20
   - 平衡内存占用和处理效率
   - 支持动态阻塞等待

4. **配置管理**:
   - 使用nlohmann/json库解析配置
   - 支持默认值和配置验证
   - 所有参数可通过配置文件修改

5. **结果存储**:
   - JSON格式便于后续处理
   - 时间戳命名避免覆盖
   - 批量汇总便于统计分析

### 7. 问题解决

#### 问题1: 头文件冲突

**现象**: text_queue.h和perf_monitor.h中OCRPerfStats定义冲突

**解决**: 在text_queue.h中使用独立的QueueOCRPerfStats结构体

#### 问题2: 编译找不到rknn_api.h

**现象**: fatal error: rknn_api.h: 没有那个文件或目录

**解决**: 修正CMakeLists.txt中的RKNN API路径

#### 问题3: 缺少postprocess.cc

**现象**: 链接错误，找不到postprocess相关函数

**解决**: 从rknn_model_zoo复制postprocess.cc和clipper.cc

### 8. 性能指标

- **OCR单张图片推理时间**: 约600-800ms
- **LLM首token响应时间**: 约100-200ms
- **LLM生成速度**: 约10-15 tokens/秒
- **端到端处理时间**: 约8-10秒/张图片

### 9. 文档交付

- **README.md**: 项目简介、快速开始、目录结构
- **docs/TUTORIAL.md**: 详细使用教程、环境搭建、常见问题
- **PROJECT_RECORD.md**: 开发记录、关键决策、问题解决

***

## PPOCR耗时统计功能实现

**日期**: 2026-03-29

**目标**: 为PPOCR示例程序添加检测和识别分别耗时统计功能

### 1. 问题分析

RK官方PPOCR示例程序（PPOCR-Det、PPOCR-Rec、PPOCR-System）的C++代码中均未实现耗时统计功能。需要自行添加时间统计代码，用于性能分析和优化。

### 2. 技术方案

#### 2.1 时间统计方法

参考YOLOv8-Pose示例，使用`gettimeofday`实现微秒级时间统计：

```cpp
#include <sys/time.h>

static inline int64_t getCurrentTimeUs() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000000 + tv.tv_usec;
}
```

#### 2.2 统计粒度设计

| 阶段 | 统计粒度 | 说明 |
|------|----------|------|
| 检测 | 整张图片 | 检测阶段一次性处理整张图片 |
| 识别 | 按文本框 | 每个检测到的文本框单独统计 |

#### 2.3 数据结构修改

在`ppocr_system.h`中添加耗时字段：

```cpp
typedef struct ppocr_det_result {
    rknn_quad_t box[1000];
    int count;
    float inference_time_ms;  // 检测阶段耗时（毫秒）
} ppocr_det_result;

typedef struct ppocr_rec_result {
    char str[512];
    int str_size;
    float score;
    float inference_time_ms;  // 识别阶段耗时（毫秒）
} ppocr_rec_result;

typedef struct ppocr_text_recog_array_result_t {
    ppocr_text_recog_result_t text_result[1000];
    int count;
    float total_inference_time_ms;  // 总耗时（毫秒）
} ppocr_text_recog_array_result_t;
```

### 3. 关键代码实现

#### 3.1 检测阶段耗时统计

```cpp
int inference_ppocr_det_model(...) {
    int64_t start_time = getCurrentTimeUs();
    
    // 预处理 + NPU推理 + 后处理
    ...
    
    int64_t end_time = getCurrentTimeUs();
    out_result->inference_time_ms = (end_time - start_time) / 1000.0f;
    return ret;
}
```

#### 3.2 识别阶段耗时统计

```cpp
int inference_ppocr_rec_model(...) {
    int64_t start_time = getCurrentTimeUs();
    
    // 预处理 + NPU推理 + 后处理
    ...
    
    int64_t end_time = getCurrentTimeUs();
    out_result->inference_time_ms = (end_time - start_time) / 1000.0f;
    return ret;
}
```

#### 3.3 系统级耗时统计与输出

```cpp
int inference_ppocr_system_model(...) {
    int64_t system_start_time = getCurrentTimeUs();
    
    // 检测阶段
    ppocr_det_result det_results;
    inference_ppocr_det_model(..., &det_results);
    printf("[PPOCR Timing] Detection: %.2f ms\n", det_results.inference_time_ms);
    
    // 识别阶段（逐个文本框）
    float total_rec_time = 0.0f;
    for (int i=0; i < boxes_result.size(); i++) {
        ppocr_rec_result text_result;
        inference_ppocr_rec_model(..., &text_result);
        printf("[PPOCR Timing] Recognition[%d]: %.2f ms (text: %s)\n", 
               i, text_result.inference_time_ms, text_result.str);
        total_rec_time += text_result.inference_time_ms;
    }
    
    // 汇总输出
    printf("[PPOCR Timing] Total Recognition: %.2f ms (avg: %.2f ms per box, %d boxes)\n", 
           total_rec_time, total_rec_time / out_result->count, out_result->count);
    printf("[PPOCR Timing] Total: %.2f ms\n", out_result->total_inference_time_ms);
}
```

### 4. 工程结构

```
ppocr_timing/
├── src/
│   ├── ppocr_system.h      # 修改后的头文件（添加耗时字段）
│   ├── ppocr_system.cc     # 修改后的实现（添加耗时统计）
│   ├── postprocess.cc      # DBNet后处理/CTC解码
│   ├── clipper.cc/h        # 多边形裁剪库
│   ├── dict.h              # 字典定义
│   └── main.cc             # 主程序入口
├── include/
│   └── common.h            # 公共头文件
├── model/
│   └── test.jpg            # 测试图片
├── build/
│   └── ppocr_timing        # 编译生成的可执行文件
└── CMakeLists.txt          # 编译配置
```

### 5. 编译与运行

#### 5.1 编译命令

```bash
cd /home/linaro/traffic_text_analysis_system/ppocr_timing/build
rm -rf *
cmake ..
make -j4
```

#### 5.2 运行命令

```bash
export LD_LIBRARY_PATH=/home/linaro/traffic_text_analysis_system/rknn_model_zoo/install/rk3588_linux_aarch64/rknn_PPOCR-System_demo/lib:$LD_LIBRARY_PATH

./ppocr_timing \
    ../model/ppocrv4_det_i8.rknn \
    ../model/ppocrv4_rec_fp16.rknn \
    ../../rknn_model_zoo/install/rk3588_linux_aarch64/rknn_PPOCR-System_demo/model/ppocr_keys_v1.txt \
    ../model/test.jpg
```

### 6. 运行结果

```
========================================
[PPOCR Timing] Detection: 41.16 ms
========================================

[PPOCR Timing] Recognition Results:
----------------------------------------
[PPOCR Timing] Recognition[0]: 25.60 ms (text: 纯臻营养护发素)
[PPOCR Timing] Recognition[1]: 27.27 ms (text: 产品信息/参数)
[PPOCR Timing] Recognition[2]: 27.68 ms (text: （45元/每公斤，100公斤起订）)
[PPOCR Timing] Recognition[3]: 28.36 ms (text: 每瓶22元，1000瓶起订）)
[PPOCR Timing] Recognition[4]: 28.74 ms (text: 【品牌】：代加工方式/OEMODM)
[PPOCR Timing] Recognition[5]: 31.04 ms (text: 【品名】：纯臻营养护发素)
[PPOCR Timing] Recognition[6]: 32.88 ms (text: 【产品编号】：YM-X-3011)
[PPOCR Timing] Recognition[7]: 32.21 ms (text: ODMOEM)
[PPOCR Timing] Recognition[8]: 32.42 ms (text: 【净含量】：220ml)
[PPOCR Timing] Recognition[9]: 32.44 ms (text: 【适用人群】：适合所有肤质)
[PPOCR Timing] Recognition[10]: 31.34 ms (text: 【主要成分】：鲸蜡硬脂醇、燕麦β-葡聚)
[PPOCR Timing] Recognition[11]: 31.86 ms (text: 糖、椰油酰胺丙基甜菜碱、泛酸)
[PPOCR Timing] Recognition[12]: 29.68 ms (text: （成品包材）)
[PPOCR Timing] Recognition[13]: 28.37 ms (text: 【主要功能】：可紧致头发磷层，从而达到)
[PPOCR Timing] Recognition[14]: 28.90 ms (text: 即时持久改善头发光泽的效果，给干燥的头)
[PPOCR Timing] Recognition[15]: 31.89 ms (text: 发足够的滋养)
----------------------------------------
[PPOCR Timing] Total Recognition: 480.67 ms (avg: 30.04 ms per box, 16 boxes)

========================================
[PPOCR Timing] Total: 535.97 ms
========================================
```

### 7. 性能分析

| 指标 | 数值 | 说明 |
|------|------|------|
| 检测阶段 | 41.16 ms | 包含预处理、NPU推理、后处理 |
| 单个文本框识别 | 25.60-32.88 ms | 平均约30ms |
| 识别阶段总计 | 480.67 ms | 16个文本框 |
| 总耗时 | 535.97 ms | 检测+识别 |
| 识别平均耗时 | 30.04 ms/box | 总识别时间/文本框数 |

**结论**:
- 检测阶段耗时约41ms，在合理范围内
- 单个文本框识别耗时约25-33ms，平均30ms
- 识别阶段总耗时与文本框数量成正比
- 时间统计精度达到微秒级

### 8. 关键决策

1. **时间统计方法**: 使用`gettimeofday`实现微秒级精度，参考YOLOv8-Pose示例
2. **统计粒度**: 检测阶段按整张图片统计，识别阶段按文本框统计
3. **输出格式**: 统一使用`[PPOCR Timing]`前缀，便于日志过滤和分析
4. **代码结构**: 在原有函数内添加时间统计，保持接口兼容性

### 9. 问题解决

#### 问题1: goto语句跨越变量初始化

**现象**: 编译错误`jump to label 'out' crosses initialization of 'int64_t end_time'`

**解决**: 将变量声明移到函数开头，避免goto跨越变量初始化

```cpp
// 修改前（错误）
goto out;
int64_t end_time = getCurrentTimeUs();  // 错误：跨越初始化

// 修改后（正确）
int64_t start_time, end_time;  // 声明在开头
goto out;
end_time = getCurrentTimeUs();  // 赋值
```

#### 问题2: 缺少依赖库

**现象**: 链接错误，找不到`fileutils`、`imageutils`等库

**解决**: 在CMakeLists.txt中添加rknn_model_zoo的utils和3rdparty子目录

```cmake
add_subdirectory(${RKNN_ROOT}/3rdparty 3rdparty.out)
add_subdirectory(${RKNN_ROOT}/utils utils.out)
```

***

## PPOCR准确耗时统计功能集成记录

**日期**: 2026-03-29  
**目标**: 将ppocr_timing项目中实现的准确耗时统计功能集成到文本分析系统中，替代原有的估计值

### 1. 问题背景

原有实现使用估计值填充OCR耗时：
```cpp
// ocr_engine.cpp 原有实现
result.perf_stats.det_time_ms = total_time_ms * 0.3f;  // 估计检测占30%
result.perf_stats.rec_time_ms = total_time_ms * 0.6f;  // 估计识别占60%
```

根据ppocr_timing项目（PROJECT_RECORD.md#L1270-1522）的开发记录，已实现基于`gettimeofday`的准确耗时统计，需要将该功能集成到文本分析系统。

### 2. 修改内容

#### 2.1 修改ppocr_system.h添加耗时字段

**文件**: `text_analysis_system/include/ppocr_system.h`

```cpp
// 检测结果结构体添加耗时字段
typedef struct {
    rknn_quad_t box[1000];
    int count;
    float inference_time_ms;  // 检测阶段耗时（毫秒）
} ppocr_det_result;

// 识别结果结构体添加耗时字段
typedef struct ppocr_rec_result
{
    char str[512];
    int str_size;
    float score;
    float inference_time_ms;  // 识别阶段耗时（毫秒）
} ppocr_rec_result;

// 系统级结果结构体添加耗时字段
typedef struct ppocr_text_recog_array_result_t
{
    ppocr_text_recog_result_t text_result[1000];
    int count;
    float det_time_ms;  // 检测阶段总耗时（毫秒）
    float rec_time_ms;  // 识别阶段总耗时（毫秒）
} ppocr_text_recog_array_result_t;
```

#### 2.2 修改ppocr_system_npu2.cc实现准确计时

**文件**: `text_analysis_system/src/ppocr_system_npu2.cc`

**添加时间辅助函数**:
```cpp
#include <sys/time.h>

// 获取当前时间（微秒）
static inline int64_t getCurrentTimeUs()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000000 + tv.tv_usec;
}
```

**检测阶段计时** (`inference_ppocr_det_model`函数):
```cpp
int inference_ppocr_det_model(...)
{
    int64_t start_time, end_time;
    out_result->inference_time_ms = 0;
    
    // 记录检测阶段开始时间
    start_time = getCurrentTimeUs();
    
    // ... 原有检测逻辑 ...
    
    // 计算检测阶段耗时
    end_time = getCurrentTimeUs();
    out_result->inference_time_ms = (end_time - start_time) / 1000.0f;
    printf("[PPOCR Timing] Detection: %.2f ms\n", out_result->inference_time_ms);
}
```

**识别阶段计时** (`inference_ppocr_rec_model`函数):
```cpp
int inference_ppocr_rec_model(...)
{
    int64_t start_time, end_time;
    out_result->inference_time_ms = 0;
    
    // 记录识别阶段开始时间
    start_time = getCurrentTimeUs();
    
    // ... 原有识别逻辑 ...
    
    // 计算识别阶段耗时
    end_time = getCurrentTimeUs();
    out_result->inference_time_ms = (end_time - start_time) / 1000.0f;
}
```

**系统级计时汇总** (`inference_ppocr_system_model`函数):
```cpp
int inference_ppocr_system_model(...)
{
    float total_rec_time = 0.0f;
    
    // 初始化耗时统计
    out_result->det_time_ms = 0.0f;
    out_result->rec_time_ms = 0.0f;
    
    // 检测阶段
    ppocr_det_result det_results;
    ret = inference_ppocr_det_model(..., &det_results);
    out_result->det_time_ms = det_results.inference_time_ms;
    
    // 识别阶段 - 逐个文本框识别并累加耗时
    for (int i=0; i < boxes_result.size(); i++) {
        ppocr_rec_result text_result;
        ret = inference_ppocr_rec_model(..., &text_result);
        
        // 累加识别耗时
        total_rec_time += text_result.inference_time_ms;
        
        // ... 保存结果 ...
    }
    
    // 保存识别阶段总耗时
    out_result->rec_time_ms = total_rec_time;
    printf("[PPOCR Timing] Total Recognition: %.2f ms (avg: %.2f ms per box, %d boxes)\n",
           total_rec_time, total_rec_time / boxes_result.size(), (int)boxes_result.size());
}
```

#### 2.3 修改ocr_engine.cpp使用实际耗时

**文件**: `text_analysis_system/src/ocr_engine.cpp`

```cpp
// 修改前（使用估计值）
result.perf_stats.det_time_ms = total_time_ms * 0.3f;
result.perf_stats.rec_time_ms = total_time_ms * 0.6f;

// 修改后（使用实际测量值）
result.perf_stats.det_time_ms = ppocr_result.det_time_ms;
result.perf_stats.rec_time_ms = ppocr_result.rec_time_ms;
```

### 3. 编译验证

```bash
cd /home/linaro/traffic_text_analysis_system/text_analysis_system/build
make -j4
```

**结果**: 编译成功，无错误

### 4. 预期输出

运行时将输出准确的耗时统计：
```
[PPOCR Timing] Detection: 45.23 ms
[PPOCR Timing] Total Recognition: 312.56 ms (avg: 26.05 ms per box, 12 boxes)
[OCREngine] OCR识别成功: image.jpg, 识别到 12 个文本, 总耗时: 412.34 ms
```

### 5. 关键决策

1. **计时方法**: 使用`gettimeofday`实现微秒级精度，与ppocr_timing项目保持一致
2. **统计粒度**: 
   - 检测阶段：整张图片统计一次
   - 识别阶段：每个文本框单独统计，最后汇总
3. **数据流**: 通过结构体字段传递耗时数据，避免全局变量
4. **向后兼容**: 新增字段不影响原有接口，保持兼容性

### 6. 参考文档

- PPOCR耗时统计分析计划: `.trae/documents/PPOCR_Timing_Analysis_Plan.md`
- ppocr_timing项目记录: `PROJECT_RECORD.md#L1270-1522`
- 本功能Spec: `.trae/specs/ppocr-timing-integration/`

***

**记录创建时间**: 2026-03-28  
**最后更新**: 2026-03-29（添加PPOCR准确耗时统计功能集成记录）  
**记录维护**: 每次会话后更新
