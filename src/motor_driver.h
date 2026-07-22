/**
 * @file motor_driver.h
 * @brief 泵驱动头文件
 */

#ifndef MOTOR_DRIVER_H
#define MOTOR_DRIVER_H

#include <stdbool.h>
#include "motor_config.h"

/**
 * @brief 初始化泵驱动
 */
void motor_driver_init(void);

/**
 * @brief 设置水泵档位
 * @param level 0=关闭, 1=低速(30%), 2=中速(50%), 3=高速(80%)
 */
void water_pump_set_level(int level);

/**
 * @brief 获取水泵档位
 */
int water_pump_get_level(void);

/**
 * @brief 设置水泵状态
 */
void water_pump_set_state(bool on);

/**
 * @brief 获取水泵状态
 */
bool water_pump_is_on(void);

/**
 * @brief 设置水泵定时模式
 * @param enabled true=启用, false=禁用
 */
void water_pump_set_timer(bool enabled);

/**
 * @brief 获取水泵定时模式状态
 */
bool water_pump_get_timer(void);

/**
 * @brief 获取水泵定时运行状态
 */
bool water_pump_is_timer_running(void);

/**
 * @brief 设置氧气泵档位
 * @param level 0=关闭, 1=低速(30%), 2=中速(50%), 3=高速(80%)
 */
void oxygen_pump_set_level(int level);

/**
 * @brief 获取氧气泵档位
 */
int oxygen_pump_get_level(void);

/**
 * @brief 设置氧气泵状态
 */
void oxygen_pump_set_state(bool on);

/**
 * @brief 获取氧气泵状态
 */
bool oxygen_pump_is_on(void);

/**
 * @brief 设置氧气泵定时模式
 * @param enabled true=启用, false=禁用
 */
void oxygen_pump_set_timer(bool enabled);

/**
 * @brief 获取氧气泵定时模式状态
 */
bool oxygen_pump_get_timer(void);

/**
 * @brief 获取氧气泵定时运行状态
 */
bool oxygen_pump_is_timer_running(void);

/**
 * @brief 设置所有泵的定时模式 (统一开关)
 * @param enabled true=启用, false=禁用
 */
void all_pumps_set_timer(bool enabled);

/**
 * @brief 获取所有泵的定时模式状态
 */
bool all_pumps_get_timer(void);

/**
 * @brief 停止所有泵
 */
void pump_stop_all(void);

#endif // MOTOR_DRIVER_H
