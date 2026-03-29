/**
 * @file result_handler.cpp
 * @brief 结果处理模块实现
 */

#include "result_handler.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <sys/stat.h>
#include <cstdio>
#include <cstring>

ResultHandler::ResultHandler() : result_count_(0) {
}

ResultHandler::~ResultHandler() {
}

int ResultHandler::initialize(const Config& config) {
    config_ = config;
    output_dir_ = config.output.result_dir;
    result_count_ = 0;

    // 确保输出目录存在
    int ret = ensureOutputDir();
    if (ret != 0) {
        printf("[ResultHandler] 创建输出目录失败: %s\n", output_dir_.c_str());
        return -1;
    }

    printf("[ResultHandler] 结果处理器初始化完成，输出目录: %s\n", output_dir_.c_str());
    return 0;
}

int ResultHandler::saveResult(const ProcessingResult& result) {
    std::lock_guard<std::mutex> lock(mutex_);

    // 生成结果文件路径
    std::string filename = generateResultFilename(result.ocr_result.image_path);
    std::string filepath = output_dir_ + "/" + filename;

    // 转换为JSON
    std::string json_content = resultToJSON(result);

    // 写入文件
    int ret = writeFile(filepath, json_content);
    if (ret != 0) {
        printf("[ResultHandler] 保存结果失败: %s\n", filepath.c_str());
        return -1;
    }

    result_count_++;
    printf("[ResultHandler] 结果已保存: %s\n", filepath.c_str());

    return 0;
}

int ResultHandler::saveBatchSummary(const std::vector<ProcessingResult>& results,
                                    const BatchStats& stats) {
    std::lock_guard<std::mutex> lock(mutex_);

    // 生成汇总文件名
    std::string filename = "batch_summary_" + getTimestampString() + ".json";
    std::string filepath = output_dir_ + "/" + filename;

    // 转换为JSON
    std::string json_content = batchStatsToJSON(results, stats);

    // 写入文件
    int ret = writeFile(filepath, json_content);
    if (ret != 0) {
        printf("[ResultHandler] 保存汇总结果失败: %s\n", filepath.c_str());
        return -1;
    }

    printf("[ResultHandler] 汇总结果已保存: %s\n", filepath.c_str());

    return 0;
}

std::string ResultHandler::resultToJSON(const ProcessingResult& result) const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(3);

    oss << "{\n";

    // 基本信息
    oss << "  \"image_path\": \"" << escapeJSON(result.ocr_result.image_path) << "\",\n";
    oss << "  \"timestamp\": \"" << result.ocr_result.timestamp << "\",\n";

    // OCR结果
    oss << "  \"ocr_result\": {\n";
    oss << "    \"success\": " << (result.ocr_result.success ? "true" : "false") << ",\n";
    if (!result.ocr_result.success) {
        oss << "    \"error_msg\": \"" << escapeJSON(result.ocr_result.error_msg) << "\",\n";
    }
    oss << "    \"text_count\": " << result.ocr_result.text_items.size() << ",\n";
    oss << "    \"texts\": [\n";
    for (size_t i = 0; i < result.ocr_result.text_items.size(); i++) {
        const auto& item = result.ocr_result.text_items[i];
        oss << "      {\n";
        oss << "        \"text\": \"" << escapeJSON(item.text) << "\",\n";
        oss << "        \"confidence\": " << item.confidence << ",\n";
        oss << "        \"box\": {\n";
        oss << "          \"left_top\": [" << item.box.left_top.x << ", " << item.box.left_top.y << "],\n";
        oss << "          \"right_top\": [" << item.box.right_top.x << ", " << item.box.right_top.y << "],\n";
        oss << "          \"right_bottom\": [" << item.box.right_bottom.x << ", " << item.box.right_bottom.y << "],\n";
        oss << "          \"left_bottom\": [" << item.box.left_bottom.x << ", " << item.box.left_bottom.y << "]\n";
        oss << "        }\n";
        oss << "      }";
        if (i < result.ocr_result.text_items.size() - 1) {
            oss << ",";
        }
        oss << "\n";
    }
    oss << "    ],\n";
    oss << "    \"det_time_ms\": " << result.ocr_result.perf_stats.det_time_ms << ",\n";
    oss << "    \"rec_time_ms\": " << result.ocr_result.perf_stats.rec_time_ms << ",\n";
    oss << "    \"inference_time_ms\": " << result.ocr_result.perf_stats.total_time_ms << "\n";
    oss << "  },\n";

    // LLM分析结果
    oss << "  \"llm_analysis\": {\n";
    oss << "    \"success\": " << (result.llm_result.success ? "true" : "false") << ",\n";
    if (!result.llm_result.success) {
        oss << "    \"error_msg\": \"" << escapeJSON(result.llm_result.error_msg) << "\",\n";
    }
    oss << "    \"analysis_text\": \"" << escapeJSON(result.llm_result.analysis_text) << "\",\n";
    oss << "    \"inference_time_ms\": " << result.llm_result.perf_stats.total_time_ms << "\n";
    oss << "  }\n";

    oss << "}";

    return oss.str();
}

