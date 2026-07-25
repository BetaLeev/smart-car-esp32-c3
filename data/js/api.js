/**
 * @file api.js
 * @brief API 调用封装
 */

const API = {
    /**
     * 发送控制命令
     * @param {string} command - 命令类型: forward, backward, left, right, stop, horn
     * @returns {Promise<object>}
     */
    async sendCommand(command) {
        try {
            const response = await fetch('/api/control', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json'
                },
                body: JSON.stringify({
                    command: command
                })
            });
            return await response.json();
        } catch (error) {
            console.error('Command failed:', error);
            return { success: false, message: error.message };
        }
    },

    /**
     * 获取系统状态
     * @returns {Promise<object>}
     */
    async getStatus() {
        try {
            const response = await fetch('/api/status');
            return await response.json();
        } catch (error) {
            console.error('Status fetch failed:', error);
            return null;
        }
    },

    /**
     * 获取内存信息
     * @returns {Promise<object>}
     */
    async getMemory() {
        try {
            const response = await fetch('/api/memory');
            return await response.json();
        } catch (error) {
            console.error('Memory fetch failed:', error);
            return null;
        }
    },

    /**
     * 获取 Wi-Fi 状态
     * @returns {Promise<object>}
     */
    async getWifiStatus() {
        try {
            const response = await fetch('/api/wifi/status');
            return await response.json();
        } catch (error) {
            console.error('WiFi status fetch failed:', error);
            return null;
        }
    }
};
