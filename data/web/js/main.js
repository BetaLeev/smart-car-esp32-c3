/**
 * 主入口 - 初始化应用并启动轮询
 */

import * as api from './api.js';
import { state } from './state.js';
import * as ui from './ui.js';

/**
 * Toast 消息提示
 */
function toast(message, type = 'info') {
    ui.showToast(message, type);
}

/**
 * 发送控制命令
 */
async function sendCommand(command) {
    ui.updateCommandUI(command);

    try {
        await api.sendControl(command, state.speed);
    } catch {
        toast('信号传输中断', 'error');
    }
}

/**
 * 发送喇叭命令
 */
function sendHornCommand() {
    sendCommand('horn');
}

/**
 * 更新 Wi-Fi 状态
 */
async function updateStatus() {
    try {
        const data = await api.fetchWifiStatus();
        ui.updateWifiUI(data);
        state.wifiConnected = data.connected ?? false;
    } catch {
        state.wifiConnected = false;
    }
}

/**
 * 更新内存监控
 */
async function updateMemory() {
    try {
        const data = await api.fetchMemoryInfo();
        ui.updateMemoryUI(data);
    } catch {
        // 静默失败
    }
}

/**
 * 初始化
 */
function init() {
    // 速度滑块
    const speedSlider = document.getElementById('speedSlider');
    speedSlider?.addEventListener('input', (e) => {
        state.speed = e.target.value;
    });

    // 方向控制按钮
    const buttons = document.querySelectorAll('[data-cmd]');
    buttons.forEach(btn => {
        const cmd = btn.dataset.cmd;

        btn.addEventListener('mousedown', () => sendCommand(cmd));
        btn.addEventListener('touchstart', (e) => {
            e.preventDefault();
            sendCommand(cmd);
        });

        btn.addEventListener('mouseup', () => sendCommand('stop'));
        btn.addEventListener('mouseleave', () => sendCommand('stop'));
        btn.addEventListener('touchend', () => sendCommand('stop'));
        btn.addEventListener('touchcancel', () => sendCommand('stop'));

        btn.addEventListener('contextmenu', e => e.preventDefault());
    });

    // 喇叭按钮
    const hornBtn = document.getElementById('hornBtn');
    hornBtn?.addEventListener('mousedown', () => sendHornCommand());
    hornBtn?.addEventListener('touchstart', (e) => {
        e.preventDefault();
        sendHornCommand();
    });

    // 启动轮询
    updateStatus();
    updateMemory();
    setInterval(updateStatus, 2000);
    setInterval(updateMemory, 3000);
}

// DOM Ready
if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', init);
} else {
    init();
}
