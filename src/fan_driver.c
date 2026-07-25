/**
 * @file fan_driver.c
 * @brief 小风扇驱动实现
 *
 * 使用 LEDC PWM 控制风扇转速
 * GPIO 3 作为 PWM 输出引脚
 */

#include "fan_driver.h"
#include "esp_log.h"
#include "driver/ledc.h"
#include "driver/gpio.h"
#include "config/pin_config.h"

static const char *TAG = "FAN_DRIVER";

// 风扇状态
static bool s_fan_enabled = false;
static uint8_t s_fan_speed = 0;

/**
 * @brief 初始化风扇驱动
 */
void fan_driver_init(void)
{
    ESP_LOGI(TAG, "===========================================");
    ESP_LOGI(TAG, "Fan Driver Initialization:");
    ESP_LOGI(TAG, "  GPIO Pin: %s", PIN_NAME(FAN_GPIO_PIN));
    ESP_LOGI(TAG, "  PWM Frequency: %d Hz", FAN_PWM_FREQUENCY);
    ESP_LOGI(TAG, "  PWM Resolution: %d bits", FAN_PWM_RESOLUTION);

    // 配置 LEDC 定时器
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_LOW_SPEED_MODE,
        .timer_num        = LEDC_TIMER_0,
        .duty_resolution  = FAN_PWM_RESOLUTION,  // 8位分辨率
        .freq_hz          = FAN_PWM_FREQUENCY,   // 25kHz
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    // 配置 LEDC 通道
    ledc_channel_config_t ledc_channel = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel    = FAN_PWM_CHANNEL,
        .gpio_num   = FAN_GPIO_PIN,
        .timer_sel  = LEDC_TIMER_0,
        .duty       = 0,  // 初始占空比为0
        .hpoint     = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

    // 关闭 LEDC 通道的 fade 功能（如果启用）
    ESP_ERROR_CHECK(ledc_fade_func_install(0));

    // 初始化为关闭状态（设置低电平）
    fan_driver_off();

    ESP_LOGI(TAG, "===========================================");
    ESP_LOGI(TAG, "Fan Driver initialized successfully!");
}

/**
 * @brief 设置风扇速度
 * @param speed 速度值 0-100
 */
void fan_driver_set_speed(uint8_t speed)
{
    if (speed > 100) {
        speed = 100;
    }

    s_fan_speed = speed;

    // 将 0-100 映射到 0-255 (8位 PWM)
    uint32_t duty = (speed * 255) / 100;

    // 设置占空比
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, FAN_PWM_CHANNEL, duty));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, FAN_PWM_CHANNEL));

    ESP_LOGI(TAG, "Fan speed set to %d%% (duty=%lu)", speed, duty);
}

/**
 * @brief 设置最大速度（用于 GPIO 驱动能力不足时尝试增强）
 * @note 注意：GPIO3 驱动能力有限，此函数效果有限
 */
void fan_driver_set_max(void)
{
    s_fan_speed = 100;
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, FAN_PWM_CHANNEL, 255));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, FAN_PWM_CHANNEL));
    ESP_LOGI(TAG, "Fan set to MAX (duty=255)");
}

/**
 * @brief 开启风扇
 * @param speed 速度值 0-100
 */
void fan_driver_on(uint8_t speed)
{
    s_fan_enabled = true;
    fan_driver_set_speed(speed);
    ESP_LOGI(TAG, "Fan turned ON at speed %d%%", speed);
}

/**
 * @brief 关闭风扇
 */
void fan_driver_off(void)
{
    s_fan_enabled = false;
    s_fan_speed = 0;

    // 设置占空比为0
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, FAN_PWM_CHANNEL, 0));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, FAN_PWM_CHANNEL));

    ESP_LOGI(TAG, "Fan turned OFF");
}

/**
 * @brief 获取当前风扇状态
 * @return true=开启, false=关闭
 */
bool fan_driver_is_on(void)
{
    return s_fan_enabled;
}

/**
 * @brief 获取当前风扇速度
 * @return 速度值 0-100
 */
uint8_t fan_driver_get_speed(void)
{
    return s_fan_speed;
}
