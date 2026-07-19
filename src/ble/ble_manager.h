/**
 * @file ble_manager.h
 * @brief BLE 管理器头文件
 *
 * ESP32-C3 BLE 控制小车轮子
 */

#ifndef BLE_MANAGER_H
#define BLE_MANAGER_H

#include <stdint.h>
#include "esp_err.h"

/**
 * @brief 初始化 BLE 管理器
 */
esp_err_t ble_manager_init(void);

/**
 * @brief BLE 命令类型
 */
typedef enum {
    BLE_CMD_STOP = 0,
    BLE_CMD_FORWARD,
    BLE_CMD_BACKWARD,
    BLE_CMD_LEFT,
    BLE_CMD_RIGHT,
    BLE_CMD_SET_SPEED,
} ble_command_t;

/**
 * @brief 发送 BLE 命令回调类型
 */
typedef void (*ble_command_callback_t)(ble_command_t cmd, uint8_t speed);

void ble_set_command_callback(ble_command_callback_t callback);

#endif // BLE_MANAGER_H
