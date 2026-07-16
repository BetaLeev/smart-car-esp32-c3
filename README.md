# ESP32-C3 智能小车控制项目

基于 ESP32-C3-MINI-1 的智能小车控制系统，支持 Wi-Fi AP 模式下的 Web 控制界面。

## 目录

- [项目概述](#项目概述)
- [硬件要求](#硬件要求)
- [软件架构](#软件架构)
- [引脚配置](#引脚配置)
- [快速开始](#快速开始)
- [API 文档](#api-文档)
- [Web 控制界面](#web-控制界面)
- [故障排除](#故障排除)

## 项目概述

本项目实现了一个可通过浏览器控制的智能小车系统：

- **Wi-Fi AP 模式**：小车作为热点，可直接连接控制
- **Web 控制界面**：响应式设计，支持手机和电脑控制
- **双向电机控制**：支持前进、后退、左转、右转、停止
- **实时状态反馈**：显示连接状态、IP 地址等信息

### 系统架构

```
┌─────────────────┐     Wi-Fi      ┌─────────────────┐
│   手机/电脑     │◄──────────────►│   ESP32-C3      │
│   浏览器        │                │   小车主控       │
└─────────────────┘                └────────┬────────┘
                                            │
                        ┌───────────────────┼───────────────────┐
                        │                   │                   │
                   ┌────▼────┐        ┌────▼────┐        ┌────▼────┐
                   │ 电机驱动 │        │  LED    │        │ Wi-Fi   │
                   │ DRV8833  │        │  指示灯  │        │ 天线    │
                   └──────────┘        └─────────┘        └─────────┘
```

## 硬件要求

### 核心组件

| 组件 | 型号/规格 | 数量 |
|------|---------|------|
| 主控芯片 | ESP32-C3-MINI-1 | 1 |
| 电机驱动 | DRV8833 或 TB6612FNG | 1 |
| 直流电机 | 3-6V 减速电机 | 2 |
| 电机轮子 | 兼容 TT 电机 | 2 |
| 电池 | 3.7V LiPo 或 4xAA | 1 |
| 面包板/PCB | - | 1 |

### 可选组件

| 组件 | 用途 |
|------|------|
| LED | 状态指示灯 |
| 电容 100μF | 电机去耦 |
| 跳线 | 连接布线 |

### ESP32-C3-MINI-1 引脚限制

**禁止使用的 GPIO：**
- GPIO 6-11：连接内部 SPI Flash
- GPIO 12-18：连接 USB Serial/JTAG
- GPIO 19-20：连接 USB D+/D-

**可用 GPIO：**
- GPIO 0-5
- GPIO 21-22

## 软件架构

```
src/
├── main.c              # 主程序入口
├── motor_driver.c/h    # 电机驱动控制
├── motor_config.h      # 电机引脚配置
├── wifi/
│   ├── wifi_manager.c  # Wi-Fi 管理实现
│   └── wifi_manager.h  # Wi-Fi 管理接口
└── web/
    ├── web_server.c    # Web 服务器实现
    └── web_server.h    # Web 服务器接口

components/
└── cjson/              # JSON 解析库

data/web/
├── index.html          # Web 界面（已嵌入）
├── main.js             # 前端脚本（已嵌入）
└── style.css           # 样式表（已嵌入）
```

### 核心模块

1. **motor_driver**：电机驱动控制，支持 DRV8833 和 TB6612FNG
2. **wifi_manager**：Wi-Fi AP 模式管理
3. **web_server**：HTTP 服务器，处理控制命令

## 引脚配置

### 默认配置（DRV8833）

| 功能 | GPIO | 说明 |
|------|------|------|
| 电机 A 正转 | GPIO 4 | IN1 |
| 电机 A 反转 | GPIO 5 | IN2 |
| 电机 B 正转 | GPIO 2 | IN1 |
| 电机 B 反转 | GPIO 3 | IN2 |
| 状态 LED | GPIO 1 | 用户指示 |

### TB6612FNG 配置

修改 `motor_config.h`：

```c
// 注释掉 DRV8833
// #define USE_DRV8833

// 启用 TB6612FNG
#define USE_TB6612FNG
```

| 功能 | GPIO | 说明 |
|------|------|------|
| 电机使能 (STBY) | GPIO 20 | 高电平使能 |
| 左电机正转 | GPIO 4 | IN1 |
| 左电机反转 | GPIO 5 | IN2 |
| 右电机正转 | GPIO 2 | IN1 |
| 右电机反转 | GPIO 3 | IN2 |

### DRV8833 电机控制逻辑

| IN1 | IN2 | 电机状态 |
|-----|-----|---------|
| 0 | 0 | 停止 |
| 1 | 0 | 正转 |
| 0 | 1 | 反转 |
| 1 | 1 | 制动 |

## 快速开始

### 环境要求

- ESP-IDF 6.0+
- Python 3.8+
- 支持的操作系统：Linux, macOS, Windows

### 构建步骤

```bash
# 1. 克隆项目并进入目录
cd smart-car-esp32-c3

# 2. 设置 ESP-IDF 环境
source $IDF_PATH/export.sh

# 3. 配置项目（使用默认配置）
idf.py set-target esp32c3

# 4. 编译项目
idf.py build

# 5. 烧录固件
idf.py -p /dev/ttyUSB0 flash monitor
```

### 首次使用

1. 上电后，小车自动创建 Wi-Fi 热点
2. 使用手机或电脑连接 Wi-Fi：**ESP32-CAR**（无密码）
3. 打开浏览器访问：`http://192.168.4.1`
4. 使用控制界面操作小车

## API 文档

### REST API 端点

#### 1. 获取系统状态
```
GET /api/status
```

**响应示例：**
```json
{
    "ip": "192.168.4.1",
    "ssid": "ESP32-CAR",
    "wifi_connected": true,
    "rssi": -45,
    "speed": 50,
    "battery": 100,
    "command": "forward"
}
```

#### 2. 发送控制命令
```
POST /api/control
Content-Type: application/json

{
    "command": "forward",
    "speed": 50
}
```

**可用命令：**
| 命令 | 说明 |
|------|------|
| `forward` | 前进 |
| `backward` | 后退 |
| `left` | 左转（原地） |
| `right` | 右转（原地） |
| `stop` | 停止 |

**响应示例：**
```json
{
    "success": true,
    "message": "Command executed"
}
```

#### 3. Wi-Fi 连接
```
POST /api/wifi/connect
Content-Type: application/json

{
    "ssid": "MyWiFi",
    "password": "mypassword"
}
```

**响应示例：**
```json
{
    "success": true,
    "message": "Connecting..."
}
```

#### 4. Wi-Fi 状态
```
GET /api/wifi/status
```

**响应示例：**
```json
{
    "connected": true,
    "ssid": "ESP32-CAR",
    "ip": "192.168.4.1",
    "rssi": 0,
    "state": "connected"
}
```

## Web 控制界面

### 界面功能

1. **状态显示区**
   - Wi-Fi 连接状态（绿色=已连接，红色=断开）
   - 当前 IP 地址
   - 当前 SSID

2. **网络配置区**
   - Wi-Fi SSID 输入框
   - Wi-Fi 密码输入框
   - 连接按钮

3. **动力控制区**
   - 速度滑块（0-100%）
   - 当前速度显示

4. **姿态控制区**
   - 十字方向键
   - 停止按钮
   - 当前命令显示

### 移动端适配

- 触摸友好的按钮设计
- 长按方向键持续移动
- 松开后自动停止
- 防止长按弹出系统菜单

## 故障排除

### 常见问题

#### 1. Wi-Fi 无法连接
**症状**：设备列表中看不到 ESP32-CAR

**解决方案：**
- 检查电源是否正常
- 按下复位按钮重置 ESP32-C3
- 确认距离在 Wi-Fi 范围内（<10m）

#### 2. Web 界面无法打开
**症状**：浏览器显示"无法访问此网站"

**解决方案：**
- 确认已连接到 ESP32-CAR 的 Wi-Fi
- 尝试访问 `http://192.168.4.1`
- 检查是否有防火墙阻止

#### 3. 电机不转动
**症状**：控制界面显示正常，但电机不动

**解决方案：**
- 检查电机驱动模块供电（3.3V 或 5V）
- 检查电机接线是否正确
- 确认 DRV8833 或 TB6612FNG 模块正常工作
- 检查 GPIO 引脚配置是否正确

#### 4. 电机转向与预期相反
**症状**：按"前进"时小车后退

**解决方案：**
- 交换同一侧电机的两个接线
- 或在代码中修改电机正反方向定义

#### 5. 编译错误
**症状**：`idf.py build` 失败

**解决方案：**
```bash
# 清理并重新编译
idf.py fullclean
idf.py build
```

#### 6. 烧录失败
**症状**：烧录时提示错误

**解决方案：**
- 检查 USB 线是否支持数据传输
- 确认正确的串口号
- 按住 BOOT 按钮同时按 RESET 进入下载模式

### 日志输出

通过串口监视器查看运行日志：

```bash
idf.py monitor
```

**关键日志标签：**
| 标签 | 模块 |
|------|------|
| `MAIN` | 主程序 |
| `MOTOR_DRIVER` | 电机驱动 |
| `WIFI` | Wi-Fi 管理 |
| `WEB` | Web 服务器 |

### 调试技巧

1. **启用详细日志：**
   修改 `sdkconfig`：
   ```
   CONFIG_LOG_DEFAULT_LEVEL_DEBUG=y
   ```

2. **禁用优化：**
   ```
   CONFIG_COMPILER_OPTIMIZATION_DEBUG=y
   ```

3. **查看 Wi-Fi 状态：**
   ```bash
   # 连接后查看分配的 IP
   192.168.4.x
   ```

## 扩展功能

### 后续可添加的功能

- [ ] PWM 电机速度控制
- [ ] 电池电压监测
- [ ] 超声波避障
- [ ] 巡线传感器
- [ ] 蓝牙控制
- [ ] OTA 无线更新
- [ ] 多小车控制

## 许可证

本项目基于 MIT 许可证开源。

## 参考资料

- [ESP-IDF 编程指南](https://docs.espressif.com/projects/esp-idf/zh_CN/latest/)
- [ESP32-C3 技术参考手册](https://www.espressif.com/sites/default/files/documentation/esp32-c3_technical_reference_manual_cn.pdf)
- [DRV8833 数据手册](https://www.ti.com/product/DRV8833)
- [TB6612FNG 数据手册](https://www.sparkfun.com/datasheets/Robotics/TB6612FNG.pdf)
