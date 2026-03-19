# Task 205: ESP32 TCP 服务器与控制命令处理

## 目标
在 ESP32 上实现 TCP 服务器，接收地面站控制命令并转发给 STM32。

## 背景上下文

### 相关代码
- 文件: `firmware/esp32/main/main.c` - 现有 UART 通信代码
- 文件: `firmware/esp32/main/wifi_sta.c` - WiFi 连接 (Task 202)
- 文件: `firmware/stm32/comm/protocol.c` - 协议格式定义

### 依赖关系
- 前置任务: Task 202 (WiFi 连接)
- 外部依赖: ESP-IDF socket API

### 代码规范
- 使用 BSD socket API
- FreeRTOS 任务架构
- 二进制协议转发

## 具体修改要求

### 文件 1: `firmware/esp32/main/tcp_server.c` (新建)
1. 创建 TCP 服务器 socket
2. 绑定端口 (建议 8080)
3. 监听连接
4. 处理客户端连接
5. 接收数据并转发给 STM32 (通过 UART)
6. 接收 STM32 数据并转发给客户端

### 文件 2: `firmware/esp32/main/tcp_server.h` (新建)
1. TCP 服务器初始化函数声明
2. 端口配置宏
3. 任务优先级定义

### 文件 3: `firmware/esp32/main/main.c` (修改)
1. 添加 WiFi 和 TCP 服务器初始化调用
2. 确保 TCP 服务器在 WiFi 连接后启动

## 完成标准 (必须可验证)

- [ ] 标准 1: TCP 服务器成功启动并监听
  - 验证方法: 日志显示 "TCP server started on port 8080"
- [ ] 标准 2: 地面站可以连接 TCP 服务器
  - 验证方法: 使用 netcat 或 telnet 连接成功
- [ ] 标准 3: 命令可正确转发到 STM32
  - 验证方法: 发送数据后 STM32 调试串口有响应
- [ ] 标准 4: STM32 响应可正确返回地面站
  - 验证方法: 双向数据转发验证

## 相关文件 (Hook范围检查用)
- `firmware/esp32/main/tcp_server.c`
- `firmware/esp32/main/tcp_server.h`
- `firmware/esp32/main/main.c`

## 注意事项
- TCP 服务器应在 WiFi 连接成功后启动
- 需处理客户端断连和重连
- 数据转发使用与 STM32 相同的二进制协议

## 验收清单 (Reviewer使用)
- [ ] Socket 创建和配置正确
- [ ] 端口绑定和监听正确
- [ ] 客户端连接处理完善
- [ ] 数据转发逻辑正确
