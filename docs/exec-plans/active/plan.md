# 执行计划 - 无人机WiFi飞行控制

> **创建日期**: 2026-03-17
> **状态**: 初始化
> **当前阶段**: Phase 1

---

## 项目目标

通过WiFi远程控制无人机飞行。

---

## 阶段划分

### Phase 1: HAL层基础
**目标**: 建立STM32硬件抽象层

- [ ] Task 001: STM32 GPIO HAL实现
- [ ] Task 002: STM32 PWM HAL实现 (电机控制)
- [ ] Task 003: STM32 UART HAL实现 (ESP8266通信)
- [ ] Task 004: STM32 I2C HAL实现 (传感器)
- [ ] Task 005: STM32 SPI HAL实现 (IMU)

### Phase 2: 传感器驱动
**目标**: 实现所有传感器驱动

- [ ] Task 006: IMU驱动 (ICM-42688-P, SPI3)
- [ ] Task 007: 气压计驱动 (LPS22HBTR, I2C1)
- [ ] Task 008: 磁力计驱动 (QMC5883P, I2C1)

### Phase 3: 姿态解算
**目标**: 实现AHRS姿态解算

- [ ] Task 009: 传感器数据读取与校准
- [ ] Task 010: AHRS算法实现 (Mahony/Madgwick)

### Phase 4: 飞控算法
**目标**: 实现PID飞行控制

- [ ] Task 011: PID控制器实现
- [ ] Task 012: 角度环/角速度环整定

### Phase 5: 通信协议
**目标**: 建立WiFi通信

- [ ] Task 013: ESP8266 WiFi STA模式配置
- [ ] Task 014: 控制协议定义与实现
- [ ] Task 015: 命令解析器实现

### Phase 6: 整合测试
**目标**: 系统集成与飞行调试

- [ ] Task 016: 系统集成测试
- [ ] Task 017: 飞行调试 (用户辅助插入电池)

---

## 当前阶段

**Phase 1: HAL层基础**

当前任务: 待分配

---

## 已知风险

1. **USB供电限制**: 电机测试动力不足，无法起飞
2. **WiFi稳定性**: ESP8266与STM32通信需验证
3. **传感器校准**: 需要实际硬件测试环境

---

## 假设

1. 用户将在飞控调试阶段按需插入电池
2. WiFi网络 `whc/12345678` 可用
3. 硬件连接正确无误

---

## 技术债务

见: `docs/exec-plans/tech-debt-tracker.md`

---

## 参考文档

- 架构地图: `CLAUDE.md`
- Harness流程: `RALPH-HARNESS.md`
- 用户意图: `docs/user-intent.md`
- 硬件引脚: `docs/pinout.md`
