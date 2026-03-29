/**
 * @file text_queue.cpp
 * @brief 线程安全队列模块实现
 */

#include "text_queue.h"

/**
 * @brief 构造函数
 */
template<typename T>
ThreadSafeQueue<T>::ThreadSafeQueue(size_t max_size) 
    : max_size_(max_size), stopped_(false) {
}

/**
 * @brief 析构函数
 */
template<typename T>
ThreadSafeQueue<T>::~ThreadSafeQueue() {
    stop();
}

/**
 * @brief 向队列中添加元素
 */
template<typename T>
bool ThreadSafeQueue<T>::push(const T& item, bool block) {
    std::unique_lock<std::mutex> lock(mutex_);
    
    // 如果队列已停止，返回失败
    if (stopped_.load()) {
        return false;
    }
    
    // 如果队列已满
    if (queue_.size() >= max_size_) {
        if (!block) {
            // 非阻塞模式，直接返回失败
            return false;
        }
        // 阻塞模式，等待队列不满
        cond_not_full_.wait(lock, [this] {
            return queue_.size() < max_size_ || stopped_.load();
        });
        
        // 再次检查是否已停止
        if (stopped_.load()) {
            return false;
        }
    }
    
    // 添加元素到队列
    queue_.push(item);
    
    // 通知等待的消费者线程
    cond_not_empty_.notify_one();
    
    return true;
}

/**
 * @brief 从队列中取出元素
 */
template<typename T>
bool ThreadSafeQueue<T>::pop(T& item, bool block) {
    std::unique_lock<std::mutex> lock(mutex_);
    
    // 如果队列空
    if (queue_.empty()) {
        if (!block) {
            // 非阻塞模式，直接返回失败
            return false;
        }
        // 阻塞模式，等待队列不空
        cond_not_empty_.wait(lock, [this] {
            return !queue_.empty() || stopped_.load();
        });
    }
    
    // 检查队列是否为空（可能被stop唤醒）
    if (queue_.empty()) {
        return false;
    }
    
    // 取出元素
    item = queue_.front();
    queue_.pop();
    
    // 通知等待的生产者线程
    cond_not_full_.notify_one();
    
    return true;
}

/**
 * @brief 获取队列当前大小
 */
template<typename T>
size_t ThreadSafeQueue<T>::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.size();
}

/**
 * @brief 检查队列是否为空
 */
template<typename T>
bool ThreadSafeQueue<T>::empty() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.empty();
}

/**
 * @brief 检查队列是否已满
 */
template<typename T>
bool ThreadSafeQueue<T>::full() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.size() >= max_size_;
}

/**
 * @brief 停止队列，唤醒所有等待的线程
 */
template<typename T>
void ThreadSafeQueue<T>::stop() {
    stopped_.store(true);
    cond_not_full_.notify_all();
    cond_not_empty_.notify_all();
}

/**
 * @brief 检查队列是否已停止
 */
template<typename T>
bool ThreadSafeQueue<T>::isStopped() const {
    return stopped_.load();
}

/**
 * @brief 清空队列
 */
template<typename T>
void ThreadSafeQueue<T>::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    while (!queue_.empty()) {
        queue_.pop();
    }
    // 通知等待的生产者线程
    cond_not_full_.notify_all();
}

// 显式实例化模板类
template class ThreadSafeQueue<OCRResult>;
template class ThreadSafeQueue<ProcessingResult>;
template class ThreadSafeQueue<std::string>;
