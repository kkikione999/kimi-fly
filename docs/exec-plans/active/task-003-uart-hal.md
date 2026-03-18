# Task 003: STM32 UART HAL实现

## 目标
实现STM32 UART硬件抽象层，支持USART2与ESP32-C3 WiFi模块通信。

## 背景上下文

### 相关代码
- 新建文件: `firmware/stm32/hal/uart.h` - UART HAL接口定义
- 新建文件: `firmware/stm32/hal/uart.c` - UART HAL实现
- 依赖文件: `firmware/stm32/hal/hal_common.h` - 错误码定义
- 依赖文件: `firmware/stm32/hal/gpio.h` - GPIO配置

### 依赖关系
- 前置任务: 无 (并行开发，需协调hal_common.h)
- 外部依赖: STM32F4xx标准库

### 硬件信息
- 外设: USART2
- 引脚: PA2 (TX), PA3 (RX)
- 波特率: 115200 (默认)
- 参考: `hardware-docs/pinout.md` 第1.3节

### 代码规范
- 遵循STM32 HAL风格
- 寄存器访问使用volatile
- 支持中断和轮询两种模式

## 具体修改要求

### 文件 1: `firmware/stm32/hal/uart.h`
1. 定义uart_handle_t结构体
2. 定义uart_config_t配置结构体 (波特率、数据位、停止位、校验)
3. 声明API函数:
   - `uart_init()` - 初始化UART
   - `uart_deinit()` - 反初始化
   - `uart_send()` - 发送数据 (阻塞)
   - `uart_receive()` - 接收数据 (阻塞)
   - `uart_send_dma()` - DMA发送 (可选)
   - `uart_receive_dma()` - DMA接收 (可选)
   - `uart_set_baudrate()` - 动态修改波特率

### 文件 2: `firmware/stm32/hal/uart.c`
1. 实现USART2初始化 (GPIO复用、时钟使能、NVIC)
2. 实现基础发送/接收函数
3. 实现中断处理函数
4. 实现错误处理 (帧错误、噪声、溢出)

## 完成标准 (必须可验证)

- [ ] 接口定义完整: 所有声明的函数都有实现
- [ ] 编译通过: 无警告无错误
- [ ] 波特率配置正确: 支持115200/921600
- [ ] GPIO配置正确: PA2/PA3复用为USART2
- [ ] 代码审查通过

## 相关文件 (Hook范围检查)
- `firmware/stm32/hal/uart.h`
- `firmware/stm32/hal/uart.c`
- `firmware/stm32/hal/hal_common.h`
- `firmware/stm32/hal/gpio.h`

## 注意事项
- USART2时钟源为APB1 (42MHz)
- 需要配置GPIO为复用推挽输出
- 中断优先级需要合理设置

## 验收清单 (Reviewer使用)
- [ ] 修改在任务范围内
- [ ] 无新引入的未声明依赖
- [ ] 代码符合HAL风格规范
- [ ] 无新的技术债务
