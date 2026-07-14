/**
 * @file motor_driver.h
 * @brief DRV8833 电机驱动头文件
 */

#ifndef MOTOR_DRIVER_H
#define MOTOR_DRIVER_H

#include "motor_config.h"

/**
 * @brief 初始化电机驱动
 */
void motor_driver_init(void);

/**
 * @brief 设置左电机状态
 * @param direction 电机方向 (MOTOR_STOP, MOTOR_FORWARD, MOTOR_BACKWARD)
 */
void motor_left_set(motor_direction_t direction);

/**
 * @brief 设置右电机状态
 * @param direction 电机方向 (MOTOR_STOP, MOTOR_FORWARD, MOTOR_BACKWARD)
 */
void motor_right_set(motor_direction_t direction);

/**
 * @brief 设置两个电机状态
 * @param left_dir 左电机方向
 * @param right_dir 右电机方向
 */
void motor_set_both(motor_direction_t left_dir, motor_direction_t right_dir);

/**
 * @brief 停止所有电机
 */
void motor_stop_all(void);

/**
 * @brief 小车前进
 */
void car_forward(void);

/**
 * @brief 小车后退
 */
void car_backward(void);

/**
 * @brief 小车左转（原地左转）
 */
void car_turn_left(void);

/**
 * @brief 小车右转（原地右转）
 */
void car_turn_right(void);

/**
 * @brief 小车停止
 */
void car_stop(void);

#endif // MOTOR_DRIVER_H
