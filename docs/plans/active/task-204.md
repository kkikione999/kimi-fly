# Task 204: STM32-ESP32 串口通信验证

## 目标
验证 STM32 与 ESP32-C3 之间的 UART 双向通信正常，数据帧格式正确。

## 背景上下文

### 相关代码
- 文件: `firmware/stm32/comm/protocol.c/h` - 通信协议实现
- 文件: `firmware/stm32/comm/wifi_command.c/h` - WiFi 命令处理器
- 文件: `firmware/stm32/hal/uart.c/h` - UART HAL 层
- 文件: `firmware/esp32/src/uart_bridge.cpp` - ESP32 UART 处理

### 依赖关系
- 前置任务: Task 201 (STM32 烧录), Task 202 (ESP32 烧录)
- 外部依赖: 无

### 硬件信息
- **STM32 USART2**: PA2 (TX), PA3 (RX), 115200 baud
- **ESP32 UART**: 默认 UART0, 115200 baud
- 参考: `hardware-docs/pinout.md` 第 3.5 节

### 通信协议
- 帧格式: [HEADER:2][LEN:1][CMD:1][DATA:N][CRC:2]
- HEADER: 0xAA 0x55
- CRC: CRC16-CCITT
- 波特率: 115200, 8N1

## 具体修改要求

### 文件 1: `firmware/stm32/comm/protocol.c` (修改或验证)
1. 验证帧打包函数 `protocol_pack_frame()` 正确
2. 验证帧解析函数 `protocol_parse_byte()` 正确
3. 验证 CRC16-CCITT 计算正确
4. 添加通信测试模式 (回环测试)

### 文件 2: `firmware/stm32/main/flight_main.c` (修改)
1. 添加串口通信测试初始化
2. 实现心跳包发送 (1Hz)
3. 实现命令接收和响应
4. 添加通信状态指示

### 文件 3: `tools/ground_station/ground_station.py` (修改或验证)
1. 验证协议解析与固件一致
2. 添加串口直连测试模式 (绕过 WiFi)
3. 添加通信状态显示

## 完成标准 (必须可验证)

- [ ] 标准 1: 物理连接正确
  - 验证方法: 万用表测量 PA2-PA3 与 ESP32 对应引脚连通
- [ ] 标准 2: 波特率配置一致
  - 验证方法: STM32 和 ESP32 均配置为 115200, 8N1
- [ ] 标准 3: 双向数据收发正常
  - 验证方法: STM32 发送心跳包，ESP32 正确接收并转发
  - 验证方法: ESP32 发送命令，STM32 正确解析并响应
- [ ] 标准 4: 协议帧格式正确
  - 验证方法: 逻辑分析仪或串口助手捕获数据，验证帧头 0xAA 0x55 和 CRC 正确
- [ ] 标准 5: 命令响应正常
  - 验证方法: 发送 ARM 命令，STM32 返回确认帧并进入 ARMED 状态

## 相关文件 (Hook范围检查用)
- `firmware/stm32/comm/protocol.c`
- `firmware/stm32/comm/protocol.h`
- `firmware/stm32/main/flight_main.c`
- `tools/ground_station/ground_station.py`

## 注意事项
- STM32 TX 接 ESP32 RX，STM32 RX 接 ESP32 TX (交叉连接)
- 共地连接必须可靠
- 如通信失败，先检查波特率和电平匹配 (均为 3.3V)
- 可使用 USB-TTL 模块单独测试 STM32 或 ESP32 串口

## 验收清单 (Reviewer使用)
- [ ] 修改在任务范围内
- [ ] 无新引入的未声明依赖
- [ ] 协议实现与文档一致
- [ ] 双向通信验证通过
