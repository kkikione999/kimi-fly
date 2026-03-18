# Task 005: 更新 spi.c 源文件常量

## 目标
将 spi.c 中的所有常量更新为新的 HAL_ 前缀，与更新后的 spi.h 和 gpio.h 保持一致。

## 背景上下文

### 相关代码
- **文件**: `firmware/stm32/hal/spi.c` - SPI 源文件
- **依赖头文件**:
  - `firmware/stm32/hal/spi.h` (Task 002 已更新)
  - `firmware/stm32/hal/gpio.h` (Task 001 已更新)
- **问题**: 使用旧常量名，需要同步更新

### 依赖关系
- 前置任务: Task 001 (gpio.h), Task 002 (spi.h)
- 阻塞任务: 无

### 代码规范
- 使用新的 HAL_ 前缀常量
- 保持代码逻辑不变
- 只替换常量名

## 具体修改要求

### 文件: `firmware/stm32/hal/spi.c`

#### 1. SPI 常量替换
| 行号 | 旧常量 | 新常量 |
|------|--------|--------|
| 32, 48, 156, 316 | SPI_PERIPH_3 | HAL_SPI_PERIPH_3 |
| 178 | SPI_DATASIZE_16BIT | HAL_SPI_DATASIZE_16BIT |
| 178 | SPI_DATASIZE_8BIT | HAL_SPI_DATASIZE_8BIT |
| 179 | SPI_CPOL_HIGH | HAL_SPI_CPOL_HIGH |
| 180 | SPI_CPHA_2EDGE | HAL_SPI_CPHA_2EDGE |
| 181 | SPI_NSS_SOFT | HAL_SPI_NSS_SOFT |
| 326 | SPI_CPOL_LOW | HAL_SPI_CPOL_LOW |
| 327 | SPI_CPHA_1EDGE | HAL_SPI_CPHA_1EDGE |
| 328 | SPI_DATASIZE_8BIT | HAL_SPI_DATASIZE_8BIT |
| 329 | SPI_NSS_SOFT | HAL_SPI_NSS_SOFT |
| 330 | SPI_BAUDRATEPRESCALER_8 | HAL_SPI_BAUDRATEPRESCALER_8 |

#### 2. GPIO 常量替换 (第 59-102 行)
| 行号 | 旧常量 | 新常量 |
|------|--------|--------|
| 59, 72, 85 | GPIO_MODE_AF | HAL_GPIO_MODE_AF |
| 60, 73, 86, 99 | GPIO_OTYPE_PP | HAL_GPIO_OTYPE_PP |
| 61, 74, 87, 100 | GPIO_SPEED_HIGH | HAL_GPIO_SPEED_HIGH |
| 62, 75, 88 | GPIO_PUPD_NONE | HAL_GPIO_PUPD_NONE |
| 63, 76, 89 | GPIO_AF_6 | HAL_GPIO_AF_6 |
| 98 | GPIO_MODE_OUTPUT | HAL_GPIO_MODE_OUTPUT |
| 101 | GPIO_PUPD_UP | HAL_GPIO_PUPD_UP |
| 102 | GPIO_AF_0 | HAL_GPIO_AF_0 |

## 完成标准 (必须可验证)

- [ ] spi.c 中所有 SPI 常量已更新为 HAL_ 前缀
- [ ] spi.c 中所有 GPIO 常量已更新为 HAL_ 前缀
- [ ] 代码逻辑保持不变
- [ ] 代码编译通过
- [ ] 代码审查通过

## 相关文件 (Hook范围检查用)
- `firmware/stm32/hal/spi.c`
- `firmware/stm32/hal/spi.h` (参考)
- `firmware/stm32/hal/gpio.h` (参考)

## 注意事项
- 只修改常量名，不修改逻辑
- 确保所有使用旧常量的地方都被替换
- 保持缩进和格式不变

## 验收清单 (Reviewer使用)
- [ ] 修改在任务范围内 (仅 spi.c)
- [ ] 所有常量已正确替换
- [ ] 代码逻辑未改变
- [ ] 无新的技术债务
