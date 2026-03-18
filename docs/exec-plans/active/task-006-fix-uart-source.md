# Task 006: 更新 uart.c 源文件常量

## 目标
将 uart.c 中的所有常量更新为新的 HAL_ 前缀，与更新后的 uart.h 保持一致。

## 背景上下文

### 相关代码
- **文件**: `firmware/stm32/hal/uart.c` - UART 源文件
- **依赖头文件**: `firmware/stm32/hal/uart.h` (Task 003 已更新)
- **问题**: 使用旧常量名，需要同步更新

### 依赖关系
- 前置任务: Task 003 (uart.h)
- 阻塞任务: 无

### 代码规范
- 使用新的 HAL_ 前缀常量
- 保持代码逻辑不变
- 只替换常量名

## 具体修改要求

### 文件: `firmware/stm32/hal/uart.c`

#### 1. UART 常量替换
| 行号 | 旧常量 | 新常量 |
|------|--------|--------|
| 97 | UART_DATABITS_9 | HAL_UART_DATABITS_9 |
| 99 | UART_DATABITS_8 | HAL_UART_DATABITS_8 |
| 113 | UART_STOPBITS_0_5 | HAL_UART_STOPBITS_0_5 |
| 115 | UART_STOPBITS_2 | HAL_UART_STOPBITS_2 |
| 117 | UART_STOPBITS_1_5 | HAL_UART_STOPBITS_1_5 |
| 119 | UART_STOPBITS_1 | HAL_UART_STOPBITS_1 |
| 133 | UART_PARITY_EVEN | HAL_UART_PARITY_EVEN |
| 135 | UART_PARITY_ODD | HAL_UART_PARITY_ODD |
| 137 | UART_PARITY_NONE | HAL_UART_PARITY_NONE |
| 151 | UART_MODE_RX | HAL_UART_MODE_RX |
| 153 | UART_MODE_TX | HAL_UART_MODE_TX |
| 155 | UART_MODE_TX_RX | HAL_UART_MODE_TX_RX |
| 169 | UART_HWCONTROL_RTS | HAL_UART_HWCONTROL_RTS |
| 171 | UART_HWCONTROL_CTS | HAL_UART_HWCONTROL_CTS |
| 173 | UART_HWCONTROL_RTS_CTS | HAL_UART_HWCONTROL_RTS_CTS |
| 175 | UART_HWCONTROL_NONE | HAL_UART_HWCONTROL_NONE |

#### 2. 错误码替换 (第 188-206 行)
| 旧常量 | 新常量 |
|--------|--------|
| UART_ERROR_NONE | HAL_UART_ERROR_NONE |
| UART_ERROR_PE | HAL_UART_ERROR_PE |
| UART_ERROR_NE | HAL_UART_ERROR_NE |
| UART_ERROR_FE | HAL_UART_ERROR_FE |
| UART_ERROR_ORE | HAL_UART_ERROR_ORE |
| UART_ERROR_DMA | HAL_UART_ERROR_DMA |

#### 3. 状态常量替换 (第 235-502 行)
| 旧常量 | 新常量 |
|--------|--------|
| HAL_STATE_BUSY | HAL_DRIVER_STATE_BUSY |
| HAL_STATE_READY | HAL_DRIVER_STATE_READY |
| HAL_STATE_ERROR | HAL_DRIVER_STATE_ERROR |
| HAL_STATE_RESET | HAL_DRIVER_STATE_RESET |

**注意**: HAL_STATE_* 需要在 hal_common.h 中定义或直接使用 HAL_DriverStateTypeDef

## 完成标准 (必须可验证)

- [ ] uart.c 中所有 UART 常量已更新为 HAL_ 前缀
- [ ] uart.c 中所有错误码已更新为 HAL_ 前缀
- [ ] uart.c 中所有状态常量已更新
- [ ] 代码逻辑保持不变
- [ ] 代码编译通过
- [ ] 代码审查通过

## 相关文件 (Hook范围检查用)
- `firmware/stm32/hal/uart.c`
- `firmware/stm32/hal/uart.h` (参考)
- `firmware/stm32/hal/hal_common.h` (参考)

## 注意事项
- 只修改常量名，不修改逻辑
- HAL_STATE_* 可能需要特殊处理（检查 hal_common.h）
- 确保所有使用旧常量的地方都被替换

## 验收清单 (Reviewer使用)
- [ ] 修改在任务范围内 (仅 uart.c)
- [ ] 所有常量已正确替换
- [ ] 代码逻辑未改变
- [ ] 无新的技术债务
