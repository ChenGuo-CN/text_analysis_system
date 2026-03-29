/**
 * @file result_handler.h
 * @brief 结果处理模块头文件
 * 
 * 实现JSON格式结果存储
 * 支持单张图片结果和批量汇总
 */

#ifndef _TEXT_ANALYSIS_RESULT_HANDLER_H_
#define _TEXT_ANALYSIS_RESULT_HANDLER_H_

#include "text_queue.h"
#include "config.h"
#include <string>
#include <vector>
#include <mutex>

/**
 * @brief 批量处理统计信息
 */
typedef struct {
    int total_count;            // 总图片数
    int success_count;          // 成功数
    int failed_count;           // 失败数
    float avg_ocr_time_ms;      // 平均OCR耗时
    float avg_llm_time_ms;      // 平均LLM耗时
    float total_time_ms;        // 总耗时
} BatchStats;

/**
 * @brief 结果处理类
 * 
 * 负责将处理结果保存为JSON格式
 * 支持单张图片结果和批量汇总
 */
class ResultHandler {
public:
    /**
     * @brief 构造函数
     */
    ResultHandler();

    /**
     * @brief 析构函数
     */
    ~ResultHandler();

    /**
     * @brief 禁止拷贝构造和赋值
     */
    ResultHandler(const ResultHandler&) = delete;
    ResultHandler& operator=(const ResultHandler&) = delete;

    /**
     * @brief 初始化结果处理器
     * @param config 配置对象
     * @return 成功返回0，失败返回-1
     */
    int initialize(const Config& config);

    /**
     * @brief 保存单张图片的处理结果
     * @param result 处理结果
     * @return 成功返回0，失败返回-1
     */
    int saveResult(const ProcessingResult& result);

    /**
     * @brief 保存批量处理的汇总结果
     * @param results 所有处理结果
     * @param stats 统计信息
     * @return 成功返回0，失败返回-1
     */
    int saveBatchSummary(const std::vector<ProcessingResult>& results, 
                         const BatchStats& stats);

    /**
     * @brief 将结果转换为JSON字符串
     * @param result 处理结果
     * @return JSON字符串
     */
    std::string resultToJSON(const ProcessingResult& result) const;

    /**
     * @brief 将批量统计转换为JSON字符串
     * @param results 所有处理结果
     * @param stats 统计信息
     * @return JSON字符串
     */
    std::string batchStatsToJSON(const std::vector<ProcessingResult>& results,
                                  const BatchStats& stats) const;

    /**
     * @brief 获取输出目录路径
     * @return 目录路径
     */
    std::string getOutputDir() const;

    /**
     * @brief 生成结果文件名
     * @param image_path 图片路径
     * @return 结果文件名
     */
    std::string generateResultFilename(const std::string& image_path) const;

private:
    Config config_;                     // 配置
    std::string output_dir_;            // 输出目录
    std::mutex mutex_;                  // 互斥锁
    int result_count_;                  // 结果计数

    /**
     * @brief 确保输出目录存在
     */
    int ensureOutputDir();

    /**
     * @brief 写入文件
     */
    int writeFile(const std::string& filepath, const std::string& content);

    /**
     * @brief 转义JSON字符串
     */
    std::string escapeJSON(const std::string& str) const;

    /**
     * @brief 从图片路径提取文件名
     */
    std::string extractFilename(const std::string& path) const;

    /**
     * @brief 获取当前时间戳字符串（用于文件名）
     */
    std::string getTimestampString() const;
};

#endif // _TEXT_ANALYSIS_RESULT_HANDLER_H_
