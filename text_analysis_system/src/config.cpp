/**
 * @file config.cpp
 * @brief 配置管理模块实现
 */

#include "config.h"
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sys/stat.h>

// 使用nlohmann/json单头文件版本
#include "json.hpp"
using json = nlohmann::json;

/**
 * @brief 获取全局配置实例（单例模式）
 */
Config& getConfig() {
    static Config instance;
    return instance;
}

/**
 * @brief 加载默认配置
 */
void Config::loadDefaults() {
    // 模型默认配置（使用resolvePath解析相对路径）
    model.det_model_path = resolvePath("model/ppocrv4_det_i8.rknn");
    model.rec_model_path = resolvePath("model/ppocrv4_rec_fp16.rknn");
    model.dict_path = resolvePath("model/ppocr_keys_v1.txt");
    model.llm_model_path = "/userdata/models/Qwen3-1.7B_W8A8_RK3588_16k_2npu.rkllm";

    // LLM默认配置
    llm.system_prompt = "你是一个文本校对助手。请分析OCR识别的文本，判断是否有错别字、漏字、多字。请以JSON格式返回：{\"is_correct\": true/false, \"corrected_text\": \"修正后的文本（无误则为空）\"}";
    llm.enable_thinking = false;
    llm.max_new_tokens = 256;
    llm.max_context_len = 2048;
    llm.temperature = 0.1;
    llm.top_p = 0.9;
    llm.top_k = 1;
    llm.repeat_penalty = 1.0;

    // OCR默认配置
    ocr.threshold = 0.3;
    ocr.box_threshold = 0.6;
    ocr.db_unclip_ratio = 1.5;
    ocr.use_dilate = false;
    ocr.db_score_mode = "slow";
    ocr.db_box_type = "poly";

    // 队列默认配置
    queue.max_size = 20;

    // 输出默认配置
    output.result_dir = resolvePath("output/results");
    output.save_annotated_image = true;

    // 性能默认配置
    performance.enable_timing = true;
    performance.log_level = 1;
}

/**
 * @brief 从JSON文件加载配置
 */
int Config::loadFromFile(const std::string& filepath, const std::string& base_dir) {
    // 保存基础目录
    base_dir_ = base_dir;

    // 首先加载默认配置（此时base_dir_已设置，可在loadDefaults中使用）
    loadDefaults();

    // 检查文件是否存在
    struct stat buffer;
    if (stat(filepath.c_str(), &buffer) != 0) {
        printf("[Config] 配置文件不存在: %s，使用默认配置\n", filepath.c_str());
        return 0;
    }

    // 读取JSON文件
    std::ifstream file(filepath);
    if (!file.is_open()) {
        printf("[Config] 无法打开配置文件: %s\n", filepath.c_str());
        return -1;
    }

    try {
        json j;
        file >> j;

        // 解析模型配置
        if (j.contains("model")) {
            parseModelConfig(&j["model"]);
        }

        // 解析LLM配置
        if (j.contains("llm")) {
            parseLLMConfig(&j["llm"]);
        }

        // 解析OCR配置
        if (j.contains("ocr")) {
            parseOCRConfig(&j["ocr"]);
        }

        // 解析队列配置
        if (j.contains("queue")) {
            parseQueueConfig(&j["queue"]);
        }

        // 解析输出配置
        if (j.contains("output")) {
            parseOutputConfig(&j["output"]);
        }

        // 解析性能配置
        if (j.contains("performance")) {
            parsePerformanceConfig(&j["performance"]);
        }

        printf("[Config] 配置文件加载成功: %s\n", filepath.c_str());
        return 0;

    } catch (const std::exception& e) {
        printf("[Config] 解析配置文件失败: %s\n", e.what());
        return -1;
    }
}

/**
 * @brief 解析模型配置
 */
