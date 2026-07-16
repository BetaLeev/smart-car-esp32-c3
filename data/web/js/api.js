/**
 * API 模块 - 封装所有与 ESP32 通信的 HTTP 请求
 */

const API_BASE = '';

/**
 * 发送控制指令
 * @param {string} command - 命令类型 (forward/backward/left/right/stop/horn)
 * @param {number} speed - 速度值 0-100
 */
export async function sendControl(command, speed) {
    const response = await fetch(`${API_BASE}/api/control`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ command, speed: parseInt(speed) })
    });

    if (!response.ok) {
        throw new Error(`Control failed: ${response.status}`);
    }

    return response.json();
}

/**
 * 获取 Wi-Fi 连接状态
 */
export async function fetchWifiStatus() {
    const response = await fetch(`${API_BASE}/api/wifi/status`);

    if (!response.ok) {
        throw new Error(`WiFi status failed: ${response.status}`);
    }

    return response.json();
}

/**
 * 获取内存监控数据
 */
export async function fetchMemoryInfo() {
    const response = await fetch(`${API_BASE}/api/memory`);

    if (!response.ok) {
        throw new Error(`Memory info failed: ${response.status}`);
    }

    return response.json();
}
