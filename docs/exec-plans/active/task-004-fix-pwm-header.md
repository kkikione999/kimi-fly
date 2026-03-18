# Task 004: 更新 PWM 头文件常量

## 目标
将 pwm.h 中的所有 TIM 常量更新为 HAL_ 前缀，避免与 STM32Cube HAL 寄存器宏冲突。

## 背景上下文

### 相关代码
- **文件**: `firmware/stm32/hal/pwm.h` - PWM 头文件
- **问题**: TIM1/TIM2/TIM3/TIM4 与 STM32 寄存器宏冲突，TIM_CH1/CH2/CH3/CH4 可能冲突

### 依赖关系
- 前置任务: 无
- 阻塞任务: Task 007 (pwm.c 依赖 pwm.h), Task 008 (flight_controller.c 依赖 pwm.h)

### 代码规范
- 所有常量添加 HAL_ 前缀
- 保持原有枚举值不变
- 保持注释不变

## 具体修改要求

### 文件: `firmware/stm32/hal/pwm.h`

#### 1. TIM 外设定义 (第 7-12 行)
```c
// 旧代码
typedef enum {
    TIM1 = 0,
    TIM2 = 1,
    TIM3 = 2,
    TIM4 = 3
} tim_t;

// 新代码
typedef enum {
    HAL_TIM_1 = 0,
    HAL_TIM_2 = 1,
    HAL_TIM_3 = 2,
    HAL_TIM_4 = 3
} tim_t;
```

#### 2. TIM 通道定义 (第 15-20 行)
```c
// 旧代码
typedef enum {
    TIM_CH1 = 0,
    TIM_CH2 = 1,
    TIM_CH3 = 2,
    TIM_CH4 = 3
} tim_channel_t;

// 新代码
typedef enum {
    HAL_TIM_CH1 = 0,
    HAL_TIM_CH2 = 1,
    HAL_TIM_CH3 = 2,
    HAL_TIM_CH4 = 3
} tim_channel_t;
```

## 完成标准 (必须可验证)

- [ ] pwm.h 中所有 TIM 常量已更新为 HAL_ 前缀
- [ ] 枚举值保持不变
- [ ] 注释保持不变
- [ ] 代码审查通过

## 相关文件 (Hook范围检查用)
- `firmware/stm32/hal/pwm.h`

## 注意事项
- 只修改常量名，不修改枚举值
- 结构体名 (pwm_init_t, pwm_state_t) 不需要修改
- 这是最关键的修复，因为 TIM1/TIM2/TIM3/TIM4 是 STM32 寄存器地址宏

## 验收清单 (Reviewer使用)
- [ ] 修改在任务范围内 (仅 pwm.h)
- [ ] 所有 TIM 常量已正确添加 HAL_ 前缀
- [ ] 枚举值未改变
- [ ] 无新的技术债务
