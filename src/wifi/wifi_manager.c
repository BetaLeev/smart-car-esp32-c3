/**
 * @file wifi_manager.c
 * @brief Wi-Fi 连接管理实现
 *
 * 支持 AP 模式、STA 模式和混合模式
 */

#include "wifi_manager.h"
#include "config/wifi_config.h"
#include <string.h>
#include <stdio.h>
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
static char s_ip[16] = "0.0.0.0";
static char s_ssid[32] = {0};
static int s_rssi = 0;
static esp_netif_t *s_ap_netif = NULL;
static esp_netif_t *s_sta_netif = NULL;
static EventGroupHandle_t s_wifi_event_group = NULL;
static SemaphoreHandle_t s_wifi_mutex = NULL;  // Wi-Fi 状态保护互斥锁

// STA 模式存储
static char s_sta_ssid[32] = {0};
static char s_sta_password[64] = {0};

#define WIFI_CONNECTED_BIT    BIT0
#define STA_CONNECTED_BIT     BIT1

// Wi-Fi 配置值解析宏
#define WIFI_CONFIG_IP_TO_INT(a,b,c,d) (((a) << 24) | ((b) << 16) | ((c) << 8) | (d))
#define WIFI_CONFIG_IP_FROM_INT(val, a, b, c, d) \
    do { (a) = (uint8_t)((val) >> 24); (b) = (uint8_t)((val) >> 16); \
         (c) = (uint8_t)((val) >> 8); (d) = (uint8_t)(val); } while(0)

/**
 * @brief Wi-Fi 事件处理
 */
