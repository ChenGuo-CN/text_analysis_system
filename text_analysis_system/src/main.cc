#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "ppocr_system.h"
#include "common.h"
#include "file_utils.h"
#include "image_utils.h"

#define DET_MODEL_PATH "model/ppocrv4_det_i8.rknn"
#define REC_MODEL_PATH "model/ppocrv4_rec_fp16.rknn"
#define EXPECTED_TEXTS_FILE "/home/linaro/traffic_text_analysis_system/datasets/test.txt"

// DBNet后处理参数
#define THRESHOLD 0.3
#define BOX_THRESHOLD 0.6
#define USE_DILATE false
#define DB_SCORE_MODE (char*)"slow"
#define DB_BOX_TYPE (char*)"poly"
#define DB_UNCLIP_RATIO 1.5

// 最大预期文本数量
#define MAX_EXPECTED_TEXTS 32
#define MAX_TEXT_LEN 256

typedef struct {
    char texts[MAX_EXPECTED_TEXTS][MAX_TEXT_LEN];
    int count;
} expected_texts_t;

// 从文本文件加载预期文本（每行一个）
int load_expected_texts(const char* filepath, expected_texts_t* out) {
    FILE* fp = fopen(filepath, "r");
    if (fp == NULL) {
        printf("Note: Could not open %s for verification\n", filepath);
        return -1;
    }

    out->count = 0;
    char line[MAX_TEXT_LEN];
    while (fgets(line, sizeof(line), fp) != NULL && out->count < MAX_EXPECTED_TEXTS) {
        // 移除换行符和回车符
        size_t len = strlen(line);
        while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r')) {
            line[--len] = '\0';
        }
        
        // 跳过空行
        if (len == 0) continue;
        
        // 移除UTF-8 BOM (EF BB BF)
        char* start = line;
        if ((unsigned char)line[0] == 0xEF && 
            (unsigned char)line[1] == 0xBB && 
            (unsigned char)line[2] == 0xBF) {
            start += 3;
        }
        
        strncpy(out->texts[out->count], start, MAX_TEXT_LEN - 1);
        out->texts[out->count][MAX_TEXT_LEN - 1] = '\0';
        out->count++;
    }
    
    fclose(fp);
    return 0;
}

void print_usage(char* argv[]) {
    printf("Usage: %s <image_path>\n", argv[0]);
    printf("   or: %s <det_model_path> <rec_model_path> <image_path>\n", argv[0]);
    printf("\nExample:\n");
    printf("  %s /home/linaro/traffic_text_analysis_system/datasets/test.jpg\n", argv[0]);
    printf("  %s model/ppocrv4_det_i8.rknn model/ppocrv4_rec_fp16.rknn test.jpg\n", argv[0]);
}

