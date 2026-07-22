/**
 * @file rotary_encoder.h
 * @brief 旋转编码器驱动头文件
 */

#ifndef __ROTARY_ENCODER_H__
#define __ROTARY_ENCODER_H__

#include <stdbool.h>

/**
 * @brief 初始化旋转编码器
 */
void rotary_encoder_init(void);

/**
 * @brief 设置编码器启用状态
 * @param enabled true=启用, false=禁用
 */
void rotary_encoder_set_enabled(bool enabled);

/**
 * @brief 编码器控制任务
 */
void rotary_encoder_task(void *pvParameters);

/**
 * @brief 获取当前速度等级 (0-10)
 */
int rotary_encoder_get_speed(void);

/**
 * @brief 设置速度等级
 * @param speed 速度等级 (0-10)
 */
void rotary_encoder_set_speed(int speed);

/**
 * @brief 获取速度百分比 (0-100)
 */
int rotary_encoder_get_speed_percent(void);

/**
 * @brief 获取当前控制目标
 * @return false=水泵, true=氧气泵
 */
bool rotary_encoder_get_target(void);

/**
 * @brief 设置当前控制目标
 * @param target false=水泵, true=氧气泵
 */
void rotary_encoder_set_target(bool target);

#endif /* __ROTARY_ENCODER_H__ */
