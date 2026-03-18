# Task 003: 更新 UART 头文件常量

## 目标
将 uart.h 中的所有常量更新为 HAL_ 前缀，避免与 STM32Cube HAL 宏冲突。

## 背景上下文

### 相关代码
- **文件**: `firmware/stm32/hal/uart.h` - UART 头文件
- **问题**: UART_STOPBITS_*, UART_PARITY_*, UART_HWCONTROL_*, UART_MODE_*, UART_DATABITS_* 可能与 HAL 宏冲突

### 依赖关系
- 前置任务: 无
- 阻塞任务: Task 006 (uart.c 依赖 uart.h)

### 代码规范
- 所有常量添加 HAL_ 前缀
- 保持原有枚举值不变
- 保持注释不变

## 具体修改要求

### 文件: `firmware/stm32/hal/uart.h`

#### 1. 数据位定义 (第 50-53 行)
```c
// 旧代码
typedef enum {
    UART_DATABITS_8 = 0,
    UART_DATABITS_9 = 1
} uart_databits_t;

// 新代码
typedef enum {
    HAL_UART_DATABITS_8 = 0,
    HAL_UART_DATABITS_9 = 1
} uart_databits_t;
```

#### 2. 停止位定义 (第 56-61 行)
```c
// 旧代码
typedef enum {
    UART_STOPBITS_1   = 0,
    UART_STOPBITS_0_5 = 1,
    UART_STOPBITS_2   = 2,
    UART_STOPBITS_1_5 = 3
} uart_stopbits_t;

// 新代码
typedef enum {
    HAL_UART_STOPBITS_1   = 0,
    HAL_UART_STOPBITS_0_5 = 1,
    HAL_UART_STOPBITS_2   = 2,
    HAL_UART_STOPBITS_1_5 = 3
} uart_stopbits_t;
```

#### 3. 校验位定义 (第 64-68 行)
```c
// 旧代码
typedef enum {
    UART_PARITY_NONE = 0,
    UART_PARITY_EVEN = 1,
    UART_PARITY_ODD  = 2
} uart_parity_t;

// 新代码
typedef enum {
    HAL_UART_PARITY_NONE = 0,
    HAL_UART_PARITY_EVEN = 1,
    HAL_UART_PARITY_ODD  = 2
} uart_parity_t;
```

#### 4. 硬件流控定义 (第 71-76 行)
```c
// 旧代码
typedef enum {
    UART_HWCONTROL_NONE    = 0,
    UART_HWCONTROL_RTS     = 1,
    UART_HWCONTROL_CTS     = 2,
    UART_HWCONTROL_RTS_CTS = 3
} uart_hwcontrol_t;

// 新代码
typedef enum {
    HAL_UART_HWCONTROL_NONE    = 0,
    HAL_UART_HWCONTROL_RTS     = 1,
    HAL_UART_HWCONTROL_CTS     = 2,
    HAL_UART_HWCONTROL_RTS_CTS = 3
} uart_hwcontrol_t;
```

#### 5. 模式定义 (第 79-83 行)
```c
// 旧代码
typedef enum {
    UART_MODE_RX    = 0x01,
    UART_MODE_TX    = 0x02,
    UART_MODE_TX_RX = 0x03
} uart_mode_t;

// 新代码
typedef enum {
    HAL_UART_MODE_RX    = 0x01,
    HAL_UART_MODE_TX    = 0x02,
    HAL_UART_MODE_TX_RX = 0x03
} uart_mode_t;
```

#### 6. 错误码定义 (第 119-126 行)
```c
// 旧代码
#define UART_ERROR_NONE         0x00000000U
#define UART_ERROR_PE           0x00000001U
...
#define UART_ERROR_TIMEOUT      0x00000040U

// 新代码
#define HAL_UART_ERROR_NONE         0x00000000U
#define HAL_UART_ERROR_PE           0x00000001U
...
#define HAL_UART_ERROR_TIMEOUT      0x00000040U
```

## 完成标准 (必须可验证)

- [ ] uart.h 中所有 UART 常量已更新为 HAL_ 前缀
- [ ] 枚举值保持不变
- [ ] 注释保持不变
- [ ] 代码审查通过

## 相关文件 (Hook范围检查用)
- `firmware/stm32/hal/uart.h`

## 注意事项
- 只修改常量名，不修改枚举值
- 波特率定义 (UART_BAUDRATE_*) 不需要修改（是数值，无冲突）
- UART_INSTANCE_* 不需要修改
- 确保所有常量都被替换

## 验收清单 (Reviewer使用)
- [ ] 修改在任务范围内 (仅 uart.h)
- [ ] 所有 UART 常量已正确添加 HAL_ 前缀
- [ ] 枚举值未改变
- [ ] 无新的技术债务
