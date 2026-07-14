/**
 * @file motor_driver.c
 * @brief 电机驱动实现
 * 
 * 支持 DRV8833 和 TB6612FNG
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "motor_config.h"

static const char *TAG = "MOTOR_DRIVER";

// ============================================
// 电机控制函数
// ============================================

/**
 * @brief 初始化电机驱动
 */
void motor_driver_init(void)
{
    ESP_LOGI(TAG, "===========================================");
    ESP_LOGI(TAG, "Motor Driver Configuration:");
    
#ifdef USE_DRV8833
    ESP_LOGI(TAG, "Driver: DRV8833");
    ESP_LOGI(TAG, "Motor A: %s, %s",
             PIN_NAME(MOTOR_A_IN1_PIN),
             PIN_NAME(MOTOR_A_IN2_PIN));
    ESP_LOGI(TAG, "Motor B: %s, %s",
             PIN_NAME(MOTOR_B_IN1_PIN),
             PIN_NAME(MOTOR_B_IN2_PIN));
    
    // 初始化电机 A 引脚
    gpio_reset_pin(MOTOR_A_IN1_PIN);
    gpio_reset_pin(MOTOR_A_IN2_PIN);
    gpio_set_direction(MOTOR_A_IN1_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(MOTOR_A_IN2_PIN, GPIO_MODE_OUTPUT);
    
    // 初始化电机 B 引脚
    gpio_reset_pin(MOTOR_B_IN1_PIN);
    gpio_reset_pin(MOTOR_B_IN2_PIN);
    gpio_set_direction(MOTOR_B_IN1_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(MOTOR_B_IN2_PIN, GPIO_MODE_OUTPUT);
    
    // 默认状态：都输出高电平（常亮）
    gpio_set_level(MOTOR_A_IN1_PIN, 1);
    gpio_set_level(MOTOR_A_IN2_PIN, 1);
    gpio_set_level(MOTOR_B_IN1_PIN, 1);
    gpio_set_level(MOTOR_B_IN2_PIN, 1);
    
#else
    ESP_LOGI(TAG, "Driver: TB6612FNG");
    
    // 初始化 STBY 使能引脚
    gpio_reset_pin(MOTOR_STBY_PIN);
    gpio_set_direction(MOTOR_STBY_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(MOTOR_STBY_PIN, 1);  // 使能电机驱动
    
    // 初始化电机引脚
    gpio_reset_pin(LEFT_MOTOR_IN1_PIN);
    gpio_reset_pin(LEFT_MOTOR_IN2_PIN);
    gpio_set_direction(LEFT_MOTOR_IN1_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(LEFT_MOTOR_IN2_PIN, GPIO_MODE_OUTPUT);
    
    gpio_reset_pin(RIGHT_MOTOR_IN1_PIN);
    gpio_reset_pin(RIGHT_MOTOR_IN2_PIN);
    gpio_set_direction(RIGHT_MOTOR_IN1_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(RIGHT_MOTOR_IN2_PIN, GPIO_MODE_OUTPUT);
    
    // 默认停止状态
    gpio_set_level(LEFT_MOTOR_IN1_PIN, 0);
    gpio_set_level(LEFT_MOTOR_IN2_PIN, 0);
    gpio_set_level(RIGHT_MOTOR_IN1_PIN, 0);
    gpio_set_level(RIGHT_MOTOR_IN2_PIN, 0);
#endif

    ESP_LOGI(TAG, "===========================================");
    ESP_LOGI(TAG, "Motor Driver initialized successfully!");
}

/**
 * @brief 设置电机 A 状态
 */
void motor_a_set(motor_direction_t direction)
{
#ifdef USE_DRV8833
    switch (direction) {
        case MOTOR_FORWARD:
            gpio_set_level(MOTOR_A_IN1_PIN, 1);
            gpio_set_level(MOTOR_A_IN2_PIN, 0);
            ESP_LOGD(TAG, "Motor A: FORWARD");
            break;
        case MOTOR_BACKWARD:
            gpio_set_level(MOTOR_A_IN1_PIN, 0);
            gpio_set_level(MOTOR_A_IN2_PIN, 1);
            ESP_LOGD(TAG, "Motor A: BACKWARD");
            break;
        case MOTOR_STOP:
        case MOTOR_BRAKE:
        default:
            gpio_set_level(MOTOR_A_IN1_PIN, 0);
            gpio_set_level(MOTOR_A_IN2_PIN, 0);
            ESP_LOGD(TAG, "Motor A: STOP");
            break;
    }
#endif
}

/**
 * @brief 设置电机 B 状态
 */
void motor_b_set(motor_direction_t direction)
{
#ifdef USE_DRV8833
    switch (direction) {
        case MOTOR_FORWARD:
            gpio_set_level(MOTOR_B_IN1_PIN, 1);
            gpio_set_level(MOTOR_B_IN2_PIN, 0);
            ESP_LOGD(TAG, "Motor B: FORWARD");
            break;
        case MOTOR_BACKWARD:
            gpio_set_level(MOTOR_B_IN1_PIN, 0);
            gpio_set_level(MOTOR_B_IN2_PIN, 1);
            ESP_LOGD(TAG, "Motor B: BACKWARD");
            break;
        case MOTOR_STOP:
        case MOTOR_BRAKE:
        default:
            gpio_set_level(MOTOR_B_IN1_PIN, 0);
            gpio_set_level(MOTOR_B_IN2_PIN, 0);
            ESP_LOGD(TAG, "Motor B: STOP");
            break;
    }
#endif
}

/**
 * @brief 设置左电机状态
 */
void motor_left_set(motor_direction_t direction)
{
#ifdef USE_DRV8833
    motor_a_set(direction);
#else
    switch (direction) {
        case MOTOR_FORWARD:
            gpio_set_level(LEFT_MOTOR_IN1_PIN, 1);
            gpio_set_level(LEFT_MOTOR_IN2_PIN, 0);
            break;
        case MOTOR_BACKWARD:
            gpio_set_level(LEFT_MOTOR_IN1_PIN, 0);
            gpio_set_level(LEFT_MOTOR_IN2_PIN, 1);
            break;
        case MOTOR_STOP:
        case MOTOR_BRAKE:
        default:
            gpio_set_level(LEFT_MOTOR_IN1_PIN, 0);
            gpio_set_level(LEFT_MOTOR_IN2_PIN, 0);
            break;
    }
#endif
}

/**
 * @brief 设置右电机状态
 */
void motor_right_set(motor_direction_t direction)
{
#ifdef USE_DRV8833
    motor_b_set(direction);
#else
    switch (direction) {
        case MOTOR_FORWARD:
            gpio_set_level(RIGHT_MOTOR_IN1_PIN, 1);
            gpio_set_level(RIGHT_MOTOR_IN2_PIN, 0);
            break;
        case MOTOR_BACKWARD:
            gpio_set_level(RIGHT_MOTOR_IN1_PIN, 0);
            gpio_set_level(RIGHT_MOTOR_IN2_PIN, 1);
            break;
        case MOTOR_STOP:
        case MOTOR_BRAKE:
        default:
            gpio_set_level(RIGHT_MOTOR_IN1_PIN, 0);
            gpio_set_level(RIGHT_MOTOR_IN2_PIN, 0);
            break;
    }
#endif
}

/**
 * @brief 设置两个电机状态
 */
void motor_set_both(motor_direction_t left_dir, motor_direction_t right_dir)
{
    motor_left_set(left_dir);
    motor_right_set(right_dir);
}

/**
 * @brief 停止所有电机
 */
void motor_stop_all(void)
{
    motor_left_set(MOTOR_STOP);
    motor_right_set(MOTOR_STOP);
    ESP_LOGI(TAG, "All motors stopped!");
}

/**
 * @brief 小车前进
 */
void car_forward(void)
{
    motor_set_both(MOTOR_FORWARD, MOTOR_FORWARD);
    ESP_LOGI(TAG, "Car: FORWARD");
}

/**
 * @brief 小车后退
 */
void car_backward(void)
{
    motor_set_both(MOTOR_BACKWARD, MOTOR_BACKWARD);
    ESP_LOGI(TAG, "Car: BACKWARD");
}

/**
 * @brief 小车左转（原地左转）
 */
void car_turn_left(void)
{
    motor_left_set(MOTOR_BACKWARD);
    motor_right_set(MOTOR_FORWARD);
    ESP_LOGI(TAG, "Car: TURN LEFT");
}

/**
 * @brief 小车右转（原地右转）
 */
void car_turn_right(void)
{
    motor_left_set(MOTOR_FORWARD);
    motor_right_set(MOTOR_BACKWARD);
    ESP_LOGI(TAG, "Car: TURN RIGHT");
}

/**
 * @brief 小车停止
 */
void car_stop(void)
{
    motor_stop_all();
    ESP_LOGI(TAG, "Car: STOP");
}