std::string ResultHandler::batchStatsToJSON(const std::vector<ProcessingResult>& results,
                                            const BatchStats& stats) const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(3);

    oss << "{\n";

    // 统计信息
    oss << "  \"summary\": {\n";
    oss << "    \"total_count\": " << stats.total_count << ",\n";
    oss << "    \"success_count\": " << stats.success_count << ",\n";
    oss << "    \"failed_count\": " << stats.failed_count << ",\n";
    oss << "    \"avg_ocr_time_ms\": " << stats.avg_ocr_time_ms << ",\n";
    oss << "    \"avg_llm_time_ms\": " << stats.avg_llm_time_ms << ",\n";
    oss << "    \"total_time_ms\": " << stats.total_time_ms << "\n";
    oss << "  },\n";

    // 详细结果
    oss << "  \"results\": [\n";
    for (size_t i = 0; i < results.size(); i++) {
        oss << "    {\n";
        oss << "      \"image_path\": \"" << escapeJSON(results[i].ocr_result.image_path) << "\",\n";
        oss << "      \"ocr_success\": " << (results[i].ocr_result.success ? "true" : "false") << ",\n";
        oss << "      \"llm_success\": " << (results[i].llm_result.success ? "true" : "false") << ",\n";
        oss << "      \"ocr_time_ms\": " << results[i].ocr_result.perf_stats.total_time_ms << ",\n";
        oss << "      \"llm_time_ms\": " << results[i].llm_result.perf_stats.total_time_ms << "\n";
        oss << "    }";
        if (i < results.size() - 1) {
            oss << ",";
        }
        oss << "\n";
    }
    oss << "  ]\n";

    oss << "}";

    return oss.str();
}

std::string ResultHandler::getOutputDir() const {
    return output_dir_;
}

std::string ResultHandler::generateResultFilename(const std::string& image_path) const {
    std::string filename = extractFilename(image_path);

    // 移除扩展名
    size_t dot_pos = filename.find_last_of('.');
    if (dot_pos != std::string::npos) {
        filename = filename.substr(0, dot_pos);
    }

    // 添加时间戳和扩展名
    filename += "_" + getTimestampString() + ".json";

    return filename;
}

int ResultHandler::ensureOutputDir() {
    struct stat st;
    if (stat(output_dir_.c_str(), &st) != 0) {
        // 目录不存在，创建它
        std::string cmd = "mkdir -p " + output_dir_;
        int ret = system(cmd.c_str());
        if (ret != 0) {
            return -1;
        }
    }
    return 0;
}

int ResultHandler::writeFile(const std::string& filepath, const std::string& content) {
    std::ofstream file(filepath);
    if (!file.is_open()) {
        return -1;
    }

    file << content;
    file.close();

    return 0;
}

std::string ResultHandler::escapeJSON(const std::string& str) const {
    std::ostringstream oss;
    for (unsigned char c : str) {
        switch (c) {
            case '"': oss << "\\\""; break;
            case '\\': oss << "\\\\"; break;
            case '\b': oss << "\\b"; break;
            case '\f': oss << "\\f"; break;
            case '\n': oss << "\\n"; break;
            case '\r': oss << "\\r"; break;
            case '\t': oss << "\\t"; break;
            default:
                // 仅转义控制字符(0x00-0x1F)，保留UTF-8字节原样
                if (c < 0x20) {
                    oss << "\\u" << std::hex << std::setw(4) << std::setfill('0') << (unsigned int)c;
                } else {
                    oss << c;  // 保留UTF-8字节不变
                }
        }
    }
    return oss.str();
}

std::string ResultHandler::extractFilename(const std::string& path) const {
    size_t last_slash = path.find_last_of("/\\");
    if (last_slash != std::string::npos) {
        return path.substr(last_slash + 1);
    }
    return path;
}

std::string ResultHandler::getTimestampString() const {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);

    std::tm tm_now;
    localtime_r(&time_t_now, &tm_now);

    std::ostringstream oss;
    oss << std::put_time(&tm_now, "%Y%m%d_%H%M%S");
    return oss.str();
}