static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                                int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_EVENT_AP_STACONNECTED:
                ESP_LOGI(TAG, "Station connected to AP");
                if (s_wifi_mutex && xSemaphoreTake(s_wifi_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                    s_wifi_state = WIFI_STATE_CONNECTED;
                    xSemaphoreGive(s_wifi_mutex);
                }
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
                ESP_LOGI(TAG, "STA connected to router");
                if (s_wifi_event_group) {
                    xEventGroupSetBits(s_wifi_event_group, STA_CONNECTED_BIT);
                }
                break;
            case WIFI_EVENT_STA_DISCONNECTED:
                ESP_LOGI(TAG, "STA disconnected");
                if (s_wifi_mutex && xSemaphoreTake(s_wifi_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                    s_wifi_state = WIFI_STATE_DISCONNECTED;
                    xSemaphoreGive(s_wifi_mutex);
                }
                xEventGroupClearBits(s_wifi_event_group, STA_CONNECTED_BIT);
#if CONFIG_STA_AUTO_RECONNECT
                esp_wifi_connect();
#endif
                break;
            default:
                ESP_LOGD(TAG, "Wi-Fi event: %d", event_id);
                break;
        }
    } else if (event_base == IP_EVENT) {
        switch (event_id) {
            case IP_EVENT_STA_GOT_IP: {
                ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
                ESP_LOGI(TAG, "STA got IP: " IPSTR, IP2STR(&event->ip_info.ip));
                if (s_wifi_mutex && xSemaphoreTake(s_wifi_mutex, portMAX_DELAY) == pdTRUE) {
                    snprintf(s_ip, sizeof(s_ip), IPSTR, IP2STR(&event->ip_info.ip));
                    s_wifi_state = WIFI_STATE_CONNECTED;
                    xSemaphoreGive(s_wifi_mutex);
                }
                break;
            }
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
 * @brief 配置 AP 模式
 */
static esp_err_t start_ap_mode(const char *ssid, const char *password)
{
    esp_err_t ret;

    ESP_LOGI(TAG, "Starting AP mode...");
    ESP_LOGI(TAG, "SSID: %s", ssid);

    // 创建 Wi-Fi AP 接口
    s_ap_netif = esp_netif_create_default_wifi_ap();
    if (s_ap_netif == NULL) {
        ESP_LOGE(TAG, "Failed to create AP netif");
        return ESP_FAIL;
    }
    ESP_LOGD(TAG, "AP netif created: %p", s_ap_netif);

    // 配置静态 IP
    esp_netif_ip_info_t ip_info;
    IP4_ADDR(&ip_info.ip, 192, 168, 4, 1);
    IP4_ADDR(&ip_info.gw, 192, 168, 4, 1);
    IP4_ADDR(&ip_info.netmask, 255, 255, 255, 0);
    ret = esp_netif_set_ip_info(s_ap_netif, &ip_info);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to set AP IP info: %s", esp_err_to_name(ret));
    }

    wifi_config_t wifi_config;
    memset(&wifi_config, 0, sizeof(wifi_config));

    strncpy((char *)wifi_config.ap.ssid, ssid, sizeof(wifi_config.ap.ssid) - 1);
    wifi_config.ap.ssid[sizeof(wifi_config.ap.ssid) - 1] = '\0';
    wifi_config.ap.ssid_len = strlen(ssid);
    wifi_config.ap.max_connection = CONFIG_AP_MAX_CONNECTIONS;
    wifi_config.ap.channel = CONFIG_AP_CHANNEL;

    // 设置认证模式
    if (password != NULL && strlen(password) >= 8) {
        strncpy((char *)wifi_config.ap.password, password, sizeof(wifi_config.ap.password) - 1);
        wifi_config.ap.password[sizeof(wifi_config.ap.password) - 1] = '\0';
        wifi_config.ap.authmode = WIFI_AUTH_WPA2_PSK;
    } else {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ret = esp_wifi_set_config(WIFI_IF_AP, &wifi_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set AP config: %s", esp_err_to_name(ret));
        esp_netif_destroy(s_ap_netif);
        s_ap_netif = NULL;
        return ret;
    }

    return ESP_OK;
}

/**
 * @brief 配置 STA 模式
 */
static esp_err_t __attribute__((unused)) start_sta_mode(const char *ssid, const char *password)
{
    esp_err_t ret;

    ESP_LOGI(TAG, "Starting STA mode...");
    ESP_LOGI(TAG, "Connecting to SSID: %s", ssid);

    // 保存 SSID 和密码
    strncpy(s_sta_ssid, ssid, sizeof(s_sta_ssid) - 1);
    s_sta_ssid[sizeof(s_sta_ssid) - 1] = '\0';
    if (password) {
        strncpy(s_sta_password, password, sizeof(s_sta_password) - 1);
        s_sta_password[sizeof(s_sta_password) - 1] = '\0';
    } else {
        s_sta_password[0] = '\0';
    }

    // 创建 Wi-Fi STA 接口
    s_sta_netif = esp_netif_create_default_wifi_sta();
    if (s_sta_netif == NULL) {
        ESP_LOGE(TAG, "Failed to create STA netif");
        return ESP_FAIL;
    }

    wifi_config_t wifi_config;
    memset(&wifi_config, 0, sizeof(wifi_config));

    strncpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid) - 1);
    strncpy((char *)wifi_config.sta.password, password ? password : "",
            sizeof(wifi_config.sta.password) - 1);

    if (password && strlen(password) > 0) {
        wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    }

    wifi_config.sta.pmf_cfg.capable = true;
    wifi_config.sta.pmf_cfg.required = false;

    ret = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set STA config: %s", esp_err_to_name(ret));
        esp_netif_destroy(s_sta_netif);
        s_sta_netif = NULL;
        return ret;
    }

    ret = esp_wifi_connect();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start STA connection: %s", esp_err_to_name(ret));
        return ret;
    }

    return ESP_OK;
}