int Config::parseModelConfig(const void* json_obj) {
    const json* j = static_cast<const json*>(json_obj);

    if (j->contains("det_model_path")) {
        model.det_model_path = resolvePath((*j)["det_model_path"].get<std::string>());
    }
    if (j->contains("rec_model_path")) {
        model.rec_model_path = resolvePath((*j)["rec_model_path"].get<std::string>());
    }
    if (j->contains("dict_path")) {
        model.dict_path = resolvePath((*j)["dict_path"].get<std::string>());
    }
    if (j->contains("llm_model_path")) {
        model.llm_model_path = resolvePath((*j)["llm_model_path"].get<std::string>());
    }

    return 0;
}

/**
 * @brief 解析LLM配置
 */
int Config::parseLLMConfig(const void* json_obj) {
    const json* j = static_cast<const json*>(json_obj);
    
    if (j->contains("system_prompt")) {
        llm.system_prompt = (*j)["system_prompt"].get<std::string>();
    }
    if (j->contains("enable_thinking")) {
        llm.enable_thinking = (*j)["enable_thinking"].get<bool>();
    }
    if (j->contains("max_new_tokens")) {
        llm.max_new_tokens = (*j)["max_new_tokens"].get<int>();
    }
    if (j->contains("max_context_len")) {
        llm.max_context_len = (*j)["max_context_len"].get<int>();
    }
    if (j->contains("temperature")) {
        llm.temperature = (*j)["temperature"].get<float>();
    }
    if (j->contains("top_p")) {
        llm.top_p = (*j)["top_p"].get<float>();
    }
    if (j->contains("top_k")) {
        llm.top_k = (*j)["top_k"].get<int>();
    }
    if (j->contains("repeat_penalty")) {
        llm.repeat_penalty = (*j)["repeat_penalty"].get<float>();
    }
    
    return 0;
}

/**
 * @brief 解析OCR配置
 */
int Config::parseOCRConfig(const void* json_obj) {
    const json* j = static_cast<const json*>(json_obj);
    
    if (j->contains("threshold")) {
        ocr.threshold = (*j)["threshold"].get<float>();
    }
    if (j->contains("box_threshold")) {
        ocr.box_threshold = (*j)["box_threshold"].get<float>();
    }
    if (j->contains("db_unclip_ratio")) {
        ocr.db_unclip_ratio = (*j)["db_unclip_ratio"].get<float>();
    }
    if (j->contains("use_dilate")) {
        ocr.use_dilate = (*j)["use_dilate"].get<bool>();
    }
    if (j->contains("db_score_mode")) {
        ocr.db_score_mode = (*j)["db_score_mode"].get<std::string>();
    }
    if (j->contains("db_box_type")) {
        ocr.db_box_type = (*j)["db_box_type"].get<std::string>();
    }
    
    return 0;
}

/**
 * @brief 解析队列配置
 */
int Config::parseQueueConfig(const void* json_obj) {
    const json* j = static_cast<const json*>(json_obj);
    
    if (j->contains("max_size")) {
        queue.max_size = (*j)["max_size"].get<int>();
    }
    
    return 0;
}

/**
 * @brief 解析输出配置
 */
int Config::parseOutputConfig(const void* json_obj) {
    const json* j = static_cast<const json*>(json_obj);
    
    if (j->contains("result_dir")) {
        output.result_dir = resolvePath((*j)["result_dir"].get<std::string>());
    }
    if (j->contains("save_annotated_image")) {
        output.save_annotated_image = (*j)["save_annotated_image"].get<bool>();
    }
    
    return 0;
}

/**
 * @brief 解析性能配置
 */
int Config::parsePerformanceConfig(const void* json_obj) {
    const json* j = static_cast<const json*>(json_obj);
    
    if (j->contains("enable_timing")) {
        performance.enable_timing = (*j)["enable_timing"].get<bool>();
    }
    if (j->contains("log_level")) {
        performance.log_level = (*j)["log_level"].get<int>();
    }
    
    return 0;
}

/**
 * @brief 验证配置有效性
 */
