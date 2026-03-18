# HAL 兼容性修复计划

> **创建日期**: 2026-03-18
> **目标**: 修复 HAL 层头文件和源文件中的常量命名冲突
> **优先级**: P0 - 阻塞编译

---

## 问题描述

HAL 层头文件中的常量命名与 STM32Cube HAL 宏冲突，导致编译失败。例如：
- `TIM1`/`TIM2`/`TIM3`/`TIM4` 与 STM32 寄存器宏冲突
- `GPIO_MODE_AF` 与 HAL 宏冲突
- `HAL_STATE_*` 与 HAL 状态宏冲突

---

## 修复策略

**方案**: 将所有 HAL 层常量添加 `HAL_` 前缀，避免与 STM32Cube HAL 宏冲突。

---

## 任务列表

### Task 1: 更新 GPIO 头文件常量
- **文件**: `firmware/stm32/hal/gpio.h`
- **修改**: 为 GPIO 模式/类型/速度/上下拉/复用功能常量添加 HAL_ 前缀

### Task 2: 更新 SPI 头文件常量
- **文件**: `firmware/stm32/hal/spi.h`
- **修改**: 为 SPI 外设/极性/相位/数据大小/NSS 常量添加 HAL_ 前缀

### Task 3: 更新 UART 头文件常量
- **文件**: `firmware/stm32/hal/uart.h`
- **修改**: 为 UART 停止位/校验位/流控/模式常量添加 HAL_ 前缀

### Task 4: 更新 PWM 头文件常量
- **文件**: `firmware/stm32/hal/pwm.h`
- **修改**: 为 TIM 枚举和通道枚举添加 HAL_ 前缀

### Task 5: 更新 spi.c 源文件
- **文件**: `firmware/stm32/hal/spi.c`
- **修改**: 同步更新为新的 HAL_ 前缀常量

### Task 6: 更新 uart.c 源文件
- **文件**: `firmware/stm32/hal/uart.c`
- **修改**: 同步更新为新的 HAL_ 前缀常量

### Task 7: 更新 pwm.c 源文件
- **文件**: `firmware/stm32/hal/pwm.c`
- **修改**: 同步更新为新的 HAL_ 前缀常量

### Task 8: 更新 flight_controller.c
- **文件**: `firmware/stm32/flight/flight_controller.c`
- **修改**: 同步更新为新的 HAL_ 前缀常量

---

## 常量映射表

### GPIO 常量 (gpio.h)
| 旧常量 | 新常量 |
|--------|--------|
| GPIO_MODE_INPUT | HAL_GPIO_MODE_INPUT |
| GPIO_MODE_OUTPUT | HAL_GPIO_MODE_OUTPUT |
| GPIO_MODE_AF | HAL_GPIO_MODE_AF |
| GPIO_MODE_ANALOG | HAL_GPIO_MODE_ANALOG |
| GPIO_OTYPE_PP | HAL_GPIO_OTYPE_PP |
| GPIO_OTYPE_OD | HAL_GPIO_OTYPE_OD |
| GPIO_SPEED_LOW | HAL_GPIO_SPEED_LOW |
| GPIO_SPEED_MEDIUM | HAL_GPIO_SPEED_MEDIUM |
| GPIO_SPEED_FAST | HAL_GPIO_SPEED_FAST |
| GPIO_SPEED_HIGH | HAL_GPIO_SPEED_HIGH |
| GPIO_PUPD_NONE | HAL_GPIO_PUPD_NONE |
| GPIO_PUPD_UP | HAL_GPIO_PUPD_UP |
| GPIO_PUPD_DOWN | HAL_GPIO_PUPD_DOWN |
| GPIO_AF_0 ~ GPIO_AF_15 | HAL_GPIO_AF_0 ~ HAL_GPIO_AF_15 |

### SPI 常量 (spi.h)
| 旧常量 | 新常量 |
|--------|--------|
| SPI_PERIPH_1/2/3 | HAL_SPI_PERIPH_1/2/3 |
| SPI_CPOL_LOW/HIGH | HAL_SPI_CPOL_LOW/HIGH |
| SPI_CPHA_1EDGE/2EDGE | HAL_SPI_CPHA_1EDGE/2EDGE |
| SPI_DATASIZE_8BIT/16BIT | HAL_SPI_DATASIZE_8BIT/16BIT |
| SPI_NSS_SOFT/HARD | HAL_SPI_NSS_SOFT/HARD |
| SPI_BAUDRATEPRESCALER_* | HAL_SPI_BAUDRATEPRESCALER_* |

### UART 常量 (uart.h)
| 旧常量 | 新常量 |
|--------|--------|
| UART_STOPBITS_1/0_5/2/1_5 | HAL_UART_STOPBITS_1/0_5/2/1_5 |
| UART_PARITY_NONE/EVEN/ODD | HAL_UART_PARITY_NONE/EVEN/ODD |
| UART_HWCONTROL_NONE/RTS/CTS/RTS_CTS | HAL_UART_HWCONTROL_NONE/RTS/CTS/RTS_CTS |
| UART_MODE_RX/TX/TX_RX | HAL_UART_MODE_RX/TX/TX_RX |
| UART_DATABITS_8/9 | HAL_UART_DATABITS_8/9 |

### PWM 常量 (pwm.h)
| 旧常量 | 新常量 |
|--------|--------|
| TIM1/TIM2/TIM3/TIM4 | HAL_TIM_1/2/3/4 |
| TIM_CH1/CH2/CH3/CH4 | HAL_TIM_CH1/CH2/CH3/CH4 |

### HAL 状态常量 (hal_common.h)
| 旧常量 | 新常量 |
|--------|--------|
| HAL_STATE_RESET | HAL_DRIVER_STATE_RESET |
| HAL_STATE_READY | HAL_DRIVER_STATE_READY |
| HAL_STATE_BUSY | HAL_DRIVER_STATE_BUSY |
| HAL_STATE_ERROR | HAL_DRIVER_STATE_ERROR |

---

## 完成标准

- [ ] 所有头文件常量已更新为 HAL_ 前缀
- [ ] 所有源文件已同步更新
- [ ] 代码编译通过 (无命名冲突)
- [ ] 代码审查通过

---

## 参考文档

- 架构地图: `CLAUDE.md`
- Harness流程: `RALPH-HARNESS.md`
- 技术债务: `docs/exec-plans/tech-debt-tracker.md`
