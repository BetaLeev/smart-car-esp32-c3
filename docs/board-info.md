# ESP32-C3-DevKitM-1 开发板基本信息

> 本文档整理自 Espressif 官方文档，方便项目开发审查。

## 1. 开发板概述

**ESP32-C3-DevKitM-1** 是乐鑫科技(Espressif)推出的一款入门级开发板，基于 **ESP32-C3-MINI-1** 模组开发。该开发板具备完整的 Wi-Fi 和低功耗蓝牙(Bluetooth LE)功能。

### 1.1 板载模组

| 型号 | 说明 |
|------|------|
| ESP32-C3-MINI-1 | 采用 PCB 板载天线 |
| ESP32-C3-MINI-1U | 采用外部天线连接器 |

两款模组都集成 **ESP32-C3FN4** 芯片和 **4 MB flash**。

### 1.2 核心芯片: ESP32-C3FN4

- **架构**: 32位 RISC-V 单核处理器，四级流水线
- **主频**: 最高 160 MHz
- **CoreMark 分数**: 483.27 CoreMark; 3.02 CoreMark/MHz
- **封装**: QFN32 (5×5 mm)

### 1.3 存储

| 类型 | 大小 |
|------|------|
| ROM | 384 KB |
| SRAM | 400 KB (其中 16 KB 用于 cache) |
| RTC SRAM | 8 KB |
| 封装内 Flash | 4 MB |

## 2. 核心特性

### 2.1 无线通信

#### Wi-Fi
- IEEE 802.11 b/g/n 协议
- 2.4 GHz 频段，支持 20 MHz 和 40 MHz 带宽
- 1T1R 模式，最高 150 Mbps
- 支持 Station、SoftAP、Station + SoftAP 模式
- WPA3 安全支持

#### 蓝牙
- Bluetooth 5.0 (LE)
- 支持 Bluetooth Mesh
- 传输速率: 125 Kbps / 500 Kbps / 1 Mbps / 2 Mbps
- Wi-Fi 与蓝牙共存

### 2.2 外设接口

#### 数字接口
- 3 × SPI
- 2 × UART
- 1 × I2C
- 1 × I2S
- 红外遥控 (2 发射 + 2 接收通道)
- LED PWM 控制器 (6 通道)
- USB Serial/JTAG 控制器
- GDMA (3 发送 + 3 接收通道)
- TWAI 控制器 (兼容 ISO 11898-1)

#### 模拟接口
- 2 × 12 位 SAR ADC (6 通道)
- 温度传感器

#### 定时器
- 2 × 54 位通用定时器
- 3 × 看门狗定时器
- 1 × 52 位系统定时器

### 2.3 安全特性

- 安全启动 (Secure Boot)
- Flash 加密
- 4096 位 eFuse (用户可用 1792 位)
- 硬件加密加速器:
  - AES-128/256 (FIPS PUB 197)
  - SHA 加速器 (FIPS PUB 180-4)
  - RSA 加速器
  - 随机数生成器 (RNG)
  - HMAC
  - 数字签名

### 2.4 功耗

| 模式 | 典型功耗 |
|------|----------|
| Active | 最高 |
| Modem-sleep | 中等 |
| Light-sleep | 低 |
| Deep-sleep | 5 µA |

## 3. 开发板组件

### 3.1 硬件布局

```
+------------------+    +------------------+
|                  |    |                  |
|  ESP32-CINI-1    |    |   Micro-USB      |
|    [ antenna ]   |    |      Port        |
|                  |    |                  |
+------------------+    +------------------+
        |                       |
   +----+----+             +----+----+
   | RGB LED |             | Reset |
   +---------+             | Button|
                          +-------+
```

### 3.2 组件说明

| 组件 | 说明 |
|------|------|
| ESP32-C3-MINI-1 | 核心模组，内置 ESP32-C3FN4 芯片和 4 MB flash |
| 5V→3.3V LDO | 电源转换器，输入 5V，输出 3.3V |
| 5V Power LED | USB 电源连接后亮起 |
| RGB LED | 可寻址 RGB LED，由 GPIO8 驱动 |
| Boot Button | 按住 Boot 再按 Reset 进入下载模式 |
| Reset Button | 复位按键 |
| Micro-USB | 供电和通信接口 |
| USB-UART Bridge | 最高 3 Mbps 传输速率 |

### 3.3 电源选项

- Micro-USB 接口供电 (默认)
- 5V 和 GND 排针供电
- 3V3 和 GND 排针供电

## 4. 引脚定义

### 4.1 排针 J1

