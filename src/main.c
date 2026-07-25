#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "motor_driver.h"
#include "buzzer/buzzer.h"
#include "esp_err.h"
#include "config/app_config.h"
#include "config/pin_config.h"
#include "config/wifi_config.h"
#include "wifi/wifi_manager.h"
#include "web/web_server.h"
// #include "fan_driver.h"  // TODO: 风扇功能暂时禁用（GPIO驱动能力不足）

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
    vTaskDelay(pdMS_TO_TICKS(100));

    // 重要：蜂鸣器必须最先初始化，防止GPIO状态不确定导致鸣叫
#if CONFIG_BUZZER_ENABLED
    // 直接设置GPIO为输出高电平，避免 gpio_reset_pin 导致的浮空状态
    gpio_set_direction(BUZZER_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(BUZZER_PIN, 1);  // 高电平关闭蜂鸣器（低电平触发）
    ESP_LOGI(TAG, "Buzzer GPIO configured: %s (HIGH = OFF)", PIN_NAME(BUZZER_PIN));
#endif

    ESP_LOGI(TAG, "===========================================");
    ESP_LOGI(TAG, "ESP32-C3-CAR Starting...");
    ESP_LOGI(TAG, "Device: %s", DEVICE_NAME);
    ESP_LOGI(TAG, "===========================================");

    // 初始化电机驱动
    ESP_LOGI(TAG, "Initializing motor driver...");
    motor_driver_init();
    ESP_LOGI(TAG, "Motor driver initialized");

    // 初始化风扇驱动 - 暂时禁用
    // ESP_LOGI(TAG, "Initializing fan driver...");
    // fan_driver_init();
    // ESP_LOGI(TAG, "Fan driver initialized");

    // 确保所有电机处于停止状态（安全措施）
    ESP_LOGI(TAG, "Ensuring all motors are stopped...");
    car_stop();

    // 初始化蜂鸣器（完整初始化，包含启动提示音）
#if CONFIG_BUZZER_ENABLED
    buzzer_init();
    buzzer_beep_multiple(2, 100, 100);  // 启动提示音
#endif

    // 初始化Wi-Fi (AP模式 - 热点)
    ESP_LOGI(TAG, "Initializing Wi-Fi (AP mode)...");
    esp_err_t ret = wifi_manager_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Wi-Fi init failed: %s", esp_err_to_name(ret));
    } else {
        ret = wifi_connect(CONFIG_AP_SSID, CONFIG_AP_PASSWORD);
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "Wi-Fi AP started! IP: %s", wifi_get_ip());

            // 初始化并启动Web服务器
            ESP_LOGI(TAG, "Starting web server...");
            web_server_init(80);
            ret = web_server_start();
            if (ret == ESP_OK) {
                ESP_LOGI(TAG, "Web server started on http://%s", wifi_get_ip());
            } else {
                ESP_LOGE(TAG, "Web server start failed: %s", esp_err_to_name(ret));
            }
        } else {
            ESP_LOGE(TAG, "Wi-Fi connect failed: %s", esp_err_to_name(ret));
        }
    }

    // 创建 LED 闪烁任务
    ESP_LOGI(TAG, "Starting LED blink task...");
    xTaskCreatePinnedToCore(
        led_blink_task,
        "LED_Blink",
        LED_TASK_STACK_SIZE,
        NULL,
        LED_TASK_PRIORITY,
        NULL,
        PRO_CPU_NUM
    );

    ESP_LOGI(TAG, "===========================================");
    ESP_LOGI(TAG, "System ready!");
    ESP_LOGI(TAG, "===========================================");
}
