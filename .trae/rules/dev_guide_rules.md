# ESP32-C3 智能小车项目 - 新功能开发指南

## 目的

在添加任何新功能之前，开发者或 AI 助手必须遵循本指南的要求，确保代码质量稳定、功能可靠。

---

## 一、必须遵循的硬性规则

### 1.1 缓冲区大小规则

| 规则 | 说明 | 违反后果 |
|------|------|----------|
| **snprintf 缓冲区** | 必须确保缓冲区足够容纳完整路径/字符串 | 编译错误 (`-Werror=format-truncation`) |
| **HTTP URI 缓冲区** | `req->uri` 最大 512 字节，路径缓冲区需 >= 576 字节 | 编译错误 |
| **IP地址缓冲区** | 至少 16 字节 (`xxx.xxx.xxx.xxx\0`) | 缓冲区溢出 |
| **SSID 缓冲区** | Wi-Fi SSID 最大 32 字节 | 截断丢失 |
| **密码缓冲区** | Wi-Fi 密码最大 64 字节 | 截断丢失 |

```c
// 错误示例：缓冲区太小
char file_path[64];
snprintf(file_path, sizeof(file_path), "/data/web%s", req->uri);  // 编译错误！

// 正确示例
char file_path[576];  // 10 (prefix) + 512 (max uri) + 54 (buffer)
snprintf(file_path, sizeof(file_path), "/data/web%s", req->uri);
```

### 1.2 内存分配与释放规则

| 规则 | 示例 |
|------|------|
| **malloc 必须配对 free** | `buf = malloc()` → `free(buf)` |
| **ESP-IDF netif 创建必须销毁** | `esp_netif_create_*` → `esp_netif_destroy` |
| **互斥锁创建必须删除** | `xSemaphoreCreateMutex` → `vSemaphoreDelete` |
| **事件组创建必须删除** | `xEventGroupCreate` → `vEventGroupDelete` |
| **cJSON_Print 结果必须 free** | `cJSON_Print()` → `free((void*)str)` |

```c
// 错误示例：资源泄漏
esp_netif_t *netif = esp_netif_create_default_wifi_ap();
if (error) {
    return;  // 泄漏！netif 未销毁
}

// 正确示例
esp_netif_t *netif = esp_netif_create_default_wifi_ap();
if (error) {
    esp_netif_destroy(netif);  // 清理
    return;
}
```

### 1.3 互斥锁使用规则

| 规则 | 说明 | 违反后果 |
|------|------|----------|
| **禁止 portMAX_DELAY** | 事件处理、HTTP 处理器中禁止 | 看门狗复位 / 系统死锁 |
| **使用限时等待** | `pdMS_TO_TICKS(100)` 最多 100ms | 确保任务响应 |

```c
// 错误示例：事件循环中无限等待
void wifi_event_handler(...) {
    xSemaphoreTake(s_mutex, portMAX_DELAY);  // 致命！会卡死事件循环
}

// 正确示例
void wifi_event_handler(...) {
    if (xSemaphoreTake(s_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        // 操作
        xSemaphoreGive(s_mutex);
    }
}
```

---

## 二、ESP32-C3 硬件限制规则

### 2.1 内存限制

| 限制项 | 数值 | 注意事项 |
|--------|------|----------|
| SRAM | ~320KB | 无 PSRAM，全部在内部 SRAM |
| 栈空间 | 建议 <= 2KB | 大缓冲区使用 heap_caps_malloc |
| 堆碎片 | 需警惕累积泄漏 | 避免频繁 malloc/free |

### 2.2 栈大小配置

| 任务类型 | 最小栈大小 | 备注 |
|----------|------------|------|
| 简单 GPIO 任务 | 2048 | LED 闪烁等 |
| Wi-Fi 事件任务 | 4096 | 含字符串处理 |
| HTTP 服务器任务 | 8192+ | cJSON 解析需要 |
| 主任务 | 4096 | 一般业务逻辑 |

```c
// 创建任务时指定合理栈大小
xTaskCreate(task_func, "task_name", 8192, NULL, 5, &task_handle);
```

---

## 三、代码规范规则

