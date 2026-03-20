# 执行计划 - 无人机WiFi飞行控制

> **创建日期**: 2026-03-17
> **状态**: 进行中
> **当前阶段**: Phase 4

---

## 项目目标

通过WiFi远程控制无人机飞行。

---

## 阶段划分

### Phase 1: HAL层基础 ✅ 已完成
**目标**: 建立STM32硬件抽象层

- [x] Task 001: STM32 GPIO HAL实现
- [x] Task 002: STM32 PWM HAL实现 (电机控制)
- [x] Task 003: STM32 UART HAL实现 (ESP8266通信)
- [x] Task 004: STM32 I2C HAL实现 (传感器)
- [x] Task 005: STM32 SPI HAL实现 (气压计)

### Phase 2: 传感器驱动 ✅ 已完成
**目标**: 实现所有传感器驱动

- [x] Task 006: IMU驱动 (ICM-42688-P, **I2C1**)
  - 已修正: 从SPI3改为I2C1 (与pinout.md一致)
  - I2C地址: 0x68

- [x] Task 007: 气压计驱动 (LPS22HBTR, **SPI3**)
  - 已修正: 从I2C1改为SPI3 (与pinout.md一致)
  - 引脚: PA15(CS), PB3(SCK), PB4(MISO), PB5(MOSI)

- [x] Task 008: 磁力计驱动 (QMC5883P, I2C1)
  - 接口: I2C1 (PB6=SCL, PB7=SDA)
  - I2C地址: 0x2C (已修正)

### Phase 2: 传感器驱动与测试 ✅ 已完成
**目标**: 实现所有传感器驱动和测试程序

- [x] Task 006: IMU驱动 (ICM-42688-P, I2C1 @ 0x68)
- [x] Task 007: 气压计驱动 (LPS22HBTR, SPI3)
- [x] Task 008: 磁力计驱动 (QMC5883P, I2C1 @ 0x2C)
- [x] Task 009: 综合传感器测试程序
  - I2C总线扫描
  - 三传感器顺序测试
  - 错误处理和结果输出

### Phase 3: 姿态解算 (AHRS) ✅ 已完成
**目标**: 实现AHRS姿态解算算法

- [x] Task 010: 传感器数据融合与校准
  - 加速度计6面校准
  - 陀螺仪静止校准
  - 磁力计硬铁/软铁补偿

- [x] Task 011: Mahony AHRS算法实现
  - 6轴IMU更新
  - 9轴MARG更新
  - 欧拉角输出 (Roll/Pitch/Yaw)
  - 四元数运算

### Phase 4: 飞控算法 (PID) ✅ 已完成
**目标**: 实现PID飞行控制

- [x] Task 012: PID控制器实现
  - 位置式和增量式PID
  - 抗积分饱和、微分滤波、输出限幅

- [x] Task 013: 角度环/角速度环整定
  - 7通道PID配置 (ROLL/PITCH/YAW角度+速率+高度)
  - 默认参数经验值

- [x] Task 014: 飞行控制核心
  - 级联PID控制 (外环角度 + 内环角速度)
  - X型四旋翼电机混控
  - 飞行模式 (DISARMED/ARMED/STABILIZE/ACRO)

### Phase 5: 通信协议 ✅ 已完成
**目标**: 建立WiFi通信

- [x] Task 015: ESP8266 WiFi STA模式配置
- [x] Task 016: 控制协议定义与实现
  - 二进制帧格式: [HEADER:2][LEN:1][CMD:1][DATA:N][CRC:2]
  - CRC16-CCITT校验
  - 最大负载64字节

- [x] Task 017: 命令解析器实现
  - ARM/DISARM命令
  - RC输入处理
  - PID参数读写
  - 遥测数据发送

- [x] Task 018: STM32-ESP32通信协议
  - 协议栈: protocol.c/h
  - 命令处理器: wifi_command.c/h

### Phase 6: 整合测试 ✅ 已完成
**目标**: 系统集成与飞行调试

- [x] Task 019: 主程序框架
  - 系统初始化序列 (HAL层 -> 传感器 -> AHRS -> 飞控)
  - 传感器校准程序

- [x] Task 020: 1kHz控制循环
  - IMU读取 -> AHRS更新 -> PID计算 -> 电机输出
  - WiFi任务处理 (200Hz)
  - 遥测发送 (50Hz)

- [x] Task 021: WiFi通信集成
  - 命令解析器集成到主循环
  - 遥测数据发送

- [x] Task 022: 地面站测试工具
  - Python CLI地面站程序
  - 键盘控制 (w/s油门, a/d横滚, i/k俯仰, j/l偏航)
  - ARM/DISARM和模式切换
  - 实时遥测数据显示

- [ ] Task 023: 飞行调试 (用户辅助插入电池)

---

## 硬件接口配置 (最终版)

基于`hardware-docs/pinout.md`硬件真源:

| 外设 | 接口 | 引脚 | 地址/配置 |
|------|------|------|-----------|
| **ICM-42688-P** | **I2C1** | PB6(SCL), PB7(SDA) | **0x68** |
| **LPS22HBTR** | **SPI3** | PA15(CS), PB3(SCK), PB4(MISO), PB5(MOSI) | Mode 0 |
| **QMC5883P** | **I2C1** | PB6(SCL), PB7(SDA) | **0x2C** |
| ESP32-C3 | USART2 | PA2(TX), PA3(RX) | 115200 baud |
| 调试串口 | USART1 | PA9(TX), PA10(RX) | 460800 baud |

---

## 当前阶段

**Phase 6: 整合测试 ✅ 软件100%完成**

**全部软件任务已完成**:
- Task 019: 主程序框架 ✅
- Task 020: 1kHz控制循环 ✅
- Task 021: WiFi通信集成 ✅
- Task 022: 地面站测试工具 ✅

**剩余任务** (需硬件和人工介入):
- Task 023: 飞行调试 (PID整定/飞行测试)

---

## 已知风险

1. **USB供电限制**: 电机测试动力不足，无法起飞
2. **WiFi稳定性**: ESP8266与STM32通信需验证
3. **传感器校准**: 需要实际硬件测试环境
4. **接口修正影响**: 驱动接口已从原始设计变更，需测试验证

---

## 假设

1. 用户将在飞控调试阶段按需插入电池
2. WiFi网络 `whc/12345678` 可用
3. 硬件连接正确无误
4. `hardware-docs/pinout.md`是硬件设计的唯一真源

---

## 技术债务

见: `docs/exec-plans/tech-debt-tracker.md`

---

## 参考文档

- 架构地图: `CLAUDE.md`
- Harness流程: `RALPH-HARNESS.md`
- 用户意图: `docs/user-intent.md`
- **硬件引脚唯一真源**: `hardware-docs/pinout.md`
