# Task 001: STM32 GPIO HAL实现

## 目标
实现STM32F411CEU6的GPIO硬件抽象层，提供统一的GPIO操作接口。

---

## 背景上下文

### 相关代码
- 当前项目尚无HAL层实现，需要从零建立
- STM32F411CEU6使用标准GPIO寄存器结构

### 依赖关系
- 前置任务: 无 (本轮第一个任务)
- 外部依赖: STM32F4xx标准外设库/HAL库头文件

### 硬件信息
- 涉及GPIO端口: PA, PB, PC (主控板主要使用的端口)
- 参考: `hardware-docs/pinout.md` 第1.3节 GPIO引脚分配

### 代码规范
- 遵循STM32 HAL风格命名
- 寄存器访问使用volatile
- 使用位带操作或标准寄存器访问

---

## 具体修改要求

### 文件 1: `firmware/stm32/hal/hal_common.h`
创建HAL通用定义文件，包含:
1. 标准类型定义 (stdint)
2. 布尔类型定义
3. HAL错误码枚举
4. 通用宏定义

### 文件 2: `firmware/stm32/hal/gpio.h`
创建GPIO HAL头文件，包含:
1. GPIO端口枚举 (GPIO_PORT_A, GPIO_PORT_B, GPIO_PORT_C)
2. GPIO引脚枚举 (GPIO_PIN_0 ~ GPIO_PIN_15)
3. GPIO模式枚举 (输入/输出/复用/模拟)
4. GPIO速度枚举 (低/中/高/超高)
5. GPIO上下拉枚举
6. GPIO初始化结构体
7. GPIO操作函数声明:
   - `gpio_init()` - 初始化GPIO
   - `gpio_write()` - 写GPIO输出
   - `gpio_read()` - 读GPIO输入
   - `gpio_toggle()` - 翻转GPIO状态
   - `gpio_set_mode()` - 设置GPIO模式

### 文件 3: `firmware/stm32/hal/gpio.c`
创建GPIO HAL实现文件，包含:
1. 端口基址映射表
2. RCC时钟使能函数
3. `gpio_init()` 实现
4. `gpio_write()` 实现
5. `gpio_read()` 实现
6. `gpio_toggle()` 实现
7. `gpio_set_mode()` 实现

---

## 完成标准 (必须可验证)

- [ ] 文件创建: `firmware/stm32/hal/hal_common.h` 存在且编译通过
- [ ] 文件创建: `firmware/stm32/hal/gpio.h` 存在且编译通过
- [ ] 文件创建: `firmware/stm32/hal/gpio.c` 存在且编译通过
- [ ] 接口完整: 所有声明的函数都有对应实现
- [ ] 代码风格: 符合STM32 HAL命名规范
- [ ] 无编译警告: GCC编译无警告 (-Wall -Wextra)
- [ ] 代码审查: 通过Reviewer审查

---

## 相关文件 (Hook范围检查用)
- `firmware/stm32/hal/hal_common.h`
- `firmware/stm32/hal/gpio.h`
- `firmware/stm32/hal/gpio.c`

---

## 注意事项

- STM32F411CEU6的GPIO寄存器基址:
  - GPIOA: 0x40020000
  - GPIOB: 0x40020400
  - GPIOC: 0x40020800
- RCC_AHB1ENR地址: 0x40023830
- 使用位带操作可以简化GPIO单个引脚的操作
- 注意GPIO模式寄存器是2位一组 (MODER)

---

## 验收清单 (Reviewer使用)
- [ ] 修改在任务范围内 (3个HAL文件)
- [ ] 无新引入的未声明依赖
- [ ] 代码风格符合项目规范
- [ ] 无新的技术债务产生
