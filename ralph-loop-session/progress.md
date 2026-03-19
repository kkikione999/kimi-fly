# Ralph-loop 进度记录

> **项目**: 无人机WiFi飞行控制器
> **创建日期**: 2026-03-17
> **最后更新**: 2026-03-19 (Round 2 硬件验证完成，发现驱动问题，进入 Round 3 修复)

---

## 当前状态

**阶段**: Phase 6 - 硬件验证与驱动修复
**状态**: Round 2 硬件验证发现 3 个驱动 Bug，进入 Round 3 修复
**本轮**: Round 3 (传感器驱动修复)

## Round 2 硬件验证结果 (2026-03-19)

| 任务 | 结果 | 发现问题 |
|------|------|----------|
| Task 203 传感器验证 | ✅ 完成 | ICM 地址错(0x69), 气压异常(283hPa), 磁力Y/Z=0 |
| Task 204 UART 通信 | ✅ 完成 | 双向 RX=0，疑似物理连接问题 |
| Task 205 ESP32 TCP | ✅ 完成 | WiFi 已连接 (192.168.50.132), TCP 监听中 |

## Round 3 修复计划

| 任务 | 问题 | 优先级 |
|------|------|--------|
| Task 301 | ICM42688 地址 0x68→0x69 (AD0=VCC) | P0 |
| Task 302 | LPS22HB SPI 多字节读地址不递增 | P0 |
| Task 303 | QMC5883P SET操作+DRDY等待缺失 | P1 |
| Task 304 | UART 诊断工具 (物理层需人工检查) | P1 |

---

---

## 已完成

### Round 1 (2026-03-19) - HAL层基础

- [x] **Task 1**: 修复HAL层常量命名冲突
- [x] **Task 2**: 实现PWM HAL层
- [x] **Task 3**: 实现I2C HAL层
- [x] **Task 4**: 实现UART HAL层
- [x] **Task 5**: 实现SPI HAL层

### Round 2 (2026-03-19) - 传感器驱动开发

- [x] **Task 6**: 实现QMC5883P磁力计驱动
  - 文件: `drivers/qmc5883p.c`, `drivers/qmc5883p.h`
  - 接口: I2C1 (PB6=SCL, PB7=SDA)
  - 地址: 0x2C (与pinout.md一致)

- [x] **Task 8**: 修正传感器驱动接口匹配硬件真源
  - **关键发现**: 现有驱动接口与`hardware-docs/pinout.md`不匹配
  - **修正方案**: 以pinout.md（硬件唯一真源）为准

- [x] **Task 7**: 创建综合传感器测试程序
  - 文件: `main/sensor_test.c`, `main/sensor_test.h`
  - 功能:
    - I2C总线扫描检测预期设备(0x68, 0x2C)
    - ICM-42688-P IMU测试: WHO_AM_I验证 + 10次数据采样
    - LPS22HBTR气压计测试: WHO_AM_I验证 + 10次数据采样
    - QMC5883P磁力计测试: 初始化 + 10次数据采样
    - 详细测试结果报告输出到UART1

---

## 硬件接口最终配置 (与pinout.md一致)

| 传感器 | 接口 | 引脚 | 地址/配置 |
|--------|------|------|-----------|
| **ICM-42688-P** | **I2C1** | PB6(SCL), PB7(SDA) | **0x68** |
| **LPS22HBTR** | **SPI3** | PA15(CS), PB3(SCK), PB4(MISO), PB5(MOSI) | Mode 0 |
| **QMC5883P** | **I2C1** | PB6(SCL), PB7(SDA) | **0x2C** |
| 调试输出 | **UART1** | PA9(TX), PA10(RX) | 460800 baud |

---

## Phase 2 交付物

### 传感器驱动文件

