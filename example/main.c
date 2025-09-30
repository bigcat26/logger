/*
 * Logger库使用示例
 * 展示如何使用重新设计的API接口
 */

#include <stdio.h>
#include "logger/logger.h"

int main(void) {
    printf("=== Logger库功能演示 ===\n");
    
    /* 1. 基本使用 - 开箱即用 */
    printf("\n1. 基本使用（开箱即用）:\n");
    logger_quick_startup();
    
    LOGI("应用程序启动");
    LOGD("调试信息: %d", 42);
    LOGW("警告: %s", "可能有问题");
    LOGE("错误: %s", "文件未找到");
    
    logger_quick_cleanup();
    
    /* 2. 高级配置 */
    printf("\n2. 高级配置:\n");
    logger_t logger;
    logger_config_t config = {0};
    char buffer[4096];
    
    /* 配置logger */
    config.buffer = buffer;
    config.buffer_size = sizeof(buffer);
    config.min_level = LOG_DEBUG;
    config.enable_thread_safety = 1;
    
    /* 初始化logger */
    logger_init(&logger, &config);
    
    /* 创建控制台appender */
    logger_appender_console_config_t console_cfg = {0};
    console_cfg.enable_colors = 1;
    console_cfg.enable_timestamp = 1;
    
    logger_appender_t *console_appender = logger_appender_create_console(&console_cfg);
    
    /* 创建layout */
    logger_layout_t *full_layout = logger_layout_create_full();
    
    /* 设置layout */
    logger_appender_set_layout(console_appender, full_layout);
    
    /* 添加appender到logger */
    logger_add_appender(&logger, console_appender);
    
    /* 记录日志 */
    logger_printf(&logger, LOG_INFO, __FILE__, __LINE__, __FUNCTION__, 
                  "高级配置测试: %s", "成功!");
    
    /* 清理资源 */
    logger_remove_appender(&logger, console_appender);
    logger_layout_destroy(full_layout);
    logger_appender_destroy(console_appender);
    logger_deinit(&logger);
    
    printf("\n=== 演示完成 ===\n");
    printf("Logger库功能正常！\n");
    return 0;
}