/**
 * @file fan_driver.h
 * @brief 小风扇驱动头文件
 */

#ifndef FAN_DRIVER_H
#define FAN_DRIVER_H

#include <stdint.h>
#include "driver/gpio.h"

// ============================================
// 引脚配置（已在 pin_config.h 中定义）
// ============================================
// FAN_GPIO_PIN 在 config/pin_config.h 中定义

// ============================================
// PWM 配置
// ============================================
#define FAN_PWM_FREQUENCY    25000       // 25kHz（无噪声频率）
#define FAN_PWM_RESOLUTION   8           // 8位分辨率 (0-255)
#define FAN_PWM_CHANNEL      LEDC_CHANNEL_0

// ============================================
// API 函数
// ============================================

/**
 * @brief 初始化风扇驱动
 */
void fan_driver_init(void);

/**
 * @brief 设置风扇速度
 * @param speed 速度值 0-100
 */
void fan_driver_set_speed(uint8_t speed);

/**
 * @brief 设置最大速度（用于 GPIO 驱动能力不足时尝试增强）
 */
void fan_driver_set_max(void);

/**
 * @brief 开启风扇
 * @param speed 速度值 0-100
 */
void fan_driver_on(uint8_t speed);

/**
 * @brief 关闭风扇
 */
void fan_driver_off(void);

/**
 * @brief 获取当前风扇状态
 * @return true=开启, false=关闭
 */
bool fan_driver_is_on(void);

/**
 * @brief 获取当前风扇速度
 * @return 速度值 0-100
 */
uint8_t fan_driver_get_speed(void);

#endif // FAN_DRIVER_H
