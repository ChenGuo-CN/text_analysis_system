/**
 * @file llm_engine.h
 * @brief LLM引擎模块头文件
 * 
 * 封装RKLLM文本分析功能
 * 使用2个NPU核心
 */

#ifndef _TEXT_ANALYSIS_LLM_ENGINE_H_
#define _TEXT_ANALYSIS_LLM_ENGINE_H_

#include <cstddef>
#include "rkllm.h"
#include "config.h"
#include "perf_monitor.h"
#include "text_queue.h"
#include <string>
#include <vector>
#include <mutex>

/**
 * @brief LLM推理回调数据
 * 
 * 用于在回调函数中收集LLM输出
 */
class LLMCallbackData {
public:
    LLMCallbackData();
    
    /**
     * @brief 追加文本
     */
    void appendText(const char* text);
    
    /**
     * @brief 获取完整文本
     */
    std::string getText() const;
    
    /**
     * @brief 清空文本
     */
    void clear();
    
    /**
     * @brief 标记完成
     */
    void setFinished();
    
    /**
     * @brief 检查是否完成
     */
    bool isFinished() const;
    
    /**
     * @brief 设置错误状态
     */
    void setError();
    
    /**
     * @brief 检查是否有错误
     */
    bool hasError() const;

private:
    std::string text_;
    mutable std::mutex mutex_;
    bool finished_;
    bool error_;
};

/**
 * @brief LLM引擎类
 * 
 * 封装RKLLM模型的初始化和推理功能
 * 使用2个NPU核心（模型导出时已配置）
 */
class LLMEngine {
public:
    /**
     * @brief 构造函数
     */
    LLMEngine();

    /**
     * @brief 析构函数，自动释放资源
     */
    ~LLMEngine();

    /**
     * @brief 禁止拷贝构造和赋值
     */
    LLMEngine(const LLMEngine&) = delete;
    LLMEngine& operator=(const LLMEngine&) = delete;

    /**
     * @brief 初始化LLM引擎
     * @param config 配置对象
     * @return 成功返回0，失败返回-1
     */
    int initialize(const Config& config);

    /**
     * @brief 释放LLM引擎资源
     * @return 成功返回0，失败返回-1
     */
    int release();

    /**
     * @brief 分析OCR识别的文本
     * @param ocr_result OCR识别结果
     * @param llm_result 输出LLM分析结果
     * @param perf_monitor 性能监控器（可选）
     * @return 成功返回0，失败返回-1
     */
    int analyze(const OCRResult& ocr_result, 
                LLMResult& llm_result, 
                PerfMonitor* perf_monitor = nullptr);

    /**
     * @brief 检查引擎是否已初始化
     * @return 已初始化返回true，否则返回false
     */
    bool isInitialized() const;

    /**
     * @brief 获取全局回调数据指针（供回调函数使用）
     */
    static LLMCallbackData* getCallbackData();

private:
    LLMHandle llm_handle_;              // LLM句柄
    Config config_;                     // 配置副本
    bool initialized_;                  // 初始化标志
    static LLMCallbackData callback_data_;  // 回调数据

    /**
     * @brief 构建分析prompt
     */
    std::string buildPrompt(const OCRResult& ocr_result);

    /**
     * @brief 从OCR结果中提取所有文本
     */
    std::string extractText(const OCRResult& ocr_result);

    /**
     * @brief 静态回调函数
     */
    static int callback(RKLLMResult* result, void* userdata, LLMCallState state);
};

#endif // _TEXT_ANALYSIS_LLM_ENGINE_H_
