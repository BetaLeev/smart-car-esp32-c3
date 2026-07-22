/**
 * @file app_config.h
 * @brief 应用主配置文件
 *
 * ESP32-C3 智能水泵/氧气泵控制系统配置
 */

#ifndef APP_CONFIG_H
#define APP_CONFIG_H

// ============================================
// 系统配置
// ============================================

// 设备名称
#define DEVICE_NAME           "ESP32-C3-PUMP-CTRL"

// 日志级别: 0=无, 1=错误, 2=警告, 3=信息, 4=调试
#define LOG_LEVEL             3

// ============================================
// 功能开关配置
// ============================================

// LED 指示灯
#define CONFIG_LED_ENABLED            1
#define CONFIG_LED_BLINK_INTERVAL_MS  1000

// 泵驱动
#define CONFIG_PUMP_ENABLED           1

// 物理按键控制
#define CONFIG_BUTTON_ENABLED         1

// Wi-Fi (已禁用)
#define CONFIG_WIFI_ENABLED           0

// Web 服务器 (已禁用)
#define CONFIG_WEB_SERVER_ENABLED     0

// BLE 蓝牙 (已禁用)
#define CONFIG_BLE_ENABLED            0

// ============================================
// 任务配置
// ============================================

// LED 任务
#define LED_TASK_PRIORITY     1
#define LED_TASK_STACK_SIZE   2048

// 按键任务
#define BUTTON_TASK_PRIORITY  3
#define BUTTON_TASK_STACK_SIZE 3072

// 状态任务
#define STATUS_TASK_PRIORITY  2
#define STATUS_TASK_STACK_SIZE 4096

#endif // APP_CONFIG_H
