/**
 * @file motor_driver.c
 * @brief DRV8833 电机驱动实现 (使用 LEDC PWM)
 *
 * 功能:
 *   - 水泵 AO (左边桥): 4档 (OFF/LOW/MED/HIGH) + 自动定时 (40s开/20s停)
 *   - 氧气泵 BO (右边桥): 4档 (OFF/LOW/MED/HIGH) + 自动定时 (40s开/20s停)
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "driver/ledc.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_err.h"
#include "config/pin_config.h"
#include "motor_config.h"
#include "motor_driver.h"

static const char *TAG = "MOTOR_DRIVER";

// 水泵档位 (0=停止, 1=低速, 2=中速, 3=高速)
static int s_water_pump_level = 0;

// 氧气泵档位 (0=停止, 1=低速, 2=中速, 3=高速)
static int s_oxygen_pump_level = 0;

// LEDC 通道
#define WATER_PUMP_LEDC_CH     LEDC_CHANNEL_0
#define OXYGEN_PUMP_LEDC_CH    LEDC_CHANNEL_1
#define LEDC_TIMER             LEDC_TIMER_0

// 定时周期 (毫秒)
#define PUMP_TIMER_RUN_MS      40000    // 运行40秒
#define PUMP_TIMER_REST_MS     20000    // 休息20秒

// 水泵定时器
static esp_timer_handle_t s_water_timer = NULL;

// 氧气泵定时器
static esp_timer_handle_t s_oxygen_timer = NULL;

// 水泵定时状态 (true=运行中, false=休息中)
static volatile bool s_water_timer_running = false;

// 氧气泵定时状态 (true=运行中, false=休息中)
static volatile bool s_oxygen_timer_running = false;

// 水泵定时模式开关
static volatile bool s_water_timer_enabled = true;

// 氧气泵定时模式开关
static volatile bool s_oxygen_timer_enabled = true;

// 目标占空比 (用于渐变)
static uint32_t s_water_target_duty = 0;
static uint32_t s_oxygen_target_duty = 0;

// 当前档位占空比 (8位分辨率 0-255)
static const uint32_t level_to_duty[] = {
    0,    // 0档: 停止 0%
    77,   // 1档: 低速 30% (255 * 30% = 76.5)
    128,  // 2档: 中速 50% (255 * 50% = 127.5)
    204,  // 3档: 高速 80% (255 * 80% = 204)
};

/**
 * @brief 设置泵的占空比 (带渐变效果)
 */
static void pump_set_duty_with_fade(ledc_channel_t channel, uint32_t duty)
{
    // 设置目标占空比，500ms渐变时间
    ledc_set_fade_with_time(LEDC_LOW_SPEED_MODE, channel, duty, 500);
    ledc_fade_start(LEDC_LOW_SPEED_MODE, channel, LEDC_FADE_NO_WAIT);
}

/**
 * @brief 直接设置泵的占空比 (无渐变)
 */
static void pump_set_duty_direct(ledc_channel_t channel, uint32_t duty)
{
    ledc_set_duty(LEDC_LOW_SPEED_MODE, channel, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, channel);
}

/**
 * @brief 水泵定时器回调
 */
static void water_timer_callback(void *arg)
{
    ESP_LOGI(TAG, "AO timer callback: enabled=%d, running=%d", s_water_timer_enabled, s_water_timer_running);

    if (!s_water_timer_enabled) {
        ESP_LOGI(TAG, "AO timer: disabled, skipping");
        return;
    }

    if (s_water_timer_running) {
        // 运行中 -> 休息 (设置占空比为0)
        s_water_timer_running = false;
        pump_set_duty_direct(WATER_PUMP_LEDC_CH, 0);
        ESP_LOGI(TAG, "AO (Water pump): REST (20s)");
        esp_timer_start_once(s_water_timer, PUMP_TIMER_REST_MS * 1000);  // 微秒
    } else {
        // 休息中 -> 运行 (恢复到之前档位的占空比)
        s_water_timer_running = true;
        uint32_t duty = level_to_duty[s_water_pump_level];
        pump_set_duty_direct(WATER_PUMP_LEDC_CH, duty);
        ESP_LOGI(TAG, "AO (Water pump): RUN (40s), duty=%lu", duty);
        esp_timer_start_once(s_water_timer, PUMP_TIMER_RUN_MS * 1000);  // 微秒
    }
}

