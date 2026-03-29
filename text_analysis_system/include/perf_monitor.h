/**
 * @file perf_monitor.h
 * @brief 性能监控模块头文件
 * 
 * 提供毫秒级性能统计功能
 * 支持OCR和LLM各阶段的耗时统计
 */

#ifndef _TEXT_ANALYSIS_PERF_MONITOR_H_
#define _TEXT_ANALYSIS_PERF_MONITOR_H_

#include <chrono>
#include <string>
#include <vector>
#include <cstdio>

/**
 * @brief 高精度计时器类
 * 
 * 使用std::chrono实现毫秒级计时
 */
class Timer {
public:
    /**
     * @brief 构造函数，自动开始计时
     */
    Timer();

    /**
     * @brief 重置计时器，重新开始计时
     */
    void reset();

    /**
     * @brief 获取经过的时间（毫秒）
     * @return 毫秒数
     */
    float elapsed_ms() const;

    /**
     * @brief 获取经过的时间（微秒）
     * @return 微秒数
     */
    float elapsed_us() const;

private:
    std::chrono::high_resolution_clock::time_point start_time_;
};

/**
 * @brief 性能统计记录
 */
typedef struct {
    std::string name;           // 阶段名称
    float time_ms;              // 耗时（毫秒）
} PerfRecord;

/**
 * @brief OCR性能统计
 */
typedef struct {
    float det_time_ms;          // 检测耗时
    float rec_time_ms;          // 识别耗时
    float total_time_ms;        // 总耗时
} OCRPerfStats;

/**
 * @brief LLM性能统计
 */
typedef struct {
    float prefill_time_ms;      // Prefill耗时
    float generate_time_ms;     // Generate耗时
    float total_time_ms;        // 总耗时
    int input_tokens;           // 输入token数
    int output_tokens;          // 输出token数
    float tokens_per_sec;       // 生成速度(tokens/秒)
} LLMPerfStats;

/**
 * @brief 端到端性能统计
 */
typedef struct {
    float ocr_time_ms;          // OCR总耗时
    float llm_time_ms;          // LLM总耗时
    float total_time_ms;        // 端到端总耗时
} EndToEndPerfStats;

/**
 * @brief 性能监控器类
 * 
 * 用于收集和输出各阶段的性能统计信息
 */
class PerfMonitor {
public:
    /**
     * @brief 构造函数
     * @param enable 是否启用性能监控
     */
    explicit PerfMonitor(bool enable = true);

    /**
     * @brief 开始一个阶段的计时
     * @param stage_name 阶段名称
     */
    void startStage(const std::string& stage_name);

    /**
     * @brief 结束当前阶段的计时
     * @return 该阶段耗时（毫秒）
     */
    float endStage();

    /**
     * @brief 记录OCR检测耗时
     * @param time_ms 耗时（毫秒）
     */
    void recordOCRDet(float time_ms);

    /**
     * @brief 记录OCR识别耗时
     * @param time_ms 耗时（毫秒）
     */
    void recordOCRRec(float time_ms);

    /**
     * @brief 记录OCR总耗时
     * @param time_ms 耗时（毫秒）
     */
    void recordOCRTotal(float time_ms);

    /**
     * @brief 记录LLM Prefill耗时
     * @param time_ms 耗时（毫秒）
     * @param input_tokens 输入token数（可选）
     */
    void recordLLMPrefill(float time_ms, int input_tokens = 0);

    /**
     * @brief 记录LLM Generate耗时
     * @param time_ms 耗时（毫秒）
     * @param output_tokens 输出token数（可选）
     */
    void recordLLMGenerate(float time_ms, int output_tokens = 0);

    /**
     * @brief 记录LLM总耗时
     * @param time_ms 耗时（毫秒）
     */
    void recordLLMTotal(float time_ms);

    /**
     * @brief 记录端到端总耗时
     * @param time_ms 耗时（毫秒）
     */
    void recordEndToEnd(float time_ms);

    /**
     * @brief 获取OCR性能统计
     * @return OCR性能统计结构
     */
    OCRPerfStats getOCRStats() const;

    /**
     * @brief 获取LLM性能统计
     * @return LLM性能统计结构
     */
    LLMPerfStats getLLMStats() const;

    /**
     * @brief 获取端到端性能统计
     * @return 端到端性能统计结构
     */
    EndToEndPerfStats getEndToEndStats() const;

    /**
     * @brief 打印性能统计到控制台
     */
    void printStats() const;

    /**
     * @brief 生成性能统计JSON字符串
     * @return JSON格式字符串
     */
    std::string toJSON() const;

    /**
     * @brief 重置所有统计
     */
    void reset();

    /**
     * @brief 检查是否启用性能监控
     * @return 启用返回true，否则返回false
     */
    bool isEnabled() const;

private:
    bool enabled_;                      // 是否启用
    Timer timer_;                       // 计时器
    std::string current_stage_;         // 当前阶段名称
    
    OCRPerfStats ocr_stats_;            // OCR统计
    LLMPerfStats llm_stats_;            // LLM统计
    EndToEndPerfStats e2e_stats_;       // 端到端统计
};

/**
 * @brief 自动作用域计时器
 * 
 * 在构造时开始计时，在析构时自动结束计时并记录
 * 用于方便地测量代码块的执行时间
 */
class ScopedTimer {
public:
    /**
     * @brief 构造函数，开始计时
     * @param monitor 性能监控器
     * @param stage_name 阶段名称
     */
    ScopedTimer(PerfMonitor& monitor, const std::string& stage_name);

    /**
     * @brief 析构函数，结束计时
     */
    ~ScopedTimer();

private:
    PerfMonitor& monitor_;
    std::string stage_name_;
};

/**
 * @brief 获取当前时间戳字符串
 * @return 格式：YYYY-MM-DD HH:MM:SS
 */
std::string getCurrentTimestamp();

/**
 * @brief 获取高精度时间戳（毫秒）
 * @return 毫秒时间戳
 */
long long getCurrentTimeMillis();

#endif // _TEXT_ANALYSIS_PERF_MONITOR_H_
