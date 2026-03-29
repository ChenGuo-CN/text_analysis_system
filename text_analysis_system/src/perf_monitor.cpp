/**
 * @file perf_monitor.cpp
 * @brief 性能监控模块实现
 */

#include "perf_monitor.h"
#include <sstream>
#include <iomanip>
#include <ctime>

// ==================== Timer 实现 ====================

Timer::Timer() {
    reset();
}

void Timer::reset() {
    start_time_ = std::chrono::high_resolution_clock::now();
}

float Timer::elapsed_ms() const {
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time_);
    return duration.count() / 1000.0f;
}

float Timer::elapsed_us() const {
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time_);
    return static_cast<float>(duration.count());
}

// ==================== PerfMonitor 实现 ====================

PerfMonitor::PerfMonitor(bool enable) 
    : enabled_(enable) {
    reset();
}

void PerfMonitor::startStage(const std::string& stage_name) {
    if (!enabled_) return;
    
    current_stage_ = stage_name;
    timer_.reset();
}

float PerfMonitor::endStage() {
    if (!enabled_) return 0.0f;
    
    float elapsed = timer_.elapsed_ms();
    return elapsed;
}

void PerfMonitor::recordOCRDet(float time_ms) {
    if (!enabled_) return;
    ocr_stats_.det_time_ms = time_ms;
}

void PerfMonitor::recordOCRRec(float time_ms) {
    if (!enabled_) return;
    ocr_stats_.rec_time_ms = time_ms;
}

void PerfMonitor::recordOCRTotal(float time_ms) {
    if (!enabled_) return;
    ocr_stats_.total_time_ms = time_ms;
}

void PerfMonitor::recordLLMPrefill(float time_ms, int input_tokens) {
    if (!enabled_) return;
    llm_stats_.prefill_time_ms = time_ms;
    llm_stats_.input_tokens = input_tokens;
}

void PerfMonitor::recordLLMGenerate(float time_ms, int output_tokens) {
    if (!enabled_) return;
    llm_stats_.generate_time_ms = time_ms;
    llm_stats_.output_tokens = output_tokens;
    
    // 计算生成速度
    if (time_ms > 0 && output_tokens > 0) {
        llm_stats_.tokens_per_sec = (output_tokens * 1000.0f) / time_ms;
    }
}

void PerfMonitor::recordLLMTotal(float time_ms) {
    if (!enabled_) return;
    llm_stats_.total_time_ms = time_ms;
}

void PerfMonitor::recordEndToEnd(float time_ms) {
    if (!enabled_) return;
    e2e_stats_.total_time_ms = time_ms;
}

OCRPerfStats PerfMonitor::getOCRStats() const {
    return ocr_stats_;
}

LLMPerfStats PerfMonitor::getLLMStats() const {
    return llm_stats_;
}

EndToEndPerfStats PerfMonitor::getEndToEndStats() const {
    return e2e_stats_;
}

void PerfMonitor::printStats() const {
    if (!enabled_) return;
    
    printf("\n========================================\n");
    printf("  性能统计\n");
    printf("========================================\n");
    
    // OCR统计
    printf("\n[OCR性能]\n");
    printf("  检测耗时: %.2f ms\n", ocr_stats_.det_time_ms);
    printf("  识别耗时: %.2f ms\n", ocr_stats_.rec_time_ms);
    printf("  总耗时:   %.2f ms\n", ocr_stats_.total_time_ms);
    
    // LLM统计
    printf("\n[LLM性能]\n");
    printf("  Prefill耗时:  %.2f ms", llm_stats_.prefill_time_ms);
    if (llm_stats_.input_tokens > 0) {
        printf(" (%d tokens)\n", llm_stats_.input_tokens);
    } else {
        printf("\n");
    }
    
    printf("  Generate耗时: %.2f ms", llm_stats_.generate_time_ms);
    if (llm_stats_.output_tokens > 0) {
        printf(" (%d tokens, %.2f tokens/s)\n", 
               llm_stats_.output_tokens, llm_stats_.tokens_per_sec);
    } else {
        printf("\n");
    }
    printf("  总耗时:       %.2f ms\n", llm_stats_.total_time_ms);
    
    // 端到端统计
    printf("\n[端到端性能]\n");
    printf("  总耗时: %.2f ms\n", e2e_stats_.total_time_ms);
    
    printf("========================================\n");
}

std::string PerfMonitor::toJSON() const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2);
    
    oss << "{\n";
    
    // OCR统计
    oss << "  \"ocr\": {\n";
    oss << "    \"det_time_ms\": " << ocr_stats_.det_time_ms << ",\n";
    oss << "    \"rec_time_ms\": " << ocr_stats_.rec_time_ms << ",\n";
    oss << "    \"total_time_ms\": " << ocr_stats_.total_time_ms << "\n";
    oss << "  },\n";
    
    // LLM统计
    oss << "  \"llm\": {\n";
    oss << "    \"prefill_time_ms\": " << llm_stats_.prefill_time_ms << ",\n";
    oss << "    \"generate_time_ms\": " << llm_stats_.generate_time_ms << ",\n";
    oss << "    \"total_time_ms\": " << llm_stats_.total_time_ms << ",\n";
    oss << "    \"input_tokens\": " << llm_stats_.input_tokens << ",\n";
    oss << "    \"output_tokens\": " << llm_stats_.output_tokens << ",\n";
    oss << "    \"tokens_per_sec\": " << llm_stats_.tokens_per_sec << "\n";
    oss << "  },\n";
    
    // 端到端统计
    oss << "  \"end_to_end\": {\n";
    oss << "    \"total_time_ms\": " << e2e_stats_.total_time_ms << "\n";
    oss << "  }\n";
    
    oss << "}";
    
    return oss.str();
}

void PerfMonitor::reset() {
    ocr_stats_ = {0.0f, 0.0f, 0.0f};
    llm_stats_ = {0.0f, 0.0f, 0.0f, 0, 0, 0.0f};
    e2e_stats_ = {0.0f, 0.0f, 0.0f};
    current_stage_.clear();
}

bool PerfMonitor::isEnabled() const {
    return enabled_;
}

// ==================== ScopedTimer 实现 ====================

ScopedTimer::ScopedTimer(PerfMonitor& monitor, const std::string& stage_name)
    : monitor_(monitor), stage_name_(stage_name) {
    monitor_.startStage(stage_name_);
}

ScopedTimer::~ScopedTimer() {
    monitor_.endStage();
}

// ==================== 工具函数实现 ====================

std::string getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    
    std::tm tm_now;
    localtime_r(&time_t_now, &tm_now);
    
    std::ostringstream oss;
    oss << std::put_time(&tm_now, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

long long getCurrentTimeMillis() {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
}
