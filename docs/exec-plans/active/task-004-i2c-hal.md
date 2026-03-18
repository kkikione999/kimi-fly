# Task 004: STM32 I2C HAL实现

## 目标
实现STM32 I2C硬件抽象层，支持I2C1与气压计(LPS22HBTR)和磁力计(QMC5883P)通信。

## 背景上下文

### 相关代码
- 新建文件: `firmware/stm32/hal/i2c.h` - I2C HAL接口定义
- 新建文件: `firmware/stm32/hal/i2c.c` - I2C HAL实现
- 依赖文件: `firmware/stm32/hal/hal_common.h` - 错误码定义
- 依赖文件: `firmware/stm32/hal/gpio.h` - GPIO配置

### 依赖关系
- 前置任务: 无 (并行开发，需协调hal_common.h)
- 外部依赖: STM32F4xx标准库

### 硬件信息
- 外设: I2C1
- 引脚: PB6 (SCL), PB7 (SDA)
- 时钟频率: 100kHz (标准模式) / 400kHz (快速模式)
- 设备地址:
  - LPS22HBTR: 0x5C (SA0=0) 或 0x5D (SA0=1)
  - QMC5883P: 0x0D
- 参考: `hardware-docs/pinout.md` 第3.2、3.3节

### 代码规范
- 遵循STM32 HAL风格
- 支持主模式发送/接收
- 支持7位地址
- 超时机制防止死锁

## 具体修改要求

### 文件 1: `firmware/stm32/hal/i2c.h`
1. 定义i2c_handle_t结构体
2. 定义i2c_config_t配置结构体 (时钟频率、地址模式)
3. 声明API函数:
   - `i2c_init()` - 初始化I2C
   - `i2c_deinit()` - 反初始化
   - `i2c_master_transmit()` - 主模式发送
   - `i2c_master_receive()` - 主模式接收
   - `i2c_mem_write()` - 向设备寄存器写数据
   - `i2c_mem_read()` - 从设备寄存器读数据
   - `i2c_scan()` - 扫描总线设备

### 文件 2: `firmware/stm32/hal/i2c.c`
1. 实现I2C1初始化 (GPIO复用、时钟使能)
2. 实现主模式发送/接收 (轮询方式)
3. 实现寄存器读写封装
4. 实现总线扫描功能
5. 实现错误处理 (仲裁丢失、总线错误、ACK失败)

## 完成标准 (必须可验证)

- [ ] 接口定义完整: 所有声明的函数都有实现
- [ ] 编译通过: 无警告无错误
- [ ] 支持100kHz和400kHz两种模式
- [ ] GPIO配置正确: PB6/PB7复用为I2C1
- [ ] 支持7位设备地址
- [ ] 代码审查通过

## 相关文件 (Hook范围检查)
- `firmware/stm32/hal/i2c.h`
- `firmware/stm32/hal/i2c.c`
- `firmware/stm32/hal/hal_common.h`
- `firmware/stm32/hal/gpio.h`

## 注意事项
- I2C1时钟源为APB1 (42MHz)
- 需要配置GPIO为开漏输出，内部上拉
- 总线需要外部上拉电阻 (4.7kΩ)
- 注意START/STOP条件生成时序

## 验收清单 (Reviewer使用)
- [ ] 修改在任务范围内
- [ ] 无新引入的未声明依赖
- [ ] 代码符合HAL风格规范
- [ ] 无新的技术债务
