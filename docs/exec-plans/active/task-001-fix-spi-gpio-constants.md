# Task 001: 修复 spi.c GPIO 常量命名

## 目标
将 spi.c 中的 GPIO 相关常量更新为 HAL_ 前缀，避免与 STM32Cube HAL 宏冲突。

## 背景上下文

### 相关代码
- **文件**: `firmware/stm32/hal/spi.c` - SPI 驱动实现
- **头文件**: `firmware/stm32/hal/gpio.h` - 已更新为 HAL_GPIO_* 常量
- **问题**: spi.c 中使用的 GPIO_MODE_AF, GPIO_OTYPE_PP 等常量与 STM32Cube HAL 宏冲突

### 依赖关系
- 前置任务: 无
- 外部依赖: STM32Cube HAL 框架

### 硬件信息
- 涉及引脚: PC10 (SCK), PC11 (MISO), PC12 (MOSI), PA15 (NSS)
- 外设: SPI3
- 参考: `hardware-docs/pinout.md`

### 代码规范
- 遵循 STM32 HAL 风格
- 使用 HAL_ 前缀常量
- 保持代码功能不变

## 具体修改要求

### 文件: `firmware/stm32/hal/spi.c`

需要替换的常量映射表：

| 行号 | 旧常量 | 新常量 |
|------|--------|--------|
| 59 | GPIO_MODE_AF | HAL_GPIO_MODE_AF |
| 60 | GPIO_OTYPE_PP | HAL_GPIO_OTYPE_PP |
| 61 | GPIO_SPEED_HIGH | HAL_GPIO_SPEED_HIGH |
| 62 | GPIO_PUPD_NONE | HAL_GPIO_PUPD_NONE |
| 63 | GPIO_AF_6 | HAL_GPIO_AF_6 |
| 72 | GPIO_MODE_AF | HAL_GPIO_MODE_AF |
| 73 | GPIO_OTYPE_PP | HAL_GPIO_OTYPE_PP |
| 74 | GPIO_SPEED_HIGH | HAL_GPIO_SPEED_HIGH |
| 75 | GPIO_PUPD_NONE | HAL_GPIO_PUPD_NONE |
| 76 | GPIO_AF_6 | HAL_GPIO_AF_6 |
| 85 | GPIO_MODE_AF | HAL_GPIO_MODE_AF |
| 86 | GPIO_OTYPE_PP | HAL_GPIO_OTYPE_PP |
| 87 | GPIO_SPEED_HIGH | HAL_GPIO_SPEED_HIGH |
| 88 | GPIO_PUPD_NONE | HAL_GPIO_PUPD_NONE |
| 89 | GPIO_AF_6 | HAL_GPIO_AF_6 |
| 98 | GPIO_MODE_OUTPUT | HAL_GPIO_MODE_OUTPUT |
| 99 | GPIO_OTYPE_PP | HAL_GPIO_OTYPE_PP |
| 100 | GPIO_SPEED_HIGH | HAL_GPIO_SPEED_HIGH |
| 101 | GPIO_PUPD_UP | HAL_GPIO_PUPD_UP |
| 102 | GPIO_AF_0 | HAL_GPIO_AF_0 |

## 完成标准 (必须可验证)

- [ ] spi.c 中所有 GPIO 常量已更新为 HAL_ 前缀
- [ ] 代码编译通过 (无 GPIO 相关冲突)
- [ ] 功能保持不变 (SPI3 初始化逻辑不变)
- [ ] 代码审查通过

## 相关文件 (Hook范围检查用)
- `firmware/stm32/hal/spi.c`
- `firmware/stm32/hal/gpio.h` (参考)

## 注意事项
- 仅修改常量名，不修改逻辑
- 确保所有 GPIO 相关常量都被替换
- 保持缩进和格式不变

## 验收清单 (Reviewer使用)
- [ ] 修改在任务范围内 (仅 spi.c)
- [ ] 无新引入的未声明依赖
- [ ] 所有 GPIO 常量已正确替换
- [ ] 无新的技术债务
