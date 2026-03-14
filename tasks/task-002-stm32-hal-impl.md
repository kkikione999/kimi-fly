# Task 002: STM32 HAL 层实现

## 目标
基于 Task-001 创建的接口定义，实现 STM32 平台的 HAL 层。

## 背景上下文

### 前置任务
- Task-001 已完成 HAL 接口定义和板级配置

### 硬件平台
- **目标芯片**: STM32F405 (建议，可根据实际情况调整)
- **时钟**: HSE 8MHz，系统时钟 168MHz
- **外设需求**:
  - GPIO: 电机PWM输出、传感器CS、LED指示
  - UART: 调试串口、ESP32-C3通信
  - SPI: IMU 高速通信
  - I2C: 气压计、磁力计
  - TIM: 电机PWM输出 (400Hz)

## 具体修改要求

### 1. 文件: `firmware/hal/stm32/stm32_hal.c` 和 `stm32_hal.h`
实现 STM32 平台的 HAL 初始化。

内容需包含:
- 时钟树配置 (HSE -> PLL -> SYSCLK=168MHz)
- GPIO 时钟使能
- NVIC 优先级分组配置
- SysTick 配置 (1ms中断)
- 错误处理回调

### 2. 文件: `firmware/hal/stm32/stm32_gpio.c`
实现 GPIO 接口。

内容需包含:
- `stm32_gpio_init()`: 初始化GPIO时钟
- `stm32_gpio_set_mode()`: 配置输入/输出/复用/模拟
- `stm32_gpio_write()`: 输出高/低电平
- `stm32_gpio_read()`: 读取输入电平
- 注册到 HAL 接口表

### 3. 文件: `firmware/hal/stm32/stm32_uart.c`
实现 UART 接口。

内容需包含:
- 调试串口初始化 (UART1, 115200)
- ESP32通信串口初始化 (UART3, 921600)
- `stm32_uart_send()`: 阻塞发送
- `stm32_uart_recv()`: 阻塞接收
- `stm32_uart_send_dma()`: DMA发送 (可选)
- 注册到 HAL 接口表

### 4. 文件: `firmware/hal/stm32/stm32_pwm.c`
实现 PWM/电机控制接口。

内容需包含:
- TIM2/TIM3 定时器初始化 (400Hz)
- 4路PWM输出配置 (电机1-4)
- `stm32_pwm_set_duty()`: 设置占空比 (0-100%)
- DShot 协议支持 (可选，高级)
- 注册到 HAL 接口表

## 编码规范
- 使用 STM32 HAL/LL 库
- LL库用于时间关键代码 (PWM、SPI)
- HAL用于复杂初始化
- 所有寄存器访问通过 HAL/LL，不直接操作寄存器

## 完成标准

- [ ] `stm32_hal.c/h` - 系统初始化和时钟配置
- [ ] `stm32_gpio.c` - GPIO 接口实现
- [ ] `stm32_uart.c` - UART 接口实现
- [ ] `stm32_pwm.c` - PWM/电机控制实现
- [ ] 代码通过编译检查 (可 mock 部分依赖)
- [ ] 每个函数有完整的文档注释

## 依赖关系

```
Depends on: task-001-hal-structure
Blocks: task-003-esp32c3-hal, task-004-sensor-drivers
```

## 相关文件
- `firmware/hal/hal_interface.h` - 接口定义
- `firmware/platform/board_config.h` - 引脚配置
- `/Users/ll/kimi-fly/docs/HARDWARE.md` - 硬件规格

## 注意事项
- 时钟配置需根据实际晶振调整
- UART3用于与ESP32-C3通信，波特率要协商一致
- PWM频率400Hz是电调的标准频率
- 使用LL库可以获得更好的实时性能
