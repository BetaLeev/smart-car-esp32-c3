/**
 * @file buzzer.c
 * @brief 蜂鸣器驱动实现
 *
 * 支持低电平触发的有源蜂鸣器
 */

#include "buzzer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "config/pin_config.h"

static const char *TAG = "BUZZER";

// 低电平触发：高电平关闭，低电平打开
#define BUZZER_OFF_LEVEL  1
#define BUZZER_ON_LEVEL   0

/**
 * @brief 初始化蜂鸣器
 */
void buzzer_init(void)
{
    ESP_LOGI(TAG, "===========================================");
    ESP_LOGI(TAG, "Buzzer Configuration:");
    ESP_LOGI(TAG, "  Pin: %s (Active Low)", PIN_NAME(BUZZER_PIN));

    gpio_set_direction(BUZZER_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(BUZZER_PIN, BUZZER_OFF_LEVEL);  // 默认关闭（高电平）

    ESP_LOGI(TAG, "===========================================");
    ESP_LOGI(TAG, "Buzzer initialized successfully!");
}

/**
 * @brief 打开蜂鸣器
 */
void buzzer_on(void)
{
    gpio_set_level(BUZZER_PIN, BUZZER_ON_LEVEL);   // 低电平触发
    ESP_LOGD(TAG, "Buzzer ON");
}

/**
 * @brief 关闭蜂鸣器
 */
void buzzer_off(void)
{
    gpio_set_level(BUZZER_PIN, BUZZER_OFF_LEVEL);  // 高电平关闭
    ESP_LOGD(TAG, "Buzzer OFF");
}

/**
 * @brief 蜂鸣一声
 * @param duration_ms 持续时间（毫秒）
 */
void buzzer_beep(int duration_ms)
{
    buzzer_on();
    vTaskDelay(pdMS_TO_TICKS(duration_ms));
    buzzer_off();
    ESP_LOGI(TAG, "Beep: %d ms", duration_ms);
}

/**
 * @brief 蜂鸣多次
 * @param times 次数
 * @param duration_ms 每次持续时间（毫秒）
 * @param interval_ms 间隔时间（毫秒）
 */
void buzzer_beep_multiple(int times, int duration_ms, int interval_ms)
{
    for (int i = 0; i < times; i++) {
        buzzer_beep(duration_ms);
        if (i < times - 1) {
            vTaskDelay(pdMS_TO_TICKS(interval_ms));
        }
    }
    ESP_LOGI(TAG, "Beep: %d times, %d ms each", times, duration_ms);
}
