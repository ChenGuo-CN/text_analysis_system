/**
 * @file text_queue.h
 * @brief 线程安全队列模块头文件
 * 
 * 提供用于OCR结果传递的线程安全队列
 * 支持最大容量限制，使用条件变量实现高效等待
 */

#ifndef _TEXT_ANALYSIS_TEXT_QUEUE_H_
#define _TEXT_ANALYSIS_TEXT_QUEUE_H_

#include <queue>
#include <mutex>
#include <condition_variable>
#include <string>
#include <vector>
#include <atomic>

/**
 * @brief 文本框坐标结构
 */
typedef struct {
    int x;
    int y;
} Point;

/**
 * @brief 文本框结构
 */
typedef struct {
    Point left_top;
    Point right_top;
    Point right_bottom;
    Point left_bottom;
    float score;
} TextBox;

/**
 * @brief 单条文本识别结果
 */
typedef struct {
    std::string text;           // 识别文本
    float confidence;           // 置信度
    TextBox box;                // 文本框坐标
} TextItem;

/**
 * @brief OCR性能统计（简化版，避免与perf_monitor.h冲突）
 */
typedef struct {
    float det_time_ms;          // 检测耗时
    float rec_time_ms;          // 识别耗时
    float total_time_ms;        // 总耗时
} QueueOCRPerfStats;

/**
 * @brief LLM性能统计（简化版）
 */
typedef struct {
    float prefill_time_ms;      // Prefill耗时
    float generate_time_ms;     // Generate耗时
    float total_time_ms;        // 总耗时
} QueueLLMPerfStats;

/**
 * @brief OCR结果结构体
 * 
 * 存储单张图片的OCR识别结果，用于在OCR线程和LLM线程间传递
 */
typedef struct {
    std::string image_path;                 // 图片路径
    std::vector<TextItem> text_items;       // 识别到的文本列表
    QueueOCRPerfStats perf_stats;           // 性能统计
    std::string timestamp;                  // 处理时间戳
    bool success;                           // 处理是否成功
    std::string error_msg;                  // 错误信息（如果失败）
} OCRResult;

/**
 * @brief LLM分析结果结构体
 */
typedef struct {
    std::string analysis_text;              // 分析结果文本
    std::string raw_response;               // 原始LLM响应
    QueueLLMPerfStats perf_stats;           // 性能统计
    bool success;                           // 分析是否成功
    std::string error_msg;                  // 错误信息（如果失败）
} LLMResult;

/**
 * @brief 完整的处理结果
 */
typedef struct {
    OCRResult ocr_result;                   // OCR结果
    LLMResult llm_result;                   // LLM分析结果
    float end_to_end_time_ms;               // 端到端总耗时
} ProcessingResult;

/**
 * @brief 线程安全队列模板类
 * 
 * 支持多生产者单消费者模式
 * 使用条件变量实现高效的阻塞等待
 */
template<typename T>
class ThreadSafeQueue {
public:
    /**
     * @brief 构造函数
     * @param max_size 队列最大容量，默认20
     */
    explicit ThreadSafeQueue(size_t max_size = 20);

    /**
     * @brief 析构函数
     */
    ~ThreadSafeQueue();

    /**
     * @brief 禁止拷贝构造
     */
    ThreadSafeQueue(const ThreadSafeQueue&) = delete;
    ThreadSafeQueue& operator=(const ThreadSafeQueue&) = delete;

    /**
     * @brief 向队列中添加元素
     * @param item 要添加的元素
     * @param block 如果队列已满是否阻塞等待，默认为true
     * @return 成功返回true，失败返回false（非阻塞模式下队列满）
     */
    bool push(const T& item, bool block = true);

    /**
     * @brief 从队列中取出元素
     * @param item 用于存储取出的元素
     * @param block 如果队列空是否阻塞等待，默认为true
     * @return 成功返回true，失败返回false（非阻塞模式下队列空或队列已停止）
     */
    bool pop(T& item, bool block = true);

    /**
     * @brief 获取队列当前大小
     * @return 队列元素数量
     */
    size_t size() const;

    /**
     * @brief 检查队列是否为空
     * @return 空返回true，否则返回false
     */
    bool empty() const;

    /**
     * @brief 检查队列是否已满
     * @return 满返回true，否则返回false
     */
    bool full() const;

    /**
     * @brief 停止队列，唤醒所有等待的线程
     * 调用后push/pop操作将返回false
     */
    void stop();

    /**
     * @brief 检查队列是否已停止
     * @return 已停止返回true，否则返回false
     */
    bool isStopped() const;

    /**
     * @brief 清空队列
     */
    void clear();

private:
    std::queue<T> queue_;                   // 底层队列
    mutable std::mutex mutex_;              // 互斥锁
    std::condition_variable cond_not_full_; // 队列不满条件变量
    std::condition_variable cond_not_empty_; // 队列不空条件变量
    size_t max_size_;                       // 最大容量
    std::atomic<bool> stopped_;             // 停止标志
};

// 类型别名，方便使用
using OCRResultQueue = ThreadSafeQueue<OCRResult>;
using ProcessingResultQueue = ThreadSafeQueue<ProcessingResult>;
using StringQueue = ThreadSafeQueue<std::string>;

#endif // _TEXT_ANALYSIS_TEXT_QUEUE_H_
