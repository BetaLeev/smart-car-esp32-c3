/**
 * @file state.js
 * @brief 小车状态管理
 */

const CarState = {
    // 当前状态
    gear: 'N',           // N, D, R
    speed: 50,
    direction: null,     // null, 'left', 'right'
    isMoving: false,

    // 风扇状态 - 暂时禁用
    // fanEnabled: false,
    // fanSpeed: 50,

    // 档位映射
    gearMap: {
        '-1': { command: 'backward', label: 'R', text: '倒挡' },
        '0':  { command: 'stop',     label: 'N', text: '空挡' },
        '1':  { command: 'forward',  label: 'D', text: '前进' }
    },

    /**
     * 设置档位
     * @param {number} value - 滑块值: -1, 0, 1
     */
    async setGear(value) {
        const gearInfo = this.gearMap[value];
        if (!gearInfo) return;

        this.gear = gearInfo.label;
        
        // 发送命令
        const result = await API.sendCommand(gearInfo.command, this.speed);
        
        if (result.success) {
            this.isMoving = gearInfo.command !== 'stop';
            // 通知 UI 更新
            if (window.UI) {
                window.UI.updateGearDisplay(this.gear, gearInfo.text);
            }
        }
        
        return result;
    },

    /**
     * 设置速度（占空比）
     * @param {number} value - 速度值 0-100
     */
    async setSpeed(value) {
        this.speed = value;
        
        // 如果正在移动，重新发送当前档位命令
        if (this.isMoving && this.gear !== 'N') {
            const command = this.gear === 'D' ? 'forward' : 'backward';
            await API.sendCommand(command, this.speed);
        }
    },

    /**
     * 设置方向
     * @param {string|null} direction - 'left', 'right', null
     */
    async setDirection(direction) {
        // 如果在空挡，不响应方向控制
        if (this.gear === 'N') return;

        this.direction = direction;
        
        if (direction === null) {
            // 恢复到当前档位
            const command = this.gear === 'D' ? 'forward' : 'backward';
            await API.sendCommand(command, this.speed);
        } else {
            await API.sendCommand(direction, this.speed);
        }
    },

    /**
     * 停止
     */
    async stop() {
        this.direction = null;
        await API.sendCommand('stop', 0);
        
        // 更新档位为空挡
        this.gear = 'N';
        this.isMoving = false;
        
        if (window.UI) {
            window.UI.updateGearDisplay('N', '空挡');
            window.UI.updateGearSlider(0);
        }
    },

    /**
     * 按喇叭
     */
    async honk() {
        await API.sendCommand('horn', 0);
    },

    // ========== 风扇控制 - 暂时禁用 ==========
    /*
    async turnOnFan(speed) {
        const result = await API.sendCommand('fan_on', speed);
        if (result.success) {
            this.fanEnabled = true;
            this.fanSpeed = speed;
            if (window.UI) {
                window.UI.updateFanDisplay(true, speed);
            }
        }
        return result;
    },

    async turnOffFan() {
        const result = await API.sendCommand('fan_off', 0);
        if (result.success) {
            this.fanEnabled = false;
            if (window.UI) {
                window.UI.updateFanDisplay(false, 0);
            }
        }
        return result;
    },

    async setFanSpeed(speed) {
        const result = await API.sendCommand('fan_speed', speed);
        if (result.success) {
            this.fanSpeed = speed;
        }
        return result;
    }
    */
};