### 3.1 编译错误处理

| 错误类型 | 原因 | 解决方案 |
|----------|------|----------|
| `undeclared (first use in this function)` | 使用了未包含的头文件中的宏/函数 | 添加对应的 `#include` |
| `implicit declaration of function` | 缺少函数声明 | 添加缺失的头文件 |
| `-Wformat-truncation` | 缓冲区太小 | **必须**扩大缓冲区 |

### 3.2 头文件包含规则

使用其他模块定义的宏、函数、结构体时，**必须包含对应的头文件**：

| 使用的定义 | 需要包含的头文件 |
|------------|------------------|
| `CONFIG_*` 宏 | 对应的 `config/*.h` |
| `wifi_*` 函数 | `wifi/wifi_manager.h` |
| `motor_*` 函数 | `motor_driver.h` |
| `gpio_*` 函数 | ESP-IDF `driver/gpio.h` |
| `ESP_LOGI`/`ESP_LOGE` | ESP-IDF `esp_log.h` |
| `vTaskDelay`/`pdMS_TO_TICKS` | ESP-IDF `freertos/FreeRTOS.h` |

```c
// 错误示例：使用了 CONFIG_AP_SSID 但未包含头文件
#include "config/app_config.h"  // 只有 app_config.h，没有 wifi_config.h

ret = wifi_connect(CONFIG_AP_SSID, CONFIG_AP_PASSWORD);  // 编译错误！

// 正确示例：包含所有需要的头文件
#include "config/app_config.h"
#include "config/wifi_config.h"  // 包含 CONFIG_AP_SSID 定义
#include "wifi/wifi_manager.h"   // 包含 wifi_connect 函数声明

ret = wifi_connect(CONFIG_AP_SSID, CONFIG_AP_PASSWORD);  // ✅ 编译通过
```

**检查清单**：新增代码时，确认所有使用的宏/函数都有对应的 `#include`。

### 3.3 禁止的写法

```c
// 禁止：在 ISR 或事件回调中使用 portMAX_DELAY
xSemaphoreTake(mutex, portMAX_DELAY);  // 事件循环中绝对禁止

// 禁止：大数组放栈上
void task_func(void *p) {
    char buffer[4096];  // 可能导致栈溢出
}

// 禁止：递归调用无深度限制
int recursive_func(int n) {
    return recursive_func(n + 1);  // 无 base case，栈溢出
}
```

---

## 四、新功能开发检查清单

添加任何新功能前，必须确认以下所有项：

- [ ] **缓冲区大小**：所有 `snprintf`、`strncpy` 的目标缓冲区是否足够大？
- [ ] **内存配对**：是否有 malloc 配对 free？ESP-IDF 创建配对销毁？
- [ ] **互斥锁安全**：所有 `xSemaphoreTake` 是否使用限时等待（非 portMAX_DELAY）？
- [ ] **栈空间**：是否有 > 2KB 的局部变量？是否考虑 heap_caps_malloc？
- [ ] **编译测试**：修改后是否执行 `platformio run` 验证无错误无警告？
- [ ] **功能测试**：新功能是否经过实际测试？

---

## 五、常见错误快速参考

| 错误信息 | 原因 | 解决方案 |
|----------|------|----------|
| `format-truncation` | 缓冲区太小 | 扩大缓冲区到足够大小 |
| 运行时复位 | 互斥锁死锁 / 栈溢出 | 检查 portMAX_DELAY / 减少栈使用 |
| 内存不足 | 内存泄漏累积 | 检查所有 malloc/free 配对 |
| 编译警告 | 未使用函数/变量 | 添加 `__attribute__((unused))` 或删除 |

---

## 六、使用方法

在请求 AI 添加新功能时，可以附加以下提示：

```
请在添加 [功能名称] 时，遵循以下要求：
1. 确保所有缓冲区大小足够（参考 dev_guide_rules.md）
2. 确保所有资源有对应的释放代码
3. 互斥锁使用 pdMS_TO_TICKS(100) 而非 portMAX_DELAY
4. 大数组使用 heap_caps_malloc 分配到堆
5. 完成后执行 platformio run 验证
```

