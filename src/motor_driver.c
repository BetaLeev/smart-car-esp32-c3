/**
 * @file motor_driver.c
 * @brief 电机驱动实现
 *
 * 支持 DRV8833 和 TB6612FNG
 * 引脚配置在 pin_config.h 中
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "config/pin_config.h"
#include "motor_config.h"

static const char *TAG = "MOTOR_DRIVER";

// ============================================
// DRV8833 电机控制逻辑
// ============================================

/**
 * @brief 设置 DRV8833 电机 A 状态
 * @param direction 电机方向
 */
static void drv8833_motor_a_set(motor_direction_t direction)
{
    switch (direction) {
        case MOTOR_FORWARD:
            gpio_set_level(MOTOR_AIN1_PIN, 1);
            gpio_set_level(MOTOR_AIN2_PIN, 0);
            ESP_LOGD(TAG, "Motor A: FORWARD");
            break;
        case MOTOR_BACKWARD:
            gpio_set_level(MOTOR_AIN1_PIN, 0);
            gpio_set_level(MOTOR_AIN2_PIN, 1);
            ESP_LOGD(TAG, "Motor A: BACKWARD");
            break;
        case MOTOR_STOP:
        case MOTOR_BRAKE:
        default:
            gpio_set_level(MOTOR_AIN1_PIN, 0);
            gpio_set_level(MOTOR_AIN2_PIN, 0);
            ESP_LOGD(TAG, "Motor A: STOP");
            break;
    }
}

/**
 * @brief 设置 DRV8833 电机 B 状态
 * @param direction 电机方向
 */
static void drv8833_motor_b_set(motor_direction_t direction)
{
    switch (direction) {
        case MOTOR_FORWARD:
            gpio_set_level(MOTOR_BIN1_PIN, 1);
            gpio_set_level(MOTOR_BIN2_PIN, 0);
            ESP_LOGD(TAG, "Motor B: FORWARD");
            break;
        case MOTOR_BACKWARD:
            gpio_set_level(MOTOR_BIN1_PIN, 0);
            gpio_set_level(MOTOR_BIN2_PIN, 1);
            ESP_LOGD(TAG, "Motor B: BACKWARD");
            break;
        case MOTOR_STOP:
        case MOTOR_BRAKE:
        default:
            gpio_set_level(MOTOR_BIN1_PIN, 0);
            gpio_set_level(MOTOR_BIN2_PIN, 0);
            ESP_LOGD(TAG, "Motor B: STOP");
            break;
    }
}

/**
 * @brief 初始化 DRV8833 电机驱动
 */
static void drv8833_init(void)
{
    ESP_LOGI(TAG, "  Driver: DRV8833");
    ESP_LOGI(TAG, "  Motor A: %s (AIN1), %s (AIN2)",
             PIN_NAME(MOTOR_AIN1_PIN), PIN_NAME(MOTOR_AIN2_PIN));
    ESP_LOGI(TAG, "  Motor B: %s (BIN1), %s (BIN2)",
             PIN_NAME(MOTOR_BIN1_PIN), PIN_NAME(MOTOR_BIN2_PIN));

    // 电机 A - 先设置为输出并拉低，再 reset（避免浮空）
    gpio_set_direction(MOTOR_AIN1_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(MOTOR_AIN2_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(MOTOR_AIN1_PIN, 0);
    gpio_set_level(MOTOR_AIN2_PIN, 0);

    // 电机 B
    gpio_set_direction(MOTOR_BIN1_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(MOTOR_BIN2_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(MOTOR_BIN1_PIN, 0);
    gpio_set_level(MOTOR_BIN2_PIN, 0);

    ESP_LOGI(TAG, "  All motor pins set to LOW (stopped)");
}

// ============================================
// TB6612FNG 电机控制逻辑
// ============================================
#ifdef MOTOR_DRIVER_TB6612FNG

/**
 * @brief 设置 TB6612FNG 左电机状态
 * @param direction 电机方向
 */
static void tb6612fng_left_motor_set(motor_direction_t direction)
{
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
}

/**
 * @brief 设置 TB6612FNG 右电机状态
 * @param direction 电机方向
 */
static void tb6612fng_right_motor_set(motor_direction_t direction)
{
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
}

/**
 * @brief 初始化 TB6612FNG 电机驱动
 */
static void tb6612fng_init(void)
{
    ESP_LOGI(TAG, "  Driver: TB6612FNG");
    ESP_LOGI(TAG, "  STBY: %s", PIN_NAME(MOTOR_STBY_PIN));
    ESP_LOGI(TAG, "  Left Motor: %s, %s",
             PIN_NAME(LEFT_MOTOR_IN1_PIN), PIN_NAME(LEFT_MOTOR_IN2_PIN));
    ESP_LOGI(TAG, "  Right Motor: %s, %s",
             PIN_NAME(RIGHT_MOTOR_IN1_PIN), PIN_NAME(RIGHT_MOTOR_IN2_PIN));

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
}

#endif  // MOTOR_DRIVER_TB6612FNG

// ============================================
// 公共 API 实现
// ============================================

/**
 * @brief 初始化电机驱动
 */
void motor_driver_init(void)
{
    ESP_LOGI(TAG, "===========================================");
    ESP_LOGI(TAG, "Motor Driver Initialization:");

#ifdef MOTOR_DRIVER_DRV8833
    drv8833_init();
#elif defined(MOTOR_DRIVER_TB6612FNG)
    tb6612fng_init();
#else
    #error "No motor driver type defined! Set MOTOR_DRIVER_TYPE in pin_config.h"
#endif

    ESP_LOGI(TAG, "===========================================");
    ESP_LOGI(TAG, "Motor Driver initialized successfully!");
}

/**
 * @brief 设置左电机状态
 */
void motor_left_set(motor_direction_t direction)
{
#ifdef MOTOR_DRIVER_DRV8833
    drv8833_motor_a_set(direction);
#elif defined(MOTOR_DRIVER_TB6612FNG)
    tb6612fng_left_motor_set(direction);
#endif
}

/**
 * @brief 设置右电机状态
 */
void motor_right_set(motor_direction_t direction)
{
#ifdef MOTOR_DRIVER_DRV8833
    drv8833_motor_b_set(direction);
#elif defined(MOTOR_DRIVER_TB6612FNG)
    tb6612fng_right_motor_set(direction);
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
