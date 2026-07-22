/**
 * @file motor_config.h
 * @brief 泵驱动配置文件
 *
 * 水泵和氧气泵速度控制配置
 */

#ifndef MOTOR_CONFIG_H
#define MOTOR_CONFIG_H

#include "config/pin_config.h"

// ============================================
// 泵速度等级定义 (占空比百分比)
// ============================================

// 水泵速度等级 (12V 水泵)
#define WATER_PUMP_SPEED_STOP     0    // 停止
#define WATER_PUMP_SPEED_SLOW     40   // 慢速 - 40% 占空比
#define WATER_PUMP_SPEED_MEDIUM   70   // 中速 - 70% 占空比 (默认)
#define WATER_PUMP_SPEED_FAST     100  // 快速 - 100% 占空比

// 氧气泵速度等级 (5V 氧气泵)
#define OXYGEN_PUMP_SPEED_STOP    0    // 停止
#define OXYGEN_PUMP_SPEED_SLOW    35   // 慢速 - 35% 占空比
#define OXYGEN_PUMP_SPEED_MEDIUM  65   // 中速 - 65% 占空比 (默认)
#define OXYGEN_PUMP_SPEED_FAST    100  // 快速 - 100% 占空比

// PWM 配置
#define PUMP_PWM_FREQ_HZ          1000  // PWM 频率 1kHz
#define PUMP_PWM_RESOLUTION       8     // 8位分辨率 (0-255)

// ============================================
// 泵速度枚举
// ============================================

typedef enum {
    PUMP_SPEED_STOP = 0,    // 停止
    PUMP_SPEED_SLOW,        // 慢速
    PUMP_SPEED_MEDIUM,      // 中速
    PUMP_SPEED_FAST         // 快速
} pump_speed_t;

// ============================================
// 泵类型枚举
// ============================================

typedef enum {
    PUMP_TYPE_WATER = 0,    // 12V 水泵
    PUMP_TYPE_OXYGEN        // 5V 氧气泵
} pump_type_t;

// ============================================
// 任务配置
// ============================================

#define PUMP_TASK_PRIORITY        2
#define BUTTON_TASK_PRIORITY      3    // 按键任务优先级较高，确保及时响应
#define TEST_TASK_PRIORITY        1

// 任务堆栈大小
#define PUMP_TASK_STACK_SIZE      2048
#define BUTTON_TASK_STACK_SIZE    3072  // 按键任务需要处理中断
#define TEST_TASK_STACK_SIZE      2048

// ============================================
// 按键配置
// ============================================

// 按键消抖时间 (ms)
#define BUTTON_DEBOUNCE_MS        50

// 按键状态
typedef enum {
    BUTTON_RELEASED = 0,
    BUTTON_PRESSED
} button_state_t;

#endif // MOTOR_CONFIG_H
