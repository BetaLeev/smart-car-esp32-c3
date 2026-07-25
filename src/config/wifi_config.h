/**
 * @file wifi_config.h
 * @brief Wi-Fi 配置文件
 *
 * 在此文件中配置 Wi-Fi 的工作模式
 */

#ifndef WIFI_CONFIG_H
#define WIFI_CONFIG_H

// ESP-IDF 头文件
#include "esp_wifi.h"

// ============================================
// Wi-Fi 模式配置
// ============================================
/**
 * Wi-Fi 工作模式:
 *   WIFI_MODE_AP       - 仅 AP 模式 (热点)
 *   WIFI_MODE_STA      - 仅 STA 模式 (连接路由器)
 *   WIFI_MODE_APSTA    - 混合模式 (同时作为热点和连接路由器)
 */
#define WIFI_MODE     WIFI_MODE_AP

// ============================================
// AP 模式配置 (热点配置)
// ============================================

// AP 模式开关
#define CONFIG_WIFI_AP_ENABLED    1

// AP 默认配置
#define CONFIG_AP_SSID            "ESP32-CAR"
#define CONFIG_AP_PASSWORD        "12345678"
#define CONFIG_AP_CHANNEL         6
#define CONFIG_AP_MAX_CONNECTIONS  4
#define CONFIG_AP_IP_ADDR         "192.168.4.1"
#define CONFIG_AP_GATEWAY         "192.168.4.1"
#define CONFIG_AP_NETMASK         "255.255.255.0"

// ============================================
// STA 模式配置 (连接路由器)
// ============================================

// STA 模式开关
#define CONFIG_WIFI_STA_ENABLED   0

// STA 连接配置 (可通过网页动态修改)
#define CONFIG_STA_DEFAULT_SSID   ""
#define CONFIG_STA_DEFAULT_PWD    ""

// STA 连接超时 (秒)
#define CONFIG_STA_CONNECT_TIMEOUT_MS  15000

// STA 自动重连
#define CONFIG_STA_AUTO_RECONNECT      1

// ============================================
// 网络配置
// ============================================

// DHCP 开关 (STA 模式)
#define CONFIG_STA_USE_DHCP       1

// 如果禁用 DHCP，在此配置静态 IP
#define CONFIG_STA_STATIC_IP      "192.168.1.100"
#define CONFIG_STA_STATIC_GW       "192.168.1.1"
#define CONFIG_STA_STATIC_NETMASK "255.255.255.0"
#define CONFIG_STA_DNS_PRIMARY    "8.8.8.8"
#define CONFIG_STA_DNS_SECONDARY  "8.8.4.4"

#endif // WIFI_CONFIG_H
