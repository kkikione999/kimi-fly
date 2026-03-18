# 执行计划 - 修复 STM32 HAL 兼容性

> **创建日期**: 2026-03-18
> **更新日期**: 2026-03-18
> **状态**: 进行中
> **当前阶段**: HAL 兼容性修复

---

## 背景

HAL 层头文件已更新为支持 STM32Cube 框架（使用 USE_HAL_DRIVER 宏条件编译），但源文件（.c 文件）仍然使用旧的常量名，导致编译失败。

**技术债务**: `docs/exec-plans/tech-debt-tracker.md` - P0 优先级

---

## 本轮目标

修复 HAL 层源文件中的常量命名，使其与头文件保持一致，确保 STM32 固件可以正常编译。

---

## 任务列表

### Task 1: 修复 spi.c GPIO 常量
- **文件**: `firmware/stm32/hal/spi.c`
- **问题**: 使用旧 GPIO 常量名（GPIO_MODE_AF, GPIO_OTYPE_PP 等）
- **修复**: 更新为 HAL_GPIO_MODE_AF, HAL_GPIO_OTYPE_PP 等

### Task 2: 修复 pwm.c TIM 枚举冲突
- **文件**: `firmware/stm32/hal/pwm.c`
- **问题**: TIM1/TIM2/TIM3/TIM4 枚举与 STM32Cube 宏冲突
- **修复**: 更新为 HAL_TIM_1, HAL_TIM_2, HAL_TIM_3, HAL_TIM_4

### Task 3: 修复 uart.c 状态常量
- **文件**: `firmware/stm32/hal/uart.c`
- **问题**: 使用 HAL_STATE_BUSY, HAL_STATE_READY, HAL_STATE_ERROR, HAL_STATE_RESET
- **修复**: 更新为 HAL_UART_STATE_*

---

## 常量映射表

### GPIO 常量 (Task 1)
| 旧常量 | 新常量 |
|--------|--------|
| GPIO_MODE_AF | HAL_GPIO_MODE_AF |
| GPIO_MODE_OUTPUT | HAL_GPIO_MODE_OUTPUT |
| GPIO_OTYPE_PP | HAL_GPIO_OTYPE_PP |
| GPIO_SPEED_HIGH | HAL_GPIO_SPEED_HIGH |
| GPIO_PUPD_NONE | HAL_GPIO_PUPD_NONE |
| GPIO_PUPD_UP | HAL_GPIO_PUPD_UP |
| GPIO_AF_6 | HAL_GPIO_AF_6 |
| GPIO_AF_0 | HAL_GPIO_AF_0 |

### TIM 常量 (Task 2)
| 旧常量 | 新常量 |
|--------|--------|
| TIM1 | HAL_TIM_1 |
| TIM2 | HAL_TIM_2 |
| TIM3 | HAL_TIM_3 |
| TIM4 | HAL_TIM_4 |
| TIM_CH1 | HAL_TIM_CH1 |
| TIM_CH2 | HAL_TIM_CH2 |
| TIM_CH3 | HAL_TIM_CH3 |
| TIM_CH4 | HAL_TIM_CH4 |

### UART 状态常量 (Task 3)
| 旧常量 | 新常量 |
|--------|--------|
| HAL_STATE_BUSY | HAL_UART_STATE_BUSY |
| HAL_STATE_READY | HAL_UART_STATE_READY |
| HAL_STATE_ERROR | HAL_UART_STATE_ERROR |
| HAL_STATE_RESET | HAL_UART_STATE_RESET |

---

## 完成标准

- [ ] 所有源文件编译通过
- [ ] 无常量命名冲突
- [ ] 代码审查通过

---

## 参考文档

- 架构地图: `CLAUDE.md`
- Harness流程: `RALPH-HARNESS.md`
- 技术债务: `docs/exec-plans/tech-debt-tracker.md`
