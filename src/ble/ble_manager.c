/**
 * @file ble_manager.c
 * @brief BLE 管理器实现 (ESP-IDF 6.0)
 */

#include "ble_manager.h"
#include "config/app_config.h"
#include "esp_log.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_defs.h"

static const char *TAG = "BLE";

// BLE 服务 UUID
#define BLE_SERVICE_UUID       0xFF00
#define BLE_CHAR_CMD_UUID      0xFF01
#define BLE_CHAR_SPEED_UUID    0xFF02

// BLE 设备名称
#define BLE_DEVICE_NAME        "ESP32-CAR"
#define PROFILE_A_APP_ID       0

// 命令回调
static ble_command_callback_t s_command_callback = NULL;
static uint8_t s_current_speed = 50;

// 服务属性句柄
static uint16_t gatt_service_handle = 0;
static uint16_t gatt_char_cmd_handle = 0;
static uint16_t gatt_char_speed_handle = 0;
static uint16_t gatt_conn_id = 0;
static bool g_connected = false;

// GATT 事件处理
static void gatts_profile_a_event_handler(esp_gatts_cb_event_t event,
                                           esp_gatt_if_t gatts_if,
                                           esp_ble_gatts_cb_param_t *param);

/**
 * @brief GATT 表
 */
static const esp_gatts_attr_db_t gatt_db[] = {
    // 服务声明
    [0] = {
        {ESP_GATT_AUTO_RSP},
        {ESP_UUID_LEN_16, (uint8_t []){0x00, 0x28}, ESP_GATT_PERM_READ,
         sizeof(uint16_t), sizeof(BLE_SERVICE_UUID), (uint8_t []){0x00, 0xFF}}
    },
    // 特征声明 (命令)
    [1] = {
        {ESP_GATT_AUTO_RSP},
        {ESP_UUID_LEN_16, (uint8_t []){0x03, 0x28}, ESP_GATT_PERM_READ,
         1, 1, (uint8_t []){ESP_GATT_CHAR_PROP_BIT_WRITE_NR}}
    },
    // 命令特征值
    [2] = {
        {ESP_GATT_RSP_BY_APP},
        {ESP_UUID_LEN_16, (uint8_t []){0x01, 0xFF}, ESP_GATT_PERM_WRITE,
         1, 0, NULL}
    },
    // 特征声明 (速度)
    [3] = {
        {ESP_GATT_AUTO_RSP},
        {ESP_UUID_LEN_16, (uint8_t []){0x03, 0x28}, ESP_GATT_PERM_READ,
         1, 1, (uint8_t []){ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE}}
    },
    // 速度特征值
    [4] = {
        {ESP_GATT_RSP_BY_APP},
        {ESP_UUID_LEN_16, (uint8_t []){0x02, 0xFF}, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
         1, 1, (uint8_t []){50}}
    },
};

/**
 * @brief 处理 BLE 命令
 */
static void handle_command(uint8_t cmd, uint8_t speed)
{
    if (cmd > BLE_CMD_SET_SPEED) {
        cmd = BLE_CMD_STOP;
    }

    if (s_command_callback != NULL) {
        s_command_callback((ble_command_t)cmd, speed);
    }

    ESP_LOGI(TAG, "Command: %d, Speed: %d", cmd, speed);
}

/**
 * @brief GAP 事件处理
 */
static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event) {
    case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
        ESP_LOGI(TAG, "Advertising data set complete");
        break;
    case ESP_GAP_BLE_SEC_REQ_EVT:
        esp_ble_gap_security_rsp(param->ble_security.ble_req.bd_addr, true);
        break;
    case ESP_GAP_BLE_AUTH_CMPL_EVT:
        ESP_LOGI(TAG, "Authentication complete");
        break;
    default:
        break;
    }
}

/**
 * @brief GATT 事件处理
 */
static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if,
                                 esp_ble_gatts_cb_param_t *param)
{
    if (event == ESP_GATTS_REG_EVT) {
        if (param->reg.status == ESP_GATT_OK) {
            ESP_LOGI(TAG, "Profile registered, app_id: %d", param->reg.app_id);
            // 创建属性表
            esp_ble_gatts_create_attr_tab(gatt_db, gatts_if, 5, PROFILE_A_APP_ID);
        } else {
            ESP_LOGE(TAG, "Profile registration failed, app_id: %d, status: %d",
                     param->reg.app_id, param->reg.status);
        }
    }
    gatts_profile_a_event_handler(event, gatts_if, param);
}

/**
 * @brief GATT Profile 事件处理
 */
