/**
 * @file rotary_encoder.c
 * @brief 旋转编码器驱动
 *
 * 功能:
 *   - 按压开关: 开/关 当前选中的泵
 *   - 左旋: 降低速度
 *   - 右旋: 升高速度
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

static const char *TAG = "ROTARY_ENCODER";

// 旋转编码器 GPIO
#ifndef ENCODER_SW_PIN
#define ENCODER_SW_PIN   GPIO_NUM_2   // 按钮
#define ENCODER_DT_PIN   GPIO_NUM_3   // B相
#define ENCODER_CLK_PIN  GPIO_NUM_4   // A相
#endif

// 速度等级
#define SPEED_LEVELS     10
#define DEFAULT_SPEED    5            // 默认中速

// 编码器状态
static volatile int s_current_speed = DEFAULT_SPEED;
static volatile bool s_encoder_enabled = false;  // 编码器是否启用
static volatile int s_last_clk = 1;
static volatile int64_t s_last_rotation_time = 0;

// 编码器事件
typedef enum {
    ENCODER_EVENT_PRESS,    // 按下
    ENCODER_EVENT_CW,       // 顺时针
    ENCODER_EVENT_CCW       // 逆时针
} encoder_event_t;

static QueueHandle_t s_encoder_queue = NULL;

// 当前控制目标 (false=水泵, true=氧气泵)
static volatile bool s_current_target = false;

/**
 * @brief 编码器 GPIO 中断处理
 */
static void IRAM_ATTR encoder_isr_handler(void *arg)
{
    BaseType_t higher_priority_woken = pdFALSE;
    uint32_t gpio_num = (uint32_t)arg;

    if (gpio_num == ENCODER_SW_PIN) {
        // 按钮按下
        xQueueSendFromISR(s_encoder_queue, &(uint8_t){ENCODER_EVENT_PRESS}, &higher_priority_woken);
    } else if (gpio_num == ENCODER_CLK_PIN || gpio_num == ENCODER_DT_PIN) {
        // 旋转检测
        int clk = gpio_get_level(ENCODER_CLK_PIN);
        int dt = gpio_get_level(ENCODER_DT_PIN);

        if (clk != s_last_clk) {
            s_last_clk = clk;
            if (clk == 0) {  // 下降沿
                if (dt == 0) {
                    xQueueSendFromISR(s_encoder_queue, &(uint8_t){ENCODER_EVENT_CW}, &higher_priority_woken);
                } else {
                    xQueueSendFromISR(s_encoder_queue, &(uint8_t){ENCODER_EVENT_CCW}, &higher_priority_woken);
                }
            }
        }
    }

    portYIELD_FROM_ISR(higher_priority_woken);
}

/**
 * @brief 初始化旋转编码器
 */
void rotary_encoder_init(void)
{
    ESP_LOGI(TAG, "Initializing rotary encoder...");
    ESP_LOGI(TAG, "  SW  (Button):  GPIO %d", ENCODER_SW_PIN);
    ESP_LOGI(TAG, "  DT  (B phase): GPIO %d", ENCODER_DT_PIN);
    ESP_LOGI(TAG, "  CLK (A phase): GPIO %d", ENCODER_CLK_PIN);

    s_encoder_queue = xQueueCreate(20, sizeof(uint8_t));
    if (s_encoder_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create encoder queue");
        return;
    }

    // 配置按钮 (上拉输入，低电平触发)
    gpio_reset_pin(ENCODER_SW_PIN);
    gpio_set_direction(ENCODER_SW_PIN, GPIO_MODE_INPUT);
    gpio_pullup_en(ENCODER_SW_PIN);
    gpio_pulldown_dis(ENCODER_SW_PIN);
    gpio_set_intr_type(ENCODER_SW_PIN, GPIO_INTR_NEGEDGE);

    // 配置 B相 (上拉输入)
    gpio_reset_pin(ENCODER_DT_PIN);
    gpio_set_direction(ENCODER_DT_PIN, GPIO_MODE_INPUT);
    gpio_pullup_en(ENCODER_DT_PIN);
    gpio_pulldown_dis(ENCODER_DT_PIN);

    // 配置 A相 (上拉输入，边沿触发)
    gpio_reset_pin(ENCODER_CLK_PIN);
    gpio_set_direction(ENCODER_CLK_PIN, GPIO_MODE_INPUT);
    gpio_pullup_en(ENCODER_CLK_PIN);
    gpio_pulldown_dis(ENCODER_CLK_PIN);
    gpio_set_intr_type(ENCODER_CLK_PIN, GPIO_INTR_ANYEDGE);

    // 注册中断
    gpio_install_isr_service(0);
    gpio_isr_handler_add(ENCODER_SW_PIN, encoder_isr_handler, (void *)ENCODER_SW_PIN);
    gpio_isr_handler_add(ENCODER_DT_PIN, encoder_isr_handler, (void *)ENCODER_DT_PIN);
    gpio_isr_handler_add(ENCODER_CLK_PIN, encoder_isr_handler, (void *)ENCODER_CLK_PIN);

    // 初始化当前速度
    s_current_speed = DEFAULT_SPEED;
    s_encoder_enabled = false;
    s_current_target = false;

    ESP_LOGI(TAG, "Rotary encoder initialized");
}

