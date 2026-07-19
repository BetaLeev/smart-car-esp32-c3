#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "motor_driver.h"
#include "esp_err.h"
#include "config/app_config.h"
#include "config/pin_config.h"

static const char *TAG = "MAIN";

/**
 * @brief 初始化喇叭 GPIO
 */
static void buzzer_init(void)
{
#if CONFIG_BUZZER_ENABLED
    gpio_reset_pin(BUZZER_PIN);
    gpio_set_direction(BUZZER_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(BUZZER_PIN, 0);
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
 * @brief 测试任务 - 自动运行测试程序
 */
void test_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Test started!");

    while (1) {
        // 1. 前进
        ESP_LOGI(TAG, ">>> Forward");
        car_forward();
        vTaskDelay(pdMS_TO_TICKS(2000));

        // 2. 后退
        ESP_LOGI(TAG, ">>> Backward");
        car_backward();
        vTaskDelay(pdMS_TO_TICKS(2000));

        // 3. 左转
        ESP_LOGI(TAG, ">>> Turn Left");
        car_turn_left();
        vTaskDelay(pdMS_TO_TICKS(2000));

        // 4. 右转
        ESP_LOGI(TAG, ">>> Turn Right");
        car_turn_right();
        vTaskDelay(pdMS_TO_TICKS(2000));

        // 5. 停止
        ESP_LOGI(TAG, ">>> Stop");
        car_stop();
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

/**
 * @brief 主函数
 */
void app_main(void)
{
    vTaskDelay(pdMS_TO_TICKS(1000));

    ESP_LOGI(TAG, "===========================================");
    ESP_LOGI(TAG, "ESP32-C3-CAR Starting...");
    ESP_LOGI(TAG, "Device: %s", DEVICE_NAME);
    ESP_LOGI(TAG, "===========================================");

    // 初始化电机驱动
    ESP_LOGI(TAG, "Initializing motor driver...");
    motor_driver_init();
    ESP_LOGI(TAG, "Motor driver initialized");

    // 初始化喇叭
    buzzer_init();

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

    // 创建测试任务
    ESP_LOGI(TAG, "Starting test task...");
    xTaskCreatePinnedToCore(
        test_task,
        "Test",
        4096,
        NULL,
        2,
        NULL,
        PRO_CPU_NUM
    );

    ESP_LOGI(TAG, "===========================================");
    ESP_LOGI(TAG, "System ready!");
    ESP_LOGI(TAG, "===========================================");
}
