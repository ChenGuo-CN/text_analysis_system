/**
 * @file ocr_engine.cpp
 * @brief OCR引擎模块实现
 */

#include "ocr_engine.h"
#include "image_utils.h"
#include <cstring>
#include <chrono>

OCREngine::OCREngine() : initialized_(false) {
    memset(&app_ctx_, 0, sizeof(app_ctx_));
}

OCREngine::~OCREngine() {
    if (initialized_) {
        release();
    }
}

int OCREngine::initialize(const Config& config) {
    if (initialized_) {
        printf("[OCREngine] 引擎已初始化\n");
        return 0;
    }

    printf("[OCREngine] 正在初始化OCR引擎...\n");

    // 设置检测后处理参数
    params_.threshold = config.ocr.threshold;
    params_.box_threshold = config.ocr.box_threshold;
    params_.use_dilate = config.ocr.use_dilate;
    params_.db_score_mode = const_cast<char*>(config.ocr.db_score_mode.c_str());
    params_.db_box_type = const_cast<char*>(config.ocr.db_box_type.c_str());
    params_.db_unclip_ratio = config.ocr.db_unclip_ratio;

    // 初始化检测模型，绑定到NPU Core 2
    printf("[OCREngine] 初始化检测模型: %s\n", config.model.det_model_path.c_str());
    int ret = init_ppocr_model(config.model.det_model_path.c_str(), 
                                &app_ctx_.det_context, 
                                RKNN_NPU_CORE_2);
    if (ret != 0) {
        printf("[OCREngine] 检测模型初始化失败! ret=%d\n", ret);
        return -1;
    }
    printf("[OCREngine] 检测模型初始化成功 (NPU Core 2)\n");

    // 初始化识别模型，绑定到NPU Core 2
    printf("[OCREngine] 初始化识别模型: %s\n", config.model.rec_model_path.c_str());
    ret = init_ppocr_model(config.model.rec_model_path.c_str(), 
                            &app_ctx_.rec_context, 
                            RKNN_NPU_CORE_2);
    if (ret != 0) {
        printf("[OCREngine] 识别模型初始化失败! ret=%d\n", ret);
        release_ppocr_model(&app_ctx_.det_context);
        return -1;
    }
    printf("[OCREngine] 识别模型初始化成功 (NPU Core 2)\n");

    initialized_ = true;
    printf("[OCREngine] OCR引擎初始化完成\n");
    return 0;
}

int OCREngine::release() {
    if (!initialized_) {
        return 0;
    }

    printf("[OCREngine] 正在释放OCR引擎资源...\n");

    int ret1 = release_ppocr_model(&app_ctx_.det_context);
    int ret2 = release_ppocr_model(&app_ctx_.rec_context);

    memset(&app_ctx_, 0, sizeof(app_ctx_));
    initialized_ = false;

    printf("[OCREngine] OCR引擎资源已释放\n");

    return (ret1 == 0 && ret2 == 0) ? 0 : -1;
}

int OCREngine::recognize(const std::string& image_path,
                         OCRResult& result,
                         PerfMonitor* perf_monitor) {
    if (!initialized_) {
        printf("[OCREngine] 错误: 引擎未初始化\n");
        result.success = false;
        result.error_msg = "引擎未初始化";
        return -1;
    }

    // 记录开始时间
    auto start_time = std::chrono::high_resolution_clock::now();

    // 读取图片
    image_buffer_t src_image;
    memset(&src_image, 0, sizeof(image_buffer_t));
    int ret = read_image(image_path.c_str(), &src_image);
    if (ret != 0) {
        printf("[OCREngine] 读取图片失败: %s\n", image_path.c_str());
        result.success = false;
        result.error_msg = "读取图片失败";
        return -1;
    }

    // 执行OCR推理
    ppocr_text_recog_array_result_t ppocr_result;
    memset(&ppocr_result, 0, sizeof(ppocr_result));

    ret = inference_ppocr_system_model(&app_ctx_, &src_image, &params_, &ppocr_result);

    // 计算总耗时
    auto end_time = std::chrono::high_resolution_clock::now();
    float total_time_ms = std::chrono::duration<float, std::milli>(end_time - start_time).count();

    // 释放图片内存
    if (src_image.virt_addr != NULL) {
        free(src_image.virt_addr);
    }

    if (ret != 0) {
        printf("[OCREngine] OCR推理失败! ret=%d\n", ret);
        result.success = false;
        result.error_msg = "OCR推理失败";
        return -1;
    }

    // 转换结果
    convertResult(ppocr_result, image_path, result);
    result.success = true;
    // 暂时使用总耗时作为检测和识别的估计值
    // 实际应用中可以通过修改ppocr_system_npu2.cc来分别计时
    result.perf_stats.det_time_ms = total_time_ms * 0.3f;  // 估计检测占30%
    result.perf_stats.rec_time_ms = total_time_ms * 0.6f;  // 估计识别占60%
    result.perf_stats.total_time_ms = total_time_ms;

    printf("[OCREngine] OCR识别成功: %s, 识别到 %zu 个文本, 总耗时: %.2f ms\n",
           image_path.c_str(), result.text_items.size(), total_time_ms);

    // 记录性能统计
    if (perf_monitor != nullptr) {
        perf_monitor->recordOCRTotal(total_time_ms);
    }

    return 0;
}

bool OCREngine::isInitialized() const {
    return initialized_;
}

void OCREngine::getDetInputSize(int& width, int& height) const {
    if (initialized_) {
        width = app_ctx_.det_context.model_width;
        height = app_ctx_.det_context.model_height;
    } else {
        width = 480;
        height = 480;
    }
}

void OCREngine::getRecInputSize(int& width, int& height) const {
    if (initialized_) {
        width = app_ctx_.rec_context.model_width;
        height = app_ctx_.rec_context.model_height;
    } else {
        width = 320;
        height = 48;
    }
}

void OCREngine::convertResult(const ppocr_text_recog_array_result_t& ppocr_result,
                              const std::string& image_path,
                              OCRResult& result) {
    result.image_path = image_path;
    result.text_items.clear();

    for (int i = 0; i < ppocr_result.count; i++) {
        TextItem item;
        item.text = ppocr_result.text_result[i].text.str;
        item.confidence = ppocr_result.text_result[i].text.score;
        
        // 转换文本框坐标
        item.box.left_top.x = ppocr_result.text_result[i].box.left_top.x;
        item.box.left_top.y = ppocr_result.text_result[i].box.left_top.y;
        item.box.right_top.x = ppocr_result.text_result[i].box.right_top.x;
        item.box.right_top.y = ppocr_result.text_result[i].box.right_top.y;
        item.box.right_bottom.x = ppocr_result.text_result[i].box.right_bottom.x;
        item.box.right_bottom.y = ppocr_result.text_result[i].box.right_bottom.y;
        item.box.left_bottom.x = ppocr_result.text_result[i].box.left_bottom.x;
        item.box.left_bottom.y = ppocr_result.text_result[i].box.left_bottom.y;
        item.box.score = ppocr_result.text_result[i].box.score;

        result.text_items.push_back(item);
    }

    result.timestamp = getCurrentTimestamp();
}
