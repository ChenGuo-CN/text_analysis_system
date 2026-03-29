/**
 * @file config.h
 * @brief 配置管理模块头文件
 * 
 * 提供JSON配置文件的解析和管理功能
 * 支持模型路径、LLM参数、OCR参数、队列大小、输出配置、性能配置等
 */

#ifndef _TEXT_ANALYSIS_CONFIG_H_
#define _TEXT_ANALYSIS_CONFIG_H_

#include <string>
#include <vector>

/**
 * @brief 模型配置结构体
 */
typedef struct {
    std::string det_model_path;     // 检测模型路径
    std::string rec_model_path;     // 识别模型路径
    std::string dict_path;          // 字典文件路径
    std::string llm_model_path;     // LLM模型路径
} ModelConfig;

/**
 * @brief LLM配置结构体
 */
typedef struct {
    std::string system_prompt;      // 系统提示词
    bool enable_thinking;           // 是否启用think模式
    int max_new_tokens;             // 最大生成token数
    int max_context_len;            // 最大上下文长度
    float temperature;              // 温度参数
    float top_p;                    // top_p采样参数
    int top_k;                      // top_k采样参数
    float repeat_penalty;           // 重复惩罚
} LLMConfig;

/**
 * @brief OCR配置结构体
 */
typedef struct {
    float threshold;                // 像素分数阈值
    float box_threshold;            // 文本框分数阈值
    float db_unclip_ratio;          // DBNet解压缩比例
    bool use_dilate;                // 是否进行膨胀操作
    std::string db_score_mode;      // slow或fast
    std::string db_box_type;        // poly或quad
} OCRConfig;

/**
 * @brief 队列配置结构体
 */
typedef struct {
    int max_size;                   // 队列最大容量
} QueueConfig;

/**
 * @brief 输出配置结构体
 */
typedef struct {
    std::string result_dir;         // 结果输出目录
    bool save_annotated_image;      // 是否保存带标注的图像
} OutputConfig;

/**
 * @brief 性能配置结构体
 */
typedef struct {
    bool enable_timing;             // 是否启用耗时统计
    int log_level;                  // 日志级别
} PerformanceConfig;

/**
 * @brief 全局配置结构体
 */
class Config {
public:
    ModelConfig model;
    LLMConfig llm;
    OCRConfig ocr;
    QueueConfig queue;
    OutputConfig output;
    PerformanceConfig performance;

    /**
     * @brief 从JSON文件加载配置
     * @param filepath 配置文件路径
     * @param base_dir 基础目录（用于解析相对路径）
     * @return 成功返回0，失败返回-1
     */
    int loadFromFile(const std::string& filepath, const std::string& base_dir = "");

    /**
     * @brief 加载默认配置
     */
    void loadDefaults();

    /**
     * @brief 验证配置有效性
     * @return 有效返回true，无效返回false
     */
    bool validate() const;

    /**
     * @brief 打印当前配置
     */
    void print() const;

private:
    std::string base_dir_;  // 基础目录（用于解析相对路径）

    /**
     * @brief 解析模型配置
     */
    int parseModelConfig(const void* json_obj);

    /**
     * @brief 解析LLM配置
     */
    int parseLLMConfig(const void* json_obj);

    /**
     * @brief 解析OCR配置
     */
    int parseOCRConfig(const void* json_obj);

    /**
     * @brief 解析队列配置
     */
    int parseQueueConfig(const void* json_obj);

    /**
     * @brief 解析输出配置
     */
    int parseOutputConfig(const void* json_obj);

    /**
     * @brief 解析性能配置
     */
    int parsePerformanceConfig(const void* json_obj);

    /**
     * @brief 将相对路径转换为绝对路径
     */
    std::string resolvePath(const std::string& path) const;
};

/**
 * @brief 获取全局配置实例
 * @return 配置对象引用
 */
Config& getConfig();

#endif // _TEXT_ANALYSIS_CONFIG_H_
