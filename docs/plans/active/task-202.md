# Task 202: ESP32 WiFi STA 模式开发

## 目标
为 ESP32-C3 添加 WiFi STA 模式连接功能，实现与 `whc` 网络的连接。

## 背景上下文

### 相关代码
- 文件: `firmware/esp32/main/main.c` - 现有 UART 通信代码
- 需要新增 WiFi 和 TCP 服务器功能

### 依赖关系
- 前置任务: 无 (可与 Task 201 并行)
- 外部依赖: ESP-IDF WiFi 库

### 硬件信息
- ESP32-C3 WiFi 模块
- 目标网络: SSID=`whc`, 密码=`12345678`

### 代码规范
- 使用 ESP-IDF API
- FreeRTOS 任务架构
- 错误处理和重连机制

## 具体修改要求

### 文件 1: `firmware/esp32/main/wifi_sta.c` (新建)
1. 实现 WiFi STA 模式初始化
2. 配置连接参数 (SSID=whc, 密码=12345678)
3. 实现连接状态回调
4. 实现自动重连机制
5. 获取并输出 IP 地址

### 文件 2: `firmware/esp32/main/wifi_sta.h` (新建)
1. WiFi STA 初始化函数声明
2. 连接状态枚举定义
3. 配置宏定义

### 文件 3: `firmware/esp32/main/CMakeLists.txt` (修改)
1. 添加 wifi_sta.c 到编译列表
2. 确保包含 WiFi 组件依赖

## 完成标准 (必须可验证)

- [ ] 标准 1: ESP32 成功连接 WiFi 网络
  - 验证方法: 日志显示 "Connected to whc" 和 IP 地址
- [ ] 标准 2: 连接断开后自动重连
  - 验证方法: 模拟断网后观察重连日志
- [ ] 标准 3: 获取有效 IP 地址
  - 验证方法: 日志显示 192.168.x.x 或 10.x.x.x 格式的 IP

## 相关文件 (Hook范围检查用)
- `firmware/esp32/main/wifi_sta.c`
- `firmware/esp32/main/wifi_sta.h`
- `firmware/esp32/main/CMakeLists.txt`

## 注意事项
- WiFi 连接可能需要几秒钟
- 如果 whc 网络不可用，需处理连接失败情况
- 密码是明文存储，这是开发调试用途

## 验收清单 (Reviewer使用)
- [ ] WiFi 初始化代码正确
- [ ] STA 模式配置正确
- [ ] 连接回调处理完善
- [ ] 自动重连机制存在
