/**
 * @file main.js
 * @brief 主控制逻辑
 */

// 全局暴露 UI 对象
window.UI = UI;

document.addEventListener('DOMContentLoaded', async () => {
    console.log('ESP32-CAR Control Panel Loaded');

    // 初始化 UI
    UI.init();

    // 绑定事件
    bindEvents();

    // 初始状态更新
    await updateStatus();

    // 启动定时更新
    startStatusPolling();
});

/**
 * 绑定所有事件
 */
function bindEvents() {
    // 档位滑块
    UI.elements.gearSlider?.addEventListener('input', async (e) => {
        const value = parseInt(e.target.value);
        await CarState.setGear(value);
    });

    // 速度滑块
    UI.elements.speedSlider?.addEventListener('input', (e) => {
        const value = parseInt(e.target.value);
        UI.updateSpeedDisplay(value);
        CarState.setSpeed(value);
    });

    // 方向按钮 - 按下
    UI.elements.btnLeft?.addEventListener('mousedown', async (e) => {
        e.preventDefault();
        await CarState.setDirection('left');
        UI.updateDirectionButtons('left');
    });

    UI.elements.btnRight?.addEventListener('mousedown', async (e) => {
        e.preventDefault();
        await CarState.setDirection('right');
        UI.updateDirectionButtons('right');
    });

    // 方向按钮 - 释放（移动端）
    UI.elements.btnLeft?.addEventListener('touchstart', async (e) => {
        e.preventDefault();
        await CarState.setDirection('left');
        UI.updateDirectionButtons('left');
    });

    UI.elements.btnRight?.addEventListener('touchstart', async (e) => {
        e.preventDefault();
        await CarState.setDirection('right');
        UI.updateDirectionButtons('right');
    });

    // 方向按钮 - 松开
    [UI.elements.btnLeft, UI.elements.btnRight].forEach(btn => {
        if (btn) {
            btn.addEventListener('mouseup', async () => {
                await CarState.setDirection(null);
                UI.updateDirectionButtons(null);
            });
            btn.addEventListener('mouseleave', async () => {
                await CarState.setDirection(null);
                UI.updateDirectionButtons(null);
            });
            btn.addEventListener('touchend', async () => {
                await CarState.setDirection(null);
                UI.updateDirectionButtons(null);
            });
            btn.addEventListener('touchcancel', async () => {
                await CarState.setDirection(null);
                UI.updateDirectionButtons(null);
            });
        }
    });

    // 停止按钮
    UI.elements.btnStop?.addEventListener('click', async () => {
        await CarState.stop();
    });

    // 喇叭按钮
    UI.elements.btnHorn?.addEventListener('click', async () => {
        await CarState.honk();
    });

    // 风扇开关 - 暂时禁用
    // UI.elements.fanSwitch?.addEventListener('change', async (e) => {
    //     if (e.target.checked) {
    //         await CarState.turnOnFan(CarState.fanSpeed);
    //     } else {
    //         await CarState.turnOffFan();
    //     }
    // });

    // 风扇速度滑块 - 暂时禁用
    // UI.elements.fanSlider?.addEventListener('input', (e) => {
    //     const value = parseInt(e.target.value);
    //     UI.updateFanDisplay(true, value);
    //     CarState.fanSpeed = value;
    //     // 如果风扇已开启，实时更新速度
    //     if (CarState.fanEnabled) {
    //         CarState.setFanSpeed(value);
    //     }
    // });

    // 阻止默认触摸行为（防止移动端长按弹出菜单）
    document.addEventListener('touchmove', (e) => {
        if (e.target.closest('.gear-slider, .speed-slider, .direction-btn, .fan-slider')) {
            e.preventDefault();
        }
    }, { passive: false });

    console.log('Events bound');
}

/**
 * 更新状态信息
 */
async function updateStatus() {
    try {
        // 获取 Wi-Fi 状态
        const wifiStatus = await API.getWifiStatus();
        if (wifiStatus) {
            UI.updateWifiStatus(wifiStatus);
        }

        // 获取内存信息
        const memory = await API.getMemory();
        if (memory) {
            UI.updateMemoryInfo(memory);
        }
    } catch (error) {
        console.error('Update status failed:', error);
    }
}

/**
 * 启动状态轮询
 */
let statusInterval = null;
function startStatusPolling() {
    // 每 2 秒更新一次状态
    statusInterval = setInterval(updateStatus, 2000);
}

/**
 * 停止状态轮询
 */
function stopStatusPolling() {
    if (statusInterval) {
        clearInterval(statusInterval);
        statusInterval = null;
    }
}

// 页面卸载时清理
window.addEventListener('beforeunload', () => {
    stopStatusPolling();
});
