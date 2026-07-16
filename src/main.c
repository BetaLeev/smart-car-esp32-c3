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
#include "config/app_config.h"
#include "config/pin_config.h"
#include "config/wifi_config.h"

static const char *TAG = "MAIN";

/**
 * @brief 初始化喇叭 GPIO
 */
static void buzzer_init(void)
{
#if CONFIG_BUZZER_ENABLED
    gpio_reset_pin(BUZZER_PIN);
    gpio_set_direction(BUZZER_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(BUZZER_PIN, 0);  // 初始关闭
    ESP_LOGI(TAG, "Buzzer initialized on GPIO %d", BUZZER_PIN);
#endif
}

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
    // 等待串口稳定，确保烧录后串口工具能接收到日志
    vTaskDelay(pdMS_TO_TICKS(1000));

    ESP_LOGI(TAG, "===========================================");
    ESP_LOGI(TAG, "ESP32-C3-CAR Starting...");
    ESP_LOGI(TAG, "Device: %s", DEVICE_NAME);
    ESP_LOGI(TAG, "===========================================");

#if CONFIG_MOTOR_ENABLED
    // 1. 初始化电机驱动
    ESP_LOGI(TAG, "Step 1: Initializing motor driver...");
    motor_driver_init();
    ESP_LOGI(TAG, "Motor driver initialized");
#endif

#if CONFIG_BUZZER_ENABLED
    // 1.5 初始化喇叭
    ESP_LOGI(TAG, "Step 1.5: Initializing buzzer...");
    buzzer_init();
    ESP_LOGI(TAG, "Buzzer initialized");
#endif

    // 2. 初始化 Wi-Fi 管理器
    ESP_LOGI(TAG, "Step 2: Initializing Wi-Fi...");
    esp_err_t ret = wifi_manager_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Wi-Fi init failed: %s", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "Wi-Fi manager initialized");

        // 3. 启动 Wi-Fi (带重试机制)
        ESP_LOGI(TAG, "Step 3: Starting Wi-Fi...");
        int retry_count = 0;
        const int max_retries = 3;
        bool wifi_started = false;

        while (retry_count < max_retries) {
            ret = wifi_connect(CONFIG_AP_SSID, CONFIG_AP_PASSWORD);
            if (ret == ESP_OK) {
                ESP_LOGI(TAG, "Wi-Fi started successfully");
                wifi_started = true;
                break;
            }
            retry_count++;
            if (retry_count < max_retries) {
                ESP_LOGW(TAG, "Wi-Fi start failed (attempt %d/%d), retrying...", retry_count, max_retries);
                vTaskDelay(pdMS_TO_TICKS(1000));
            } else {
                ESP_LOGE(TAG, "Wi-Fi start failed after %d attempts: %s", max_retries, esp_err_to_name(ret));
            }
        }

        if (wifi_started) {
#if CONFIG_WEB_SERVER_ENABLED
            // 4. 初始化 Web 服务器
            ESP_LOGI(TAG, "Step 4: Starting web server...");
            web_server_init(CONFIG_WEB_SERVER_PORT);
            web_server_start();
            ESP_LOGI(TAG, "Web server started on port %d", CONFIG_WEB_SERVER_PORT);
#endif
        }
    }

#if CONFIG_LED_ENABLED
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
#endif

    ESP_LOGI(TAG, "===========================================");
    ESP_LOGI(TAG, "System ready!");
    ESP_LOGI(TAG, "Wi-Fi SSID: %s", CONFIG_AP_SSID);
    ESP_LOGI(TAG, "===========================================");
}
