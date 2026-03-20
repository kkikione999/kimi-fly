# Task 203: 传感器读取验证

## 目标
验证 IMU、气压计、磁力计三传感器数据读取正常，输出合理的传感器数值。

## 背景上下文

### 相关代码
- 文件: `firmware/stm32/drivers/icm42688.c/h` - IMU 驱动
- 文件: `firmware/stm32/drivers/lps22hb.c/h` - 气压计驱动
- 文件: `firmware/stm32/drivers/qmc5883p.c/h` - 磁力计驱动
- 文件: `firmware/stm32/main/sensor_test.c/h` - 传感器测试程序

### 依赖关系
- 前置任务: Task 201 (STM32 固件烧录)
- 外部依赖: 无

### 硬件信息
- **IMU ICM-42688-P**: I2C1 (PB6=SCL, PB7=SDA), 地址 0x68
- **气压计 LPS22HBTR**: SPI3 (PA15=CS, PB3=SCK, PB4=MISO, PB5=MOSI)
- **磁力计 QMC5883P**: I2C1 (PB6=SCL, PB7=SDA), 地址 0x2C
- 参考: `hardware-docs/pinout.md` 第 3.1-3.3 节

### 代码规范
- 使用 HAL I2C/SPI API
- 错误处理: 返回 HAL_StatusTypeDef
- 数据输出: 通过调试串口 (USART1, 460800 baud)

## 具体修改要求

### 文件 1: `firmware/stm32/main/sensor_test.c`
1. 实现 I2C 总线扫描功能，检测设备地址
2. 实现三传感器顺序初始化
3. 实现循环读取并输出传感器数据
4. 添加错误检测和重试机制

### 文件 2: `firmware/stm32/main/sensor_test.h`
1. 定义测试函数声明
2. 定义输出格式宏

### 文件 3: `firmware/stm32/main/flight_main.c` (修改)
1. 在初始化阶段添加传感器自检
2. 在主循环中添加传感器数据输出 (调试用)
3. 添加传感器状态 LED 指示

## 完成标准 (必须可验证)

- [ ] 标准 1: I2C 总线扫描检测到两个设备 (0x68, 0x2C)
  - 验证方法: 串口输出显示 "Found device at 0x68" 和 "Found device at 0x2C"
- [ ] 标准 2: IMU 数据读取正常
  - 验证方法: 串口输出加速度 (X/Y/Z, 单位 g) 和陀螺仪 (X/Y/Z, 单位 dps) 数据
  - 合理性: 静止时 Z 轴加速度约 1g，其他轴约 0g；陀螺仪各轴接近 0 dps
- [ ] 标准 3: 气压计数据读取正常
  - 验证方法: 串口输出温度 (°C) 和气压 (hPa) 数据
  - 合理性: 温度约 20-30°C，气压约 1000-1020 hPa
- [ ] 标准 4: 磁力计数据读取正常
  - 验证方法: 串口输出磁场强度 (X/Y/Z, 单位 uT) 数据
  - 合理性: 各轴数值在合理范围内 (地球磁场约 25-65 uT)

## 相关文件 (Hook范围检查用)
- `firmware/stm32/main/sensor_test.c`
- `firmware/stm32/main/sensor_test.h`
- `firmware/stm32/main/flight_main.c`

## 注意事项
- 传感器数据需要合理范围验证，不仅仅是读取成功
- 磁力计在室内可能有干扰，数值可能偏差较大
- 如传感器无响应，检查 I2C/SPI 线路连接
- 气压计使用 SPI3，注意 CS 引脚 (PA15) 控制

## 验收清单 (Reviewer使用)
- [ ] 修改在任务范围内
- [ ] 无新引入的未声明依赖
- [ ] 传感器数据在合理范围内
- [ ] 错误处理完善
