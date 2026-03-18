# Task 005: STM32 SPI HAL实现

## 目标
实现STM32 SPI硬件抽象层，支持SPI3与IMU(ICM-42688-P)通信。

## 背景上下文

### 相关代码
- 新建文件: `firmware/stm32/hal/spi.h` - SPI HAL接口定义
- 新建文件: `firmware/stm32/hal/spi.c` - SPI HAL实现
- 依赖文件: `firmware/stm32/hal/hal_common.h` - 错误码定义
- 依赖文件: `firmware/stm32/hal/gpio.h` - GPIO配置

### 依赖关系
- 前置任务: 无 (并行开发，需协调hal_common.h)
- 外部依赖: STM32F4xx标准库

### 硬件信息
- 外设: SPI3
- 引脚:
  - STM_SPI_NSS3 (片选)
  - STM_SPI_CLK3 (时钟)
  - STM_SPI_MOSI3 (数据入)
  - STM_SPI_MISO3 (数据出)
- 模式: 主模式，全双工
- 时钟极性: CPOL=0, CPHA=0 (Mode 0)
- 时钟频率: 最高8MHz (IMU支持)
- 参考: `hardware-docs/pinout.md` 第3.1节

### 代码规范
- 遵循STM32 HAL风格
- 支持软件NSS控制
- 支持全双工传输
- 支持8位/16位数据帧

## 具体修改要求

### 文件 1: `firmware/stm32/hal/spi.h`
1. 定义spi_handle_t结构体
2. 定义spi_config_t配置结构体 (模式、时钟分频、数据大小)
3. 声明API函数:
   - `spi_init()` - 初始化SPI
   - `spi_deinit()` - 反初始化
   - `spi_transmit()` - 发送数据 (阻塞)
   - `spi_receive()` - 接收数据 (阻塞)
   - `spi_transmit_receive()` - 全双工传输
   - `spi_set_nss()` - 软件控制片选

### 文件 2: `firmware/stm32/hal/spi.c`
1. 实现SPI3初始化 (GPIO复用、时钟使能)
2. 实现主模式发送/接收 (轮询方式)
3. 实现全双工传输
4. 实现软件NSS控制
5. 实现错误处理 (溢出、模式错误、帧错误)

## 完成标准 (必须可验证)

- [ ] 接口定义完整: 所有声明的函数都有实现
- [ ] 编译通过: 无警告无错误
- [ ] 支持Mode 0 (CPOL=0, CPHA=0)
- [ ] 支持8位数据帧
- [ ] GPIO配置正确: SPI3引脚复用
- [ ] 支持软件NSS控制
- [ ] 代码审查通过

## 相关文件 (Hook范围检查)
- `firmware/stm32/hal/spi.h`
- `firmware/stm32/hal/spi.c`
- `firmware/stm32/hal/hal_common.h`
- `firmware/stm32/hal/gpio.h`

## 注意事项
- SPI3时钟源为APB1 (42MHz)
- NSS使用软件控制，GPIO推挽输出
- IMU要求Mode 0
- 时钟分频需要根据APB1时钟计算

## 验收清单 (Reviewer使用)
- [ ] 修改在任务范围内
- [ ] 无新引入的未声明依赖
- [ ] 代码符合HAL风格规范
- [ ] 无新的技术债务