/**
 * @brief 初始化 Wi-Fi 管理器
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

    // 创建默认事件循环
    ret = esp_event_loop_create_default();
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "Event loop create failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // 注册 Wi-Fi 事件
    ret = esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Wi-Fi event handler register failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // 注册 IP 事件
    ret = esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "IP event handler register failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // 初始化 Wi-Fi
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ret = esp_wifi_init(&cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Wi-Fi init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    s_wifi_state = WIFI_STATE_IDLE;
    s_wifi_event_group = xEventGroupCreate();
    s_wifi_mutex = xSemaphoreCreateMutex();  // 创建互斥锁保护状态变量

    ESP_LOGI(TAG, "Wi-Fi manager ready");

    return ESP_OK;
}

/**
 * @brief 连接 Wi-Fi (启动 AP/STA/混合模式)
 *
 * 在混合模式下,ssid 是 AP 的 SSID, password 是 AP 密码(可选)
 * 如果设置了 CONFIG_STA_DEFAULT_SSID,STA 也会尝试连接该网络
 */
esp_err_t wifi_connect(const char *ssid, const char *password)
{
    if (ssid == NULL || strlen(ssid) == 0) {
        ESP_LOGE(TAG, "SSID cannot be empty");
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t ret;
    wifi_mode_t mode = WIFI_MODE;

    strncpy(s_ssid, ssid, sizeof(s_ssid) - 1);
    s_ssid[sizeof(s_ssid) - 1] = '\0';

    // 根据模式配置 Wi-Fi
    switch (mode) {
#if CONFIG_WIFI_AP_ENABLED
        case WIFI_MODE_AP:
        case WIFI_MODE_APSTA:
            // AP 模式总是启用
            break;
#endif
        default:
            break;
    }

    // 设置 Wi-Fi 模式
    ret = esp_wifi_set_mode(mode);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set Wi-Fi mode: %s", esp_err_to_name(ret));
        return ret;
    }

    // 启动 Wi-Fi
    ret = esp_wifi_start();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start Wi-Fi: %s", esp_err_to_name(ret));
        return ret;
    }

#if CONFIG_WIFI_AP_ENABLED
    // 配置并启动 AP
    const char *ap_ssid = CONFIG_AP_SSID;
    const char *ap_password = CONFIG_AP_PASSWORD;
    if (strcmp(ssid, CONFIG_AP_SSID) != 0) {
        ap_ssid = ssid;
        ap_password = password;
    }
    ret = start_ap_mode(ap_ssid, ap_password);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start AP: %s", esp_err_to_name(ret));
        // 继续尝试,AP 失败不阻塞
    }
#endif

#if CONFIG_WIFI_STA_ENABLED
    // 配置并启动 STA
    const char *sta_ssid = CONFIG_STA_DEFAULT_SSID;
    const char *sta_password = CONFIG_STA_DEFAULT_PWD;
    if (strcmp(ssid, CONFIG_AP_SSID) != 0 && strlen(CONFIG_STA_DEFAULT_SSID) == 0) {
        sta_ssid = ssid;
        sta_password = password;
    }

    if (strlen(sta_ssid) > 0) {
        ret = start_sta_mode(sta_ssid, sta_password);
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "Failed to start STA: %s", esp_err_to_name(ret));
        }
    }
#endif

    // 等待 AP 启动完成
#if CONFIG_WIFI_AP_ENABLED
    if (mode == WIFI_MODE_AP || mode == WIFI_MODE_APSTA) {
        ESP_LOGI(TAG, "Waiting for AP to start...");
        EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                                WIFI_CONNECTED_BIT,
                                                pdFALSE,
                                                pdFALSE,
                                                pdMS_TO_TICKS(10000));
        if (!(bits & WIFI_CONNECTED_BIT)) {
            ESP_LOGW(TAG, "AP start timeout after 10 seconds");
        } else {
            s_wifi_state = WIFI_STATE_CONNECTED;
            snprintf(s_ip, sizeof(s_ip), CONFIG_AP_IP_ADDR);
        }
    }
