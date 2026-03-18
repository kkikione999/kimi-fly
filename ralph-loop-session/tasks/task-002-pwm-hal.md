# Task 002: STM32 PWM HAL实现

## 目标
实现STM32F411CEU6的PWM硬件抽象层，为4个电机提供可变占空比控制。

---

## 背景上下文

### 相关代码
- `firmware/stm32/hal/hal_common.h` - HAL通用定义
- `firmware/stm32/hal/gpio.h` - GPIO定义 (Task 001)

### 依赖关系
- 前置任务: Task 001 (GPIO HAL已提供hal_common.h)
- 外部依赖: STM32F4xx定时器外设

### 硬件信息
- **MCU**: STM32F411CEU6
- **可用TIM通道**:
  - TIM1: PA8(CH1), PA9(CH2), PA10(CH3), PA11(CH4)
  - TIM2: PA0(CH1), PA1(CH2), PA2(CH3), PA3(CH4)
  - TIM3: PA6(CH1), PA7(CH2), PB0(CH3), PB1(CH4)
  - TIM4: PB6(CH1), PB7(CH2), PB8(CH3), PB9(CH4)
- **电机驱动**: 4x空心杯电机 via MOSFET
- **PWM频率**: 通常50Hz-400Hz用于电机控制

### 代码规范
- 遵循STM32 HAL风格命名
- 寄存器访问使用volatile
- 支持动态频率和占空比调整

---

## 具体修改要求

### 文件 1: `firmware/stm32/hal/pwm.h`
创建PWM HAL头文件，包含:
1. TIM外设枚举 (TIM1, TIM2, TIM3, TIM4)
2. TIM通道枚举 (CH1, CH2, CH3, CH4)
3. PWM初始化结构体:
   - TIM外设选择
   - 通道选择
   - 频率(Hz)
   - 初始占空比(0-1000表示0-100.0%)
4. PWM操作函数声明:
   - `pwm_init()` - 初始化PWM定时器
   - `pwm_set_duty()` - 设置占空比(0-1000)
   - `pwm_set_frequency()` - 设置频率
   - `pwm_start()` - 启动PWM输出
   - `pwm_stop()` - 停止PWM输出

### 文件 2: `firmware/stm32/hal/pwm.c`
创建PWM HAL实现文件，包含:
1. TIM寄存器结构定义
2. RCC时钟使能 (TIM1=APB2, TIM2/3/4=APB1)
3. 定时器基础地址映射
4. `pwm_init()` 实现:
   - 使能TIM时钟
   - 配置预分频器和自动重装载值
   - 配置PWM模式1
   - 配置输出比较
5. `pwm_set_duty()` 实现 - 修改CCR寄存器
6. `pwm_set_frequency()` 实现 - 修改ARR和PSC
7. `pwm_start()` / `pwm_stop()` 实现

### 关键硬件寄存器
- TIM1: 0x40010000 (APB2)
- TIM2: 0x40000000 (APB1)
- TIM3: 0x40000400 (APB1)
- TIM4: 0x40000800 (APB1)
- RCC_APB1ENR: 0x40023840 (bits 0,1,2 for TIM2,3,4)
- RCC_APB2ENR: 0x40023844 (bit 0 for TIM1)

---

## 完成标准 (必须可验证)

- [ ] 文件创建: `firmware/stm32/hal/pwm.h` 存在且编译通过
- [ ] 文件创建: `firmware/stm32/hal/pwm.c` 存在且编译通过
- [ ] 接口完整: 所有声明的函数都有对应实现
- [ ] 支持4路独立PWM输出配置
- [ ] 占空比设置精度: 0.1% (0-1000范围)
- [ ] 频率可调范围: 50Hz - 20kHz
- [ ] 代码风格: 符合STM32 HAL命名规范
- [ ] 无编译警告: GCC编译无警告 (-Wall -Wextra)
- [ ] 代码审查: 通过Reviewer审查

---

## 相关文件 (Hook范围检查用)
- `firmware/stm32/hal/pwm.h`
- `firmware/stm32/hal/pwm.c`
- `firmware/stm32/hal/hal_common.h` (只读引用)

---

## 注意事项

- TIM1在APB2总线上，TIM2/3/4在APB1总线上
- APB1时钟通常=SYSCLK/2 (50MHz @ 100MHz SYSCLK)
- APB2时钟通常=SYSCLK/1 (100MHz @ 100MHz SYSCLK)
- PWM频率计算公式: Fpwm = Fclk / (PSC+1) / (ARR+1)
- 占空比设置: CCR = (ARR+1) * duty / 1000
- 需要GPIO复用功能配置(AF mode) - 使用已有的gpio_init()

---

## 验收清单 (Reviewer使用)
- [ ] 修改在任务范围内 (2个HAL文件)
- [ ] 无新引入的未声明依赖
- [ ] 代码风格符合项目规范
- [ ] TIM寄存器地址正确
- [ ] 无新的技术债务产生