static void gatts_profile_a_event_handler(esp_gatts_cb_event_t event,
                                           esp_gatt_if_t gatts_if,
                                           esp_ble_gatts_cb_param_t *param)
{
    switch (event) {
    case ESP_GATTS_REG_EVT:
        break;

    case ESP_GATTS_CREATE_EVT:
        gatt_service_handle = param->create.service_handle;
        esp_ble_gatts_start_service(gatt_service_handle);
        break;

    case ESP_GATTS_ADD_CHAR_EVT: {
        uint16_t uuid = param->add_char.char_uuid.uuid.uuid16;
        if (uuid == BLE_CHAR_CMD_UUID) {
            gatt_char_cmd_handle = param->add_char.attr_handle;
            ESP_LOGI(TAG, "Command char added, handle: %d", gatt_char_cmd_handle);
        } else if (uuid == BLE_CHAR_SPEED_UUID) {
            gatt_char_speed_handle = param->add_char.attr_handle;
            ESP_LOGI(TAG, "Speed char added, handle: %d", gatt_char_speed_handle);
        }
        break;
    }

    case ESP_GATTS_CONNECT_EVT:
        gatt_conn_id = param->connect.conn_id;
        g_connected = true;
        ESP_LOGI(TAG, "Client connected, conn_id: %d", gatt_conn_id);
        break;

    case ESP_GATTS_DISCONNECT_EVT:
        ESP_LOGI(TAG, "Client disconnected");
        g_connected = false;
        gatt_conn_id = 0;
        break;

    case ESP_GATTS_WRITE_EVT: {
        uint16_t handle = param->write.handle;
        uint16_t len = param->write.len;
        ESP_LOGI(TAG, "Write event, handle: %d, len: %d", handle, len);

        if (handle == gatt_char_cmd_handle && len >= 1) {
            uint8_t cmd = param->write.value[0];
            uint8_t speed = (len >= 2) ? param->write.value[1] : s_current_speed;
            handle_command(cmd, speed);

            esp_ble_gatts_send_response(gatts_if, param->write.conn_id,
                                        param->write.trans_id, ESP_GATT_OK, NULL);
        } else if (handle == gatt_char_speed_handle && len >= 1) {
            s_current_speed = param->write.value[0];
            ESP_LOGI(TAG, "Speed updated: %d", s_current_speed);

            esp_ble_gatts_send_response(gatts_if, param->write.conn_id,
                                        param->write.trans_id, ESP_GATT_OK, NULL);
        }
        break;
    }

    case ESP_GATTS_READ_EVT: {
        uint16_t handle = param->read.handle;
        if (handle == gatt_char_speed_handle) {
            esp_gatt_rsp_t rsp;
            memset(&rsp, 0, sizeof(esp_gatt_rsp_t));
            rsp.attr_value.len = 1;
            rsp.attr_value.value[0] = s_current_speed;
            esp_ble_gatts_send_response(gatts_if, param->read.conn_id,
                                        param->read.trans_id, ESP_GATT_OK, &rsp);
        }
        break;
    }

    default:
        break;
    }
}

/**
 * @brief 初始化 BLE 管理器
 */
esp_err_t ble_manager_init(void)
{
    esp_err_t ret;

    ESP_LOGI(TAG, "Initializing BLE...");

    // 释放经典蓝牙 RAM
    ret = esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "BT controller mem release: %s", esp_err_to_name(ret));
    }

    // 初始化蓝牙控制器
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "BT controller init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // 启用 BLE 模式
    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "BT controller enable failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // 初始化 Bluedroid
    ret = esp_bluedroid_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Bluedroid init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // 启用 Bluedroid
    ret = esp_bluedroid_enable();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Bluedroid enable failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // 注册 GAP 回调
    ret = esp_ble_gap_register_callback(gap_event_handler);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "GAP callback register failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // 注册 GATT 回调
    ret = esp_ble_gatts_register_callback(gatts_event_handler);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "GATT callback register failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // 注册应用
    ret = esp_ble_gatts_app_register(PROFILE_A_APP_ID);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "App register failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // 设置设备名称
    ret = esp_ble_gap_set_device_name(BLE_DEVICE_NAME);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Set device name failed: %s", esp_err_to_name(ret));
    }

    ESP_LOGI(TAG, "BLE initialized, Device Name: %s", BLE_DEVICE_NAME);
    ESP_LOGI(TAG, "Service UUID: 0x%04X", BLE_SERVICE_UUID);
    ESP_LOGI(TAG, "Command UUID: 0x%04X, Speed UUID: 0x%04X", BLE_CHAR_CMD_UUID, BLE_CHAR_SPEED_UUID);

    return ESP_OK;
}

/**
 * @brief 设置命令回调
 */
void ble_set_command_callback(ble_command_callback_t callback)
{
    s_command_callback = callback;
}
