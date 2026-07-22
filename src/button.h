/**
 * @file button.h
 * @brief 物理按键控制头文件
 */

#ifndef BUTTON_H
#define BUTTON_H

#include <stdbool.h>

/**
 * @brief 初始化按键
 */
void button_init(void);

/**
 * @brief 获取当前控制目标
 * @return false=水泵, true=氧气泵
 */
bool button_get_target(void);

/**
 * @brief 按键控制任务
 */
void button_task(void *pvParameters);

#endif // BUTTON_H
