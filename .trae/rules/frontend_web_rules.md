# ESP32-C3 智能小车项目 - 前端 Web 资源规范

## 目的

本规范定义了 ESP32-C3 智能小车项目前端 Web 资源（HTML、CSS、JavaScript）的存放位置、开发规范和部署要求，确保前端代码与 ESP32 固件代码分离，便于维护和更新。

---

## 1. 存储位置规范

### 强制要求

| 资源类型 | 存放路径 | 说明 |
|----------|----------|------|
| HTML | `/data/web/*.html` | 页面入口文件 |
| CSS | `/data/web/*.css` | 样式表文件 |
| JavaScript | `/data/web/*.js` | 脚本文件 |
| 图片资源 | `/data/web/images/*` | 图标、图片等 |
| 字体文件 | `/data/web/fonts/*` | 自定义字体 |
| 其他资源 | `/data/web/*` | JSON 配置文件等 |

### 禁止事项

- **严禁**在 `/src/web/` 目录下放置 HTML、CSS、JavaScript 文件
- **严禁**在 C/C++ 源文件中内嵌 HTML/CSS/JS 代码
- **严禁**使用 `R"rawliteral(...)"` 或类似宏内嵌前端代码

### 目录结构示例

```
smart-car-esp32-c3/
├── data/
│   └── web/                    # 前端资源目录（烧录到 SPIFFS）
│       ├── index.html          # 主页
│       ├── style.css           # 样式表
│       ├── main.js             # 主脚本
│       └── images/
│           └── favicon.ico     # 网站图标
├── src/
│   └── web/                    # C 语言代码目录
│       ├── web_server.c        # Web 服务器实现
│       └── web_server.h        # Web 服务器头文件
```

---

## 2. 文件命名规范

### 命名规则

- 使用小写字母和数字
- 使用下划线 `_` 分隔单词（如 `main_page.html`）
- CSS/JS 文件名应与功能模块对应

### 推荐命名

```
index.html          # 主页入口
style.css           # 全局样式
main.js             # 主逻辑
control_panel.js    # 控制面板逻辑
wifi_status.js      # Wi-Fi 状态模块
```

---

## 3. HTML 开发规范

### 基本结构要求

```html
<!DOCTYPE html>
<html lang="zh-CN">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>智能小车控制面板</title>
    <!-- 外部样式 -->
    <link rel="stylesheet" href="style.css">
</head>
<body>
    <!-- 页面内容 -->

    <!-- 外部脚本（放在 body 末尾） -->
    <script src="main.js"></script>
</body>
</html>
```

### 禁止事项

- 禁止使用 `<style>` 标签内嵌 CSS
- 禁止使用 `<script>` 标签内嵌 JavaScript（业务逻辑）
- 禁止使用内联样式（`style="..."`），应使用外部 CSS 类
- 禁止使用内联事件处理器（`onclick="..."`），应使用 `addEventListener`

### 推荐实践

- 使用语义化 HTML 标签
- 保持结构清晰，便于维护
- 移动端优先的响应式设计

---

## 4. CSS 开发规范

### 文件组织

- 使用**外部样式表**（`.css` 文件）
- 不使用 `<style>` 标签或内联样式
- 按功能模块组织 CSS 类名

### 推荐类名命名（语义化）

```css
/* 推荐：语义化命名 */
.status-card {}
.control-panel {}
.speed-slider {}

/* 避免：无意义命名 */
.box1 {}
.div-abc {}
```

### ESP32 内存考虑

| 规范 | 说明 |
|------|------|
| 减少选择器复杂度 | 避免深层嵌套（超过 3 层） |
| 合并重复规则 | 减少 CSS 文件大小 |
| 避免过度使用 flex/grid | 复杂布局会增加解析时间 |
| 压缩 CSS | 生产环境使用压缩版本 |

---

## 5. JavaScript 开发规范

### 文件组织

- 业务逻辑放在 `.js` 文件中
- 不使用 `<script>` 标签内嵌脚本
- 模块化组织代码

### API 调用规范

