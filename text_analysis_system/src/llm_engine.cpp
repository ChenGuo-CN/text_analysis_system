/**
 * @file llm_engine.cpp
 * @brief LLM引擎模块实现
 */

#include "llm_engine.h"
#include <cstring>
#include <chrono>
#include <sstream>
#include <thread>

// 静态成员初始化
LLMCallbackData LLMEngine::callback_data_;

// ==================== LLMCallbackData 实现 ====================

LLMCallbackData::LLMCallbackData() : finished_(false), error_(false) {
}

void LLMCallbackData::appendText(const char* text) {
    std::lock_guard<std::mutex> lock(mutex_);
    text_ += text;
}

std::string LLMCallbackData::getText() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return text_;
}

void LLMCallbackData::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    text_.clear();
    finished_ = false;
    error_ = false;
}

void LLMCallbackData::setFinished() {
    std::lock_guard<std::mutex> lock(mutex_);
    finished_ = true;
}

bool LLMCallbackData::isFinished() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return finished_;
}

void LLMCallbackData::setError() {
    std::lock_guard<std::mutex> lock(mutex_);
    error_ = true;
}

bool LLMCallbackData::hasError() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return error_;
}

// ==================== LLMEngine 实现 ====================

LLMEngine::LLMEngine() : llm_handle_(nullptr), initialized_(false) {
}

LLMEngine::~LLMEngine() {
    if (initialized_) {
        release();
    }
}

int LLMEngine::initialize(const Config& config) {
    if (initialized_) {
        printf("[LLMEngine] 引擎已初始化\n");
        return 0;
    }

    printf("[LLMEngine] 正在初始化LLM引擎...\n");
    config_ = config;

    // 创建默认参数
    RKLLMParam param = rkllm_createDefaultParam();
    param.model_path = config.model.llm_model_path.c_str();
    param.max_context_len = config.llm.max_context_len;
    param.max_new_tokens = config.llm.max_new_tokens;
    param.top_k = config.llm.top_k;
    param.top_p = config.llm.top_p;
    param.temperature = config.llm.temperature;
    param.repeat_penalty = config.llm.repeat_penalty;
    param.skip_special_token = true;
    param.extend_param.base_domain_id = 0;
    param.extend_param.embed_flash = 1;

    // 初始化模型
    int ret = rkllm_init(&llm_handle_, &param, callback);
    if (ret != 0) {
        printf("[LLMEngine] 模型初始化失败! ret=%d\n", ret);
        return -1;
    }

    // 设置chat_template和system prompt
    const char* prompt_prefix = "<|im_start|>user\n";
    const char* prompt_postfix = "<|im_end|>\n<|im_start|>assistant\n";
    ret = rkllm_set_chat_template(llm_handle_, config.llm.system_prompt.c_str(), 
                                   prompt_prefix, prompt_postfix);
    if (ret != 0) {
        printf("[LLMEngine] 警告: chat_template设置失败，使用默认配置\n");
    } else {
        printf("[LLMEngine] chat_template设置成功\n");
    }

    initialized_ = true;
    printf("[LLMEngine] LLM引擎初始化完成\n");
    return 0;
}

int LLMEngine::release() {
    if (!initialized_) {
        return 0;
    }

    printf("[LLMEngine] 正在释放LLM引擎资源...\n");

    if (llm_handle_ != nullptr) {
        rkllm_destroy(llm_handle_);
        llm_handle_ = nullptr;
    }

    initialized_ = false;
    printf("[LLMEngine] LLM引擎资源已释放\n");

    return 0;
}

