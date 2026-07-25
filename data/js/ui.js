/**
 * @file ui.js
 * @brief UI 更新管理
 */

const UI = {
    // DOM 元素缓存
    elements: {},

    /**
     * 初始化 UI 元素
     */
    init() {
        // 状态显示
        this.elements.wifiStatus = document.getElementById('wifi-status');
        this.elements.ipAddress = document.getElementById('ip-address');
        this.elements.rssi = document.getElementById('rssi');

        // 档位显示
        this.elements.gearIndicator = document.getElementById('current-gear');
        this.elements.gearText = document.getElementById('gear-text');
        this.elements.gearSlider = document.getElementById('gear-slider');

        // 控制按钮
        this.elements.btnLeft = document.getElementById('btn-left');
        this.elements.btnRight = document.getElementById('btn-right');
        this.elements.btnStop = document.getElementById('btn-stop');
        this.elements.btnHorn = document.getElementById('btn-horn');

        // 风扇控制
        this.elements.fanSwitch = document.getElementById('fan-switch');
        this.elements.fanSlider = document.getElementById('fan-slider');
        this.elements.fanValue = document.getElementById('fan-value');

        // 内存信息
        this.elements.freeHeap = document.getElementById('free-heap');
        this.elements.uptime = document.getElementById('uptime');

        console.log('UI initialized');
    },

    /**
     * 更新档位显示
     * @param {string} gear - N, D, R
     * @param {string} text - 显示文本
     */
    updateGearDisplay(gear, text) {
        if (!this.elements.gearIndicator) return;

        // 移除所有状态类
        this.elements.gearIndicator.classList.remove('reverse', 'neutral', 'drive');
        
        // 添加对应状态类
        const classMap = {
            'R': 'reverse',
            'N': 'neutral',
            'D': 'drive'
        };
        this.elements.gearIndicator.classList.add(classMap[gear]);
        
        // 更新文字
        this.elements.gearIndicator.textContent = gear;
        this.elements.gearText.textContent = text;
    },

    /**
     * 更新档位滑块
     * @param {number} value - 滑块值
     */
    updateGearSlider(value) {
        if (this.elements.gearSlider) {
            this.elements.gearSlider.value = value;
        }
    },

    /**
     * 更新 Wi-Fi 状态
     * @param {object} status
     */
    updateWifiStatus(status) {
        if (!status) return;

        if (this.elements.wifiStatus) {
            const connected = status.connected || status.wifi_connected;
            this.elements.wifiStatus.textContent = connected ? '已连接' : '断开';
            this.elements.wifiStatus.className = 'status-value ' + 
                (connected ? 'connected' : 'disconnected');
        }

        if (this.elements.ipAddress) {
            this.elements.ipAddress.textContent = status.ip || '-';
        }

        if (this.elements.rssi) {
            const rssi = status.rssi || 0;
            this.elements.rssi.textContent = rssi + ' dBm';
        }
    },

    /**
     * 更新内存信息
     * @param {object} memory
     */
    updateMemoryInfo(memory) {
        if (!memory) return;

        if (this.elements.freeHeap) {
            const freeKB = Math.round(memory.free_heap_size / 1024);
            this.elements.freeHeap.textContent = freeKB;
        }

        if (this.elements.uptime) {
            const secs = Math.floor(memory.uptime_secs || 0);
            const hours = Math.floor(secs / 3600);
            const mins = Math.floor((secs % 3600) / 60);
            const s = secs % 60;
            this.elements.uptime.textContent = 
                `${hours.toString().padStart(2, '0')}:${mins.toString().padStart(2, '0')}:${s.toString().padStart(2, '0')}`;
        }
    },

    /**
     * 更新方向按钮状态
     * @param {string|null} direction
     */
    updateDirectionButtons(direction) {
        // 清除所有按钮的激活状态
        [this.elements.btnLeft, this.elements.btnRight].forEach(btn => {
            if (btn) btn.classList.remove('active');
        });

        // 激活对应按钮
        if (direction === 'left' && this.elements.btnLeft) {
            this.elements.btnLeft.classList.add('active');
        } else if (direction === 'right' && this.elements.btnRight) {
            this.elements.btnRight.classList.add('active');
        }
    },

    // ========== 风扇控制 UI ==========

    /**
     * 更新风扇显示
     * @param {boolean} isOn - 风扇是否开启
     * @param {number} speed - 速度值 0-100
     */
    updateFanDisplay(isOn, speed) {
        if (this.elements.fanSwitch) {
            this.elements.fanSwitch.checked = isOn;
        }
        if (this.elements.fanSlider) {
            this.elements.fanSlider.value = speed;
        }
        if (this.elements.fanValue) {
            this.elements.fanValue.textContent = speed;
        }
    }
};
