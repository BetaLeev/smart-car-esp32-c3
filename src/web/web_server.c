/**
 * @file web_server.c
 * @brief Web 服务器实现
 */

#include "web_server.h"
#include "motor_driver.h"
#include "../wifi/wifi_manager.h"
#include <string.h>
#include <stdlib.h>
#include "esp_log.h"
#include "esp_http_server.h"
#include "cJSON.h"

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

/**
 * @brief 处理根路径 - 返回控制页面
 */
static esp_err_t root_handler(httpd_req_t *req)
{
    const char *html = "<!DOCTYPE html><html><head><meta charset='utf-8'>"
        "<title>ESP32-C3 Car</title>"
        "<style>body{font-family:Arial;padding:20px;background:#f0f0f0}"
        "h1{color:#333}.btn{padding:15px 30px;margin:5px;font-size:18px;"
        "border:none;border-radius:5px;cursor:pointer}"
        ".fwd{background:#4CAF50;color:white}.bwd{background:#f44336;color:white}"
        ".left{background:#2196F3;color:white}.right{background:#FF9800;color:white}"
        ".stop{background:#333;color:white}</style></head>"
        "<body><h1>ESP32-C3 Car Controller</h1>"
        "<button class='btn fwd' onclick=\"fetch('/api/control',{method:'POST',"
        "headers:{'Content-Type':'application/json'},"
        "body:JSON.stringify({command:'forward'})})\">前进</button><br>"
        "<button class='btn left' onclick=\"fetch('/api/control',{method:'POST',"
        "headers:{'Content-Type':'application/json'},"
        "body:JSON.stringify({command:'left'})})\">左转</button>"
        "<button class='btn stop' onclick=\"fetch('/api/control',{method:'POST',"
        "headers:{'Content-Type':'application/json'},"
        "body:JSON.stringify({command:'stop'})})\">停止</button>"
        "<button class='btn right' onclick=\"fetch('/api/control',{method:'POST',"
        "headers:{'Content-Type':'application/json'},"
        "body:JSON.stringify({command:'right'})})\">右转</button><br>"
        "<button class='btn bwd' onclick=\"fetch('/api/control',{method:'POST',"
        "headers:{'Content-Type':'application/json'},"
        "body:JSON.stringify({command:'backward'})})\">后退</button>"
        "<p id='status'></p><script>setInterval(()=>{fetch('/api/status')"
        ".then(r=>r.json()).then(d=>{document.getElementById('status').innerText="
        "'IP: '+d.ip+' | Status: '+d.command})},1000)</script></body></html>";

    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, html, strlen(html));
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
    char content[256];
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
    char content[256];
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
    .handler  = root_handler,
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
 * @brief 初始化 Web 服务器
 */
esp_err_t web_server_init(uint16_t port)
{
    s_server_port = port;
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
    config.stack_size = 8192;

    ESP_LOGI(TAG, "Starting web server on port %d", s_server_port);

    esp_err_t ret = httpd_start(&s_httpd_handle, &config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start web server: %s", esp_err_to_name(ret));
        return ret;
    }

    // 注册 URI 处理器
    httpd_register_uri_handler(s_httpd_handle, &root_uri);
    httpd_register_uri_handler(s_httpd_handle, &api_status_uri);
    httpd_register_uri_handler(s_httpd_handle, &api_control_uri);
    httpd_register_uri_handler(s_httpd_handle, &api_wifi_connect_uri);
    httpd_register_uri_handler(s_httpd_handle, &api_wifi_status_uri);

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
}

/**
 * @brief 获取当前小车状态
 */
car_status_t* web_get_car_status(void)
{
    return &s_car_status;
}
