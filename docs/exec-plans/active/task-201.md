# Task 201: STM32 固件编译和烧录验证

## 目标
编译 STM32 飞控固件并通过 ST-Link 烧录到目标板，验证烧录成功。

## 背景上下文

### 相关代码
- 文件: `firmware/stm32/` - STM32 飞控固件完整代码
- 文件: `firmware/stm32/hal/` - HAL层 (gpio, pwm, uart, i2c, spi)
- 文件: `firmware/stm32/drivers/` - 传感器驱动 (icm42688, lps22hb, qmc5883p)
- 文件: `firmware/stm32/algorithm/` - 算法层 (ahrs, pid, flight_controller)
- 文件: `firmware/stm32/comm/` - 通信层 (protocol, wifi_command)
- 文件: `firmware/stm32/main/` - 主程序 (flight_main)

### 依赖关系
- 前置任务: 无
- 外部依赖: PlatformIO 或 STM32CubeIDE, ST-Link 驱动

### 硬件信息
- 涉及引脚: SWDIO (PA13), SWCLK (PA14), NRST
- 外设: ST-Link 调试器
- 参考: `hardware-docs/pinout.md` 第 2.3 节

### 代码规范
- 使用 STM32 HAL 库
- 编译器: ARM GCC
- 目标芯片: STM32F411CEU6

## 具体修改要求

### 文件 1: `firmware/stm32/platformio.ini` (如不存在则创建)
1. 创建 PlatformIO 配置文件
2. 配置 board = genericSTM32F411CE
3. 配置 framework = stm32cube
4. 配置 upload_protocol = stlink
5. 配置 build_flags 包含所有必要定义

### 文件 2: `firmware/stm32/CMakeLists.txt` (如使用 CLion/CMake)
1. 配置 CMake 构建系统
2. 添加所有源文件到编译列表
3. 配置交叉编译工具链

## 完成标准 (必须可验证)

- [ ] 标准 1: 固件成功编译，无错误
  - 验证方法: `pio run` 或 `make` 命令成功完成
- [ ] 标准 2: 固件通过 ST-Link 成功烧录
  - 验证方法: `pio run --target upload` 成功，或 STM32CubeProgrammer 显示烧录成功
- [ ] 标准 3: 烧录后程序运行正常
  - 验证方法: 调试串口 (PA9/PA10, 460800 baud) 有输出数据
- [ ] 标准 4: LED 指示灯正常
  - 验证方法: PB14 (LED_R) 或 PB15 (LED_L) 有状态指示

## 相关文件 (Hook范围检查用)
- `firmware/stm32/platformio.ini`
- `firmware/stm32/CMakeLists.txt`
- `firmware/stm32/Makefile` (如创建)

## 注意事项
- 确保 ST-Link 驱动已安装
- USB 供电模式下电机不会转动 (安全设计)
- 首次烧录建议使用 STM32CubeProgrammer 验证连接
- 如使用 PlatformIO，需要安装 `platformio` 命令行工具

## 验收清单 (Reviewer使用)
- [ ] 修改在任务范围内
- [ ] 无新引入的未声明依赖
- [ ] 编译配置正确
- [ ] 烧录步骤文档化
