/**
 * @file llm_thread.cpp
 * @brief LLM工作线程模块实现
 */

#include "llm_thread.h"
#include <cstdio>

LLMThread::LLMThread() 
    : llm_engine_(nullptr)
    , input_queue_(nullptr)
    , output_queue_(nullptr)
    , running_(false)
    , processed_count_(0) {
}

LLMThread::~LLMThread() {
    stop();
    join();
}

int LLMThread::start(LLMEngine& llm_engine, 
                     OCRResultQueue& input_queue, 
                     ProcessingResultQueue& output_queue) {
    if (running_.load()) {
        printf("[LLMThread] 线程已在运行\n");
        return -1;
    }

    llm_engine_ = &llm_engine;
    input_queue_ = &input_queue;
    output_queue_ = &output_queue;
    running_.store(true);
    processed_count_.store(0);

    thread_ = std::thread(&LLMThread::run, this);
    printf("[LLMThread] LLM线程已启动\n");
    
    return 0;
}

void LLMThread::stop() {
    running_.store(false);
    
    // 停止队列，唤醒等待的线程
    if (input_queue_ != nullptr) {
        input_queue_->stop();
    }
    if (output_queue_ != nullptr) {
        output_queue_->stop();
    }
}

void LLMThread::join() {
    if (thread_.joinable()) {
        thread_.join();
    }
}

bool LLMThread::isRunning() const {
    return running_.load();
}

int LLMThread::getProcessedCount() const {
    return processed_count_.load();
}

void LLMThread::run() {
    printf("[LLMThread] 线程主循环开始\n");

    while (running_.load()) {
        OCRResult ocr_result;
        
        // 从输入队列获取OCR结果
        bool got_item = input_queue_->pop(ocr_result, true);
        
        if (!got_item) {
            // 队列已停止或为空，退出循环
            if (input_queue_->isStopped()) {
                printf("[LLMThread] 输入队列已停止，线程退出\n");
                break;
            }
            continue;
        }

        printf("[LLMThread] 分析图片: %s\n", ocr_result.image_path.c_str());

        // 创建处理结果
        ProcessingResult processing_result;
        processing_result.ocr_result = ocr_result;

        // 执行LLM分析
        LLMResult llm_result;
        int ret = llm_engine_->analyze(ocr_result, llm_result, nullptr);

        if (ret != 0) {
            printf("[LLMThread] LLM分析失败: %s\n", ocr_result.image_path.c_str());
            llm_result.success = false;
            llm_result.error_msg = "LLM分析失败";
        } else {
            printf("[LLMThread] LLM分析成功: %s\n", ocr_result.image_path.c_str());
        }

        processing_result.llm_result = llm_result;

        // 将结果放入输出队列
        bool pushed = output_queue_->push(processing_result, true);
        if (!pushed) {
            printf("[LLMThread] 无法将结果放入输出队列\n");
        }

        // 增加处理计数
        processed_count_.fetch_add(1);
    }

    printf("[LLMThread] 线程主循环结束，共处理 %d 个OCR结果\n", 
           processed_count_.load());
}