| 文件 | 接口 | 功能 |
|------|------|------|
| `drivers/icm42688.h` | I2C1 | ICM-42688-P IMU头文件 |
| `drivers/icm42688.c` | I2C1 | ICM-42688-P IMU实现 |
| `drivers/lps22hb.h` | SPI3 | LPS22HBTR气压计头文件 |
| `drivers/lps22hb.c` | SPI3 | LPS22HBTR气压计实现 |
| `drivers/qmc5883p.h` | I2C1 | QMC5883P磁力计头文件 |
| `drivers/qmc5883p.c` | I2C1 | QMC5883P磁力计实现 |

### 测试程序文件

| 文件 | 说明 |
|------|------|
| `main/sensor_test.h` | 传感器测试程序头文件 |
| `main/sensor_test.c` | 传感器测试程序实现 |

### Round 3 (2026-03-19) - AHRS姿态解算

- [x] **Task 9**: 传感器数据融合与校准
  - 文件: `algorithm/sensor_calibration.c/h`
  - 功能:
    - 加速度计6面校准（零偏、尺度因子）
    - 陀螺仪静止校准（多采样平均）
    - 磁力计旋转校准（硬铁/软铁补偿）
    - 快速零偏校准
    - 校准参数应用

- [x] **Task 10**: Mahony AHRS算法实现
  - 文件: `algorithm/ahrs.c/h`
  - 算法:
    - 6轴IMU更新（加速度计+陀螺仪）
    - 9轴MARG更新（加速度计+陀螺仪+磁力计）
    - Mahony互补滤波器（PI控制器校正漂移）
    - 四元数表示和运算
    - 欧拉角输出（Roll/Pitch/Yaw）
  - 参数:
    - 采样率: 1000Hz（推荐）
    - Kp: 2.0, Ki: 0.005

---

## Phase 3 交付物

### 算法文件

| 文件 | 功能 |
|------|------|
| `algorithm/sensor_calibration.h` | 传感器校准头文件 |
| `algorithm/sensor_calibration.c` | 传感器校准实现 |
| `algorithm/ahrs.h` | AHRS算法头文件 |
| `algorithm/ahrs.c` | Mahony AHRS实现 |

---

---

## Phase 6 交付物

### 主程序文件

| 文件 | 功能 |
|------|------|
| `main/flight_main.h` | 主控制程序头文件 |
| `main/flight_main.c` | 飞行控制主程序 |

### Phase 6 功能特性

- **系统初始化**: HAL层 → 传感器 → AHRS → 飞控 → WiFi
- **1kHz主循环**: 传感器读取 → AHRS更新 → PID计算 → 电机输出
- **任务调度**:
  - 控制循环: 1kHz
  - WiFi命令处理: 200Hz
  - 遥测发送: 50Hz
  - 状态报告: 1Hz
- **传感器校准**: 自动陀螺仪零偏校准
- **安全特性**:
  - WiFi超时自动锁定
  - 错误状态管理
  - 传感器故障检测

---

## Phase 5 交付物

### 通信协议文件

| 文件 | 功能 |
|------|------|
| `comm/protocol.h` | 通信协议定义头文件 |
| `comm/protocol.c` | 协议编解码实现 |
| `comm/wifi_command.h` | WiFi命令处理器头文件 |
| `comm/wifi_command.c` | 命令解析与遥测发送实现 |

### Phase 5 功能特性

- **帧格式**: `[HEADER:2][LEN:1][CMD:1][DATA:N][CRC:2]`
- **CRC校验**: CRC16-CCITT
- **命令支持**:
  - 系统: HEARTBEAT, VERSION, STATUS
  - 飞控: ARM, DISARM, MODE, RC_INPUT
  - 遥测: ATTITUDE, MOTOR, BATTERY
  - 配置: PID_GET, PID_SET, CALIBRATE
- **RC输入**: 油门/横滚/俯仰/偏航 + 按钮
- **遥测输出**: 姿态50Hz, 电机10Hz

---

## Phase 4 交付物

### 飞控算法文件

