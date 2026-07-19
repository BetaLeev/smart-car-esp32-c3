#!/usr/bin/env python3
"""
ESP32-CAR BLE 控制脚本
用法: python3 ble_control.py [命令]
命令: stop=0, forward=1, backward=2, left=3, right=4
示例: python3 ble_control.py 1  # 前进
"""

import asyncio
import sys
from bleak import BleakClient

# BLE 服务和特征 UUID
DEVICE_NAME = "ESP32-CAR"
SERVICE_UUID = "0000FF00-0000-1000-8000-00805F9B34FB"
CHAR_CMD_UUID = "0000FF01-0000-1000-8000-00805F9B34FB"

# 命令映射
COMMANDS = {
    "stop": 0,
    "forward": 1,
    "backward": 2,
    "left": 3,
    "right": 4,
    "0": 0,
    "1": 1,
    "2": 2,
    "3": 3,
    "4": 4,
}

async def main():
    if len(sys.argv) < 2:
        print("用法: python3 ble_control.py [命令]")
        print("命令: stop=0, forward=1, backward=2, left=3, right=4")
        return

    cmd_str = sys.argv[1]
    if cmd_str not in COMMANDS:
        print(f"未知命令: {cmd_str}")
        return

    cmd = COMMANDS[cmd_str]

    # 扫描设备
    print(f"扫描设备: {DEVICE_NAME}...")
    devices = await BleakScanner.discover(timeout=5.0)

    target_device = None
    for d in devices:
        if d.name and DEVICE_NAME in d.name:
            target_device = d
            break

    if not target_device:
        print(f"未找到设备: {DEVICE_NAME}")
        return

    print(f"找到设备: {target_device.name} ({target_device.address})")
    print(f"发送命令: {cmd} ({cmd_str})")

    try:
        async with BleakClient(target_device.address) as client:
            await client.write_gatt_char(CHAR_CMD_UUID, bytes([cmd]))
            print("命令已发送!")
    except Exception as e:
        print(f"错误: {e}")

if __name__ == "__main__":
    asyncio.run(main())
