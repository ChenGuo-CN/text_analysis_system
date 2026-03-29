/**
 * @file main.cpp
 * @brief 文本分析系统主程序
 * 
 * 整合PPOCR和RKLLM的端到端文本分析系统
 * 支持多线程处理、NPU核心分配、JSON配置和结果存储
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <signal.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <linux/limits.h>
#include <vector>
#include <string>
#include <algorithm>

#include "config.h"
#include "ocr_engine.h"
#include "ocr_thread.h"
#include "llm_engine.h"
#include "llm_thread.h"
#include "result_handler.h"
#include "text_queue.h"
#include "perf_monitor.h"

// 全局变量，用于信号处理
static volatile bool g_running = true;

// 可执行文件所在目录
static std::string g_exec_dir;

/**
 * @brief 获取可执行文件所在目录
 */
std::string getExecutableDir() {
    char path[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", path, sizeof(path) - 1);
    if (len != -1) {
        path[len] = '\0';
        std::string full_path(path);
        size_t last_slash = full_path.find_last_of("/");
        if (last_slash != std::string::npos) {
            return full_path.substr(0, last_slash);
        }
    }
    return ".";
}

/**
 * @brief 将相对路径转换为基于可执行文件目录的绝对路径
 */
std::string resolvePath(const std::string& path) {
    if (path.empty() || path[0] == '/') {
        // 已经是绝对路径或空路径
        return path;
    }
    return g_exec_dir + "/" + path;
}

/**
 * @brief 信号处理函数
 */
void signalHandler(int sig) {
    printf("\n[Main] 接收到信号 %d，正在优雅退出...\n", sig);
    g_running = false;
}

/**
 * @brief 打印使用说明
 */
void printUsage(const char* program_name) {
    printf("用法: %s <图片路径或文件夹> [选项]\n", program_name);
    printf("\n选项:\n");
    printf("  -c, --config <路径>    指定配置文件路径 (默认: config/config.json)\n");
    printf("  -h, --help             显示此帮助信息\n");
    printf("\n示例:\n");
    printf("  %s /path/to/image.jpg\n", program_name);
    printf("  %s /path/to/images/ --config my_config.json\n", program_name);
}

/**
 * @brief 检查文件是否为支持的图片格式
 */
bool isImageFile(const std::string& filename) {
    std::string lower = filename;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    
    return (lower.find(".jpg") != std::string::npos ||
            lower.find(".jpeg") != std::string::npos ||
            lower.find(".png") != std::string::npos ||
            lower.find(".bmp") != std::string::npos);
}

/**
 * @brief 获取文件夹中的所有图片文件
 */
std::vector<std::string> getImageFiles(const std::string& path) {
    std::vector<std::string> files;
    
    struct stat st;
    if (stat(path.c_str(), &st) != 0) {
        printf("[Main] 路径不存在: %s\n", path.c_str());
        return files;
    }
    
    if (S_ISREG(st.st_mode)) {
        // 是文件
        if (isImageFile(path)) {
            files.push_back(path);
        }
    } else if (S_ISDIR(st.st_mode)) {
        // 是目录
        DIR* dir = opendir(path.c_str());
        if (dir != nullptr) {
            struct dirent* entry;
            while ((entry = readdir(dir)) != nullptr) {
                std::string name = entry->d_name;
                if (name != "." && name != ".." && isImageFile(name)) {
                    std::string full_path = path;
                    if (full_path.back() != '/') {
                        full_path += "/";
                    }
                    full_path += name;
                    files.push_back(full_path);
                }
            }
            closedir(dir);
        }
    }
    
    return files;
}

/**
 * @brief 文本分析系统主类
 */
class TextAnalysisSystem {
public:
    TextAnalysisSystem() : ocr_thread_(), llm_thread_() {}
    
    ~TextAnalysisSystem() {
        shutdown();
    }
    
    /**
     * @brief 初始化系统
     */
    int initialize(const std::string& config_path, const std::string& base_dir) {
        printf("========================================\n");
        printf("  文本分析系统\n");
        printf("========================================\n\n");

        // 加载配置
        printf("[Main] 加载配置文件: %s\n", config_path.c_str());
        Config& config = getConfig();
        int ret = config.loadFromFile(config_path, base_dir);
        if (ret != 0) {
            printf("[Main] 加载配置文件失败，使用默认配置\n");
        }
        
        // 验证配置
        if (!config.validate()) {
            printf("[Main] 配置验证失败\n");
            return -1;
        }
        
        // 打印配置
        config.print();
        
        // 初始化OCR引擎
        printf("\n[Main] 初始化OCR引擎...\n");
        ret = ocr_engine_.initialize(config);
        if (ret != 0) {
            printf("[Main] OCR引擎初始化失败\n");
            return -1;
        }
        
        // 初始化LLM引擎
        printf("\n[Main] 初始化LLM引擎...\n");
        ret = llm_engine_.initialize(config);
        if (ret != 0) {
            printf("[Main] LLM引擎初始化失败\n");
            ocr_engine_.release();
            return -1;
        }
        
        // 初始化结果处理器
        printf("\n[Main] 初始化结果处理器...\n");
        ret = result_handler_.initialize(config);
        if (ret != 0) {
            printf("[Main] 结果处理器初始化失败\n");
            llm_engine_.release();
            ocr_engine_.release();
            return -1;
        }
        
        // 创建队列
        int queue_size = config.queue.max_size;
        image_queue_ = new StringQueue(queue_size);
        ocr_result_queue_ = new OCRResultQueue(queue_size);
        processing_result_queue_ = new ProcessingResultQueue(queue_size);
        
        printf("\n[Main] 系统初始化完成\n");
        return 0;
    }
    
    /**
     * @brief 启动系统
     */
    int start() {
        printf("\n[Main] 启动工作线程...\n");
        
        // 启动OCR线程
        int ret = ocr_thread_.start(ocr_engine_, *image_queue_, *ocr_result_queue_);
        if (ret != 0) {
            printf("[Main] 启动OCR线程失败\n");
            return -1;
        }
        
        // 启动LLM线程
        ret = llm_thread_.start(llm_engine_, *ocr_result_queue_, *processing_result_queue_);
        if (ret != 0) {
            printf("[Main] 启动LLM线程失败\n");
            ocr_thread_.stop();
            ocr_thread_.join();
            return -1;
        }
        
        printf("[Main] 工作线程已启动\n");
        return 0;
    }
    
    /**
     * @brief 处理图片
     */
    int processImages(const std::vector<std::string>& image_files) {
        printf("\n[Main] 开始处理 %zu 张图片...\n\n", image_files.size());
        
        // 将图片路径加入队列
        for (const auto& path : image_files) {
            if (!g_running) break;
            
            printf("[Main] 添加图片到队列: %s\n", path.c_str());
            bool pushed = image_queue_->push(path, true);
            if (!pushed) {
                printf("[Main] 添加图片失败: %s\n", path.c_str());
            }
        }
        
        // 等待所有图片处理完成
        printf("\n[Main] 等待处理完成...\n");
        
        int processed_count = 0;
        int expected_count = image_files.size();
        std::vector<ProcessingResult> all_results;
        
        while (g_running && processed_count < expected_count) {
            ProcessingResult result;
            bool got_result = processing_result_queue_->pop(result, false);
            
            if (got_result) {
                // 保存结果
                result_handler_.saveResult(result);
                all_results.push_back(result);
                processed_count++;
                
                printf("[Main] 已完成 %d/%d\n", processed_count, expected_count);
            } else {
                // 短暂休眠，避免CPU占用过高
                usleep(10000);  // 10ms
            }
            
            // 检查线程是否还在运行
            if (!ocr_thread_.isRunning() && !llm_thread_.isRunning()) {
                break;
            }
        }
        
        // 停止队列
        image_queue_->stop();
        ocr_result_queue_->stop();
        processing_result_queue_->stop();
        
        // 等待线程结束
        ocr_thread_.stop();
        llm_thread_.stop();
        ocr_thread_.join();
        llm_thread_.join();
        
        // 保存批量汇总
        if (!all_results.empty()) {
            BatchStats stats;
            stats.total_count = all_results.size();
            stats.success_count = 0;
            stats.failed_count = 0;
            float total_ocr_time = 0;
            float total_llm_time = 0;
            
            for (const auto& result : all_results) {
                if (result.ocr_result.success && result.llm_result.success) {
                    stats.success_count++;
                } else {
                    stats.failed_count++;
                }
                total_ocr_time += result.ocr_result.perf_stats.total_time_ms;
                total_llm_time += result.llm_result.perf_stats.total_time_ms;
            }
            
            stats.avg_ocr_time_ms = total_ocr_time / all_results.size();
            stats.avg_llm_time_ms = total_llm_time / all_results.size();
            stats.total_time_ms = total_ocr_time + total_llm_time;
            
            result_handler_.saveBatchSummary(all_results, stats);
        }
        
        printf("\n[Main] 处理完成，共处理 %d 张图片\n", processed_count);
        
        return 0;
    }
    
    /**
     * @brief 关闭系统
     */
    void shutdown() {
        printf("\n[Main] 正在关闭系统...\n");
        
        // 停止队列
        if (image_queue_ != nullptr) {
            image_queue_->stop();
        }
        if (ocr_result_queue_ != nullptr) {
            ocr_result_queue_->stop();
        }
        if (processing_result_queue_ != nullptr) {
            processing_result_queue_->stop();
        }
        
        // 停止线程
        ocr_thread_.stop();
        llm_thread_.stop();
        
        // 等待线程结束
        ocr_thread_.join();
        llm_thread_.join();
        
        // 释放引擎
        llm_engine_.release();
        ocr_engine_.release();
        
        // 释放队列
        delete image_queue_;
        delete ocr_result_queue_;
        delete processing_result_queue_;
        image_queue_ = nullptr;
        ocr_result_queue_ = nullptr;
        processing_result_queue_ = nullptr;
        
        printf("[Main] 系统已关闭\n");
    }

private:
    OCREngine ocr_engine_;
    LLMEngine llm_engine_;
    OCRThread ocr_thread_;
    LLMThread llm_thread_;
    ResultHandler result_handler_;
    
    StringQueue* image_queue_ = nullptr;
    OCRResultQueue* ocr_result_queue_ = nullptr;
    ProcessingResultQueue* processing_result_queue_ = nullptr;
};

/**
 * @brief 主函数
 */
int main(int argc, char** argv) {
    // 检查参数
    if (argc < 2) {
        printUsage(argv[0]);
        return -1;
    }
    
    // 获取可执行文件所在目录
    g_exec_dir = getExecutableDir();
    printf("[Main] 可执行文件目录: %s\n", g_exec_dir.c_str());
    
    // 解析参数
    std::string image_path;
    std::string config_path = "config/config.json";
    
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            printUsage(argv[0]);
            return 0;
        } else if ((arg == "-c" || arg == "--config") && i + 1 < argc) {
            config_path = argv[++i];
        } else if (image_path.empty()) {
            image_path = arg;
        }
    }
    
    if (image_path.empty()) {
        printf("[Main] 错误: 未指定图片路径\n");
        printUsage(argv[0]);
        return -1;
    }
    
    // 解析配置文件路径（如果是相对路径，则基于可执行文件目录）
    std::string full_config_path = resolvePath(config_path);
    
    // 如果配置文件不存在，尝试在父目录查找（适用于从build目录运行的情况）
    struct stat st;
    if (stat(full_config_path.c_str(), &st) != 0) {
        std::string parent_dir_config = g_exec_dir + "/../" + config_path;
        if (stat(parent_dir_config.c_str(), &st) == 0) {
            full_config_path = parent_dir_config;
        }
    }
    
    printf("[Main] 配置文件路径: %s\n", full_config_path.c_str());
    
    // 设置信号处理
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    // 获取图片文件列表
    std::vector<std::string> image_files = getImageFiles(image_path);
    if (image_files.empty()) {
        printf("[Main] 未找到图片文件: %s\n", image_path.c_str());
        return -1;
    }
    
    printf("[Main] 找到 %zu 个图片文件\n", image_files.size());
    
    // 创建系统实例
    TextAnalysisSystem system;

    // 初始化系统（传入基础目录用于解析相对路径）
    int ret = system.initialize(full_config_path, g_exec_dir);
    if (ret != 0) {
        printf("[Main] 系统初始化失败\n");
        return -1;
    }
    
    // 启动系统
    ret = system.start();
    if (ret != 0) {
        printf("[Main] 系统启动失败\n");
        return -1;
    }
    
    // 处理图片
    ret = system.processImages(image_files);
    if (ret != 0) {
        printf("[Main] 处理图片失败\n");
        return -1;
    }
    
    printf("\n========================================\n");
    printf("  文本分析系统运行完成\n");
    printf("========================================\n");
    
    return 0;
}
