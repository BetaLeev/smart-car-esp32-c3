# ESP32-C3 定时循环功能开发问题总结

## 项目背景

为 ESP32-C3 双泵控制系统实现以下功能：
- GPIO 1 按键：控制 AO 和 BO 两个泵的定时开关
- 通电自动启动：AO 和 BO 同时开始循环（5秒开/2秒停）
- 档位切换：GPIO 20 按键控制水泵档位

---

## 问题列表与解决方案

### 问题 1：定时器回调不触发

**现象**：定时器创建后，回调函数从未被调用。

**根源**：
ESP-IDF v5+ 在创建定时器时，必须显式指定 `dispatch_method`。默认值在某些情况下不生效。

**解决方案**：
```c
esp_timer_create_args_t timer_args = {
    .callback = water_timer_callback,
    .arg = NULL,
    .name = "WaterTimer",
    .dispatch_method = ESP_TIMER_TASK  // 关键：必须添加
};
esp_timer_create(&timer_args, &s_water_timer);
```

---

### 问题 2：定时器触发但时间间隔错误（10ms vs 5秒）

**现象**：回调函数被触发了，但时间间隔是 10ms，而不是预期的 5秒。

**日志表现**：
```
I (39946) MOTOR_DRIVER: BO timer callback: enabled=1, running=0
I (39956) MOTOR_DRIVER: BO timer callback: enabled=1, running=1   // 仅 10ms 间隔！
```

**根源**：
`esp_timer_start_once()` 的时间参数单位是**微秒(μs)**，而宏定义用的是**毫秒(ms)**。

```c
// 错误：传入 5000 微秒 = 5ms（不是 5秒！）
#define PUMP_TIMER_RUN_MS      5000    // 毫秒
esp_timer_start_once(s_water_timer, PUMP_TIMER_RUN_MS);

// 正确：转换为微秒
esp_timer_start_once(s_water_timer, PUMP_TIMER_RUN_MS * 1000);  // 5000000 微秒 = 5秒
```

**解决方案**：
所有 `esp_timer_start_once()` 调用都需要乘以 1000：

```c
// 初始化时
esp_timer_start_once(s_water_timer, PUMP_TIMER_RUN_MS * 1000);

// 回调中
esp_timer_start_once(s_water_timer, PUMP_TIMER_REST_MS * 1000);
esp_timer_start_once(s_water_timer, PUMP_TIMER_RUN_MS * 1000);

// set_timer 函数中
esp_timer_start_once(s_water_timer, PUMP_TIMER_RUN_MS * 1000);
```

---

### 问题 3：档位和定时循环不生效

**现象**：GPIO 1 按键可以切换定时开关（ON/OFF），但泵的档位调节和定时循环不起作用。

**根源**：
原代码使用 `gpio_set_level()` 直接控制 GPIO 引脚为高低电平，这不是真正的 PWM 控制。

```c
// 原代码（错误）：只有 ON/OFF，没有速度控制
gpio_set_level(OXYGEN_PUMP_CTRL_PIN, 1);  // 开启
gpio_set_level(OXYGEN_PUMP_CTRL_PIN, 0);  // 关闭
```

**解决方案**：
使用 ESP-IDF LEDC API 实现真正的 PWM 控制：

```c
// 1. 包含头文件
#include "driver/ledc.h"

// 2. 配置 LEDC 定时器
ledc_timer_config_t ledc_timer = {
    .speed_mode = LEDC_LOW_SPEED_MODE,
    .timer_num = LEDC_TIMER_0,
    .clk_cfg = LEDC_AUTO_CLK,
    .freq_hz = 1000,                      // 1kHz
    .duty_resolution = LEDC_TIMER_10_BIT,  // 10位分辨率 (0-1024)
};
ledc_timer_config(&ledc_timer);

// 3. 配置 LEDC 通道
ledc_channel_config_t channel = {
    .speed_mode = LEDC_LOW_SPEED_MODE,
    .channel = LEDC_CHANNEL_0,
    .timer_sel = LEDC_TIMER_0,
    .gpio_num = WATER_PUMP_CTRL_PIN,
    .duty = 0,
};
ledc_channel_config(&channel);

// 4. 设置占空比
ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty_value);
ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
```

**CMakeLists.txt 需要添加依赖**：
```cmake
idf_component_register(
    ...
    REQUIRES esp_timer esp_driver_ledc driver freertos
)
```

