# Task 002: 修复 pwm.c TIM 枚举冲突

## 目标
将 pwm.c 中的 TIM 枚举更新为 HAL_TIM_* 命名，避免与 STM32Cube HAL 的 TIM1/TIM2/TIM3/TIM4 宏冲突。

## 背景上下文

### 相关代码
- **文件**: `firmware/stm32/hal/pwm.c` - PWM 驱动实现
- **头文件**: `firmware/stm32/hal/pwm.h` - 已定义 tim_t 枚举 (TIM1, TIM2, TIM3, TIM4)
- **问题**: TIM1/TIM2/TIM3/TIM4 与 STM32Cube HAL 的寄存器宏冲突

### 依赖关系
- 前置任务: 无
- 外部依赖: STM32Cube HAL 框架

### 硬件信息
- 涉及外设: TIM1, TIM2, TIM3, TIM4
- 用途: 电机 PWM 控制
- 参考: `hardware-docs/pinout.md`

### 代码规范
- 遵循 STM32 HAL 风格
- 使用 HAL_TIM_* 前缀
- 保持代码功能不变

## 具体修改要求

### 文件: `firmware/stm32/hal/pwm.c`

需要修改的内容：

1. **第 23-28 行**: TIM 实例映射数组
   ```c
   // 旧代码
   static TIM_TypeDef *const tim_instances[] = {
       TIM1,
       TIM2,
       TIM3,
       TIM4
   };

   // 需要改为使用 HAL_TIM_1, HAL_TIM_2 等枚举值作为索引
   // 注意: 这里 TIM1/TIM2/TIM3/TIM4 是 STM32Cube 的寄存器指针宏
   // 需要改为直接映射到对应的 TIM 实例地址
   ```

2. **第 33-46 行**: tim_clock_enable 函数中的 case 语句
   - `case TIM1:` → `case HAL_TIM_1:`
   - `case TIM2:` → `case HAL_TIM_2:`
   - `case TIM3:` → `case HAL_TIM_3:`
   - `case TIM4:` → `case HAL_TIM_4:`

3. **第 52 行**: tim_get_clock_freq 函数
   - `if (tim == TIM1)` → `if (tim == HAL_TIM_1)`

4. **第 99, 159, 191, 244, 267 行**: 边界检查
   - `if (init->tim > TIM4` → `if (init->tim > HAL_TIM_4`
   - `if (tim > TIM4` → `if (tim > HAL_TIM_4`

5. **tim_instances 数组**: 需要改为使用条件编译或函数来获取 TIM 实例
   ```c
   // 建议改为函数方式
   static TIM_TypeDef* get_tim_instance(tim_t tim) {
       switch(tim) {
           case HAL_TIM_1: return TIM1;
           case HAL_TIM_2: return TIM2;
           case HAL_TIM_3: return TIM3;
           case HAL_TIM_4: return TIM4;
           default: return NULL;
       }
   }
   ```

## 完成标准 (必须可验证)

- [ ] pwm.c 中所有 TIM 枚举使用已更新为 HAL_TIM_*
- [ ] tim_instances 数组冲突已解决
- [ ] 代码编译通过 (无 TIM 相关冲突)
- [ ] 功能保持不变 (PWM 初始化逻辑不变)
- [ ] 代码审查通过

## 相关文件 (Hook范围检查用)
- `firmware/stm32/hal/pwm.c`
- `firmware/stm32/hal/pwm.h` (参考)

## 注意事项
- TIM1/TIM2/TIM3/TIM4 在 STM32Cube 中是寄存器地址宏，不能直接用作枚举值
- 需要使用函数或条件编译来映射枚举到实际寄存器地址
- 确保所有使用 tim_instances 的地方都相应更新

## 验收清单 (Reviewer使用)
- [ ] 修改在任务范围内 (仅 pwm.c)
- [ ] 无新引入的未声明依赖
- [ ] TIM 枚举冲突已解决
- [ ] 无新的技术债务
