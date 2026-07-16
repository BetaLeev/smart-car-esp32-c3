/**
 * @file app_config.h
 * @brief 应用主配置文件
 *
 * 所有配置项统一管理在此文件中
 */

#ifndef APP_CONFIG_H
#define APP_CONFIG_H

// ============================================
// 系统配置
// ============================================

// 设备名称
#define DEVICE_NAME           "ESP32-CAR"

// 日志级别: 0=无, 1=错误, 2=警告, 3=信息, 4=调试
#define LOG_LEVEL             3

// ============================================
// 功能开关配置
// ============================================

// Web 服务器
#define CONFIG_WEB_SERVER_ENABLED     1
#define CONFIG_WEB_SERVER_PORT        80

// LED 指示灯
#define CONFIG_LED_ENABLED            1
#define CONFIG_LED_BLINK_INTERVAL_MS  1000

// 电机驱动
#define CONFIG_MOTOR_ENABLED          1

// 喇叭/有源蜂鸣器
#define CONFIG_BUZZER_ENABLED         1

// ============================================
// 任务配置
// ============================================

// LED 任务
#define LED_TASK_PRIORITY     1
#define LED_TASK_STACK_SIZE  3072  // 增加栈空间避免溢出

// Web 服务器任务
#define WEB_TASK_PRIORITY    3
#define WEB_TASK_STACK_SIZE  8192

#endif // APP_CONFIG_H
