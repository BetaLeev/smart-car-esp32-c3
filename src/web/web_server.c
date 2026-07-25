/**
 * @file web_server.c
 * @brief Web 服务器实现
 */

#include "web_server.h"
#include "motor_driver.h"
#include "../buzzer/buzzer.h"
#include "../wifi/wifi_manager.h"
// #include "../fan_driver.h"  // TODO: 风扇功能暂时禁用
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "esp_log.h"
#include "esp_http_server.h"
#include "esp_heap_caps.h"
#include "esp_system.h"
#include "cJSON.h"
#include "esp_vfs.h"
#include "esp_spiffs.h"
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

// SPIFFS 分区标签
#define SPIFFS_PARTITION_LABEL "spiffs"

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
    char file_path[576];  // max URI (512) + 64 buffer
    FILE *fp = NULL;

    // 构建文件路径 - 处理根路径和普通路径
    // req->uri 可能以 "/" 开头，需要去掉前导斜杠以匹配 SPIFFS 文件系统路径
    const char *uri = req->uri;
    if (strcmp(uri, "/") == 0) {
        uri = "/index.html";
    } else if (uri[0] == '/') {
        // 保持前导斜杠，因为 SPIFFS 挂载时 base_path="" 需要完整路径
    }

    snprintf(file_path, sizeof(file_path), "%s", uri);
    ESP_LOGD(TAG, "[STATIC] Requested file: %s", file_path);

    // 打开文件
    fp = fopen(file_path, "r");
    if (fp == NULL) {
        ESP_LOGW(TAG, "[STATIC] File not found: %s", file_path);
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
    if (s_car_mutex && xSemaphoreTake(s_car_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
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

    // 添加风扇状态（暂时禁用）
    // cJSON_AddBoolToObject(root, "fan_on", fan_driver_is_on());
    // cJSON_AddNumberToObject(root, "fan_speed", fan_driver_get_speed());

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
        ESP_LOGW(TAG, "[CONTROL] Failed to receive data, ret=%d", ret);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to receive data");
        return ESP_FAIL;
    }
    content[recv_size] = '\0';

    ESP_LOGI(TAG, "[CONTROL] Received raw data: %s", content);

    cJSON *root = cJSON_Parse(content);
    if (root == NULL) {
        ESP_LOGW(TAG, "[CONTROL] JSON parse failed");
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_FAIL;
    }

    cJSON *cmd_item = cJSON_GetObjectItem(root, "command");
    cJSON *speed_item = cJSON_GetObjectItem(root, "speed");

    // 解析速度值
    int speed = 50;
    if (speed_item && cJSON_IsNumber(speed_item)) {
        speed = speed_item->valueint;
    }
    ESP_LOGI(TAG, "[CONTROL] Speed value: %d", speed);

#if CONFIG_BUZZER_ENABLED
    // 喇叭命令无需小车状态锁，独立处理
    if (cmd_item && cJSON_IsString(cmd_item) && strcmp(cmd_item->valuestring, "horn") == 0) {
        ESP_LOGI(TAG, "[CONTROL] >>> HORN command received, activating buzzer!");

        // 使用 buzzer_beep 函数（已处理低电平触发逻辑）
        buzzer_beep(200);

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

    // 解析命令
    const char *cmd_str = "unknown";
    bool command_valid = false;

    if (cmd_item && cJSON_IsString(cmd_item)) {
        cmd_str = cmd_item->valuestring;

        // 风扇控制命令（暂时禁用）
        #if 0  // 风扇功能暂时禁用
        if (strcmp(cmd_str, "fan_on") == 0) {
            ESP_LOGI(TAG, "[CONTROL] >>> Fan ON, speed: %d", speed);
            fan_driver_on(speed);
            cJSON_Delete(root);
            cJSON *resp = cJSON_CreateObject();
            cJSON_AddBoolToObject(resp, "success", true);
            cJSON_AddStringToObject(resp, "message", "Fan turned on");
            const char *json_str = cJSON_Print(resp);
            httpd_resp_set_type(req, "application/json");
            httpd_resp_sendstr(req, json_str);
            free((void *)json_str);
            cJSON_Delete(resp);
            return ESP_OK;
        } else if (strcmp(cmd_str, "fan_off") == 0) {
            ESP_LOGI(TAG, "[CONTROL] >>> Fan OFF");
            fan_driver_off();
            cJSON_Delete(root);
            cJSON *resp = cJSON_CreateObject();
            cJSON_AddBoolToObject(resp, "success", true);
            cJSON_AddStringToObject(resp, "message", "Fan turned off");
            const char *json_str = cJSON_Print(resp);
            httpd_resp_set_type(req, "application/json");
            httpd_resp_sendstr(req, json_str);
            free((void *)json_str);
            cJSON_Delete(resp);
            return ESP_OK;
        } else if (strcmp(cmd_str, "fan_speed") == 0) {
            ESP_LOGI(TAG, "[CONTROL] >>> Fan speed set: %d", speed);
            fan_driver_set_speed(speed);
            cJSON_Delete(root);
            cJSON *resp = cJSON_CreateObject();
            cJSON_AddBoolToObject(resp, "success", true);
            cJSON_AddNumberToObject(resp, "fan_speed", speed);
            const char *json_str = cJSON_Print(resp);
            httpd_resp_set_type(req, "application/json");
            httpd_resp_sendstr(req, json_str);
            free((void *)json_str);
            cJSON_Delete(resp);
            return ESP_OK;
        }
        #endif

        // 小车控制命令（需要互斥锁保护）
        if (s_car_mutex && xSemaphoreTake(s_car_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            ESP_LOGI(TAG, "[CONTROL] >>> Processing command: '%s', speed: %d", cmd_str, speed);

            if (strcmp(cmd_str, "forward") == 0) {
                ESP_LOGI(TAG, "[CONTROL] Executing: car_forward()");
                car_forward();
                s_car_status.command = CMD_FORWARD;
                ESP_LOGI(TAG, "[CONTROL] State updated: CMD_FORWARD");
                command_valid = true;
            } else if (strcmp(cmd_str, "backward") == 0) {
                ESP_LOGI(TAG, "[CONTROL] Executing: car_backward()");
                car_backward();
                s_car_status.command = CMD_BACKWARD;
                ESP_LOGI(TAG, "[CONTROL] State updated: CMD_BACKWARD");
                command_valid = true;
            } else if (strcmp(cmd_str, "left") == 0) {
                ESP_LOGI(TAG, "[CONTROL] Executing: car_turn_left()");
                car_turn_left();
                s_car_status.command = CMD_TURN_LEFT;
                ESP_LOGI(TAG, "[CONTROL] State updated: CMD_TURN_LEFT");
                command_valid = true;
            } else if (strcmp(cmd_str, "right") == 0) {
                ESP_LOGI(TAG, "[CONTROL] Executing: car_turn_right()");
                car_turn_right();
                s_car_status.command = CMD_TURN_RIGHT;
                ESP_LOGI(TAG, "[CONTROL] State updated: CMD_TURN_RIGHT");
                command_valid = true;
            } else if (strcmp(cmd_str, "stop") == 0) {
                ESP_LOGI(TAG, "[CONTROL] Executing: car_stop()");
                car_stop();
                s_car_status.command = CMD_STOP;
                ESP_LOGI(TAG, "[CONTROL] State updated: CMD_STOP");
                command_valid = true;
            } else {
                ESP_LOGW(TAG, "[CONTROL] Unknown command: '%s'", cmd_str);
            }

            // 更新速度值
            s_car_status.speed = speed;

            xSemaphoreGive(s_car_mutex);
        } else {
            ESP_LOGW(TAG, "[CONTROL] Failed to acquire mutex for command '%s'", cmd_str);
        }
    } else {
        ESP_LOGW(TAG, "[CONTROL] No valid 'command' field in request");
    }

    cJSON_Delete(root);

    cJSON *resp = cJSON_CreateObject();
    cJSON_AddBoolToObject(resp, "success", command_valid);
    cJSON_AddStringToObject(resp, "message", command_valid ? "Command executed" : "Command failed");

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
    .uri      = "/css/style.css",
    .method   = HTTP_GET,
    .handler  = static_file_handler,
    .user_ctx = NULL
};

static const httpd_uri_t main_js_uri = {
    .uri      = "/js/main.js",
    .method   = HTTP_GET,
    .handler  = static_file_handler,
    .user_ctx = NULL
};

static const httpd_uri_t api_js_uri = {
    .uri      = "/js/api.js",
    .method   = HTTP_GET,
    .handler  = static_file_handler,
    .user_ctx = NULL
};

static const httpd_uri_t state_js_uri = {
    .uri      = "/js/state.js",
    .method   = HTTP_GET,
    .handler  = static_file_handler,
    .user_ctx = NULL
};

static const httpd_uri_t ui_js_uri = {
    .uri      = "/js/ui.js",
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

    // 挂载 SPIFFS 文件系统
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "",
        .partition_label = SPIFFS_PARTITION_LABEL,
        .max_files = 10,
        .format_if_mount_failed = false  // 不要自动格式化！
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount SPIFFS: %s", esp_err_to_name(ret));
        vSemaphoreDelete(s_car_mutex);
        s_car_mutex = NULL;
        return ret;
    }
    
    ESP_LOGI(TAG, "SPIFFS mounted successfully");
    // 列出根目录文件
    struct dirent *entry;
    DIR *dir = opendir("");
    if (dir) {
        ESP_LOGI(TAG, "SPIFFS files:");
        while ((entry = readdir(dir)) != NULL) {
            ESP_LOGI(TAG, "  - %s", entry->d_name);
        }
        closedir(dir);
    }

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
    config.max_uri_handlers = 16;  // 增加 URI 处理器数量

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
    httpd_register_uri_handler(s_httpd_handle, &api_js_uri);
    httpd_register_uri_handler(s_httpd_handle, &state_js_uri);
    httpd_register_uri_handler(s_httpd_handle, &ui_js_uri);
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
    // 卸载 SPIFFS 文件系统
    esp_vfs_spiffs_unregister(SPIFFS_PARTITION_LABEL);
    ESP_LOGI(TAG, "SPIFFS unregistered");
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
