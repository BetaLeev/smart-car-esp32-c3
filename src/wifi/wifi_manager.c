/**
 * @file wifi_manager.c
 * @brief Wi-Fi 连接管理实现
 */

#include "wifi_manager.h"
#include <string.h>
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"

static const char *TAG = "WIFI";

static wifi_state_t s_wifi_state = WIFI_STATE_IDLE;
static char s_ip[16] = "0.0.0.0";
static char s_ssid[32] = {0};
static int s_rssi = 0;

/**
 * @brief Wi-Fi 事件处理
 */
static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                                int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, "Wi-Fi started, connecting to AP...");
        s_wifi_state = WIFI_STATE_CONNECTING;
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED) {
        wifi_event_sta_connected_t *event = (wifi_event_sta_connected_t *)event_data;
        ESP_LOGI(TAG, "Connected to SSID: %s", event->ssid);
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        wifi_event_sta_disconnected_t *event = (wifi_event_sta_disconnected_t *)event_data;
        ESP_LOGW(TAG, "Disconnected from SSID: %s, reason: %d",
                 event->ssid, event->reason);
        s_wifi_state = WIFI_STATE_DISCONNECTED;
        memset(s_ip, 0, sizeof(s_ip));
        strcpy(s_ip, "0.0.0.0");
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        esp_ip4addr_ntoa(&event->ip_info.ip, s_ip, 16);
        ESP_LOGI(TAG, "Got IP: %s", s_ip);
        s_wifi_state = WIFI_STATE_CONNECTED;
    }
}

/**
 * @brief 初始化 Wi-Fi 管理器
 */
void wifi_manager_init(void)
{
    ESP_LOGI(TAG, "Initializing Wi-Fi...");

    // 初始化网络接口
    ESP_ERROR_CHECK(esp_netif_init());

    // 创建默认事件循环
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // 注册 Wi-Fi 事件
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));

    // 创建 Wi-Fi STA 接口
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif != NULL);

    // 初始化 Wi-Fi 配置
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    s_wifi_state = WIFI_STATE_IDLE;
    ESP_LOGI(TAG, "Wi-Fi manager initialized");
}

/**
 * @brief 连接 Wi-Fi 网络
 */
esp_err_t wifi_connect(const char *ssid, const char *password)
{
    if (ssid == NULL || strlen(ssid) == 0) {
        ESP_LOGE(TAG, "SSID cannot be empty");
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Connecting to SSID: %s", ssid);

    strncpy(s_ssid, ssid, sizeof(s_ssid) - 1);

    wifi_config_t wifi_config;
    memset(&wifi_config, 0, sizeof(wifi_config_t));

    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    strncpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid) - 1);
    if (password != NULL) {
        strncpy((char *)wifi_config.sta.password, password, sizeof(wifi_config.sta.password) - 1);
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    s_wifi_state = WIFI_STATE_CONNECTING;
    return ESP_OK;
}

/**
 * @brief 断开 Wi-Fi 连接
 */
void wifi_disconnect(void)
{
    ESP_LOGI(TAG, "Disconnecting Wi-Fi...");
    esp_wifi_disconnect();
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
    if (s_wifi_state != WIFI_STATE_CONNECTED) {
        return 0;
    }

    wifi_ap_record_t ap_info;
    if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
        s_rssi = ap_info.rssi;
    }
    return s_rssi;
}
