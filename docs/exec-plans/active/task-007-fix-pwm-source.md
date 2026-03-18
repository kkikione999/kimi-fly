# Task 007: 更新 pwm.c 源文件常量

## 目标
将 pwm.c 中的所有 TIM 常量更新为新的 HAL_ 前缀，与更新后的 pwm.h 保持一致。

## 背景上下文

### 相关代码
- **文件**: `firmware/stm32/hal/pwm.c` - PWM 源文件
- **依赖头文件**: `firmware/stm32/hal/pwm.h` (Task 004 已更新)
- **问题**: 使用旧常量名 TIM1/TIM2/TIM3/TIM4 和 TIM_CH1/CH2/CH3/CH4，需要同步更新

### 依赖关系
- 前置任务: Task 004 (pwm.h)
- 阻塞任务: 无

### 代码规范
- 使用新的 HAL_ 前缀常量
- 保持代码逻辑不变
- 只替换常量名

## 具体修改要求

### 文件: `firmware/stm32/hal/pwm.c`

#### 1. TIM 枚举替换
| 行号 | 旧常量 | 新常量 |
|------|--------|--------|
| 34 | TIM1 | HAL_TIM_1 |
| 37 | TIM2 | HAL_TIM_2 |
| 40 | TIM3 | HAL_TIM_3 |
| 43 | TIM4 | HAL_TIM_4 |
| 52 | TIM1 | HAL_TIM_1 |
| 99, 159, 191, 244, 267 | TIM4 | HAL_TIM_4 |

#### 2. TIM 通道替换
| 行号 | 旧常量 | 新常量 |
|------|--------|--------|
| 99, 159, 244, 267 | TIM_CH4 | HAL_TIM_CH4 |

#### 3. tim_instances 数组处理 (第 23-28 行)
```c
// 当前代码 (需要修改)
static TIM_TypeDef *const tim_instances[] = {
    TIM1,   // 这是 STM32Cube 宏，不是枚举
    TIM2,
    TIM3,
    TIM4
};

// 建议改为函数方式，避免混淆
static TIM_TypeDef* get_tim_instance(tim_t tim) {
    switch(tim) {
        case HAL_TIM_1: return TIM1;  // TIM1 是 STM32Cube 寄存器宏
        case HAL_TIM_2: return TIM2;
        case HAL_TIM_3: return TIM3;
        case HAL_TIM_4: return TIM4;
        default: return NULL;
    }
}
```

#### 4. 数组索引访问修改
当前代码使用 `tim_instances[init->tim]`，需要改为函数调用 `get_tim_instance(init->tim)`。

需要修改的位置：
- 第 111 行: `&tim_handles[init->tim]` - 数组索引仍然有效（枚举值 0-3）
- 第 121 行: `tim_instances[init->tim]` - 需要改为函数调用
- 第 167 行: `&tim_handles[tim]` - 数组索引仍然有效

## 完成标准 (必须可验证)

- [ ] pwm.c 中所有 TIM 常量已更新为 HAL_ 前缀
- [ ] tim_instances 数组冲突已解决
- [ ] 代码逻辑保持不变
- [ ] 代码编译通过
- [ ] 代码审查通过

## 相关文件 (Hook范围检查用)
- `firmware/stm32/hal/pwm.c`
- `firmware/stm32/hal/pwm.h` (参考)

## 注意事项
- TIM1/TIM2/TIM3/TIM4 在 STM32Cube 中是寄存器地址宏，不是枚举
- 需要使用函数来映射 HAL_TIM_* 枚举到实际寄存器地址
- 数组索引（如 tim_handles[init->tim]）仍然有效，因为枚举值是 0-3

## 验收清单 (Reviewer使用)
- [ ] 修改在任务范围内 (仅 pwm.c)
- [ ] 所有 TIM 常量已正确替换
- [ ] tim_instances 冲突已解决
- [ ] 代码逻辑未改变
- [ ] 无新的技术债务
