/**
 * @file pin_config.h
 * @brief ESP32-C3 引脚配置
 *
 * 双泵控制系统 - 按键控制
 *
 * 引脚布局 (左右对称):
 *   8  4   GPIO 8: STBY,     GPIO 4: 预留
 *   9  3   GPIO 9: LED,      GPIO 3: 预留
 *  10  2   GPIO 10: 预留,    GPIO 2: 预留
 *  20  1   GPIO 20: 水泵按键, GPIO 1: 氧气泵按键
 *  21  0   GPIO 21: 水泵控制, GPIO 0: 氧气泵控制
 */

#ifndef __PIN_CONFIG_H__
#define __PIN_CONFIG_H__

#include "driver/gpio.h"

// ============================================================
// 按键 (用于控制泵)
// ============================================================
#define WATER_PUMP_BTN_PIN    GPIO_NUM_20   // 水泵按键
#define OXYGEN_PUMP_BTN_PIN   GPIO_NUM_1    // 氧气泵按键

// ============================================================
// DRV8833 泵控制
// ============================================================
#define PUMP_STBY_PIN         GPIO_NUM_8    // 使能脚

#define WATER_PUMP_CTRL_PIN   GPIO_NUM_21   // 水泵控制 (AIN1)
#define WATER_PUMP_IN2_PIN    GPIO_NUM_10   // 水泵方向 (接地)

#define OXYGEN_PUMP_CTRL_PIN  GPIO_NUM_0    // 氧气泵控制 (BIN1)
#define OXYGEN_PUMP_IN2_PIN   GPIO_NUM_11   // 氧气泵方向 (接地)

// ============================================================
// LED 状态指示
// ============================================================
#define LED_PIN               GPIO_NUM_9    // 状态 LED

#endif /* __PIN_CONFIG_H__ */