/**
 * @brief 氧气泵定时器回调
 */
static void oxygen_timer_callback(void *arg)
{
    ESP_LOGI(TAG, "BO timer callback: enabled=%d, running=%d", s_oxygen_timer_enabled, s_oxygen_timer_running);

    if (!s_oxygen_timer_enabled) {
        ESP_LOGI(TAG, "BO timer: disabled, skipping");
        return;
    }

    if (s_oxygen_timer_running) {
        // 运行中 -> 休息
        s_oxygen_timer_running = false;
        pump_set_duty_direct(OXYGEN_PUMP_LEDC_CH, 0);
        ESP_LOGI(TAG, "BO (Oxygen pump): REST (20s)");
        esp_timer_start_once(s_oxygen_timer, PUMP_TIMER_REST_MS * 1000);  // 微秒
    } else {
        // 休息中 -> 运行
        s_oxygen_timer_running = true;
        uint32_t duty = level_to_duty[s_oxygen_pump_level];
        pump_set_duty_direct(OXYGEN_PUMP_LEDC_CH, duty);
        ESP_LOGI(TAG, "BO (Oxygen pump): RUN (40s), duty=%lu", duty);
        esp_timer_start_once(s_oxygen_timer, PUMP_TIMER_RUN_MS * 1000);  // 微秒
    }
}

/**
 * @brief 初始化 LEDC 定时器
 */
static void ledc_timer_init(void)
{
    // 配置 LEDC 定时器
    // PWM 频率 20kHz：完全消除电机噪音
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER,
        .clk_cfg = LEDC_AUTO_CLK,
        .freq_hz = 20000,                    // 20kHz PWM 频率（消除鸣叫）
        .duty_resolution = LEDC_TIMER_8_BIT,  // 8位分辨率 (0-255)
    };
    ledc_timer_config(&ledc_timer);
    ESP_LOGI(TAG, "LEDC timer initialized: 20kHz, 8-bit");
}

/**
 * @brief 初始化 LEDC 通道
 */
static void ledc_channel_init(void)
{
    // 配置水泵 LEDC 通道
    ledc_channel_config_t water_channel = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = WATER_PUMP_LEDC_CH,
        .timer_sel = LEDC_TIMER,
        .gpio_num = WATER_PUMP_CTRL_PIN,
        .duty = 0,
        .hpoint = 0,
    };
    ledc_channel_config(&water_channel);

    // 配置氧气泵 LEDC 通道
    ledc_channel_config_t oxygen_channel = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = OXYGEN_PUMP_LEDC_CH,
        .timer_sel = LEDC_TIMER,
        .gpio_num = OXYGEN_PUMP_CTRL_PIN,
        .duty = 0,
        .hpoint = 0,
    };
    ledc_channel_config(&oxygen_channel);

    ESP_LOGI(TAG, "LEDC channels initialized");
}

/**
 * @brief 初始化电机驱动
 */
