/**
 * @file wifi_manager.h
 * @brief Wi-Fi 连接管理头文件
 */

#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <stdbool.h>
#include "esp_err.h"

/**
 * @brief Wi-Fi 连接状态
 */
typedef enum {
    WIFI_STATE_IDLE = 0,
    WIFI_STATE_CONNECTING,
    WIFI_STATE_CONNECTED,
    WIFI_STATE_DISCONNECTED,
    WIFI_STATE_FAILED
} wifi_state_t;

/**
 * @brief Wi-Fi 连接配置参数
 */
typedef struct {
    char ssid[32];
    char password[64];
    bool auto_reconnect;
} wifi_conn_config_t;

/**
 * @brief 初始化 Wi-Fi 管理器
 * @return ESP_OK 成功, 其他 失败
 */
esp_err_t wifi_manager_init(void);

/**
 * @brief 连接 Wi-Fi 网络
 * @param ssid SSID
 * @param password 密码
 * @return ESP_OK 成功, 其他 失败
 */
esp_err_t wifi_connect(const char *ssid, const char *password);

/**
 * @brief 断开 Wi-Fi 连接
 */
void wifi_disconnect(void);

/**
 * @brief 获取当前 Wi-Fi 状态
 * @return wifi_state_t 当前状态
 */
wifi_state_t wifi_get_state(void);

/**
 * @brief 检查是否已连接
 * @return true 已连接, false 未连接
 */
bool wifi_is_connected(void);

/**
 * @brief 获取本地 IP 地址
 * @return IP 地址字符串
 */
const char* wifi_get_ip(void);

/**
 * @brief 获取当前 SSID
 * @return SSID 字符串
 */
const char* wifi_get_ssid(void);

/**
 * @brief 获取信号强度
 * @return RSSI 值
 */
int wifi_get_rssi(void);

#endif // WIFI_MANAGER_H
