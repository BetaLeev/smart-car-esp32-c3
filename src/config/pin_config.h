/**
 * @file pin_config.h
 * @brief 引脚配置文件
 *
 * 在此文件中配置所有 GPIO 引脚
 * 注意: ESP32-C3-MINI-1 的 GPIO 6-11 连接到内部 SPI Flash，禁止使用！
 */

#ifndef PIN_CONFIG_H
#define PIN_CONFIG_H

#include "driver/gpio.h"

// ============================================
// 电机驱动类型配置
// ============================================
/**
 * 电机驱动类型:
 *   - MOTOR_DRIVER_DRV8833   : 双 H 桥电机驱动模块
 *   - MOTOR_DRIVER_TB6612FNG : 双 H 桥电机驱动模块 (带 STBY 引脚)
 */
// #define MOTOR_DRIVER_DRV8833
#define MOTOR_DRIVER_TB6612FNG

// ============================================
// DRV8833 引脚配置
// ============================================
#ifdef MOTOR_DRIVER_DRV8833
    #define MOTOR_A_IN1_PIN      GPIO_NUM_4
    #define MOTOR_A_IN2_PIN      GPIO_NUM_5
    #define MOTOR_B_IN1_PIN      GPIO_NUM_2
    #define MOTOR_B_IN2_PIN      GPIO_NUM_3
#endif

// ============================================
// TB6612FNG 引脚配置
// ============================================
#ifdef MOTOR_DRIVER_TB6612FNG
    #define MOTOR_STBY_PIN       GPIO_NUM_20
    #define LEFT_MOTOR_IN1_PIN   GPIO_NUM_8
    #define LEFT_MOTOR_IN2_PIN   GPIO_NUM_9
    #define RIGHT_MOTOR_IN1_PIN  GPIO_NUM_2
    #define RIGHT_MOTOR_IN2_PIN  GPIO_NUM_3
#endif

// ============================================
// LED 引脚配置
// ============================================
// 注意: GPIO 8 连接到 SPI Flash，改用 GPIO 1
#define LED_PIN              GPIO_NUM_1

// ============================================
// 喇叭/有源蜂鸣器引脚配置
// ============================================
#define BUZZER_PIN           GPIO_NUM_21

// ============================================
// 预留测试引脚
// ============================================
#define TEST_PIN_0           GPIO_NUM_0
#define TEST_PIN_1           GPIO_NUM_7
#define TEST_PIN_2           GPIO_NUM_10
#define TEST_PIN_3           GPIO_NUM_11
#define TEST_PIN_4           GPIO_NUM_21

// ============================================
// ADC 引脚 (预留)
// ============================================
#define BATTERY_ADC_PIN      GPIO_NUM_0

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
        case GPIO_NUM_11: return "GPIO_11";
        case GPIO_NUM_20: return "GPIO_20";
        case GPIO_NUM_21: return "GPIO_21";
        default: return "UNKNOWN";
    }
}

// 宏版本（兼容旧代码）
#define PIN_NAME(x) pin_to_name(x)

#endif // PIN_CONFIG_H
