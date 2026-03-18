# Agent Assignment - Phase 2

> **迭代**: 4
> **阶段**: Phase 2 - 传感器驱动
> **创建日期**: 2026-03-18
> **Harness-Architect**: 审核通过

---

## 任务分析

### Task 006: IMU驱动 (ICM-42688-P)
- **复杂度**: 高
- **域**: STM32驱动开发
- **接口**: SPI3
- **关键挑战**:
  - 大端序数据处理
  - Burst read连续读取多个寄存器
  - 陀螺仪和加速度计独立配置
- **预估时间**: 60-90分钟
- **推荐Agent**: **stm32-embedded-engineer**
- **原因**: 需要SPI通信知识，STM32寄存器操作经验，传感器驱动开发能力

### Task 007: 气压计驱动 (LPS22HBTR)
- **复杂度**: 中
- **域**: STM32驱动开发
- **接口**: I2C1
- **关键挑战**:
  - 24位有符号数处理（需要符号扩展）
  - BDU数据一致性机制
- **预估时间**: 45-60分钟
- **推荐Agent**: **stm32-embedded-engineer**
- **原因**: 需要I2C通信知识，熟悉I2C寄存器读写模式

### Task 008: 磁力计驱动 (QMC5883P)
- **复杂度**: 中
- **域**: STM32驱动开发
- **接口**: I2C1 (与气压计共享)
- **关键挑战**:
  - 小端序数据处理（与IMU不同）
  - CHIP_ID值未确认，需要Worker自适应
  - 多种配置参数（量程、ODR、过采样率）
- **预估时间**: 45-60分钟
- **推荐Agent**: **stm32-embedded-engineer**
- **原因**: 需要I2C通信知识，熟悉传感器配置模式

---

## 任务分配表

| 任务 | Agent类型 | 优先级 | 预估时间 | 依赖 |
|------|-----------|--------|----------|------|
| Task 006 (IMU) | stm32-embedded-engineer | P0 | 60-90min | Task 005 (SPI HAL) |
| Task 007 (气压计) | stm32-embedded-engineer | P1 | 45-60min | Task 004 (I2C HAL) |
| Task 008 (磁力计) | stm32-embedded-engineer | P1 | 45-60min | Task 004 (I2C HAL) |

---

## 执行策略

### 并行执行建议

```
Phase 2 执行时序:

Time -------------------------------------------------->

Task 006 (IMU)     [==========]
                          |
Task 007 (气压计)         [========]
                          |
Task 008 (磁力计)              [========]

说明:
- Task 006 单独优先执行（6轴数据核心，飞行控制最重要）
- Task 007 和 Task 008 可并行执行（共享I2C1但地址不同，无冲突）
- 建议先完成Task 007再启动Task 008，避免同时修改I2C HAL
```

### 执行顺序建议

1. **首先执行 Task 006 (IMU)**
   - 优先级最高，提供6轴运动数据
   - 使用SPI3接口，与其他任务无资源冲突
   - 完成后可立即为Phase 3 (AHRS)提供数据

2. **然后并行执行 Task 007 和 Task 008**
   - 两者都使用I2C1接口
   - 设备地址不同(0x5C vs 0x0D)，可同时通信
   - 但建议串行执行，避免潜在的总线竞争问题

---

## Hook 配置要点

### EnterWorktree Hook 检查

| 任务 | 分支名格式 | 基础Commit | 任务文档读取确认 |
|------|-----------|-----------|-----------------|
| Task 006 | `task-006-imu-driver` | main最新 | 必须读取task-006-imu-driver.md |
| Task 007 | `task-007-barometer-driver` | main最新 | 必须读取task-007-barometer-driver.md |
| Task 008 | `task-008-magnetometer-driver` | main最新 | 必须读取task-008-magnetometer-driver.md |

### Edit/Write Hook 检查

**Task 006 允许修改文件:**
- `/Users/ll/kimi-fly/firmware/stm32/drivers/icm42688.h` (新建)
- `/Users/ll/kimi-fly/firmware/stm32/drivers/icm42688.c` (新建)
- `/Users/ll/kimi-fly/firmware/stm32/hal/spi.h` (只读引用)

**Task 007 允许修改文件:**
- `/Users/ll/kimi-fly/firmware/stm32/drivers/lps22hb.h` (新建)
- `/Users/ll/kimi-fly/firmware/stm32/drivers/lps22hb.c` (新建)
- `/Users/ll/kimi-fly/firmware/stm32/hal/i2c.h` (只读引用)

**Task 008 允许修改文件:**
- `/Users/ll/kimi-fly/firmware/stm32/drivers/qmc5883p.h` (新建)
- `/Users/ll/kimi-fly/firmware/stm32/drivers/qmc5883p.c` (新建)
- `/Users/ll/kimi-fly/firmware/stm32/hal/i2c.h` (只读引用)

### Pre-merge Hook 检查

- [ ] Reviewer审批通过 (code-reviewer)
- [ ] WHO_AM_I/CHIP_ID读取验证通过
- [ ] 代码风格符合项目规范
- [ ] 技术债务记录到tech-debt-tracker.md
- [ ] 测试代码片段可编译运行

---

## 风险监控点

### 高风险项

1. **Task 008 CHIP_ID值未确认**
   - **风险**: Worker可能无法验证设备存在
   - **缓解**: Worker应实现自适应逻辑，读取CHIP_ID并打印，不强制检查特定值
   - **监控**: Harness-Architect需确认Worker处理了此不确定性

2. **I2C1总线共享**
   - **风险**: Task 007和008同时修改I2C HAL可能产生冲突
   - **缓解**: 建议串行执行，先完成Task 007再启动Task 008
   - **监控**: 检查是否有并发修改HAL文件的情况

3. **字节序混淆**
   - **风险**: IMU(大端)和磁力计(小端)字节序不同，容易出错
   - **缓解**: 任务文档已明确标注，Worker需特别注意
   - **监控**: Reviewer需重点检查数据转换代码

### 中等风险项

4. **24位气压数据符号扩展**
   - **风险**: Task 007中24位有符号数处理容易出错
   - **缓解**: 任务文档已提供处理指导
   - **监控**: Reviewer验证符号扩展逻辑

5. **SPI3 NSS控制**
   - **风险**: IMU需要精确的NSS控制时序
   - **缓解**: 使用已实现的软件NSS控制
   - **监控**: 检查NSS拉高拉低时序

---

## Worker启动前检查清单

- [x] 任务文档已审核通过
- [x] 前置任务已完成 (Task 004, 005)
- [x] Agent类型已确定
- [x] Hook规则已配置
- [x] 风险点已识别
- [ ] Worker已读取任务文档
- [ ] Worker已确认理解字节序要求
- [ ] Worker已确认理解CHIP_ID自适应策略 (Task 008)

---

## 状态

**Agent分配状态**: READY FOR ASSIGNMENT

Harness-Architect审核完成，可以启动Workers执行任务。

---

*文档版本: 2.0 - Phase 2*
*审核人: Harness-Architect*
*日期: 2026-03-18*
