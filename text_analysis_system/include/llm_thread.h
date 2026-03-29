/**
 * @file llm_thread.h
 * @brief LLM工作线程模块头文件
 * 
 * 实现LLM线程的主循环，从OCR结果队列获取数据，
 * 调用LLM引擎执行分析，将结果传递给结果处理模块
 */

#ifndef _TEXT_ANALYSIS_LLM_THREAD_H_
#define _TEXT_ANALYSIS_LLM_THREAD_H_

#include "llm_engine.h"
#include "text_queue.h"
#include <thread>
#include <atomic>

/**
 * @brief LLM工作线程类
 * 
 * 负责从OCR结果队列获取数据，执行LLM分析，
 * 将分析结果放入处理结果队列
 */
class LLMThread {
public:
    /**
     * @brief 构造函数
     */
    LLMThread();

    /**
     * @brief 析构函数
     */
    ~LLMThread();

    /**
     * @brief 禁止拷贝构造和赋值
     */
    LLMThread(const LLMThread&) = delete;
    LLMThread& operator=(const LLMThread&) = delete;

    /**
     * @brief 启动LLM线程
     * @param llm_engine LLM引擎引用
     * @param input_queue 输入队列（OCR结果）
     * @param output_queue 输出队列（处理结果）
     * @return 成功返回0，失败返回-1
     */
    int start(LLMEngine& llm_engine, 
              OCRResultQueue& input_queue, 
              ProcessingResultQueue& output_queue);

    /**
     * @brief 停止LLM线程
     */
    void stop();

    /**
     * @brief 等待线程结束
     */
    void join();

    /**
     * @brief 检查线程是否正在运行
     * @return 运行中返回true，否则返回false
     */
    bool isRunning() const;

    /**
     * @brief 获取已处理的OCR结果数量
     * @return 处理数量
     */
    int getProcessedCount() const;

private:
    /**
     * @brief 线程主循环函数
     */
    void run();

    LLMEngine* llm_engine_;                 // LLM引擎指针
    OCRResultQueue* input_queue_;           // 输入队列（OCR结果）
    ProcessingResultQueue* output_queue_;   // 输出队列（处理结果）
    std::thread thread_;                    // 工作线程
    std::atomic<bool> running_;             // 运行标志
    std::atomic<int> processed_count_;      // 已处理数量
};

#endif // _TEXT_ANALYSIS_LLM_THREAD_H_
