/**
 * @file ocr_thread.cpp
 * @brief OCR工作线程模块实现
 */

#include "ocr_thread.h"
#include <cstdio>

OCRThread::OCRThread() 
    : ocr_engine_(nullptr)
    , input_queue_(nullptr)
    , output_queue_(nullptr)
    , running_(false)
    , processed_count_(0) {
}

OCRThread::~OCRThread() {
    stop();
    join();
}

int OCRThread::start(OCREngine& ocr_engine, 
                     StringQueue& input_queue, 
                     OCRResultQueue& output_queue) {
    if (running_.load()) {
        printf("[OCRThread] 线程已在运行\n");
        return -1;
    }

    ocr_engine_ = &ocr_engine;
    input_queue_ = &input_queue;
    output_queue_ = &output_queue;
    running_.store(true);
    processed_count_.store(0);

    thread_ = std::thread(&OCRThread::run, this);
    printf("[OCRThread] OCR线程已启动\n");
    
    return 0;
}

void OCRThread::stop() {
    running_.store(false);
    
    // 停止队列，唤醒等待的线程
    if (input_queue_ != nullptr) {
        input_queue_->stop();
    }
    if (output_queue_ != nullptr) {
        output_queue_->stop();
    }
}

void OCRThread::join() {
    if (thread_.joinable()) {
        thread_.join();
    }
}

bool OCRThread::isRunning() const {
    return running_.load();
}

int OCRThread::getProcessedCount() const {
    return processed_count_.load();
}

void OCRThread::run() {
    printf("[OCRThread] 线程主循环开始\n");

    while (running_.load()) {
        std::string image_path;
        
        // 从输入队列获取图片路径
        bool got_item = input_queue_->pop(image_path, true);
        
        if (!got_item) {
            // 队列已停止或为空，退出循环
            if (input_queue_->isStopped()) {
                printf("[OCRThread] 输入队列已停止，线程退出\n");
                break;
            }
            continue;
        }

        printf("[OCRThread] 处理图片: %s\n", image_path.c_str());

        // 执行OCR识别
        OCRResult result;
        int ret = ocr_engine_->recognize(image_path, result, nullptr);

        if (ret != 0) {
            printf("[OCRThread] OCR识别失败: %s\n", image_path.c_str());
            result.success = false;
            result.error_msg = "OCR识别失败";
        } else {
            printf("[OCRThread] OCR识别成功: %s, 识别到 %zu 个文本\n", 
                   image_path.c_str(), result.text_items.size());
        }

        // 将结果放入输出队列
        bool pushed = output_queue_->push(result, true);
        if (!pushed) {
            printf("[OCRThread] 无法将结果放入输出队列\n");
        }

        // 增加处理计数
        processed_count_.fetch_add(1);
    }

    printf("[OCRThread] 线程主循环结束，共处理 %d 张图片\n", 
           processed_count_.load());
}
