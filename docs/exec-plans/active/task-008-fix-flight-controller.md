# Task 008: 更新 flight_controller.c 常量

## 目标
将 flight_controller.c 中的所有 TIM 常量更新为新的 HAL_ 前缀，与更新后的 pwm.h 保持一致。

## 背景上下文

### 相关代码
- **文件**: `firmware/stm32/flight/flight_controller.c` - 飞行控制器实现
- **依赖头文件**: `firmware/stm32/hal/pwm.h` (Task 004 已更新)
- **问题**: 使用旧常量名 TIM2，需要同步更新

### 依赖关系
- 前置任务: Task 004 (pwm.h)
- 阻塞任务: 无

### 代码规范
- 使用新的 HAL_ 前缀常量
- 保持代码逻辑不变
- 只替换常量名

## 具体修改要求

### 文件: `firmware/stm32/flight/flight_controller.c`

#### 1. TIM 常量替换
| 行号 | 旧常量 | 新常量 |
|------|--------|--------|
| 92 | TIM2 | HAL_TIM_2 |
| 98 | TIM2 | HAL_TIM_2 |
| 163 | TIM2 | HAL_TIM_2 |
| 355 | TIM2 | HAL_TIM_2 |

#### 2. UART 常量替换
| 行号 | 旧常量 | 新常量 |
|------|--------|--------|
| 104 | UART_DATABITS_8 | HAL_UART_DATABITS_8 |
| 105 | UART_STOPBITS_1 | HAL_UART_STOPBITS_1 |
| 106 | UART_PARITY_NONE | HAL_UART_PARITY_NONE |
| 107 | UART_HWCONTROL_NONE | HAL_UART_HWCONTROL_NONE |
| 108 | UART_MODE_TX_RX | HAL_UART_MODE_TX_RX |

## 完成标准 (必须可验证)

- [ ] flight_controller.c 中所有 TIM 常量已更新为 HAL_ 前缀
- [ ] flight_controller.c 中所有 UART 常量已更新为 HAL_ 前缀
- [ ] 代码逻辑保持不变
- [ ] 代码编译通过
- [ ] 代码审查通过

## 相关文件 (Hook范围检查用)
- `firmware/stm32/flight/flight_controller.c`
- `firmware/stm32/hal/pwm.h` (参考)
- `firmware/stm32/hal/uart.h` (参考)

## 注意事项
- 只修改常量名，不修改逻辑
- 确保所有使用旧常量的地方都被替换
- 保持缩进和格式不变

## 验收清单 (Reviewer使用)
- [ ] 修改在任务范围内 (仅 flight_controller.c)
- [ ] 所有常量已正确替换
- [ ] 代码逻辑未改变
- [ ] 无新的技术债务
