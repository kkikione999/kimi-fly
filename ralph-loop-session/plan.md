# Ralph-Loop Iteration 4 Plan - Phase 2

> **创建日期**: 2026-03-18
> **迭代**: 4
> **目标**: 完成Phase 2传感器驱动实现 (IMU/气压计/磁力计)

---

## 本轮目标

完成Phase 2传感器驱动层开发，为后续姿态解算(AHRS)提供数据基础。

---

## 任务清单

### Task 006: IMU驱动 (ICM-42688-P)
- **文件**: `docs/exec-plans/active/task-006-imu-driver.md`
- **目标**: 实现ICM-42688-P 6轴IMU驱动 (SPI3接口)
- **输出文件**:
  - `firmware/stm32/drivers/icm42688.h`
  - `firmware/stm32/drivers/icm42688.c`
- **依赖**: Task 005 (SPI HAL) - 已完成
- **状态**: 待启动

### Task 007: 气压计驱动 (LPS22HBTR)
- **文件**: `docs/exec-plans/active/task-007-barometer-driver.md`
- **目标**: 实现LPS22HBTR气压计驱动 (I2C1接口)
- **输出文件**:
  - `firmware/stm32/drivers/lps22hb.h`
  - `firmware/stm32/drivers/lps22hb.c`
- **依赖**: Task 004 (I2C HAL) - 已完成
- **状态**: 待启动

### Task 008: 磁力计驱动 (QMC5883P)
- **文件**: `docs/exec-plans/active/task-008-magnetometer-driver.md`
- **目标**: 实现QMC5883P磁力计驱动 (I2C1接口)
- **输出文件**:
  - `firmware/stm32/drivers/qmc5883p.h`
  - `firmware/stm32/drivers/qmc5883p.c`
- **依赖**: Task 004 (I2C HAL) - 已完成
- **状态**: 待启动

---

## 执行顺序

三个任务相互独立，可并行执行:
- IMU使用SPI3接口
- 气压计和磁力计共享I2C1接口但地址不同

建议执行顺序:
1. Task 006 (IMU) - 优先级最高，6轴数据最重要
2. Task 007 (气压计) 和 Task 008 (磁力计) - 可并行

---

## 验收标准

每个任务完成后需要验证:
- [ ] 代码编译通过，无警告
- [ ] WHO_AM_I/CHIP_ID读取正确
- [ ] 数据读取功能正常
- [ ] 数据转换公式正确
- [ ] 代码风格符合项目规范

---

## 技术债务记录

本轮产生的技术债务需记录到 `docs/exec-plans/tech-debt-tracker.md`

---

## 风险与注意事项

1. **SPI3引脚注意**: IMU使用PA15(NSS), PB3(SCK), PB4(MISO), PB5(MOSI)
2. **I2C地址冲突**: 气压计(0x5C)和磁力计(0x0D)地址不同，无冲突
3. **数据字节序**:
   - IMU (ICM-42688-P): 大端序 (Big Endian)
   - 气压计 (LPS22HBTR): 大端序
   - 磁力计 (QMC5883P): 小端序 (Little Endian)
4. **传感器校准**: 本阶段只实现原始数据读取，校准在Phase 3处理

---

## 参考文档

- 硬件规格: `hardware-docs/components.md`
- 引脚定义: `hardware-docs/pinout.md`
- SPI HAL: `firmware/stm32/hal/spi.c`, `firmware/stm32/hal/spi.h`
- I2C HAL: `firmware/stm32/hal/i2c.c`, `firmware/stm32/hal/i2c.h`
- Harness流程: `RALPH-HARNESS.md`

---

## 上一轮回顾 (Iteration 3)

**已完成**:
- Task 003: UART HAL
- Task 004: I2C HAL
- Task 005: SPI HAL

**状态**: Phase 1 (HAL层基础) 全部完成