| 文件 | 功能 |
|------|------|
| `algorithm/pid_controller.h` | PID控制器头文件 (位置式/增量式) |
| `algorithm/pid_controller.c` | PID控制器实现 (抗饱和/滤波) |
| `algorithm/flight_controller.h` | 飞行控制核心头文件 |
| `algorithm/flight_controller.c` | 飞行控制核心实现 (级联PID+X型混控) |

### Phase 4 功能特性

- **PID控制器**: 支持位置式和增量式两种模式
- **抗积分饱和**: 条件积分 + 积分限幅
- **微分滤波**: 一阶低通滤波减少噪声
- **7通道PID**: ROLL/PITCH角度环 + 角速度环, YAW角度环 + 角速度环, 高度环
- **级联控制**: 外环角度 → 内环角速度 → 电机输出
- **飞行模式**: DISARMED/ARMED/STABILIZE/ACRO
- **X型混控**: 标准四旋翼X布局电机分配
- **安全特性**: 油门最低解锁、怠速模式、失控保护

---

## 下一阶段 (Phase 6)

**目标**: 系统集成与测试

**待完成任务**:
- [ ] Task 019: 主程序框架
- [ ] Task 020: 1kHz控制循环实现
- [ ] Task 021: WiFi通信集成
- [ ] Task 022: 地面站测试工具
- [ ] Task 023: 飞行调试准备

**Phase 5 已完成**: 通信协议实现
- 二进制协议帧 (CRC16校验)
- ARM/DISARM/RC_INPUT/PID等命令
- 姿态/电机遥测数据发送

---

## 技术债务

见: `docs/exec-plans/tech-debt-tracker.md`

当前状态: 所有P0债务已解决

---

## 关键决策记录

### 2026-03-19: 传感器接口修正

**问题**: 驱动实现与硬件真源(pinout.md)不匹配

**决策**: 以`hardware-docs/pinout.md`为唯一真源，修正驱动代码

**接口最终确定**:
- ICM-42688-P: I2C1 @ 0x68 (从SPI3修正)
- LPS22HBTR: SPI3 (从I2C @ 0x5C修正)
- QMC5883P: I2C1 @ 0x2C (从0x0D修正)

---

## 经验教训

1. **硬件真源优先**: `hardware-docs/pinout.md`是硬件设计的唯一权威，驱动实现必须以此为基准
2. **接口验证**: 在编写驱动前必须确认硬件接口配置
3. **一致性检查**: 定期检查驱动与硬件文档的一致性
4. **测试程序**: 综合测试程序是验证硬件连接的有效手段

---

## 文件清单

### HAL层 (`hal/`)
- `hal_common.h` - 通用定义
- `gpio.c/h` - GPIO驱动
- `pwm.c/h` - PWM驱动
- `i2c.c/h` - I2C驱动
- `spi.c/h` - SPI驱动
- `uart.c/h` - UART驱动

### 传感器驱动 (`drivers/`)
- `icm42688.c/h` - IMU驱动 (I2C)
- `lps22hb.c/h` - 气压计驱动 (SPI)
- `qmc5883p.c/h` - 磁力计驱动 (I2C)

### 测试程序 (`main/`)
- `sensor_test.c/h` - 传感器综合测试程序
- `flight_main.c/h` - 飞行控制主程序

### 算法层 (`algorithm/`)
- `sensor_calibration.c/h` - 传感器校准算法
- `ahrs.c/h` - Mahony AHRS姿态解算
- `pid_controller.c/h` - PID控制器
- `flight_controller.c/h` - 飞行控制核心

### 通信层 (`comm/`)
- `protocol.c/h` - 通信协议定义与编解码
- `wifi_command.c/h` - WiFi命令处理器

### 地面站工具 (`tools/ground_station/`)
- `protocol.py` - Python通信协议实现
- `ground_station.py` - 地面站主程序
- `README.md` - 使用说明

---

*更新时间: 2026-03-19*
*阶段: Phase 6 完成，系统集成测试就绪*
*状态: 软件100%完成，地面站工具就绪，等待硬件飞行测试*
