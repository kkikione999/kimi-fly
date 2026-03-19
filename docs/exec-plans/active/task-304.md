# Task 304: STM32-ESP32 UART 通信硬件连接诊断与软件补偿

## 目标
诊断 STM32-ESP32 UART 双向通信 RX=0 问题，通过软件手段验证并添加回环测试功能。

## 背景上下文

### 相关代码
- 文件: `firmware/stm32/main/uart_comm_test.c` - UART 通信测试代码
- 文件: `firmware/stm32/hal/uart.c` - UART HAL 实现
- 文件: `firmware/stm32/comm/protocol.c` - 通信协议

### 问题说明
**当前现象**: STM32↔ESP32 双向 RX=0，没有数据收发。

**硬件连接 (应为):**
- STM32 PA2 (USART2_TX) → ESP32 GPIO4 (UART0_RX)
- STM32 PA3 (USART2_RX) ← ESP32 GPIO5 (UART0_TX)
- 共地 (GND)

**可能原因:**
1. TX/RX 未交叉连接（最可能）
2. GPIO 引脚配置错误
3. 波特率不匹配
4. 软件 UART 初始化问题

**注意**: 物理连接问题需要人工检查硬件，但可以通过软件回环测试先确认软件层正常。

### 依赖关系
- 前置任务: 无
- 外部依赖: 无

### 硬件信息
- STM32 USART2: PA2 (TX), PA3 (RX), 115200 baud, 8N1
- ESP32 UART0: GPIO4 (RX), GPIO5 (TX), 115200 baud
- 参考: `hardware-docs/pinout.md` 第 3.5 节

## 具体修改要求

### 文件 1: `firmware/stm32/main/uart_comm_test.c`
1. 添加 STM32 UART 软件自回环测试函数 `uart_loopback_test()`:
   - 通过调试串口 (USART1) 发送测试指令触发
   - 从 USART2 发送固定字节序列
   - 读取 USART2 RX 缓冲（需要物理回环线或等待 ESP32 回响）
   - 输出测试结果到调试串口

2. 添加 GPIO 状态诊断函数 `uart_gpio_diag()`:
   - 读取并打印 PA2/PA3 的 GPIO 配置（模式、AF 编号）
   - 打印 USART2 控制寄存器状态
   - 打印 USART2 波特率寄存器计算值

3. 修改主测试函数，先运行 GPIO 诊断，再运行通信测试

### 文件 2: `firmware/stm32/hal/uart.h`
1. 添加 UART GPIO 诊断函数声明:
   ```c
   void uart_print_config(uart_handle_t *huart);
   ```

## 完成标准 (必须可验证)

- [ ] 标准 1: GPIO 诊断函数能输出 PA2/PA3 配置
  - 验证方法: 调试串口打印 "PA2: AF7, PA3: AF7"（AF7=USART2）
- [ ] 标准 2: USART2 波特率寄存器配置正确
  - 验证方法: 打印 BRR 值计算出的波特率约为 115200
- [ ] 标准 3: 调试文档说明硬件连接检查步骤
  - 验证方法: 代码注释说明 TX/RX 交叉连接要求
- [ ] 代码审查通过

## 相关文件 (Hook范围检查用)
- `firmware/stm32/main/uart_comm_test.c`
- `firmware/stm32/hal/uart.h`

## 注意事项
- **硬件连接检查清单** (人工确认，AI无法自动执行):
  - [ ] STM32 PA2 (TX) 接 ESP32 GPIO4 (RX)，非 GPIO5
  - [ ] STM32 PA3 (RX) 接 ESP32 GPIO5 (TX)，非 GPIO4
  - [ ] GND 已共地
  - [ ] 线路无断路（万用表导通测试）
- 物理连接问题无法通过代码修复，本任务专注软件层诊断工具
- 若 GPIO 配置打印正确，则问题在物理层

## 验收清单 (Reviewer使用)
- [ ] 诊断函数能打印有用的配置信息
- [ ] 注释清晰说明物理连接要求
- [ ] 无新引入的未声明依赖
- [ ] 不修改现有通信逻辑
