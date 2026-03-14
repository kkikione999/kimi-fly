# Task 004: IMU 传感器驱动 (MPU6050/ICM42688)

## 目标
实现 IMU (惯性测量单元) 传感器驱动，提供加速度计和陀螺仪数据读取。

## 背景上下文

### 硬件信息
- **候选传感器**: MPU6050 或 ICM42688
- **接口**: SPI1 (高速，8MHz)
- **引脚**: SPI1_SCK(PA5), SPI1_MISO(PA6), SPI1_MOSI(PA7), SPI1_CS(PA4)
- **中断**: IMU_INT(PA0) - 数据就绪中断

### 前置任务
- Task-001: HAL 层接口定义 ✅
- Task-002: STM32 HAL 实现 ✅

## 具体修改要求

### 1. 文件: `firmware/drivers/imu/imu.h`
IMU 驱动公共接口头文件。

内容需包含:
```c
// IMU 配置结构体
typedef struct {
    uint32_t sample_rate_hz;    // 采样率 (1000Hz)
    uint8_t accel_range;        // 加速度量程 (±2g, ±4g, ±8g, ±16g)
    uint8_t gyro_range;         // 陀螺仪量程 (±250, ±500, ±1000, ±2000 dps)
    bool use_interrupt;         // 是否使用中断模式
} imu_cfg_t;

// IMU 原始数据
typedef struct {
    int16_t accel_x, accel_y, accel_z;  // 原始ADC值
    int16_t gyro_x, gyro_y, gyro_z;     // 原始ADC值
    int16_t temp;                        // 温度传感器
    uint32_t timestamp_us;               // 时间戳 (微秒)
} imu_raw_data_t;

// IMU 物理数据 (转换后的工程单位)
typedef struct {
    float accel_x, accel_y, accel_z;     // m/s²
    float gyro_x, gyro_y, gyro_z;        // rad/s
    float temp;                           // °C
    uint32_t timestamp_us;
} imu_data_t;

// 函数声明
int imu_init(const imu_cfg_t *cfg);
int imu_read_raw(imu_raw_data_t *data);
int imu_read(imu_data_t *data);
int imu_data_ready(void);
int imu_calibrate(void);
```

### 2. 文件: `firmware/drivers/imu/mpu6050.c`
MPU6050 具体实现。

内容需包含:
```c
// MPU6050 寄存器定义
#define MPU6050_WHO_AM_I        0x75
#define MPU6050_PWR_MGMT_1      0x6B
#define MPU6050_CONFIG          0x1A
#define MPU6050_GYRO_CONFIG     0x1B
#define MPU6050_ACCEL_CONFIG    0x1C
#define MPU6050_INT_ENABLE      0x38
#define MPU6050_INT_STATUS      0x3A
#define MPU6050_ACCEL_XOUT_H    0x3B

// 初始化流程
// 1. 读取 WHO_AM_I 验证设备 (应为 0x68)
// 2. 退出睡眠模式 (PWR_MGMT_1 = 0x00)
// 3. 配置数字低通滤波器 (DLPF)
// 4. 设置陀螺仪量程
// 5. 设置加速度计量程
// 6. 使能数据就绪中断 (可选)

// 数据读取
// 读取 14 字节: accel(6) + temp(2) + gyro(6)
```

### 3. 文件: `firmware/drivers/imu/icm42688.c` (可选/高级)
ICM42688 实现（如果硬件选用此传感器）。

### 4. 文件: `firmware/drivers/imu/imu.c`
IMU 驱动抽象层。

内容需包含:
```c
// 根据配置选择具体实现 (MPU6050 或 ICM42688)
// 数据转换: raw -> physical units
// 校准功能: 零偏校准
```

## 编码规范
- 使用 HAL SPI 接口进行通信
- 传感器读取使用 DMA (如果支持)
- 数据转换使用查找表或公式
- 校准数据存储在静态变量中

## 完成标准

- [ ] `firmware/drivers/imu/imu.h` - 公共接口
- [ ] `firmware/drivers/imu/mpu6050.c` - MPU6050 实现
- [ ] `firmware/drivers/imu/imu.c` - 抽象层
- [ ] 实现 WHO_AM_I 验证
- [ ] 实现配置设置
- [ ] 实现原始数据读取
- [ ] 实现物理单位转换
- [ ] 实现简单的零偏校准
- [ ] 代码通过编译检查

## 依赖关系

```
Depends on: task-001, task-002
Blocks: task-005-sensor-fusion
```

## 相关文件
- `firmware/hal/hal_interface.h` - HAL SPI 接口
- `firmware/platform/board_config.h` - 引脚配置

## 注意事项
- MPU6050 上电默认处于睡眠模式，必须退出睡眠
- SPI 读取时需要发送虚拟字节来接收数据
- 数据是大端格式，需要转换
- 陀螺仪零偏会随温度漂移，需要考虑温度补偿
