/**
 * @file web_server.c
 * @brief Web 服务器实现
 */

#include "web_server.h"
#include "motor_driver.h"
#include "../wifi/wifi_manager.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "esp_log.h"
#include "esp_http_server.h"
#include "esp_heap_caps.h"
#include "esp_system.h"
#include "cJSON.h"
#include "../config/wifi_config.h"
#include "../config/pin_config.h"
#include "../config/app_config.h"

static const char *TAG = "WEB";

// Web 服务器句柄
static httpd_handle_t s_httpd_handle = NULL;
static uint16_t s_server_port = 80;

// 小车状态
static car_status_t s_car_status = {
    .speed = 50,
    .command = CMD_STOP,
    .battery = 100
};
static SemaphoreHandle_t s_car_mutex = NULL;  // 小车状态保护互斥锁

// 静态文件路径前缀
#define WEB_DATA_PATH "/data/web"

// MIME 类型映射
static const char *get_mime_type(const char *file_path)
{
    const char *ext = strrchr(file_path, '.');
    if (ext) {
        if (strcmp(ext, ".html") == 0) return "text/html";
        if (strcmp(ext, ".css") == 0) return "text/css";
        if (strcmp(ext, ".js") == 0) return "application/javascript";
        if (strcmp(ext, ".json") == 0) return "application/json";
        if (strcmp(ext, ".png") == 0) return "image/png";
        if (strcmp(ext, ".jpg") == 0) return "image/jpeg";
        if (strcmp(ext, ".ico") == 0) return "image/x-icon";
    }
    return "application/octet-stream";
}

/**
 * @brief 处理静态文件请求
 */
static esp_err_t static_file_handler(httpd_req_t *req)
{
    char file_path[576];  // WEB_DATA_PATH (10) + max URI (512) + buffer (54)
    FILE *fp = NULL;

    // 构建文件路径
    if (strcmp(req->uri, "/") == 0) {
        snprintf(file_path, sizeof(file_path), WEB_DATA_PATH "/index.html");
    } else {
        snprintf(file_path, sizeof(file_path), WEB_DATA_PATH "%s", req->uri);
    }

    // 打开文件
    fp = fopen(file_path, "r");
    if (fp == NULL) {
        ESP_LOGW(TAG, "File not found: %s", file_path);
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "File not found");
        return ESP_FAIL;
    }

    // 获取文件大小
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    // 分配缓冲区
    char *buffer = malloc(file_size + 1);
    if (buffer == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for file: %s", file_path);
        fclose(fp);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Memory allocation failed");
        return ESP_FAIL;
    }

    // 读取文件内容
    size_t read_size = fread(buffer, 1, file_size, fp);
    buffer[read_size] = '\0';
    fclose(fp);

    // 设置 MIME 类型并发送响应
    httpd_resp_set_type(req, get_mime_type(file_path));
    httpd_resp_send(req, buffer, read_size);

    free(buffer);
    return ESP_OK;
}

/**
 * @brief 处理根路径 - 重定向到 index.html
 */
static esp_err_t __attribute__((unused)) root_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    httpd_resp_sendstr(req,
        "<!DOCTYPE html><html><head><meta charset='UTF-8'>"
        "<meta http-equiv='refresh' content='0;url=/index.html'>"
        "</head><body><p>Redirecting to <a href='/index.html'>/index.html</a></p></body></html>");
    return ESP_OK;
}

/**
 * @brief 处理 API 状态请求
 */
static esp_err_t api_status_handler(httpd_req_t *req)
{
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "ip", wifi_get_ip());
    cJSON_AddStringToObject(root, "ssid", wifi_get_ssid());
    cJSON_AddBoolToObject(root, "wifi_connected", wifi_is_connected());
    cJSON_AddNumberToObject(root, "rssi", wifi_get_rssi());

    // 受保护地读取小车状态
    if (s_car_mutex && xSemaphoreTake(s_car_mutex, portMAX_DELAY) == pdTRUE) {
        cJSON_AddNumberToObject(root, "speed", s_car_status.speed);
        cJSON_AddNumberToObject(root, "battery", s_car_status.battery);

        const char *status_str = "stop";
        switch (s_car_status.command) {
            case CMD_FORWARD:    status_str = "forward";  break;
            case CMD_BACKWARD:   status_str = "backward"; break;
            case CMD_TURN_LEFT:  status_str = "left";     break;
            case CMD_TURN_RIGHT: status_str = "right";    break;
            default:             status_str = "stop";      break;
        }
        cJSON_AddStringToObject(root, "command", status_str);
        xSemaphoreGive(s_car_mutex);
    } else {
        cJSON_AddNumberToObject(root, "speed", 0);
        cJSON_AddNumberToObject(root, "battery", 0);
        cJSON_AddStringToObject(root, "command", "stop");
    }

    const char *json_str = cJSON_Print(root);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, json_str);

    free((void *)json_str);
    cJSON_Delete(root);
    return ESP_OK;
}

