/**
 * @file motor_config.h
 * @brief 电机驱动配置文件
 *
 * 支持 DRV8833 和 TB6612FNG
 * 在此文件中配置电机驱动的引脚和参数
 */

#ifndef MOTOR_CONFIG_H
#define MOTOR_CONFIG_H

#include "driver/gpio.h"

// ============================================
// 选择电机驱动模块
// ============================================
#define USE_DRV8833
// #define USE_TB6612FNG

// ============================================
// 电机引脚配置 - 根据实际硬件调整这里
// ============================================

#ifdef USE_DRV8833
    // DRV8833 引脚配置
    // 电机 A: GPIO 9, 10
    // 电机 B: GPIO 2, 3
    #define MOTOR_A_IN1_PIN    GPIO_NUM_9
    #define MOTOR_A_IN2_PIN    GPIO_NUM_10
    #define MOTOR_B_IN1_PIN    GPIO_NUM_2
    #define MOTOR_B_IN2_PIN    GPIO_NUM_3
#else
    // TB6612FNG 引脚配置
    #define MOTOR_STBY_PIN     GPIO_NUM_20
    #define LEFT_MOTOR_IN1_PIN GPIO_NUM_9
    #define LEFT_MOTOR_IN2_PIN GPIO_NUM_10
    #define RIGHT_MOTOR_IN1_PIN GPIO_NUM_2
    #define RIGHT_MOTOR_IN2_PIN GPIO_NUM_3
#endif

// LED 引脚配置
#define LED_PIN             GPIO_NUM_8

// 测试引脚配置（预留扩展）
#define TEST_PIN_0          GPIO_NUM_0
#define TEST_PIN_1          GPIO_NUM_1
#define TEST_PIN_2          GPIO_NUM_2
#define TEST_PIN_3          GPIO_NUM_3
#define TEST_PIN_4          GPIO_NUM_4

// ============================================
// 电机控制参数
// ============================================

// 电机速度等级 (0-100)
#define MOTOR_SPEED_STOP    0
#define MOTOR_SPEED_SLOW    30
#define MOTOR_SPEED_MEDIUM  60
#define MOTOR_SPEED_FAST    100

// 任务优先级
#define MOTOR_TASK_PRIORITY     2
#define LED_TASK_PRIORITY       1
#define TEST_TASK_PRIORITY      1

// 任务堆栈大小
#define MOTOR_TASK_STACK_SIZE   2048
#define LED_TASK_STACK_SIZE     2048
#define TEST_TASK_STACK_SIZE    2048

// ============================================
// 电机状态枚举
// ============================================

typedef enum {
    MOTOR_STOP = 0,    // 停止
    MOTOR_FORWARD,     // 正转
    MOTOR_BACKWARD,    // 反转
    MOTOR_BRAKE        // 制动
} motor_direction_t;

// ============================================
// 引脚功能映射 - 用于调试和显示
// ============================================

static inline const char* pin_to_name(gpio_num_t pin) {
    switch (pin) {
        case GPIO_NUM_0: return "GPIO_0";
        case GPIO_NUM_1: return "GPIO_1";
        case GPIO_NUM_2: return "GPIO_2";
        case GPIO_NUM_3: return "GPIO_3";
        case GPIO_NUM_4: return "GPIO_4";
        case GPIO_NUM_5: return "GPIO_5";
        case GPIO_NUM_6: return "GPIO_6";
        case GPIO_NUM_7: return "GPIO_7";
        case GPIO_NUM_8: return "GPIO_8";
        case GPIO_NUM_9: return "GPIO_9";
        case GPIO_NUM_10: return "GPIO_10";
        case GPIO_NUM_20: return "GPIO_20";
        case GPIO_NUM_21: return "GPIO_21";
        default: return "UNKNOWN";
    }
}

// 宏版本（兼容旧代码）
#define PIN_NAME(x) pin_to_name(x)

#endif // MOTOR_CONFIG_H
