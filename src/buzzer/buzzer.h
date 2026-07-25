/**
 * @file buzzer.h
 * @brief 蜂鸣器驱动头文件
 */

#ifndef BUZZER_H
#define BUZZER_H

#include "driver/gpio.h"

/**
 * @brief 初始化蜂鸣器
 */
void buzzer_init(void);

/**
 * @brief 打开蜂鸣器
 */
void buzzer_on(void);

/**
 * @brief 关闭蜂鸣器
 */
void buzzer_off(void);

/**
 * @brief 蜂鸣一声
 * @param duration_ms 持续时间（毫秒）
 */
void buzzer_beep(int duration_ms);

/**
 * @brief 蜂鸣多次
 * @param times 次数
 * @param duration_ms 每次持续时间（毫秒）
 * @param interval_ms 间隔时间（毫秒）
 */
void buzzer_beep_multiple(int times, int duration_ms, int interval_ms);

#endif // BUZZER_H