bool Config::validate() const {
    // 检查模型文件路径是否为空
    if (model.det_model_path.empty() || model.rec_model_path.empty() || 
        model.dict_path.empty() || model.llm_model_path.empty()) {
        printf("[Config] 错误: 模型路径不能为空\n");
        return false;
    }

    // 检查LLM参数范围
    if (llm.max_new_tokens <= 0 || llm.max_context_len <= 0) {
        printf("[Config] 错误: LLM参数必须大于0\n");
        return false;
    }

    // 检查队列大小
    if (queue.max_size <= 0 || queue.max_size > 100) {
        printf("[Config] 错误: 队列大小必须在1-100之间\n");
        return false;
    }

    // 检查OCR参数范围
    if (ocr.threshold < 0 || ocr.threshold > 1 || ocr.box_threshold < 0 || ocr.box_threshold > 1) {
        printf("[Config] 错误: OCR阈值必须在0-1之间\n");
        return false;
    }

    return true;
}

/**
 * @brief 打印当前配置
 */
void Config::print() const {
    printf("========================================\n");
    printf("  系统配置信息\n");
    printf("========================================\n");
    
    printf("\n[模型配置]\n");
    printf("  检测模型: %s\n", model.det_model_path.c_str());
    printf("  识别模型: %s\n", model.rec_model_path.c_str());
    printf("  字典文件: %s\n", model.dict_path.c_str());
    printf("  LLM模型: %s\n", model.llm_model_path.c_str());
    
    printf("\n[LLM配置]\n");
    printf("  System Prompt: %s\n", llm.system_prompt.substr(0, 50).c_str());
    printf("  Enable Thinking: %s\n", llm.enable_thinking ? "true" : "false");
    printf("  Max New Tokens: %d\n", llm.max_new_tokens);
    printf("  Max Context Len: %d\n", llm.max_context_len);
    printf("  Temperature: %.2f\n", llm.temperature);
    printf("  Top P: %.2f\n", llm.top_p);
    printf("  Top K: %d\n", llm.top_k);
    printf("  Repeat Penalty: %.2f\n", llm.repeat_penalty);
    
    printf("\n[OCR配置]\n");
    printf("  Threshold: %.2f\n", ocr.threshold);
    printf("  Box Threshold: %.2f\n", ocr.box_threshold);
    printf("  DB Unclip Ratio: %.2f\n", ocr.db_unclip_ratio);
    printf("  Use Dilate: %s\n", ocr.use_dilate ? "true" : "false");
    printf("  DB Score Mode: %s\n", ocr.db_score_mode.c_str());
    printf("  DB Box Type: %s\n", ocr.db_box_type.c_str());
    
    printf("\n[队列配置]\n");
    printf("  Max Size: %d\n", queue.max_size);
    
    printf("\n[输出配置]\n");
    printf("  Result Dir: %s\n", output.result_dir.c_str());
    printf("  Save Annotated Image: %s\n", output.save_annotated_image ? "true" : "false");
    
    printf("\n[性能配置]\n");
    printf("  Enable Timing: %s\n", performance.enable_timing ? "true" : "false");
    printf("  Log Level: %d\n", performance.log_level);
    
    printf("========================================\n");
}

/**
 * @brief 将相对路径转换为绝对路径
 * 如果文件在base_dir下不存在，尝试在父目录查找
 */
std::string Config::resolvePath(const std::string& path) const {
    if (path.empty() || path[0] == '/') {
        // 已经是绝对路径或空路径
        return path;
    }
    if (base_dir_.empty()) {
        return path;
    }
    
    // 首先尝试基于base_dir的路径
    std::string full_path = base_dir_ + "/" + path;
    struct stat st;
    if (stat(full_path.c_str(), &st) == 0) {
        return full_path;
    }
    
    // 如果文件不存在，尝试在父目录查找（适用于从build目录运行的情况）
    std::string parent_path = base_dir_ + "/../" + path;
    if (stat(parent_path.c_str(), &st) == 0) {
        return parent_path;
    }
    
    // 返回原始组合路径（文件可能尚不存在）
    return full_path;
}