/**
 * @brief 设置编码器启用状态
 */
void rotary_encoder_set_enabled(bool enabled)
{
    s_encoder_enabled = enabled;
    if (enabled) {
        ESP_LOGI(TAG, "Encoder enabled (controlling current pump)");
    }
}

/**
 * @brief 获取当前速度等级 (0-SPEED_LEVELS)
 */
int rotary_encoder_get_speed(void)
{
    return s_current_speed;
}

/**
 * @brief 设置速度等级
 */
void rotary_encoder_set_speed(int speed)
{
    if (speed < 0) speed = 0;
    if (speed > SPEED_LEVELS) speed = SPEED_LEVELS;
    s_current_speed = speed;
}

/**
 * @brief 获取速度百分比 (0-100)
 */
int rotary_encoder_get_speed_percent(void)
{
    return (s_current_speed * 100) / SPEED_LEVELS;
}

/**
 * @brief 获取当前控制目标 (false=水泵, true=氧气泵)
 */
bool rotary_encoder_get_target(void)
{
    return s_current_target;
}

/**
 * @brief 设置当前控制目标
 */
void rotary_encoder_set_target(bool target)
{
    s_current_target = target;
    ESP_LOGI(TAG, "Encoder target changed to: %s", target ? "Oxygen pump" : "Water pump");
}

/**
 * @brief 编码器控制任务
 */
void rotary_encoder_task(void *pvParameters)
{
    uint8_t event;

    ESP_LOGI(TAG, "Rotary encoder task started");

    while (1) {
        if (xQueueReceive(s_encoder_queue, &event, portMAX_DELAY) == pdTRUE) {
            if (!s_encoder_enabled) {
                continue;
            }

            if (event == ENCODER_EVENT_PRESS) {
                // 消抖延迟
                vTaskDelay(pdMS_TO_TICKS(50));
                if (gpio_get_level(ENCODER_SW_PIN) == 0) {
                    // 切换泵的开关状态
                    bool current_state;
                    if (s_current_target) {
                        current_state = oxygen_pump_is_on();
                        oxygen_pump_set_state(!current_state);
                        ESP_LOGI(TAG, "Encoder: Oxygen pump %s", !current_state ? "ON" : "OFF");
                    } else {
                        current_state = water_pump_is_on();
                        water_pump_set_state(!current_state);
                        ESP_LOGI(TAG, "Encoder: Water pump %s", !current_state ? "ON" : "OFF");
                    }
                    // 等待按键释放
                    while (gpio_get_level(ENCODER_SW_PIN) == 0) {
                        vTaskDelay(pdMS_TO_TICKS(20));
                    }
                }
            } else if (event == ENCODER_EVENT_CW) {
                // 顺时针 - 增加速度
                if (s_current_speed < SPEED_LEVELS) {
                    s_current_speed++;
                    int percent = rotary_encoder_get_speed_percent();
                    if (s_current_target) {
                        oxygen_pump_set_speed(percent);
                        ESP_LOGI(TAG, "Encoder: Oxygen pump speed %d%%", percent);
                    } else {
                        water_pump_set_speed(percent);
                        ESP_LOGI(TAG, "Encoder: Water pump speed %d%%", percent);
                    }
                }
            } else if (event == ENCODER_EVENT_CCW) {
                // 逆时针 - 降低速度
                if (s_current_speed > 0) {
                    s_current_speed--;
                    int percent = rotary_encoder_get_speed_percent();
                    if (s_current_target) {
                        oxygen_pump_set_speed(percent);
                        ESP_LOGI(TAG, "Encoder: Oxygen pump speed %d%%", percent);
                    } else {
                        water_pump_set_speed(percent);
                        ESP_LOGI(TAG, "Encoder: Water pump speed %d%%", percent);
                    }
                }
            }
        }
    }
}
