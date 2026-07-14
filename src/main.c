#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "motor_driver.h"

static const char *TAG = "MAIN";

/**
 * @brief LED 闪烁任务
 */
void led_blink_task(void *pvParameters)
{
    gpio_reset_pin(LED_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);

    ESP_LOGI(TAG, "LED Blink Task Started (GPIO %s)", PIN_NAME(LED_PIN));

    bool led_state = false;
    while (1) {
        led_state = !led_state;
        gpio_set_level(LED_PIN, led_state ? 1 : 0);
        ESP_LOGI(TAG, "LED: %s", led_state ? "ON" : "OFF");
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

/**
 * @brief 测试引脚扫描任务
 */
void pin_test_task(void *pvParameters)
{
    gpio_num_t test_pins[] = {
        TEST_PIN_0, TEST_PIN_1, TEST_PIN_2,
        TEST_PIN_3, TEST_PIN_4
    };

    ESP_LOGI(TAG, "Pin Test Task Started");

    // 初始化测试引脚
    for (int i = 0; i < 5; i++) {
        gpio_reset_pin(test_pins[i]);
        gpio_set_direction(test_pins[i], GPIO_MODE_OUTPUT);
    }

    int pin_index = 0;
    while (1) {
        // 关闭所有引脚
        for (int i = 0; i < 5; i++) {
            gpio_set_level(test_pins[i], 0);
        }

        // 依次点亮每个引脚
        gpio_set_level(test_pins[pin_index], 1);
        ESP_LOGI(TAG, "Testing Pin: %s (HIGH)", pin_to_name(test_pins[pin_index]));

        pin_index = (pin_index + 1) % 5;
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/**
 * @brief 电机演示任务 - 小车运动测试
 */
void motor_demo_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Motor Demo Task Started");
    ESP_LOGI(TAG, "Testing Mode: Car Movement Demo");

    // 等待其他任务初始化
    vTaskDelay(pdMS_TO_TICKS(1000));

    while (1) {
        // 前进 2秒
        ESP_LOGI(TAG, ">>> 前进");
        gpio_set_level(MOTOR_A_IN1_PIN, 1);  gpio_set_level(MOTOR_A_IN2_PIN, 0);
        gpio_set_level(MOTOR_B_IN1_PIN, 1);  gpio_set_level(MOTOR_B_IN2_PIN, 0);
        vTaskDelay(pdMS_TO_TICKS(2000));

        // 停止 1秒
        ESP_LOGI(TAG, ">>> 停止");
        gpio_set_level(MOTOR_A_IN1_PIN, 0);  gpio_set_level(MOTOR_A_IN2_PIN, 0);
        gpio_set_level(MOTOR_B_IN1_PIN, 0);  gpio_set_level(MOTOR_B_IN2_PIN, 0);
        vTaskDelay(pdMS_TO_TICKS(1000));

        // 后退 2秒
        ESP_LOGI(TAG, ">>> 后退");
        gpio_set_level(MOTOR_A_IN1_PIN, 0);  gpio_set_level(MOTOR_A_IN2_PIN, 1);
        gpio_set_level(MOTOR_B_IN1_PIN, 0);  gpio_set_level(MOTOR_B_IN2_PIN, 1);
        vTaskDelay(pdMS_TO_TICKS(2000));

        // 停止 1秒
        ESP_LOGI(TAG, ">>> 停止");
        gpio_set_level(MOTOR_A_IN1_PIN, 0);  gpio_set_level(MOTOR_A_IN2_PIN, 0);
        gpio_set_level(MOTOR_B_IN1_PIN, 0);  gpio_set_level(MOTOR_B_IN2_PIN, 0);
        vTaskDelay(pdMS_TO_TICKS(1000));

        // 左转 2秒（原地左转）
        ESP_LOGI(TAG, ">>> 左转");
        gpio_set_level(MOTOR_A_IN1_PIN, 0);  gpio_set_level(MOTOR_A_IN2_PIN, 1);  // 左轮后退
        gpio_set_level(MOTOR_B_IN1_PIN, 1);  gpio_set_level(MOTOR_B_IN2_PIN, 0);  // 右轮前进
        vTaskDelay(pdMS_TO_TICKS(2000));

        // 停止 1秒
        ESP_LOGI(TAG, ">>> 停止");
        gpio_set_level(MOTOR_A_IN1_PIN, 0);  gpio_set_level(MOTOR_A_IN2_PIN, 0);
        gpio_set_level(MOTOR_B_IN1_PIN, 0);  gpio_set_level(MOTOR_B_IN2_PIN, 0);
        vTaskDelay(pdMS_TO_TICKS(1000));

        // 右转 2秒（原地右转）
        ESP_LOGI(TAG, ">>> 右转");
        gpio_set_level(MOTOR_A_IN1_PIN, 1);  gpio_set_level(MOTOR_A_IN2_PIN, 0);  // 左轮前进
        gpio_set_level(MOTOR_B_IN1_PIN, 0);  gpio_set_level(MOTOR_B_IN2_PIN, 1);  // 右轮后退
        vTaskDelay(pdMS_TO_TICKS(2000));

        // 停止 1秒
        ESP_LOGI(TAG, ">>> 停止");
        gpio_set_level(MOTOR_A_IN1_PIN, 0);  gpio_set_level(MOTOR_A_IN2_PIN, 0);
        gpio_set_level(MOTOR_B_IN1_PIN, 0);  gpio_set_level(MOTOR_B_IN2_PIN, 0);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/**
 * @brief 主函数
 */
void app_main(void)
{
    ESP_LOGI(TAG, "===========================================");
    ESP_LOGI(TAG, "ESP32-C3-CAR Test Program");
    ESP_LOGI(TAG, "Board: ESP32-C3-Supermini");
    ESP_LOGI(TAG, "Motor Driver: DRV8833");
    ESP_LOGI(TAG, "===========================================");

    // 初始化电机驱动
    motor_driver_init();

    // 创建 LED 闪烁任务
    xTaskCreatePinnedToCore(
        led_blink_task,
        "LED_Blink",
        LED_TASK_STACK_SIZE,
        NULL,
        LED_TASK_PRIORITY,
        NULL,
        PRO_CPU_NUM
    );

    // 创建引脚测试任务
    xTaskCreatePinnedToCore(
        pin_test_task,
        "Pin_Test",
        TEST_TASK_STACK_SIZE,
        NULL,
        TEST_TASK_PRIORITY,
        NULL,
        PRO_CPU_NUM
    );

    // 创建电机演示任务
    xTaskCreatePinnedToCore(
        motor_demo_task,
        "Motor_Demo",
        MOTOR_TASK_STACK_SIZE,
        NULL,
        MOTOR_TASK_PRIORITY,
        NULL,
        PRO_CPU_NUM
    );
}
