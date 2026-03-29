/**
 * @file ocr_thread.h
 * @brief OCR工作线程模块头文件
 * 
 * 实现OCR线程的主循环，从输入队列获取图片路径，
 * 调用OCR引擎执行推理，将结果放入输出队列
 */

#ifndef _TEXT_ANALYSIS_OCR_THREAD_H_
#define _TEXT_ANALYSIS_OCR_THREAD_H_

#include "ocr_engine.h"
#include "text_queue.h"
#include <thread>
#include <atomic>
#include <string>

/**
 * @brief OCR工作线程类
 * 
 * 负责从图片路径队列读取图片，执行OCR识别，
 * 将识别结果放入OCR结果队列
 */
class OCRThread {
public:
    /**
     * @brief 构造函数
     */
    OCRThread();

    /**
     * @brief 析构函数
     */
    ~OCRThread();

    /**
     * @brief 禁止拷贝构造和赋值
     */
    OCRThread(const OCRThread&) = delete;
    OCRThread& operator=(const OCRThread&) = delete;

    /**
     * @brief 启动OCR线程
     * @param ocr_engine OCR引擎引用
     * @param input_queue 输入队列（图片路径）
     * @param output_queue 输出队列（OCR结果）
     * @return 成功返回0，失败返回-1
     */
    int start(OCREngine& ocr_engine, 
              StringQueue& input_queue, 
              OCRResultQueue& output_queue);

    /**
     * @brief 停止OCR线程
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
     * @brief 获取已处理的图片数量
     * @return 处理数量
     */
    int getProcessedCount() const;

private:
    /**
     * @brief 线程主循环函数
     */
    void run();

    OCREngine* ocr_engine_;             // OCR引擎指针
    StringQueue* input_queue_;          // 输入队列
    OCRResultQueue* output_queue_;      // 输出队列
    std::thread thread_;                // 工作线程
    std::atomic<bool> running_;         // 运行标志
    std::atomic<int> processed_count_;  // 已处理数量
};

#endif // _TEXT_ANALYSIS_OCR_THREAD_H_
