/**
 * @file button.c
 * @brief 物理按键控制实现
 *
 * 按键控制:
 * - GPIO 20 按键: 水泵 4档 (OFF/LOW/MED/HIGH)
 * - GPIO 1 按键: 所有泵定时模式开关 (ON/OFF 循环: 40s开/20s停)
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_err.h"
#include "config/pin_config.h"
#include "motor_driver.h"

static const char *TAG = "BUTTON";

// 按键队列
static QueueHandle_t s_button_queue = NULL;

// 按键事件
typedef enum {
    BUTTON_EVENT_WATER_PUMP,
    BUTTON_EVENT_ALL_TIMER
} button_event_t;

/**
 * @brief 按键 GPIO 中断处理
 */
static void IRAM_ATTR button_isr_handler(void *arg)
{
    BaseType_t higher_priority_woken = pdFALSE;
    uint32_t gpio_num = (uint32_t)arg;

    if (gpio_num == WATER_PUMP_BTN_PIN) {
        xQueueSendFromISR(s_button_queue, &(uint8_t){BUTTON_EVENT_WATER_PUMP}, &higher_priority_woken);
    } else if (gpio_num == OXYGEN_PUMP_BTN_PIN) {
        xQueueSendFromISR(s_button_queue, &(uint8_t){BUTTON_EVENT_ALL_TIMER}, &higher_priority_woken);
    }

    portYIELD_FROM_ISR(higher_priority_woken);
}

/**
 * @brief 初始化按键
 */
void button_init(void)
{
    ESP_LOGI(TAG, "Initializing buttons...");
    ESP_LOGI(TAG, "  GPIO %d: Water pump (OFF->LOW->MED->HIGH)", WATER_PUMP_BTN_PIN);
    ESP_LOGI(TAG, "  GPIO %d: All pumps timer toggle (ON/OFF)", OXYGEN_PUMP_BTN_PIN);

    s_button_queue = xQueueCreate(10, sizeof(uint8_t));
    if (s_button_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create button queue");
        return;
    }

    // 配置水泵按键
    gpio_reset_pin(WATER_PUMP_BTN_PIN);
    gpio_set_direction(WATER_PUMP_BTN_PIN, GPIO_MODE_INPUT);
    gpio_pullup_en(WATER_PUMP_BTN_PIN);
    gpio_pulldown_dis(WATER_PUMP_BTN_PIN);
    gpio_set_intr_type(WATER_PUMP_BTN_PIN, GPIO_INTR_NEGEDGE);

    // 配置定时开关按键
    gpio_reset_pin(OXYGEN_PUMP_BTN_PIN);
    gpio_set_direction(OXYGEN_PUMP_BTN_PIN, GPIO_MODE_INPUT);
    gpio_pullup_en(OXYGEN_PUMP_BTN_PIN);
    gpio_pulldown_dis(OXYGEN_PUMP_BTN_PIN);
    gpio_set_intr_type(OXYGEN_PUMP_BTN_PIN, GPIO_INTR_NEGEDGE);

    // 注册中断
    gpio_install_isr_service(0);
    gpio_isr_handler_add(WATER_PUMP_BTN_PIN, button_isr_handler, (void *)WATER_PUMP_BTN_PIN);
    gpio_isr_handler_add(OXYGEN_PUMP_BTN_PIN, button_isr_handler, (void *)OXYGEN_PUMP_BTN_PIN);

    ESP_LOGI(TAG, "Buttons initialized");
}

/**
 * @brief 按键控制任务
 */
void button_task(void *pvParameters)
{
    uint8_t event;

    ESP_LOGI(TAG, "Button task started");

    while (1) {
        if (xQueueReceive(s_button_queue, &event, portMAX_DELAY) == pdTRUE) {
            // 消抖
            vTaskDelay(pdMS_TO_TICKS(50));

            if (event == BUTTON_EVENT_WATER_PUMP) {
                if (gpio_get_level(WATER_PUMP_BTN_PIN) == 0) {
                    // 循环切换: 0->1->2->3->0
                    int level = water_pump_get_level();
                    level = (level + 1) % 4;
                    water_pump_set_level(level);
                    ESP_LOGI(TAG, "AO (Water pump): level=%d", level);
                    while (gpio_get_level(WATER_PUMP_BTN_PIN) == 0) {
                        vTaskDelay(pdMS_TO_TICKS(20));
                    }
                }
            } else if (event == BUTTON_EVENT_ALL_TIMER) {
                if (gpio_get_level(OXYGEN_PUMP_BTN_PIN) == 0) {
                    // 切换所有泵的定时模式: ON <-> OFF
                    bool timer_enabled = all_pumps_get_timer();
                    all_pumps_set_timer(!timer_enabled);
                    ESP_LOGI(TAG, "All pumps timer: %s", timer_enabled ? "DISABLED" : "ENABLED (40s ON / 20s OFF)");
                    while (gpio_get_level(OXYGEN_PUMP_BTN_PIN) == 0) {
                        vTaskDelay(pdMS_TO_TICKS(20));
                    }
                }
            }
        }
    }
}