int main(int argc, char** argv) {
    int ret;
    char* det_model_path = NULL;
    char* rec_model_path = NULL;
    char* image_path = NULL;

    // 解析命令行参数
    if (argc == 2) {
        // 使用默认模型路径
        det_model_path = DET_MODEL_PATH;
        rec_model_path = REC_MODEL_PATH;
        image_path = argv[1];
    } else if (argc == 4) {
        // 自定义模型路径
        det_model_path = argv[1];
        rec_model_path = argv[2];
        image_path = argv[3];
    } else {
        print_usage(argv);
        return -1;
    }

    printf("========================================\n");
    printf("  PPOCR NPU Core 2 Binding Test\n");
    printf("========================================\n\n");

    printf("[Config]\n");
    printf("  Detection Model: %s\n", det_model_path);
    printf("  Recognition Model: %s\n", rec_model_path);
    printf("  Image Path: %s\n", image_path);
    printf("  NPU Core: 2 (RKNN_NPU_CORE_2)\n\n");

    // 加载图片
    image_buffer_t src_img;
    memset(&src_img, 0, sizeof(image_buffer_t));
    ret = read_image(image_path, &src_img);
    if (ret != 0) {
        printf("Error: read_image fail! ret=%d\n", ret);
        return -1;
    }
    printf("[Image Info]\n");
    printf("  Width: %d, Height: %d\n\n", src_img.width, src_img.height);

    // 加载预期文本
    expected_texts_t expected;
    memset(&expected, 0, sizeof(expected_texts_t));
    load_expected_texts(EXPECTED_TEXTS_FILE, &expected);

    // 初始化系统上下文
    ppocr_system_app_context sys_ctx;
    memset(&sys_ctx, 0, sizeof(ppocr_system_app_context));

    // 初始化检测模型，绑定到NPU Core 2
    printf("[1/2] Initializing Detection Model...\n");
    ret = init_ppocr_model(det_model_path, &sys_ctx.det_context, RKNN_NPU_CORE_2);
    if (ret != 0) {
        printf("Error: init_ppocr_model (det) fail! ret=%d\n", ret);
        return -1;
    }
    printf("  Detection Model initialized successfully on NPU Core 2\n\n");

    // 初始化识别模型，绑定到NPU Core 2
    printf("[2/2] Initializing Recognition Model...\n");
    ret = init_ppocr_model(rec_model_path, &sys_ctx.rec_context, RKNN_NPU_CORE_2);
    if (ret != 0) {
        printf("Error: init_ppocr_model (rec) fail! ret=%d\n", ret);
        release_ppocr_model(&sys_ctx.det_context);
        return -1;
    }
    printf("  Recognition Model initialized successfully on NPU Core 2\n\n");

    // 设置检测后处理参数
    ppocr_det_postprocess_params params;
    params.threshold = THRESHOLD;
    params.box_threshold = BOX_THRESHOLD;
    params.use_dilate = USE_DILATE;
    params.db_score_mode = DB_SCORE_MODE;
    params.db_box_type = DB_BOX_TYPE;
    params.db_unclip_ratio = DB_UNCLIP_RATIO;

    // 执行OCR推理
    printf("========================================\n");
    printf("  Starting OCR Inference...\n");
    printf("========================================\n\n");

    struct timeval start_time, end_time;
    gettimeofday(&start_time, NULL);

    ppocr_text_recog_array_result_t result;
    memset(&result, 0, sizeof(ppocr_text_recog_array_result_t));
    ret = inference_ppocr_system_model(&sys_ctx, &src_img, &params, &result);
    
    gettimeofday(&end_time, NULL);
    float total_time = (end_time.tv_sec - start_time.tv_sec) * 1000.0f + 
                       (end_time.tv_usec - start_time.tv_usec) / 1000.0f;

    if (ret != 0) {
        printf("Error: inference_ppocr_system_model fail! ret=%d\n", ret);
        release_ppocr_model(&sys_ctx.det_context);
        release_ppocr_model(&sys_ctx.rec_context);
        return -1;
    }

    // 输出结果
    printf("\n========================================\n");
    printf("  OCR Results (Total: %d texts)\n", result.count);
    printf("  Inference Time: %.2f ms\n", total_time);
    printf("========================================\n\n");

    for (int i = 0; i < result.count; i++) {
        printf("[%d] Text: %s\n", i, result.text_result[i].text.str);
        printf("    Confidence: %.3f\n", result.text_result[i].text.score);
        printf("    Box: [(%d,%d), (%d,%d), (%d,%d), (%d,%d)]\n",
               result.text_result[i].box.left_top.x,
               result.text_result[i].box.left_top.y,
               result.text_result[i].box.right_top.x,
               result.text_result[i].box.right_top.y,
               result.text_result[i].box.right_bottom.x,
               result.text_result[i].box.right_bottom.y,
               result.text_result[i].box.left_bottom.x,
               result.text_result[i].box.left_bottom.y);
        printf("\n");
    }

    // 检查是否识别到预期文本
    printf("========================================\n");
    printf("  Verification\n");
    printf("========================================\n");
    
    if (expected.count > 0) {
        printf("Expected Texts (%d items):\n", expected.count);
        int total_found = 0;
        
        for (int e = 0; e < expected.count; e++) {
            printf("  [%d] \"%s\" - ", e, expected.texts[e]);
            
            int found = 0;
            for (int i = 0; i < result.count; i++) {
                if (strstr(result.text_result[i].text.str, expected.texts[e]) != NULL) {
                    found = 1;
                    total_found++;
                    break;
                }
            }
            
            if (found) {
                printf("FOUND\n");
            } else {
                printf("NOT FOUND\n");
            }
        }
        
        printf("\nResult: %d/%d expected texts found (%.1f%%)\n", 
               total_found, expected.count, 
               100.0f * total_found / expected.count);
        
        if (total_found == expected.count) {
            printf("Status: SUCCESS - All expected texts recognized!\n");
        } else if (total_found >= expected.count / 2) {
            printf("Status: PARTIAL - Most expected texts recognized.\n");
        } else {
            printf("Status: WARNING - Many expected texts not found.\n");
        }
    } else {
        printf("Note: No expected texts loaded for verification\n");
    }

    // 释放资源
    release_ppocr_model(&sys_ctx.det_context);
    release_ppocr_model(&sys_ctx.rec_context);

    if (src_img.virt_addr != NULL) {
        free(src_img.virt_addr);
    }

    printf("\n========================================\n");
    printf("  Test completed successfully!\n");
    printf("========================================\n");

    return 0;
}
