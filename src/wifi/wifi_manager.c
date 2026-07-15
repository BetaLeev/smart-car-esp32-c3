/**
 * @file wifi_manager.c
 * @brief Wi-Fi AP 模式实现
 */

#include "wifi_manager.h"
#include <string.h>
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_netif_types.h"
#include "esp_netif_ip_addr.h"
#include "lwip/ip_addr.h"
#include "nvs_flash.h"
#include "esp_mac.h"
#include "esp_err.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

static const char *TAG = "WIFI";

static wifi_state_t s_wifi_state = WIFI_STATE_IDLE;
static char s_ip[16] = "192.168.4.1";
static char s_ssid[32] = {0};
static int s_rssi = 0;
static esp_netif_t *s_ap_netif = NULL;
static EventGroupHandle_t s_wifi_event_group = NULL;

#define WIFI_CONNECTED_BIT BIT0

/**
 * @brief Wi-Fi 事件处理
 */
static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                                int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_EVENT_AP_STACONNECTED:
                ESP_LOGI(TAG, "Station connected to AP!");
                s_wifi_state = WIFI_STATE_CONNECTED;
                break;
            case WIFI_EVENT_AP_STADISCONNECTED:
                ESP_LOGI(TAG, "Station disconnected from AP");
                break;
            case WIFI_EVENT_AP_START:
                ESP_LOGI(TAG, "AP started successfully");
                if (s_wifi_event_group) {
                    xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
                }
                break;
            case WIFI_EVENT_AP_STOP:
                ESP_LOGI(TAG, "AP stopped");
                break;
            case WIFI_EVENT_STA_START:
                ESP_LOGI(TAG, "STA started");
                break;
            case WIFI_EVENT_STA_CONNECTED:
                ESP_LOGI(TAG, "STA connected");
                break;
            case WIFI_EVENT_STA_DISCONNECTED:
                ESP_LOGI(TAG, "STA disconnected");
                break;
            default:
                ESP_LOGD(TAG, "Wi-Fi event: %d", event_id);
                break;
        }
    } else if (event_base == IP_EVENT) {
        switch (event_id) {
            case IP_EVENT_ASSIGNED_IP_TO_CLIENT:
                ESP_LOGI(TAG, "AP assigned IP to station");
                break;
            default:
                ESP_LOGD(TAG, "IP event: %d", event_id);
                break;
        }
    }
}

/**
 * @brief 初始化 Wi-Fi 管理器 (AP模式)
 */
esp_err_t wifi_manager_init(void)
{
    ESP_LOGI(TAG, "Initializing Wi-Fi...");

    // 初始化 NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS flash issue, erasing...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "NVS init failed: %s", esp_err_to_name(ret));
        return ret;
    }
    ESP_LOGI(TAG, "NVS initialized");

    // 初始化网络接口
    ret = esp_netif_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Netif init failed: %s", esp_err_to_name(ret));
        return ret;
    }
    ESP_LOGI(TAG, "Netif initialized");

    // 创建默认事件循环
    ret = esp_event_loop_create_default();
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "Event loop create failed: %s", esp_err_to_name(ret));
        return ret;
    }
    if (ret == ESP_ERR_INVALID_STATE) {
        ESP_LOGI(TAG, "Event loop already exists");
    } else {
        ESP_LOGI(TAG, "Event loop created");
    }

    // 注册 Wi-Fi 事件
    ret = esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Wi-Fi event handler register failed: %s", esp_err_to_name(ret));
        return ret;
    }
    ESP_LOGI(TAG, "Wi-Fi event handler registered");

    // 注册 IP 事件
    ret = esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "IP event handler register failed: %s", esp_err_to_name(ret));
        return ret;
    }
    ESP_LOGI(TAG, "IP event handler registered");

    // 初始化 Wi-Fi
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ret = esp_wifi_init(&cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Wi-Fi init failed: %s", esp_err_to_name(ret));
        return ret;
    }
    ESP_LOGI(TAG, "Wi-Fi stack initialized");

    s_wifi_state = WIFI_STATE_IDLE;
    s_wifi_event_group = xEventGroupCreate();
    ESP_LOGI(TAG, "Wi-Fi manager ready");

    return ESP_OK;
}

/**
 * @brief 启动 AP 模式
 * @param ssid SSID 名称
 * @param password 密码 (至少8位，不设置密码则填 NULL 或空字符串)
 */
