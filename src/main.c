#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "motor_driver.h"
#include "wifi_manager.h"
#include "web_server.h"
#include "esp_err.h"

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
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/**
 * @brief 主函数
 */
void app_main(void)
{
    ESP_LOGI(TAG, "===========================================");
    ESP_LOGI(TAG, "ESP32-C3-CAR Starting...");
    ESP_LOGI(TAG, "===========================================");

    // 1. 初始化电机驱动
    ESP_LOGI(TAG, "Step 1: Initializing motor driver...");
    motor_driver_init();
    ESP_LOGI(TAG, "Motor driver initialized");

    // 2. 初始化 Wi-Fi 管理器
    ESP_LOGI(TAG, "Step 2: Initializing Wi-Fi...");
    esp_err_t ret = wifi_manager_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Wi-Fi init failed: %s", esp_err_to_name(ret));
        // 继续运行，但Wi-Fi可能不可用
    } else {
        ESP_LOGI(TAG, "Wi-Fi manager initialized");

        // 3. 启动 AP 模式
        ESP_LOGI(TAG, "Step 3: Starting Wi-Fi AP...");
        ret = wifi_connect("ESP32-CAR", NULL);  // 开放网络，无密码
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Wi-Fi AP start failed: %s", esp_err_to_name(ret));
        } else {
            ESP_LOGI(TAG, "Wi-Fi AP started successfully");

            // 4. 初始化 Web 服务器
            ESP_LOGI(TAG, "Step 4: Starting web server...");
            web_server_init(80);
            web_server_start();
            ESP_LOGI(TAG, "Web server started");
        }
    }

    // 5. 创建 LED 闪烁任务
    ESP_LOGI(TAG, "Step 5: Starting LED blink task...");
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
    ESP_LOGI(TAG, "Wi-Fi SSID: ESP32-CAR (no password)");
    ESP_LOGI(TAG, "Web Interface: http://192.168.4.1");
    ESP_LOGI(TAG, "===========================================");
}