/**
 * @brief 处理控制命令
 */
static esp_err_t api_control_handler(httpd_req_t *req)
{
    char content[512];  // 增加缓冲区大小以处理更大的 JSON 请求
    size_t recv_size = MIN(req->content_len, sizeof(content) - 1);

    int ret = httpd_req_recv(req, content, recv_size);
    if (ret <= 0) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to receive data");
        return ESP_FAIL;
    }
    content[recv_size] = '\0';

    cJSON *root = cJSON_Parse(content);
    if (root == NULL) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_FAIL;
    }

    cJSON *cmd_item = cJSON_GetObjectItem(root, "command");
    cJSON *speed_item = cJSON_GetObjectItem(root, "speed");

#if CONFIG_BUZZER_ENABLED
    // 喇叭命令无需小车状态锁，独立处理
    if (cmd_item && cJSON_IsString(cmd_item) && strcmp(cmd_item->valuestring, "horn") == 0) {
        gpio_set_level(BUZZER_PIN, 1);
        vTaskDelay(pdMS_TO_TICKS(200));
        gpio_set_level(BUZZER_PIN, 0);
        cJSON_Delete(root);
        // 发送响应
        cJSON *resp = cJSON_CreateObject();
        cJSON_AddBoolToObject(resp, "success", true);
        cJSON_AddStringToObject(resp, "message", "Horn activated");
        const char *json_str = cJSON_Print(resp);
        httpd_resp_set_type(req, "application/json");
        httpd_resp_sendstr(req, json_str);
        free((void *)json_str);
        cJSON_Delete(resp);
        return ESP_OK;
    }
#endif

    // 受保护地更新小车状态
    if (s_car_mutex && xSemaphoreTake(s_car_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        if (cmd_item && cJSON_IsString(cmd_item)) {
            const char *cmd = cmd_item->valuestring;

            if (strcmp(cmd, "forward") == 0) {
                car_forward();
                s_car_status.command = CMD_FORWARD;
            } else if (strcmp(cmd, "backward") == 0) {
                car_backward();
                s_car_status.command = CMD_BACKWARD;
            } else if (strcmp(cmd, "left") == 0) {
                car_turn_left();
                s_car_status.command = CMD_TURN_LEFT;
            } else if (strcmp(cmd, "right") == 0) {
                car_turn_right();
                s_car_status.command = CMD_TURN_RIGHT;
            } else if (strcmp(cmd, "stop") == 0) {
                car_stop();
                s_car_status.command = CMD_STOP;
            }
        }

        if (speed_item && cJSON_IsNumber(speed_item)) {
            s_car_status.speed = speed_item->valueint;
        }
        xSemaphoreGive(s_car_mutex);
    }

    cJSON_Delete(root);

    cJSON *resp = cJSON_CreateObject();
    cJSON_AddBoolToObject(resp, "success", true);
    cJSON_AddStringToObject(resp, "message", "Command executed");

    const char *json_str = cJSON_Print(resp);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, json_str);

    free((void *)json_str);
    cJSON_Delete(resp);

    return ESP_OK;
}

/**
 * @brief Wi-Fi 连接 API
 */
