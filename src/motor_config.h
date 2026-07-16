/**
 * @file motor_config.h
 * @brief 电机驱动配置文件
 *
 * 从 pin_config.h 导入引脚配置
 */

#ifndef MOTOR_CONFIG_H
#define MOTOR_CONFIG_H

#include "config/pin_config.h"

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
#define TEST_TASK_PRIORITY      1

// 任务堆栈大小
#define MOTOR_TASK_STACK_SIZE   2048
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

#endif // MOTOR_CONFIG_H