```javascript
// 推荐：封装 API 调用
async function sendCommand(command, speed) {
    try {
        const response = await fetch('/api/control', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ command, speed })
        });
        return await response.json();
    } catch (error) {
        console.error('Command failed:', error);
        return null;
    }
}

// 推荐：状态轮询
async function updateStatus() {
    try {
        const response = await fetch('/api/wifi/status');
        const data = await response.json();
        // 更新 UI
    } catch (error) {
        // 静默失败，不影响用户体验
    }
}

// 定时更新
updateStatus();
setInterval(updateStatus, 2000);
```

### ESP32 内存考虑

| 规范 | 说明 |
|------|------|
| 避免创建大量临时对象 | ESP32-C3 堆内存有限（约 300KB） |
| 及时释放不再使用的大数组 | 避免内存泄漏 |
| 减少 DOM 操作 | 使用事件委托 |
| 避免复杂计算 | 简化 JavaScript 逻辑 |

### 禁止事项

- 禁止使用 `eval()`
- 禁止使用 `document.write()`
- 禁止使用大型第三方库（如 jQuery、Lodash）
- 禁止无限递归或无限循环

---

## 6. API 接口规范

### 前端必须调用的 API

| 接口 | 方法 | 路径 | 说明 |
|------|------|------|------|
| 控制命令 | POST | `/api/control` | 发送小车控制命令 |
| Wi-Fi 状态 | GET | `/api/wifi/status` | 获取连接状态 |
| 内存监控 | GET | `/api/memory` | 获取堆内存信息 |

### 请求/响应格式

```javascript
// POST /api/control
// 请求体
{ "command": "forward", "speed": 50 }

// 响应
{ "success": true, "message": "Command executed" }

// GET /api/wifi/status
// 响应
{
    "connected": true,
    "ssid": "MyNetwork",
    "ip": "192.168.1.100",
    "rssi": -65,
    "state": "connected"
}
```

---

## 7. 性能优化建议

### 文件大小限制

| 文件类型 | 开发版建议大小 | 生产版建议大小 |
|----------|----------------|----------------|
| HTML | < 10KB | < 5KB |
| CSS | < 15KB | < 8KB |
| JavaScript | < 20KB | < 10KB |
| 单个图片 | < 50KB | < 30KB |

### 优化措施

- 合并多个 CSS/JS 文件
- 压缩图片资源
- 使用 CSS 变量减少重复代码
- 延迟加载非关键资源

---

## 8. 部署说明

### SPIFFS 文件系统

前端资源通过 SPIFFS 文件系统部署到 ESP32-C3 的 Flash 中：

1. 将前端文件放入 `/data/web/` 目录
2. 使用 `idf.py partition-table` 确保 SPIFFS 分区足够大
3. 使用 `idf.py app-flash` 烧录时自动包含 SPIFFS 数据

### 分区配置示例

在 `partitions.csv` 中确保有 SPIFFS 分区：

```csv
# Name,   Type, SubType, Offset,  Size, Flags
nvs,      data, nvs,     0x9000,  0x5000,
spiffs,   data, spiffs,  0xe000,  0x20000,
```

### 验证部署

烧录后通过浏览器访问 ESP32 的 IP 地址验证页面加载。

---

## 9. 开发工作流

```
┌─────────────────────────────────────────────────────────┐
│                    前端开发流程                          │
├─────────────────────────────────────────────────────────┤
│  1. 编辑 /data/web/ 下的 HTML/CSS/JS 文件                │
│  2. 本地浏览器测试（可选：启动本地 HTTP 服务器）          │
│  3. 将文件复制到 /data/web/ 目录                        │
│  4. 烧录固件：idf.py flash                              │
│  5. 通过浏览器访问 ESP32 Web 界面                        │
└─────────────────────────────────────────────────────────┘
```

### 本地测试（可选）

```bash
# 在 data/web 目录启动本地 HTTP 服务器
cd data/web
python3 -m http.server 8080

# 浏览器访问 http://localhost:8080
```

---

## 10. 检查清单

开发完成后，请确认以下事项：

- [ ] 所有 HTML/CSS/JS 文件位于 `/data/web/` 目录
- [ ] C/C++ 代码中无内嵌的前端代码
- [ ] HTML 使用外部 CSS 和 JS 文件
- [ ] API 调用格式正确
- [ ] 移动端适配正常
- [ ] 页面加载时间可接受