int LLMEngine::analyze(const OCRResult& ocr_result, 
                       LLMResult& llm_result, 
                       PerfMonitor* perf_monitor) {
    if (!initialized_) {
        printf("[LLMEngine] 错误: 引擎未初始化\n");
        llm_result.success = false;
        llm_result.error_msg = "引擎未初始化";
        return -1;
    }

    // 清空回调数据
    callback_data_.clear();

    // 构建prompt
    std::string prompt = buildPrompt(ocr_result);
    printf("[LLMEngine] 分析文本长度: %zu 字符\n", prompt.length());

    // 准备输入
    RKLLMInput input;
    memset(&input, 0, sizeof(RKLLMInput));
    input.input_type = RKLLM_INPUT_PROMPT;
    input.role = "user";
    input.prompt_input = const_cast<char*>(prompt.c_str());

    // 准备推理参数
    RKLLMInferParam infer_param;
    memset(&infer_param, 0, sizeof(RKLLMInferParam));
    infer_param.mode = RKLLM_INFER_GENERATE;
    infer_param.keep_history = 0;

    // 记录开始时间
    auto start_time = std::chrono::high_resolution_clock::now();

    // 执行推理
    printf("[LLMEngine] 开始LLM推理...\n");
    int ret = rkllm_run(llm_handle_, &input, &infer_param, nullptr);

    // 等待推理完成
    int wait_count = 0;
    const int max_wait = 600;  // 最多等待600秒
    while (!callback_data_.isFinished() && !callback_data_.hasError() && wait_count < max_wait * 10) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        wait_count++;
    }

    // 计算总耗时
    auto end_time = std::chrono::high_resolution_clock::now();
    float total_time_ms = std::chrono::duration<float, std::milli>(end_time - start_time).count();

    if (ret != 0 || callback_data_.hasError()) {
        printf("[LLMEngine] LLM推理失败! ret=%d\n", ret);
        llm_result.success = false;
        llm_result.error_msg = "LLM推理失败";
        return -1;
    }

    // 获取结果
    llm_result.raw_response = callback_data_.getText();
    llm_result.analysis_text = llm_result.raw_response;
    llm_result.success = true;
    llm_result.perf_stats.total_time_ms = total_time_ms;

    printf("[LLMEngine] LLM推理完成，耗时 %.2f ms\n", total_time_ms);
    printf("[LLMEngine] 分析结果长度: %zu 字符\n", llm_result.analysis_text.length());

    // 记录性能统计
    if (perf_monitor != nullptr) {
        perf_monitor->recordLLMTotal(total_time_ms);
    }

    return 0;
}

bool LLMEngine::isInitialized() const {
    return initialized_;
}

LLMCallbackData* LLMEngine::getCallbackData() {
    return &callback_data_;
}

std::string LLMEngine::buildPrompt(const OCRResult& ocr_result) {
    std::ostringstream oss;
    
    // 提取所有文本
    std::string all_text = extractText(ocr_result);
    
    // 构建分析prompt
    oss << "请分析以下OCR识别的文本内容，找出其中的错别字、漏字、多字等问题，并给出修改建议。\n\n";
    oss << "识别的文本内容：\n";
    oss << "```\n";
    oss << all_text;
    oss << "\n```\n\n";
    oss << "请按以下JSON格式返回结果：\n";
    oss << "{\n";
    oss << "  \"analysis\": \"整体分析说明\",\n";
    oss << "  \"errors\": [\n";
    oss << "    {\n";
    oss << "      \"position\": \"问题位置\",\n";
    oss << "      \"original\": \"原文\",\n";
    oss << "      \"suggestion\": \"建议修改\",\n";
    oss << "      \"confidence\": 0.95\n";
    oss << "    }\n";
    oss << "  ]\n";
    oss << "}\n";
    
    return oss.str();
}

std::string LLMEngine::extractText(const OCRResult& ocr_result) {
    std::ostringstream oss;
    
    for (size_t i = 0; i < ocr_result.text_items.size(); i++) {
        oss << ocr_result.text_items[i].text;
        if (i < ocr_result.text_items.size() - 1) {
            oss << "\n";
        }
    }
    
    return oss.str();
}

int LLMEngine::callback(RKLLMResult* result, void* userdata, LLMCallState state) {
    if (state == RKLLM_RUN_NORMAL) {
        // 正常生成token
        if (result->text != nullptr) {
            callback_data_.appendText(result->text);
            // 实时输出
            printf("%s", result->text);
            fflush(stdout);
        }
    } else if (state == RKLLM_RUN_FINISH) {
        // 生成完成
        printf("\n[LLMEngine] 推理完成\n");
        callback_data_.setFinished();
    } else if (state == RKLLM_RUN_ERROR) {
        // 发生错误
        printf("\n[LLMEngine] 推理错误\n");
        callback_data_.setError();
    }
    return 0;
}
