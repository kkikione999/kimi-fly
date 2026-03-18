# Task 002: 更新 SPI 头文件常量

## 目标
将 spi.h 中的所有常量更新为 HAL_ 前缀，避免与 STM32Cube HAL 宏冲突。

## 背景上下文

### 相关代码
- **文件**: `firmware/stm32/hal/spi.h` - SPI 头文件
- **问题**: SPI_PERIPH_*, SPI_CPOL_*, SPI_CPHA_*, SPI_DATASIZE_*, SPI_NSS_*, SPI_BAUDRATEPRESCALER_* 可能与 HAL 宏冲突

### 依赖关系
- 前置任务: 无
- 阻塞任务: Task 005 (spi.c 依赖 spi.h)

### 代码规范
- 所有常量添加 HAL_ 前缀
- 保持原有枚举值不变
- 保持注释不变

## 具体修改要求

### 文件: `firmware/stm32/hal/spi.h`

#### 1. SPI 外设标识符 (第 8-13 行)
```c
// 旧代码
typedef enum {
    SPI_PERIPH_1 = 0,
    SPI_PERIPH_2 = 1,
    SPI_PERIPH_3 = 2,
    SPI_PERIPH_MAX
} spi_periph_t;

// 新代码
typedef enum {
    HAL_SPI_PERIPH_1 = 0,
    HAL_SPI_PERIPH_2 = 1,
    HAL_SPI_PERIPH_3 = 2,
    HAL_SPI_PERIPH_MAX
} spi_periph_t;
```

#### 2. SPI 时钟极性 (第 16-19 行)
```c
// 旧代码
typedef enum {
    SPI_CPOL_LOW = 0,
    SPI_CPOL_HIGH = 1
} spi_cpol_t;

// 新代码
typedef enum {
    HAL_SPI_CPOL_LOW = 0,
    HAL_SPI_CPOL_HIGH = 1
} spi_cpol_t;
```

#### 3. SPI 时钟相位 (第 22-25 行)
```c
// 旧代码
typedef enum {
    SPI_CPHA_1EDGE = 0,
    SPI_CPHA_2EDGE = 1
} spi_cpha_t;

// 新代码
typedef enum {
    HAL_SPI_CPHA_1EDGE = 0,
    HAL_SPI_CPHA_2EDGE = 1
} spi_cpha_t;
```

#### 4. SPI 数据大小 (第 28-31 行)
```c
// 旧代码
typedef enum {
    SPI_DATASIZE_8BIT = 0,
    SPI_DATASIZE_16BIT = 1
} spi_datasize_t;

// 新代码
typedef enum {
    HAL_SPI_DATASIZE_8BIT = 0,
    HAL_SPI_DATASIZE_16BIT = 1
} spi_datasize_t;
```

#### 5. SPI NSS 管理 (第 34-37 行)
```c
// 旧代码
typedef enum {
    SPI_NSS_SOFT = 0,
    SPI_NSS_HARD = 1
} spi_nss_mode_t;

// 新代码
typedef enum {
    HAL_SPI_NSS_SOFT = 0,
    HAL_SPI_NSS_HARD = 1
} spi_nss_mode_t;
```

#### 6. SPI 波特率预分频器 (第 40-49 行)
```c
// 旧代码
typedef enum {
    SPI_BAUDRATEPRESCALER_2 = 0,
    SPI_BAUDRATEPRESCALER_4 = 1,
    ...
    SPI_BAUDRATEPRESCALER_256 = 7
} spi_baudrate_prescaler_t;

// 新代码
typedef enum {
    HAL_SPI_BAUDRATEPRESCALER_2 = 0,
    HAL_SPI_BAUDRATEPRESCALER_4 = 1,
    ...
    HAL_SPI_BAUDRATEPRESCALER_256 = 7
} spi_baudrate_prescaler_t;
```

## 完成标准 (必须可验证)

- [ ] spi.h 中所有 SPI 常量已更新为 HAL_ 前缀
- [ ] 枚举值保持不变
- [ ] 注释保持不变
- [ ] 代码审查通过

## 相关文件 (Hook范围检查用)
- `firmware/stm32/hal/spi.h`

## 注意事项
- 只修改常量名，不修改枚举值
- 结构体名 (spi_config_t, spi_handle_t) 不需要修改
- 确保所有常量都被替换

## 验收清单 (Reviewer使用)
- [ ] 修改在任务范围内 (仅 spi.h)
- [ ] 所有 SPI 常量已正确添加 HAL_ 前缀
- [ ] 枚举值未改变
- [ ] 无新的技术债务