void motor_driver_init(void)
{
    ESP_LOGI(TAG, "Initializing DRV8833 motor driver with LEDC...");

    // 配置 STBY 引脚
    gpio_reset_pin(PUMP_STBY_PIN);
    gpio_set_direction(PUMP_STBY_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(PUMP_STBY_PIN, 1);
    ESP_LOGI(TAG, "STBY (GPIO %d) = HIGH", PUMP_STBY_PIN);

    // 配置方向引脚 (固定低电平 = 正转)
    gpio_reset_pin(WATER_PUMP_IN2_PIN);
    gpio_set_direction(WATER_PUMP_IN2_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(WATER_PUMP_IN2_PIN, 0);

    gpio_reset_pin(OXYGEN_PUMP_IN2_PIN);
    gpio_set_direction(OXYGEN_PUMP_IN2_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(OXYGEN_PUMP_IN2_PIN, 0);

    // 初始化 LEDC
    ledc_timer_init();
    ledc_channel_init();

    // 安装渐变功能
    ledc_fade_func_install(0);

    // 创建水泵定时器
    esp_timer_create_args_t water_timer_args = {
        .callback = water_timer_callback,
        .arg = NULL,
        .name = "WaterTimer",
        .dispatch_method = ESP_TIMER_TASK
    };
    esp_err_t ret = esp_timer_create(&water_timer_args, &s_water_timer);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create water timer: %s", esp_err_to_name(ret));
    }

    // 创建氧气泵定时器
    esp_timer_create_args_t oxygen_timer_args = {
        .callback = oxygen_timer_callback,
        .arg = NULL,
        .name = "OxygenTimer",
        .dispatch_method = ESP_TIMER_TASK
    };
    ret = esp_timer_create(&oxygen_timer_args, &s_oxygen_timer);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create oxygen timer: %s", esp_err_to_name(ret));
    }

    // 自动启动两个泵的定时器 (5秒开/2秒停)
    s_water_pump_level = 2;  // 默认中速档
    s_oxygen_pump_level = 2; // 默认中速档

    s_water_timer_running = true;
    pump_set_duty_direct(WATER_PUMP_LEDC_CH, level_to_duty[2]);
    esp_timer_start_once(s_water_timer, PUMP_TIMER_RUN_MS * 1000);  // 微秒
    ESP_LOGI(TAG, "AO (Water pump) timer: AUTO START (5s ON / 2s OFF)");

    s_oxygen_timer_running = true;
    pump_set_duty_direct(OXYGEN_PUMP_LEDC_CH, level_to_duty[2]);
    esp_timer_start_once(s_oxygen_timer, PUMP_TIMER_RUN_MS * 1000);  // 微秒
    ESP_LOGI(TAG, "BO (Oxygen pump) timer: AUTO START (40s ON / 20s OFF)");

    ESP_LOGI(TAG, "Motor driver initialized");
    ESP_LOGI(TAG, "  AO (Water pump): Level 0=OFF, 1=SLOW(30%%), 2=MED(50%%), 3=HIGH(80%%)");
    ESP_LOGI(TAG, "  BO (Oxygen pump): Level 0=OFF, 1=SLOW(30%%), 2=MED(50%%), 3=HIGH(80%%)");
    ESP_LOGI(TAG, "  GPIO 1: Toggle ALL pumps timer (ON/OFF)");
}

/**
 * @brief 设置水泵档位
 */
void water_pump_set_level(int level)
{
    if (level < 0) level = 0;
    if (level > 3) level = 3;
    s_water_pump_level = level;

    // 如果定时器正在运行，更新目标占空比
    if (s_water_timer_running) {
        pump_set_duty_direct(WATER_PUMP_LEDC_CH, level_to_duty[level]);
    }

    ESP_LOGI(TAG, "AO (Water pump): level=%d, duty=%lu", level, level_to_duty[level]);
}

/**
 * @brief 获取水泵档位
 */
int water_pump_get_level(void)
{
    return s_water_pump_level;
}

/**
 * @brief 设置水泵状态
 */
void water_pump_set_state(bool on)
{
    water_pump_set_level(on ? 2 : 0);
}

/**
 * @brief 获取水泵状态
 */
bool water_pump_is_on(void)
{
    return s_water_pump_level > 0;
}

/**
 * @brief 设置氧气泵档位
 */
void oxygen_pump_set_level(int level)
{
    if (level < 0) level = 0;
    if (level > 3) level = 3;
    s_oxygen_pump_level = level;

    // 如果定时器正在运行，更新目标占空比
    if (s_oxygen_timer_running) {
        pump_set_duty_direct(OXYGEN_PUMP_LEDC_CH, level_to_duty[level]);
    }

    ESP_LOGI(TAG, "BO (Oxygen pump): level=%d, duty=%lu", level, level_to_duty[level]);
}

/**
 * @brief 获取氧气泵档位
 */
int oxygen_pump_get_level(void)
{
    return s_oxygen_pump_level;
}

/**
 * @brief 设置氧气泵状态
 */
void oxygen_pump_set_state(bool on)
{
    oxygen_pump_set_level(on ? 2 : 0);
}

/**
 * @brief 获取氧气泵状态
 */
bool oxygen_pump_is_on(void)
{
    return s_oxygen_pump_level > 0;
}

/**
 * @brief 停止所有泵
 */
void pump_stop_all(void)
{
    water_pump_set_level(0);
    oxygen_pump_set_level(0);
}

/**
 * @brief 设置水泵定时模式
 */
void water_pump_set_timer(bool enabled)
{
    s_water_timer_enabled = enabled;

    if (enabled) {
        s_water_timer_running = true;
        pump_set_duty_direct(WATER_PUMP_LEDC_CH, level_to_duty[s_water_pump_level]);
        esp_timer_stop(s_water_timer);
        esp_timer_start_once(s_water_timer, PUMP_TIMER_RUN_MS * 1000);  // 微秒
        ESP_LOGI(TAG, "AO timer: ENABLED (5s ON / 2s OFF)");
    } else {
        esp_timer_stop(s_water_timer);
        pump_set_duty_direct(WATER_PUMP_LEDC_CH, 0);
        s_water_timer_running = false;
        ESP_LOGI(TAG, "AO timer: DISABLED");
    }
}

/**
 * @brief 获取水泵定时模式状态
 */
bool water_pump_get_timer(void)
{
    return s_water_timer_enabled;
}

/**
 * @brief 获取水泵定时运行状态
 */
bool water_pump_is_timer_running(void)
{
    return s_water_timer_running;
}

/**
 * @brief 设置氧气泵定时模式
 */
void oxygen_pump_set_timer(bool enabled)
{
    s_oxygen_timer_enabled = enabled;

    if (enabled) {
        s_oxygen_timer_running = true;
        pump_set_duty_direct(OXYGEN_PUMP_LEDC_CH, level_to_duty[s_oxygen_pump_level]);
        esp_timer_stop(s_oxygen_timer);
        esp_timer_start_once(s_oxygen_timer, PUMP_TIMER_RUN_MS * 1000);  // 微秒
        ESP_LOGI(TAG, "BO timer: ENABLED (5s ON / 2s OFF)");
    } else {
        esp_timer_stop(s_oxygen_timer);
        pump_set_duty_direct(OXYGEN_PUMP_LEDC_CH, 0);
        s_oxygen_timer_running = false;
        ESP_LOGI(TAG, "BO timer: DISABLED");
    }
}

/**
 * @brief 获取氧气泵定时模式状态
 */
bool oxygen_pump_get_timer(void)
{
    return s_oxygen_timer_enabled;
}

/**
 * @brief 获取氧气泵定时运行状态
 */
bool oxygen_pump_is_timer_running(void)
{
    return s_oxygen_timer_running;
}

/**
 * @brief 设置所有泵的定时模式 (统一开关)
 */
void all_pumps_set_timer(bool enabled)
{
    water_pump_set_timer(enabled);
    oxygen_pump_set_timer(enabled);
    ESP_LOGI(TAG, "All pumps timer: %s", enabled ? "ENABLED" : "DISABLED");
}

/**
 * @brief 获取所有泵的定时模式状态
 */
bool all_pumps_get_timer(void)
{
    return s_water_timer_enabled && s_oxygen_timer_enabled;
}
