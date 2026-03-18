# Task 003: 修复 uart.c 状态常量命名

## 目标
将 uart.c 中的 HAL 状态常量更新为 HAL_UART_STATE_* 命名，避免与通用 HAL 状态冲突。

## 背景上下文

### 相关代码
- **文件**: `firmware/stm32/hal/uart.c` - UART 驱动实现
- **头文件**: `firmware/stm32/hal/uart.h` - UART 相关定义
- **问题**: 使用 HAL_STATE_BUSY, HAL_STATE_READY 等通用状态名，可能与 HAL 框架冲突

### 依赖关系
- 前置任务: 无
- 外部依赖: STM32Cube HAL 框架

### 硬件信息
- 涉及引脚: PA2 (TX), PA3 (RX)
- 外设: USART2
- 用途: ESP32-C3 WiFi 通信
- 参考: `hardware-docs/pinout.md`

### 代码规范
- 遵循 STM32 HAL 风格
- 使用 HAL_UART_STATE_* 前缀
- 保持代码功能不变

## 具体修改要求

### 文件: `firmware/stm32/hal/uart.c`

需要替换的状态常量映射表：

| 行号 | 旧常量 | 新常量 |
|------|--------|--------|
| 235 | HAL_STATE_BUSY | HAL_UART_STATE_BUSY |
| 261 | HAL_STATE_ERROR | HAL_UART_STATE_ERROR |
| 267 | HAL_STATE_READY | HAL_UART_STATE_READY |
| 289 | HAL_STATE_BUSY | HAL_UART_STATE_BUSY |
| 295 | HAL_STATE_ERROR | HAL_UART_STATE_ERROR |
| 306 | HAL_STATE_RESET | HAL_UART_STATE_RESET |
| 328 | HAL_STATE_READY | HAL_UART_STATE_READY |
| 328 | HAL_STATE_BUSY | HAL_UART_STATE_BUSY |
| 337 | HAL_STATE_BUSY | HAL_UART_STATE_BUSY |
| 347 | HAL_STATE_READY | HAL_UART_STATE_READY |
| 351 | HAL_STATE_ERROR | HAL_UART_STATE_ERROR |
| 376 | HAL_STATE_READY | HAL_UART_STATE_READY |
| 376 | HAL_STATE_BUSY | HAL_UART_STATE_BUSY |
| 385 | HAL_STATE_BUSY | HAL_UART_STATE_BUSY |
| 395 | HAL_STATE_READY | HAL_UART_STATE_READY |
| 399 | HAL_STATE_ERROR | HAL_UART_STATE_ERROR |
| 501 | HAL_STATE_ERROR | HAL_UART_STATE_ERROR |
| 502 | HAL_STATE_READY | HAL_UART_STATE_READY |

## 完成标准 (必须可验证)

- [ ] uart.c 中所有 HAL 状态常量已更新为 HAL_UART_STATE_* 前缀
- [ ] 代码编译通过 (无状态常量相关冲突)
- [ ] 功能保持不变 (UART 初始化逻辑不变)
- [ ] 代码审查通过

## 相关文件 (Hook范围检查用)
- `firmware/stm32/hal/uart.c`
- `firmware/stm32/hal/uart.h` (参考)

## 注意事项
- 仅修改状态常量名，不修改逻辑
- 确保所有状态常量都被替换
- 保持缩进和格式不变

## 验收清单 (Reviewer使用)
- [ ] 修改在任务范围内 (仅 uart.c)
- [ ] 无新引入的未声明依赖
- [ ] 所有状态常量已正确替换
- [ ] 无新的技术债务