esp_err_t wifi_connect(const char *ssid, const char *password)
{
    if (ssid == NULL || strlen(ssid) == 0) {
        ESP_LOGE(TAG, "SSID cannot be empty");
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t ret;
    ESP_LOGI(TAG, "Starting AP mode...");
    ESP_LOGI(TAG, "SSID: %s", ssid);
    strncpy(s_ssid, ssid, sizeof(s_ssid) - 1);
    s_ssid[sizeof(s_ssid) - 1] = '\0';

    // 创建 Wi-Fi AP 接口
    s_ap_netif = esp_netif_create_default_wifi_ap();
    if (s_ap_netif == NULL) {
        ESP_LOGE(TAG, "Failed to create AP netif");
        return ESP_FAIL;
    }

    // 配置静态 IP
    esp_netif_ip_info_t ip_info;
    IP4_ADDR(&ip_info.ip, 192, 168, 4, 1);
    IP4_ADDR(&ip_info.gw, 192, 168, 4, 1);
    IP4_ADDR(&ip_info.netmask, 255, 255, 255, 0);
    ret = esp_netif_set_ip_info(s_ap_netif, &ip_info);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to set IP info: %s", esp_err_to_name(ret));
    }

    wifi_config_t wifi_config;
    memset(&wifi_config, 0, sizeof(wifi_config));

    strncpy((char *)wifi_config.ap.ssid, ssid, sizeof(wifi_config.ap.ssid) - 1);
    wifi_config.ap.ssid[sizeof(wifi_config.ap.ssid) - 1] = '\0';
    wifi_config.ap.ssid_len = strlen(ssid);
    wifi_config.ap.max_connection = 4;
    wifi_config.ap.channel = 1;  // 使用 channel 1 避免干扰

    // 设置认证模式
    if (password != NULL && strlen(password) >= 8) {
        strncpy((char *)wifi_config.ap.password, password, sizeof(wifi_config.ap.password) - 1);
        wifi_config.ap.password[sizeof(wifi_config.ap.password) - 1] = '\0';
        wifi_config.ap.authmode = WIFI_AUTH_WPA2_PSK;
        ESP_LOGI(TAG, "Auth mode: WPA2-PSK");
    } else {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
        ESP_LOGI(TAG, "Auth mode: Open (no password)");
    }

    // 设置 Wi-Fi 模式
    ret = esp_wifi_set_mode(WIFI_MODE_AP);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set Wi-Fi mode: %s", esp_err_to_name(ret));
        return ret;
    }

    // 设置 AP 配置
    ret = esp_wifi_set_config(WIFI_IF_AP, &wifi_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set AP config: %s", esp_err_to_name(ret));
        return ret;
    }

    // 启动 Wi-Fi
    ret = esp_wifi_start();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start Wi-Fi: %s", esp_err_to_name(ret));
        return ret;
    }

    // 等待 AP 真正启动（WIFI_EVENT_AP_START 事件）
    ESP_LOGI(TAG, "Waiting for AP to start...");
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                            WIFI_CONNECTED_BIT,
                                            pdFALSE,
                                            pdFALSE,
                                            pdMS_TO_TICKS(10000));
    if (!(bits & WIFI_CONNECTED_BIT)) {
        ESP_LOGE(TAG, "AP start timeout after 10 seconds!");
        return ESP_FAIL;
    }

    s_wifi_state = WIFI_STATE_CONNECTED;
    strncpy(s_ip, "192.168.4.1", sizeof(s_ip) - 1);

    ESP_LOGI(TAG, "===========================================");
    ESP_LOGI(TAG, "AP started successfully!");
    ESP_LOGI(TAG, "SSID: %s", ssid);
    ESP_LOGI(TAG, "IP: 192.168.4.1");
    ESP_LOGI(TAG, "Connect to this network to access the web interface");
    ESP_LOGI(TAG, "===========================================");

    return ESP_OK;
}

/**
 * @brief 断开连接 (AP模式下停止)
 */
void wifi_disconnect(void)
{
    esp_wifi_stop();
    s_wifi_state = WIFI_STATE_IDLE;
}

/**
 * @brief 获取当前 Wi-Fi 状态
 */
wifi_state_t wifi_get_state(void)
{
    return s_wifi_state;
}

/**
 * @brief 检查是否已连接
 */
bool wifi_is_connected(void)
{
    return s_wifi_state == WIFI_STATE_CONNECTED;
}

/**
 * @brief 获取本地 IP 地址
 */
const char* wifi_get_ip(void)
{
    return s_ip;
}

/**
 * @brief 获取当前 SSID
 */
const char* wifi_get_ssid(void)
{
    return s_ssid;
}

/**
 * @brief 获取信号强度
 */
int wifi_get_rssi(void)
{
    return s_rssi;
}