static esp_err_t api_wifi_connect_handler(httpd_req_t *req)
{
    char content[512];  // 增加缓冲区大小以处理更大的 JSON 请求
    size_t recv_size = MIN(req->content_len, sizeof(content) - 1);

    int ret = httpd_req_recv(req, content, recv_size);
    if (ret <= 0) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to receive data");
        return ESP_FAIL;
    }
    content[recv_size] = '\0';

    cJSON *root = cJSON_Parse(content);
    if (root == NULL) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_FAIL;
    }

    cJSON *ssid_item = cJSON_GetObjectItem(root, "ssid");
    cJSON *password_item = cJSON_GetObjectItem(root, "password");

    cJSON *resp = cJSON_CreateObject();

    if (ssid_item && cJSON_IsString(ssid_item)) {
        const char *ssid = ssid_item->valuestring;
        const char *password = password_item && cJSON_IsString(password_item)
                                ? password_item->valuestring : "";

        esp_err_t err = wifi_connect(ssid, password);
        cJSON_AddBoolToObject(resp, "success", err == ESP_OK);
        cJSON_AddStringToObject(resp, "message",
                                 err == ESP_OK ? "Connecting..." : "Connection failed");
    } else {
        cJSON_AddBoolToObject(resp, "success", false);
        cJSON_AddStringToObject(resp, "message", "Missing SSID");
    }

    cJSON_Delete(root);

    const char *json_str = cJSON_Print(resp);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, json_str);

    free((void *)json_str);
    cJSON_Delete(resp);

    return ESP_OK;
}

/**
 * @brief Wi-Fi 状态 API
 */
static esp_err_t api_wifi_status_handler(httpd_req_t *req)
{
    cJSON *root = cJSON_CreateObject();

    cJSON_AddBoolToObject(root, "connected", wifi_is_connected());
    cJSON_AddStringToObject(root, "ssid", wifi_get_ssid());
    cJSON_AddStringToObject(root, "ip", wifi_get_ip());
    cJSON_AddNumberToObject(root, "rssi", wifi_get_rssi());

    const char *state_str = "idle";
    switch (wifi_get_state()) {
        case WIFI_STATE_CONNECTING:   state_str = "connecting";   break;
        case WIFI_STATE_CONNECTED:    state_str = "connected";    break;
        case WIFI_STATE_DISCONNECTED: state_str = "disconnected"; break;
        case WIFI_STATE_FAILED:       state_str = "failed";      break;
        default:                       state_str = "idle";       break;
    }
    cJSON_AddStringToObject(root, "state", state_str);

    const char *json_str = cJSON_Print(root);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, json_str);

    free((void *)json_str);
    cJSON_Delete(root);
    return ESP_OK;
}

// URI 路由配置
static const httpd_uri_t root_uri = {
    .uri      = "/",
    .method   = HTTP_GET,
    .handler  = static_file_handler,
    .user_ctx = NULL
};

static const httpd_uri_t index_html_uri = {
    .uri      = "/index.html",
    .method   = HTTP_GET,
    .handler  = static_file_handler,
    .user_ctx = NULL
};

static const httpd_uri_t style_css_uri = {
    .uri      = "/style.css",
    .method   = HTTP_GET,
    .handler  = static_file_handler,
    .user_ctx = NULL
};

static const httpd_uri_t main_js_uri = {
    .uri      = "/main.js",
    .method   = HTTP_GET,
    .handler  = static_file_handler,
    .user_ctx = NULL
};

static const httpd_uri_t api_status_uri = {
    .uri      = "/api/status",
    .method   = HTTP_GET,
    .handler  = api_status_handler,
    .user_ctx = NULL
};

static const httpd_uri_t api_control_uri = {
    .uri      = "/api/control",
    .method   = HTTP_POST,
    .handler  = api_control_handler,
    .user_ctx = NULL
};

static const httpd_uri_t api_wifi_connect_uri = {
    .uri      = "/api/wifi/connect",
    .method   = HTTP_POST,
    .handler  = api_wifi_connect_handler,
    .user_ctx = NULL
};

static const httpd_uri_t api_wifi_status_uri = {
    .uri      = "/api/wifi/status",
    .method   = HTTP_GET,
    .handler  = api_wifi_status_handler,
    .user_ctx = NULL
};

/**
 * @brief 内存监控 API
 *
 * @note ESP32-C3 兼容版本,避免使用仅在高端芯片上可用的函数
 */
