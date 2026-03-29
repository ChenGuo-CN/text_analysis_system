#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <string>
#include <iostream>
#include "rkllm.h"

// 全局句柄，用于信号处理
LLMHandle g_llmHandle = nullptr;

// 回调函数，处理模型输出
int callback(RKLLMResult* result, void* userdata, LLMCallState state) {
    if (state == RKLLM_RUN_NORMAL) {
        // 正常生成token，输出文本
        printf("%s", result->text);
        fflush(stdout);
    } else if (state == RKLLM_RUN_FINISH) {
        // 生成完成
        printf("\n\n[系统] 推理完成\n");
    } else if (state == RKLLM_RUN_ERROR) {
        // 发生错误
        printf("\n[错误] 推理过程中发生错误\n");
    }
    return 0;  // 返回0表示继续推理
}

// 信号处理函数，优雅退出
void exit_handler(int sig) {
    printf("\n[系统] 接收到退出信号，正在清理资源...\n");
    if (g_llmHandle != nullptr) {
        rkllm_destroy(g_llmHandle);
        g_llmHandle = nullptr;
    }
    exit(0);
}

int main(int argc, char* argv[]) {
    // 设置信号处理
    signal(SIGINT, exit_handler);

    // 检查命令行参数
    if (argc < 2) {
        printf("用法: %s <模型路径> [max_new_tokens] [max_context_len]\n", argv[0]);
        printf("示例: %s ./DeepSeek-R1-Distill-Qwen-1.5B_W8A8_RK3588.rkllm 2048 4096\n", argv[0]);
        return -1;
    }

    const char* model_path = argv[1];
    int max_new_tokens = (argc > 2) ? atoi(argv[2]) : 2048;
    int max_context_len = (argc > 3) ? atoi(argv[3]) : 4096;

    printf("========================================\n");
    printf("  RKLLM Chat Template 测试程序\n");
    printf("========================================\n");
    printf("模型路径: %s\n", model_path);
    printf("max_new_tokens: %d\n", max_new_tokens);
    printf("max_context_len: %d\n", max_context_len);
    printf("----------------------------------------\n\n");

    // 1. 配置模型参数
    printf("[1/4] 正在配置模型参数...\n");
    RKLLMParam param = rkllm_createDefaultParam();
    param.model_path = model_path;
    param.max_context_len = max_context_len;
    param.max_new_tokens = max_new_tokens;
    param.top_k = 1;
    param.top_p = 0.95;
    param.temperature = 0.8;
    param.repeat_penalty = 1.1;
    param.skip_special_token = true;
    param.extend_param.embed_flash = 1;  // 使用Flash Attention

    // 2. 初始化模型
    printf("[2/4] 正在初始化模型...\n");
    int ret = rkllm_init(&g_llmHandle, &param, callback);
    if (ret != 0) {
        printf("[错误] 模型初始化失败，错误码: %d\n", ret);
        return -1;
    }
    printf("[成功] 模型初始化完成\n\n");

    // 3. 配置自定义chat_template和system prompt
    printf("[3/4] 正在配置chat_template...\n");
    const char* system_prompt = "你是一个东南大学的研究生";
    // Qwen格式的chat_template
    const char* prompt_prefix = "<|im_start|>user\n";
    const char* prompt_postfix = "<|im_end|>\n<|im_start|>assistant\n";

    printf("  System Prompt: %s\n", system_prompt);
    printf("  Prompt Prefix: %s", prompt_prefix);
    printf("  Prompt Postfix: %s", prompt_postfix);

    ret = rkllm_set_chat_template(g_llmHandle, system_prompt, prompt_prefix, prompt_postfix);
    if (ret != 0) {
        printf("[警告] chat_template配置失败，错误码: %d\n", ret);
        printf("       将继续使用默认配置\n");
    } else {
        printf("[成功] chat_template配置完成\n");
    }
    printf("\n");

    // 4. 开始交互式对话
    printf("[4/4] 进入交互模式\n");
    printf("========================================\n");
    printf("  输入提示:\n");
    printf("  - 直接输入问题进行对话\n");
    printf("  - 输入 'exit' 退出程序\n");
    printf("  - 输入 'clear' 清除KV缓存\n");
    printf("========================================\n\n");

    // 准备推理参数
    RKLLMInferParam infer_param;
    memset(&infer_param, 0, sizeof(RKLLMInferParam));
    infer_param.mode = RKLLM_INFER_GENERATE;
    infer_param.keep_history = 1;  // 保留历史记录，支持多轮对话

    char input_buffer[1024];
    while (true) {
        printf("user: ");
        fflush(stdout);

        // 读取用户输入
        if (fgets(input_buffer, sizeof(input_buffer), stdin) == nullptr) {
            break;
        }

        // 去除末尾换行符
        size_t len = strlen(input_buffer);
        if (len > 0 && input_buffer[len - 1] == '\n') {
            input_buffer[len - 1] = '\0';
        }

        // 处理特殊命令
        if (strcmp(input_buffer, "exit") == 0) {
            printf("[系统] 正在退出...\n");
            break;
        } else if (strcmp(input_buffer, "clear") == 0) {
            printf("[系统] 清除KV缓存...\n");
            rkllm_clear_kv_cache(g_llmHandle, true, nullptr, nullptr);
            continue;
        } else if (strlen(input_buffer) == 0) {
            continue;
        }

        // 构建输入
        RKLLMInput input;
        memset(&input, 0, sizeof(RKLLMInput));
        input.input_type = RKLLM_INPUT_PROMPT;
        input.role = "user";
        input.prompt_input = input_buffer;
        input.enable_thinking = false;

        // 执行推理
        printf("\nassistant: ");
        fflush(stdout);

        ret = rkllm_run(g_llmHandle, &input, &infer_param, nullptr);
        if (ret != 0) {
            printf("\n[错误] 推理执行失败，错误码: %d\n", ret);
        }
        printf("\n\n");
    }

    // 清理资源
    printf("[系统] 正在释放资源...\n");
    if (g_llmHandle != nullptr) {
        rkllm_destroy(g_llmHandle);
        g_llmHandle = nullptr;
    }
    printf("[系统] 程序已退出\n");

    return 0;
}
