/**
 * State 模块 - 应用状态管理
 */

// 私有状态
let _speed = 50;
let _currentCommand = 'stop';
let _wifiConnected = false;

// 状态变更监听器
const _listeners = new Set();

/**
 * 订阅状态变更
 * @param {Function} callback - (key, value) => void
 * @returns {Function} 取消订阅函数
 */
export function subscribe(callback) {
    _listeners.add(callback);
    return () => _listeners.delete(callback);
}

/**
 * 通知所有监听器
 */
function _notify(key, value) {
    _listeners.forEach(cb => cb(key, value));
}

// Getters & Setters
export const state = {
    get speed() { return _speed; },
    set speed(v) {
        _speed = Math.max(0, Math.min(100, Number(v)));
        _notify('speed', _speed);
    },

    get currentCommand() { return _currentCommand; },
    set currentCommand(v) {
        _currentCommand = String(v).toLowerCase();
        _notify('currentCommand', _currentCommand);
    },

    get wifiConnected() { return _wifiConnected; },
    set wifiConnected(v) {
        _wifiConnected = Boolean(v);
        _notify('wifiConnected', _wifiConnected);
    }
};