static esp_err_t api_memory_handler(httpd_req_t *req)
{
    cJSON *root = cJSON_CreateObject();

    // 基础堆内存信息
    uint32_t free_heap = esp_get_free_heap_size();
    uint32_t total_heap = heap_caps_get_total_size(MALLOC_CAP_8BIT);
    uint32_t used_heap = (total_heap > free_heap) ? (total_heap - free_heap) : 0;
    uint32_t used_percent = (total_heap > 0) ? (used_heap * 100) / total_heap : 0;

    cJSON_AddNumberToObject(root, "free_heap_size", free_heap);
    cJSON_AddNumberToObject(root, "heap_size", total_heap);
    cJSON_AddNumberToObject(root, "used_heap", used_heap);
    cJSON_AddNumberToObject(root, "used_percent", used_percent);

    // DRAM 信息 (可分配给 DMA 的内存)
    cJSON_AddNumberToObject(root, "dram_free", heap_caps_get_free_size(MALLOC_CAP_8BIT));
    cJSON_AddNumberToObject(root, "dram_size", heap_caps_get_total_size(MALLOC_CAP_8BIT));

    // 最小剩余堆内存 (系统运行期间的最低点,用于检测内存泄漏)
    cJSON_AddNumberToObject(root, "min_free_heap_size", esp_get_minimum_free_heap_size());

    // 内核版本
    cJSON_AddStringToObject(root, "idf_version", IDF_VER);

    // 系统运行时间 (秒)
    cJSON_AddNumberToObject(root, "uptime_secs", esp_log_timestamp() / 1000);

    const char *json_str = cJSON_Print(root);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, json_str);

    free((void *)json_str);
    cJSON_Delete(root);
    return ESP_OK;
}

static const httpd_uri_t api_memory_uri = {
    .uri      = "/api/memory",
    .method   = HTTP_GET,
    .handler  = api_memory_handler,
    .user_ctx = NULL
};

/**
 * @brief 初始化 Web 服务器
 */
esp_err_t web_server_init(uint16_t port)
{
    s_server_port = port;
    s_car_mutex = xSemaphoreCreateMutex();  // 创建互斥锁
    ESP_LOGI(TAG, "Web server configured on port %d", port);
    return ESP_OK;
}

/**
 * @brief 启动 Web 服务器
 */
esp_err_t web_server_start(void)
{
    if (s_httpd_handle != NULL) {
        ESP_LOGW(TAG, "Web server already started");
        return ESP_OK;
    }

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = s_server_port;
    config.stack_size = 10240;  // 增加栈空间以处理 cJSON 解析

    ESP_LOGI(TAG, "Starting web server on port %d", s_server_port);

    esp_err_t ret = httpd_start(&s_httpd_handle, &config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start web server: %s", esp_err_to_name(ret));
        return ret;
    }

    // 注册 URI 处理器
    httpd_register_uri_handler(s_httpd_handle, &root_uri);
    httpd_register_uri_handler(s_httpd_handle, &index_html_uri);
    httpd_register_uri_handler(s_httpd_handle, &style_css_uri);
    httpd_register_uri_handler(s_httpd_handle, &main_js_uri);
    httpd_register_uri_handler(s_httpd_handle, &api_status_uri);
    httpd_register_uri_handler(s_httpd_handle, &api_control_uri);
    httpd_register_uri_handler(s_httpd_handle, &api_wifi_connect_uri);
    httpd_register_uri_handler(s_httpd_handle, &api_wifi_status_uri);
    httpd_register_uri_handler(s_httpd_handle, &api_memory_uri);

    ESP_LOGI(TAG, "Web server started successfully");
    return ESP_OK;
}

/**
 * @brief 停止 Web 服务器
 */
void web_server_stop(void)
{
    if (s_httpd_handle != NULL) {
        httpd_stop(s_httpd_handle);
        s_httpd_handle = NULL;
        ESP_LOGI(TAG, "Web server stopped");
    }
    if (s_car_mutex != NULL) {
        vSemaphoreDelete(s_car_mutex);
        s_car_mutex = NULL;
    }
}

/**
 * @brief 获取当前小车状态（值传递，线程安全）
 * @param out_status 输出参数，存储复制的小车状态
 */
void web_get_car_status(car_status_t *out_status)
{
    if (out_status == NULL) return;
    if (s_car_mutex && xSemaphoreTake(s_car_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        *out_status = s_car_status;
        xSemaphoreGive(s_car_mutex);
    }
}