---

### 问题 4：只有氧气泵有定时，缺少水泵定时

**现象**：只有 BO（氧气泵）有定时循环，AO（水泵）没有。

**根源**：
初始实现只给氧气泵创建了定时器，没有为水泵创建独立的定时器。

**解决方案**：
为两个泵分别创建独立的定时器：

```c
// 水泵定时器
static esp_timer_handle_t s_water_timer = NULL;
static void water_timer_callback(void *arg);

// 氧气泵定时器
static esp_timer_handle_t s_oxygen_timer = NULL;
static void oxygen_timer_callback(void *arg);

// 初始化时分别创建
esp_timer_create_args_t water_args = {
    .callback = water_timer_callback,
    .dispatch_method = ESP_TIMER_TASK,
    ...
};
esp_timer_create(&water_args, &s_water_timer);

esp_timer_create_args_t oxygen_args = {
    .callback = oxygen_timer_callback,
    .dispatch_method = ESP_TIMER_TASK,
    ...
};
esp_timer_create(&oxygen_args, &s_oxygen_timer);
```

---

## 最终实现的功能

### 档位定义（10位 PWM 分辨率，0-1024）
| 档位 | 占空比 | PWM 值 | 说明 |
|------|--------|--------|------|
| 0 | 0% | 0 | 停止 |
| 1 | 30% | 307 | 低速 |
| 2 | 50% | 512 | 中速 |
| 3 | 80% | 819 | 高速 |

### 定时循环
| 参数 | 值 | 说明 |
|------|-----|------|
| 运行时间 | 40秒 | 泵开启 |
| 休息时间 | 20秒 | 泵停止 |
| 自动启动 | 是 | 通电即开始 |

### 按键功能
| 按键 | 功能 |
|------|------|
| GPIO 20 | 水泵档位切换（0→1→2→3→0） |
| GPIO 1 | 所有泵定时开关（ON/OFF） |

---

## 关键代码片段

### 定时器回调（带状态切换）
```c
static void water_timer_callback(void *arg)
{
    if (!s_water_timer_enabled) {
        return;
    }

    if (s_water_timer_running) {
        // 运行中 -> 休息
        s_water_timer_running = false;
        pump_set_duty_direct(WATER_PUMP_LEDC_CH, 0);  // 停止
        esp_timer_start_once(s_water_timer, PUMP_TIMER_REST_MS * 1000);  // 微秒！
    } else {
        // 休息中 -> 运行
        s_water_timer_running = true;
        uint32_t duty = level_to_duty[s_water_pump_level];
        pump_set_duty_direct(WATER_PUMP_LEDC_CH, duty);  // 按档位运行
        esp_timer_start_once(s_water_timer, PUMP_TIMER_RUN_MS * 1000);  // 微秒！
    }
}
```

### LEDC 占空比设置
```c
static void pump_set_duty_direct(ledc_channel_t channel, uint32_t duty)
{
    ledc_set_duty(LEDC_LOW_SPEED_MODE, channel, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, channel);
}
```

---

## ESP-IDF 定时器注意事项

1. **时间单位**：所有 `esp_timer_*` 函数的延迟参数都是**微秒(μs)**
2. **dispatch_method**：ESP-IDF v5+ 必须显式指定
3. **定时器类型**：
   - `esp_timer_start_once()` - 单次触发
   - `esp_timer_start_periodic()` - 周期性触发

---

## 调试建议

添加日志以便观察定时器行为：

```c
static void water_timer_callback(void *arg)
{
    ESP_LOGI(TAG, "Timer callback: enabled=%d, running=%d",
             s_water_timer_enabled, s_water_timer_running);
    // ...
}
```

正常日志应该显示：
```
I (5000) MOTOR_DRIVER: AO timer callback: enabled=1, running=0
I (5000) MOTOR_DRIVER: AO (Water pump): RUN (40s), duty=512
I (45000) MOTOR_DRIVER: AO timer callback: enabled=1, running=1
I (45000) MOTOR_DRIVER: AO (Water pump): REST (20s)
I (65000) MOTOR_DRIVER: AO timer callback: enabled=1, running=0
I (65000) MOTOR_DRIVER: AO (Water pump): RUN (40s), duty=512
```

---

*文档版本：1.0*
*最后更新：2026-07-21*
