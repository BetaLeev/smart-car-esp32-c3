/**
 * @file web_server.h
 * @brief Web 服务器头文件
 */

#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <stdint.h>
#include "esp_err.h"

/**
 * @brief 命令类型
 */
typedef enum {
    CMD_NONE = 0,
    CMD_FORWARD,
    CMD_BACKWARD,
    CMD_TURN_LEFT,
    CMD_TURN_RIGHT,
    CMD_STOP,
    CMD_SPEED_SET
} command_type_t;

/**
 * @brief 小车状态
 */
typedef struct {
    uint8_t speed;
    command_type_t command;
    uint8_t battery;
} car_status_t;

/**
 * @brief 初始化 Web 服务器
 * @param port 端口号
 * @return ESP_OK 成功, 其他 失败
 */
esp_err_t web_server_init(uint16_t port);

/**
 * @brief 启动 Web 服务器
 * @return ESP_OK 成功, 其他 失败
 */
esp_err_t web_server_start(void);

/**
 * @brief 停止 Web 服务器
 */
void web_server_stop(void);

/**
 * @brief 获取当前小车状态
 * @return car_status_t* 状态指针
 */
car_status_t* web_get_car_status(void);

#endif // WEB_SERVER_H
