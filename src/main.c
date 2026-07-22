/**
 * @file main.c
 * @brief ESP32-C3 双泵控制系统主程序
 *
 * 功能:
 *   - GPIO 20 按键: 水泵 4档 (OFF -> LOW -> MED -> HIGH -> OFF...)
 *   - GPIO 1 按键: 所有泵定时开关 (开启/关闭 40秒开/20秒停循环)
 *   - 通电自动: AO 和 BO 都开始 40秒开/20秒停循环
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "motor_driver.h"
#include "button.h"
#include "esp_err.h"
#include "config/app_config.h"
#include "config/pin_config.h"

static const char *TAG = "MAIN";

/**
 * @brief LED 闪烁任务
 */
void led_blink_task(void *pvParameters)
{
    gpio_reset_pin(LED_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);

    bool led_state = false;
    while (1) {
        led_state = !led_state;
        gpio_set_level(LED_PIN, led_state ? 1 : 0);
        vTaskDelay(pdMS_TO_TICKS(CONFIG_LED_BLINK_INTERVAL_MS));
    }
}

/**
 * @brief 主函数
 */
void app_main(void)
{
    vTaskDelay(pdMS_TO_TICKS(1000));

    ESP_LOGI(TAG, "===========================================");
    ESP_LOGI(TAG, "ESP32-C3 Dual Pump Control System");
    ESP_LOGI(TAG, "===========================================");

    // 1. 初始化泵驱动
    ESP_LOGI(TAG, "Initializing pump driver...");
    motor_driver_init();

    // 2. 初始化按键
    ESP_LOGI(TAG, "Initializing buttons...");
    button_init();

    // 3. 创建 LED 闪烁任务
    xTaskCreatePinnedToCore(
        led_blink_task,
        "LED_Blink",
        LED_TASK_STACK_SIZE,
        NULL,
        LED_TASK_PRIORITY,
        NULL,
        PRO_CPU_NUM
    );

    // 4. 创建按键任务
    xTaskCreatePinnedToCore(
        button_task,
        "Button",
        BUTTON_TASK_STACK_SIZE,
        NULL,
        BUTTON_TASK_PRIORITY,
        NULL,
        PRO_CPU_NUM
    );

    ESP_LOGI(TAG, "===========================================");
    ESP_LOGI(TAG, "System ready!");
    ESP_LOGI(TAG, "Controls:");
    ESP_LOGI(TAG, "  GPIO 20: Water pump (OFF->LOW->MED->HIGH->OFF)");
    ESP_LOGI(TAG, "  GPIO 1:  All pumps timer toggle (ON/OFF)");
    ESP_LOGI(TAG, "  Auto:     AO & BO timer (40s ON / 20s OFF)");
    ESP_LOGI(TAG, "===========================================");
}