| 编号 | 名称 | 类型 | 功能 |
|------|------|------|------|
| 1 | GND | G | 地 |
| 2 | 3V3 | P | 3.3V 电源 |
| 3 | 3V3 | P | 3.3V 电源 |
| 4 | IO2 | I/O/T | GPIO2, ADC1_CH2, FSPIQ |
| 5 | IO3 | I/O/T | GPIO3, ADC1_CH3 |
| 6 | GND | G | 地 |
| 7 | RST | I | CHIP_PU (复位) |
| 8 | GND | G | 地 |
| 9 | IO0 | I/O/T | GPIO0, ADC1_CH0, XTAL_32K_P |
| 10 | IO1 | I/O/T | GPIO1, ADC1_CH1, XTAL_32K_N |
| 11 | IO10 | I/O/T | GPIO10, FSPICS0 |
| 12 | GND | G | 地 |
| 13 | 5V | P | 5V 电源 |
| 14 | 5V | P | 5V 电源 |
| 15 | GND | G | 地 |

### 4.2 排针 J3

| 编号 | 名称 | 类型 | 功能 |
|------|------|------|------|
| 1 | GND | G | 地 |
| 2 | TX | I/O/T | GPIO21, U0TXD |
| 3 | RX | I/O/T | GPIO20, U0RXD |
| 4 | GND | G | 地 |
| 5 | IO9 | I/O/T | GPIO9 (strap) |
| 6 | IO8 | I/O/T | GPIO8, RGB LED (strap) |
| 7 | GND | G | 地 |
| 8 | IO7 | I/O/T | GPIO7, FSPID |
| 9 | IO6 | I/O/T | GPIO6, FSPICLK |
| 10 | IO5 | I/O/T | GPIO5, ADC2_CH0, FSPIWP |
| 11 | IO4 | I/O/T | GPIO4, ADC1_CH4, FSPIHD |
| 12 | GND | G | 地 |
| 13 | IO18 | I/O/T | GPIO18, USB_D- |
| 14 | IO19 | I/O/T | GPIO19, USB_D+ |
| 15 | GND | G | 地 |

> **注意**: 并非所有芯片引脚都引出至排针。

## 5. 项目配置

本项目使用的配置:

| 配置项 | 值 |
|--------|-----|
| 开发板 | ESP32-C3-DevKitM-1 |
| 框架 | ESP-IDF |
| Flash 大小 | 2 MB |
| CPU 频率 | 160 MHz |
| XTAL 频率 | 40 MHz |
| 日志级别 | INFO |
| 编译优化 | Debug |

### 5.1 sdkconfig 关键配置

```
CONFIG_IDF_TARGET="esp32c3"
CONFIG_IDF_TARGET_ESP32C3=y
CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ_160=y
CONFIG_XTAL_FREQ_40=y
CONFIG_BOOTLOADER_CPU_CLK_FREQ_MHZ_80=y
CONFIG_ESPTOOLPY_FLASHSIZE="2MB"
CONFIG_ESPTOOLPY_FLASHFREQ="80m"
CONFIG_ESP32C3_REV_MIN_FULL=3
```

## 6. 典型应用

ESP32-C3 适用于以下场景:

- 智能家居 (灯具控制、智能开关)
- 工业自动化 (工业机器人、Mesh 网络)
- 医疗健康 (健康监测)
- 消费电子 (智能手表、OTT 设备)
- 智慧农业 (智能温室、灌溉)
- 零售餐饮 (POS 机、服务机器人)
- 通用低功耗 IoT 传感器/数据记录器

## 7. 相关资料

### 官方文档
- [ESP32-C3 技术规格书](https://www.espressif.com/sites/default/files/documentation/esp32-c3_datasheet_en.pdf)
- [ESP32-C3-MINI-1 规格书](https://www.espressif.com/sites/default/files/documentation/esp32-c3-mini-1_datasheet_en.pdf)
- [ESP32-C3-DevKitM-1 用户指南](https://docs.espressif.com/projects/esp-idf/zh_CN/latest/esp32c3/hw-reference/esp32c3/user-guide-devkitm-1.html)
- [ESP-IDF 编程指南](https://docs.espressif.com/projects/esp-idf/zh_CN/latest/esp32c3/index.html)

### 开发资源
- [ESP-IDF GitHub](https://github.com/espressif/esp-idf)
- [Arduino ESP32](https://docs.espressif.com/projects/arduino-esp32/)

## 8. 版本信息

| 项目 | 值 |
|------|-----|
| 文档版本 | v1.0 |
| 创建日期 | 2026-07-16 |
| 芯片固件版本 | ESP-IDF v6.0.1 |
| 最低支持芯片版本 | Rev 3.0 |