#endif

    ESP_LOGI(TAG, "===========================================");
    ESP_LOGI(TAG, "Wi-Fi started in mode: %s",
             mode == WIFI_MODE_AP ? "AP" :
             mode == WIFI_MODE_STA ? "STA" : "AP+STA");
    ESP_LOGI(TAG, "===========================================");

    return ESP_OK;
}

/**
 * @brief 断开连接
 */
void wifi_disconnect(void)
{
    esp_wifi_disconnect();
    esp_wifi_stop();
    s_wifi_state = WIFI_STATE_IDLE;
}

/**
 * @brief 清理 Wi-Fi 资源
 */
void wifi_manager_cleanup(void)
{
    // 先停止 Wi-Fi 确保 netif 可以被安全销毁
    esp_wifi_stop();
    esp_wifi_deinit();

    // 销毁 netif 资源
    if (s_ap_netif != NULL) {
        esp_netif_destroy(s_ap_netif);
        s_ap_netif = NULL;
    }
    if (s_sta_netif != NULL) {
        esp_netif_destroy(s_sta_netif);
        s_sta_netif = NULL;
    }

    if (s_wifi_event_group != NULL) {
        vEventGroupDelete(s_wifi_event_group);
        s_wifi_event_group = NULL;
    }
    if (s_wifi_mutex != NULL) {
        vSemaphoreDelete(s_wifi_mutex);
        s_wifi_mutex = NULL;
    }
}

/**
 * @brief 获取当前 Wi-Fi 状态
 */
wifi_state_t wifi_get_state(void)
{
    wifi_state_t state;
    if (s_wifi_mutex && xSemaphoreTake(s_wifi_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        state = s_wifi_state;
        xSemaphoreGive(s_wifi_mutex);
    } else {
        state = s_wifi_state;
    }
    return state;
}

/**
 * @brief 检查是否已连接
 */
bool wifi_is_connected(void)
{
    bool connected;
    if (s_wifi_mutex && xSemaphoreTake(s_wifi_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        connected = (s_wifi_state == WIFI_STATE_CONNECTED);
        xSemaphoreGive(s_wifi_mutex);
    } else {
        connected = (s_wifi_state == WIFI_STATE_CONNECTED);
    }
    return connected;
}

/**
 * @brief 获取本地 IP 地址
 */
const char* wifi_get_ip(void)
{
    static char ip_copy[16] = {0};
    if (s_wifi_mutex && xSemaphoreTake(s_wifi_mutex, portMAX_DELAY) == pdTRUE) {
        strncpy(ip_copy, s_ip, sizeof(ip_copy) - 1);
        ip_copy[sizeof(ip_copy) - 1] = '\0';
        xSemaphoreGive(s_wifi_mutex);
    } else {
        strncpy(ip_copy, s_ip, sizeof(ip_copy) - 1);
        ip_copy[sizeof(ip_copy) - 1] = '\0';
    }
    return ip_copy;
}

/**
 * @brief 获取当前 SSID
 */
const char* wifi_get_ssid(void)
{
    static char ssid_copy[32] = {0};
    if (s_wifi_mutex && xSemaphoreTake(s_wifi_mutex, portMAX_DELAY) == pdTRUE) {
        strncpy(ssid_copy, s_ssid, sizeof(ssid_copy) - 1);
        ssid_copy[sizeof(ssid_copy) - 1] = '\0';
        xSemaphoreGive(s_wifi_mutex);
    } else {
        strncpy(ssid_copy, s_ssid, sizeof(ssid_copy) - 1);
        ssid_copy[sizeof(ssid_copy) - 1] = '\0';
    }
    return ssid_copy;
}

/**
 * @brief 获取信号强度
 */
int wifi_get_rssi(void)
{
    int rssi;
    if (s_wifi_mutex && xSemaphoreTake(s_wifi_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        rssi = s_rssi;
        xSemaphoreGive(s_wifi_mutex);
    } else {
        rssi = s_rssi;
    }
    return rssi;
}
