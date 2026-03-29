/**
 * @file ocr_engine.h
 * @brief OCR引擎模块头文件
 * 
 * 封装PPOCR检测和识别功能
 * 支持NPU Core 2绑定
 */

#ifndef _TEXT_ANALYSIS_OCR_ENGINE_H_
#define _TEXT_ANALYSIS_OCR_ENGINE_H_

#include "ppocr_system.h"
#include "config.h"
#include "perf_monitor.h"
#include "text_queue.h"
#include <string>

/**
 * @brief OCR引擎类
 * 
 * 封装PPOCR模型的初始化和推理功能
 * 检测和识别模型都绑定到NPU Core 2
 */
class OCREngine {
public:
    /**
     * @brief 构造函数
     */
    OCREngine();

    /**
     * @brief 析构函数，自动释放资源
     */
    ~OCREngine();

    /**
     * @brief 禁止拷贝构造和赋值
     */
    OCREngine(const OCREngine&) = delete;
    OCREngine& operator=(const OCREngine&) = delete;

    /**
     * @brief 初始化OCR引擎
     * @param config 配置对象
     * @return 成功返回0，失败返回-1
     */
    int initialize(const Config& config);

    /**
     * @brief 释放OCR引擎资源
     * @return 成功返回0，失败返回-1
     */
    int release();

    /**
     * @brief 对单张图片进行OCR识别
     * @param image_path 图片路径
     * @param result 输出识别结果
     * @param perf_monitor 性能监控器（可选）
     * @return 成功返回0，失败返回-1
     */
    int recognize(const std::string& image_path, 
                  OCRResult& result, 
                  PerfMonitor* perf_monitor = nullptr);

    /**
     * @brief 检查引擎是否已初始化
     * @return 已初始化返回true，否则返回false
     */
    bool isInitialized() const;

    /**
     * @brief 获取检测模型输入尺寸
     * @param width 输出宽度
     * @param height 输出高度
     */
    void getDetInputSize(int& width, int& height) const;

    /**
     * @brief 获取识别模型输入尺寸
     * @param width 输出宽度
     * @param height 输出高度
     */
    void getRecInputSize(int& width, int& height) const;

private:
    ppocr_system_app_context app_ctx_;      // PPOCR应用上下文
    ppocr_det_postprocess_params params_;   // 检测后处理参数
    bool initialized_;                      // 初始化标志

    /**
     * @brief 将PPOCR结果转换为OCRResult
     */
    void convertResult(const ppocr_text_recog_array_result_t& ppocr_result,
                       const std::string& image_path,
                       OCRResult& result);
};

#endif // _TEXT_ANALYSIS_OCR_ENGINE_H_
