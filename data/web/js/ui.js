/**
 * UI 模块 - DOM 操作与界面渲染
 */

/**
 * 初始化 UI 事件绑定
 */
export function init() {
    // 速度滑块
    const speedSlider = document.getElementById('speedSlider');
    const speedValue = document.getElementById('speedValue');

    speedSlider?.addEventListener('input', (e) => {
        speedValue.textContent = e.target.value;
    });

    // 阻止控制按钮长按弹出菜单
    document.querySelectorAll('.control-btn').forEach(button => {
        button.addEventListener('contextmenu', e => e.preventDefault());
    });
}

/**
 * Toast 消息提示
 */
export function showToast(message, type = 'info') {
    const toast = document.getElementById('toast');
    if (!toast) return;

    toast.textContent = message;
    toast.className = `toast show ${type}`;

    setTimeout(() => {
        toast.className = 'toast';
    }, 3000);
}

/**
 * 更新 Wi-Fi 状态显示
 */
export function updateWifiUI(data) {
    const wifiDot = document.getElementById('wifiDot');
    const wifiStatus = document.getElementById('wifiStatus');
    const signalStrength = document.getElementById('signalStrength');
    const ipAddress = document.getElementById('ipAddress');
    const ssid = document.getElementById('ssid');

    if (data.connected) {
        wifiStatus.textContent = 'CONNECTED';
        wifiDot?.classList.add('connected');
    } else {
        wifiStatus.textContent = 'DISCONNECTED';
        wifiDot?.classList.remove('connected');
    }

    signalStrength.textContent = `${data.rssi ?? 0} dBm`;
    ipAddress.textContent = data.ip ?? '0.0.0.0';
    ssid.textContent = data.ssid ?? '--';
}

/**
 * 更新内存监控显示
 */
export function updateMemoryUI(data) {
    const freeKB = Math.round((data.free_heap_size ?? 0) / 1024);
    const usedKB = Math.round((data.used_heap ?? 0) / 1024);
    const minFreeKB = Math.round((data.min_free_heap_size ?? 0) / 1024);

    document.getElementById('freeHeap').textContent = `${freeKB} KB`;
    document.getElementById('usedHeap').textContent = `${usedKB} KB`;
    document.getElementById('minFreeHeap').textContent = `${minFreeKB} KB`;
    document.getElementById('minFreeHeapVal').textContent = minFreeKB;

    const freePercent = Math.round(((data.free_heap_size ?? 0) / (data.heap_size ?? 1)) * 100);
    const usedPercent = data.used_percent ?? 0;

    const freeBar = document.getElementById('freeHeapBar');
    const usedBar = document.getElementById('usedHeapBar');

    if (freeBar) {
        freeBar.style.width = `${freePercent}%`;
        freeBar.className = 'memory-bar-fill' + (freePercent < 20 ? ' danger' : freePercent < 40 ? ' warning' : '');
    }

    if (usedBar) {
        usedBar.style.width = `${usedPercent}%`;
        usedBar.className = 'memory-bar-fill' + (usedPercent > 80 ? ' danger' : usedPercent > 60 ? ' warning' : '');
    }

    // 格式化运行时间
    const secs = data.uptime_secs ?? 0;
    const hours = Math.floor(secs / 3600);
    const mins = Math.floor((secs % 3600) / 60);
    const s = secs % 60;

    const uptimeStr = [hours > 0 && `${hours}h`, mins > 0 && `${mins}m`, `${s}s`]
        .filter(Boolean)
        .join(' ');

    document.getElementById('uptime').textContent = uptimeStr;
}

/**
 * 更新当前命令显示
 */
export function updateCommandUI(command) {
    const el = document.getElementById('currentCommand');
    if (el) {
        el.textContent = String(command).toUpperCase();
    }
}
