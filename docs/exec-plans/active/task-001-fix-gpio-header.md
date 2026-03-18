# Task 001: 更新 GPIO 头文件常量

## 目标
将 gpio.h 中的所有常量更新为 HAL_ 前缀，避免与 STM32Cube HAL 宏冲突。

## 背景上下文

### 相关代码
- **文件**: `firmware/stm32/hal/gpio.h` - GPIO 头文件
- **问题**: GPIO_MODE_*, GPIO_OTYPE_*, GPIO_SPEED_*, GPIO_PUPD_*, GPIO_AF_* 可能与 HAL 宏冲突

### 依赖关系
- 前置任务: 无
- 阻塞任务: Task 005 (spi.c 依赖 gpio.h)

### 代码规范
- 所有常量添加 HAL_ 前缀
- 保持原有枚举值不变
- 保持注释不变

## 具体修改要求

### 文件: `firmware/stm32/hal/gpio.h`

#### 1. GPIO 模式定义 (第 58-63 行)
```c
// 旧代码
typedef enum {
    GPIO_MODE_INPUT     = 0x00U,
    GPIO_MODE_OUTPUT    = 0x01U,
    GPIO_MODE_AF        = 0x02U,
    GPIO_MODE_ANALOG    = 0x03U
} gpio_mode_t;

// 新代码
typedef enum {
    HAL_GPIO_MODE_INPUT     = 0x00U,
    HAL_GPIO_MODE_OUTPUT    = 0x01U,
    HAL_GPIO_MODE_AF        = 0x02U,
    HAL_GPIO_MODE_ANALOG    = 0x03U
} gpio_mode_t;
```

#### 2. GPIO 输出类型定义 (第 69-72 行)
```c
// 旧代码
typedef enum {
    GPIO_OTYPE_PP = 0x00U,
    GPIO_OTYPE_OD = 0x01U
} gpio_otype_t;

// 新代码
typedef enum {
    HAL_GPIO_OTYPE_PP = 0x00U,
    HAL_GPIO_OTYPE_OD = 0x01U
} gpio_otype_t;
```

#### 3. GPIO 速度定义 (第 78-83 行)
```c
// 旧代码
typedef enum {
    GPIO_SPEED_LOW    = 0x00U,
    GPIO_SPEED_MEDIUM = 0x01U,
    GPIO_SPEED_FAST   = 0x02U,
    GPIO_SPEED_HIGH   = 0x03U
} gpio_speed_t;

// 新代码
typedef enum {
    HAL_GPIO_SPEED_LOW    = 0x00U,
    HAL_GPIO_SPEED_MEDIUM = 0x01U,
    HAL_GPIO_SPEED_FAST   = 0x02U,
    HAL_GPIO_SPEED_HIGH   = 0x03U
} gpio_speed_t;
```

#### 4. GPIO 上下拉定义 (第 89-93 行)
```c
// 旧代码
typedef enum {
    GPIO_PUPD_NONE = 0x00U,
    GPIO_PUPD_UP   = 0x01U,
    GPIO_PUPD_DOWN = 0x02U
} gpio_pupd_t;

// 新代码
typedef enum {
    HAL_GPIO_PUPD_NONE = 0x00U,
    HAL_GPIO_PUPD_UP   = 0x01U,
    HAL_GPIO_PUPD_DOWN = 0x02U
} gpio_pupd_t;
```

#### 5. GPIO 复用功能定义 (第 99-116 行)
```c
// 旧代码
typedef enum {
    GPIO_AF_0  = 0x00U,
    GPIO_AF_1  = 0x01U,
    ...
    GPIO_AF_15 = 0x0FU
} gpio_af_t;

// 新代码
typedef enum {
    HAL_GPIO_AF_0  = 0x00U,
    HAL_GPIO_AF_1  = 0x01U,
    ...
    HAL_GPIO_AF_15 = 0x0FU
} gpio_af_t;
```

## 完成标准 (必须可验证)

- [ ] gpio.h 中所有 GPIO 常量已更新为 HAL_ 前缀
- [ ] 枚举值保持不变
- [ ] 注释保持不变
- [ ] 代码审查通过

## 相关文件 (Hook范围检查用)
- `firmware/stm32/hal/gpio.h`

## 注意事项
- 只修改常量名，不修改枚举值
- gpio_port_t 和 gpio_pin_t 不需要修改（无冲突）
- 确保所有常量都被替换

## 验收清单 (Reviewer使用)
- [ ] 修改在任务范围内 (仅 gpio.h)
- [ ] 所有 GPIO 常量已正确添加 HAL_ 前缀
- [ ] 枚举值未改变
- [ ] 无新的技术债务
