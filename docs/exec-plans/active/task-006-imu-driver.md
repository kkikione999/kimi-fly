# Task 006: IMU驱动 (ICM-42688-P)

## 目标
实现ICM-42688-P 6轴IMU传感器的驱动程序，提供陀螺仪和加速度计数据读取功能。

## 背景上下文

### 相关代码
- 文件: `/Users/ll/kimi-fly/firmware/stm32/hal/spi.c` - SPI3 HAL实现，用于IMU通信
- 文件: `/Users/ll/kimi-fly/firmware/stm32/hal/spi.h` - SPI接口定义
- 参考: 现有HAL代码使用寄存器直接操作风格

### 依赖关系
- 前置任务: Task 005 (SPI HAL) - 已完成
- 外部依赖: SPI3 HAL接口
- 硬件接口: SPI3

### 硬件信息
- **芯片型号**: ICM-42688-P
- **接口**: SPI3
- **引脚连接**:
  - NSS: PA15 (STM_SPI_NSS3)
  - SCK: PB3 (STM_SPI_CLK3)
  - MISO: PB4 (STM_SPI_MISO3)
  - MOSI: PB5 (STM_SPI_MOSI3)
  - INT1: IMU_INT1 (中断引脚，可选使用)
- **SPI配置**: Mode 0 (CPOL=0, CPHA=0), 8-bit, 最大8MHz
- **WHO_AM_I寄存器**: 0x75，预期值 0x47

### ICM-42688-P 关键寄存器
| 寄存器 | 地址 | 说明 |
|--------|------|------|
| WHO_AM_I | 0x75 | 设备ID (0x47) |
| PWR_MGMT0 | 0x4E | 电源管理 |
| GYRO_CONFIG0 | 0x4F | 陀螺仪配置 |
| ACCEL_CONFIG0 | 0x50 | 加速度计配置 |
| TEMP_DATA1 | 0x1D | 温度数据高字节 |
| TEMP_DATA0 | 0x1E | 温度数据低字节 |
| ACCEL_DATA_X1 | 0x1F | 加速度X高字节 |
| ACCEL_DATA_X0 | 0x20 | 加速度X低字节 |
| ACCEL_DATA_Y1 | 0x21 | 加速度Y高字节 |
| ACCEL_DATA_Y0 | 0x22 | 加速度Y低字节 |
| ACCEL_DATA_Z1 | 0x23 | 加速度Z高字节 |
| ACCEL_DATA_Z0 | 0x24 | 加速度Z低字节 |
| GYRO_DATA_X1 | 0x25 | 陀螺仪X高字节 |
| GYRO_DATA_X0 | 0x26 | 陀螺仪X低字节 |
| GYRO_DATA_Y1 | 0x27 | 陀螺仪Y高字节 |
| GYRO_DATA_Y0 | 0x28 | 陀螺仪Y低字节 |
| GYRO_DATA_Z1 | 0x29 | 陀螺仪Z高字节 |
| GYRO_DATA_Z0 | 0x2A | 陀螺仪Z低字节 |

### 代码规范
- 遵循 STM32 HAL 风格
- 寄存器访问使用 volatile
- 使用 SPI HAL 接口进行通信
- 提供初始化、读取、配置等标准接口

## 具体修改要求

### 文件 1: `/Users/ll/kimi-fly/firmware/stm32/drivers/icm42688.h`
1. 定义 ICM-42688-P 寄存器地址常量
2. 定义陀螺仪/加速度计量程枚举
3. 定义 ODR (输出数据率) 枚举
4. 定义 ICM-42688-P 句柄结构体 (包含spi_handle_t指针)
5. 定义数据读取结构体 (加速度、陀螺仪、温度)
6. 声明API函数:
   - `icm42688_init()` - 初始化传感器
   - `icm42688_deinit()` - 反初始化
   - `icm42688_read_id()` - 读取WHO_AM_I
   - `icm42688_read_data()` - 读取所有传感器数据
   - `icm42688_set_gyro_range()` - 设置陀螺仪量程
   - `icm42688_set_accel_range()` - 设置加速度计量程
   - `icm42688_set_odr()` - 设置输出数据率

### 文件 2: `/Users/ll/kimi-fly/firmware/stm32/drivers/icm42688.c`
1. 包含 `icm42688.h` 和 `spi.h`
2. 实现寄存器读写函数 (内部使用，基于spi_transmit_receive)
3. 实现 `icm42688_init()`:
   - 验证 WHO_AM_I (应为0x47)
   - 配置电源管理 (开启陀螺仪和加速度计)
   - 设置默认量程和ODR
4. 实现 `icm42688_read_data()`:
   - 读取温度、加速度、陀螺仪原始数据
   - 数据格式: 16位有符号数，大端序
5. 实现量程和ODR配置函数
6. 实现数据转换函数 (原始值转物理单位)

## 完成标准 (必须可验证)

- [ ] 代码编译通过，无警告
- [ ] `icm42688_read_id()` 返回 0x47
- [ ] 提供数据读取测试代码片段 (可在main中调用验证)
- [ ] 陀螺仪数据范围符合配置的量程
- [ ] 加速度计数据范围符合配置的量程
- [ ] 代码遵循项目HAL风格规范

## 相关文件 (Hook范围检查用)
- `/Users/ll/kimi-fly/firmware/stm32/drivers/icm42688.h`
- `/Users/ll/kimi-fly/firmware/stm32/drivers/icm42688.c`
- `/Users/ll/kimi-fly/firmware/stm32/hal/spi.h` (只读引用)

## 注意事项
- ICM-42688-P 使用SPI Mode 0，已由spi3_init_for_imu配置
- 数据读取需要连续读取多个寄存器，使用burst read提高效率
- 传感器上电后需要短暂延时才能稳定
- 陀螺仪和加速度计可以独立使能/禁用
- 默认推荐配置: 加速度计 ±8g, 陀螺仪 ±2000dps, ODR 1kHz

## 验收清单 (Reviewer使用)
- [ ] 修改在任务范围内
- [ ] 无新引入的未声明依赖
- [ ] 寄存器地址定义正确
- [ ] WHO_AM_I验证逻辑正确
- [ ] 数据读取使用正确的字节序
- [ ] 无新的技术债务
