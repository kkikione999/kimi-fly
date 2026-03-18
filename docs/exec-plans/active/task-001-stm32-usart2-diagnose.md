# Task 001: STM32 USART2 配置诊断

## 目标
诊断STM32 USART2 TX不输出数据的根本原因，检查GPIO配置、时钟使能、波特率设置。

## 背景上下文

### 相关代码
- 文件: `/firmware/stm32/hal/uart.c` - UART HAL实现
  - `uart_gpio_init()`: GPIO初始化，PA2使用GPIO_AF7_USART2
  - `uart_init()`: USART2时钟使能在APB1总线
- 文件: `/firmware/stm32/main/uart_comm_test.c` - 测试程序
  - 系统时钟配置: APB1 = 50MHz
  - USART2波特率: 115200

### 硬件信息
- 涉及引脚: PA2 (USART2_TX), PA3 (USART2_RX)
- 外设: USART2
- 参考: `docs/UART_COMMUNICATION.md`

### 已知问题
根据测试报告，已排除的问题:
1. STM32程序卡死 - 已修复
2. ESP32发送功能 - 正常

待排查问题:
1. PA2 GPIO复用功能配置是否正确
2. USART2时钟是否已使能
3. 波特率计算是否正确 (BRR = 50MHz / 115200 = 434.027)
4. USART2发送使能位是否设置

## 具体修改要求

### 文件 1: `/firmware/stm32/main/uart_comm_test.c`
1. 在 `wifi_uart_init()` 函数后添加USART2状态检查代码
2. 添加调试输出，打印以下寄存器值:
   - `USART2->SR` (状态寄存器)
   - `USART2->CR1` (控制寄存器1)
   - `USART2->BRR` (波特率寄存器)
   - `GPIOA->MODER` (GPIO模式寄存器)
   - `GPIOA->AFR[0]` (GPIO复用功能寄存器)
   - `RCC->APB1ENR` (APB1时钟使能寄存器)
3. 添加简单的GPIO翻转测试，验证PA2引脚物理连接

### 文件 2: `/firmware/stm32/hal/uart.c`
1. 在 `uart_init()` 函数中添加调试输出，确认时钟使能执行
2. 在 `uart_gpio_init()` 函数中添加调试输出，确认GPIO配置执行

## 完成标准 (必须可验证)

- [ ] 标准 1: 编译通过，无警告
- [ ] 标准 2: 烧写后调试串口输出USART2寄存器状态
- [ ] 标准 3: 能够确认以下状态:
  - [ ] RCC->APB1ENR 的 USART2EN 位是否为1
  - [ ] GPIOA->MODER 的 PA2 模式是否为复用功能 (10)
  - [ ] GPIOA->AFR[0] 的 PA2 复用功能是否为 AF7 (0111)
  - [ ] USART2->CR1 的 TE 位是否为1 (发送使能)
  - [ ] USART2->BRR 的值是否为 0x1B2 (434) 或相近值
- [ ] 标准 4: 代码审查通过

## 相关文件 (Hook范围检查用)
- `/firmware/stm32/main/uart_comm_test.c`
- `/firmware/stm32/hal/uart.c`
- `/firmware/stm32/hal/uart.h`

## 注意事项
- 寄存器检查代码仅在诊断阶段使用，最终代码应移除或条件编译
- 不要修改原有的初始化逻辑，只添加诊断输出
- 调试输出使用USART1 (PA9/PA10 @ 921600 baud)

## 验收清单 (Reviewer使用)
- [ ] 修改在任务范围内
- [ ] 无新引入的未声明依赖
- [ ] 测试覆盖修改点
- [ ] 无新的技术债务
