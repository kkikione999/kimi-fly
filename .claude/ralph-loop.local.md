---
active: true
iteration: 4
session_id: "kimi-fly-iteration-4"
max_iterations: 0
completion_promise: null
started_at: "2026-03-18T04:01:04Z"
phase: "Phase 2 - 传感器驱动"
status: "APPROVED - Ready for Worker assignment"
---

# Ralph-Loop v2.0 Hook Configuration

> **迭代**: 4
> **阶段**: Phase 2 - 传感器驱动 (IMU/气压计/磁力计)
> **状态**: 计划已审核通过，等待Worker启动

---

## Hook系统配置

### EnterWorktree Hook (工作区创建前拦截)

**触发条件**: Worker调用EnterWorktree创建工作区前

**验证规则**:

| 检查项 | 验证规则 | 拦截动作 |
|--------|----------|----------|
| 分支名格式 | 必须符合 `task-{NNN}-{description}` | 拒绝创建，提示正确格式 |
| Task 006 | `task-006-imu-driver` | - |
| Task 007 | `task-007-barometer-driver` | - |
| Task 008 | `task-008-magnetometer-driver` | - |
| 基础Commit | 必须基于main分支最新commit | 拒绝创建，要求更新 |
| 任务文档读取 | Worker必须确认已读取对应task文档 | 拒绝创建，要求先读取 |

**工作区路径规范**:
```
.claude/worktrees/task-006-{agent-id}/
.claude/worktrees/task-007-{agent-id}/
.claude/worktrees/task-008-{agent-id}/
```

---

### Edit/Write Hook (文件修改前拦截)

**触发条件**: Worker修改任何文件前

**Task 006 (IMU驱动) 允许修改范围**:
```
ALLOWED:
- firmware/stm32/drivers/icm42688.h    [新建]
- firmware/stm32/drivers/icm42688.c    [新建]

READONLY (引用):
- firmware/stm32/hal/spi.h             [只读引用]
- firmware/stm32/hal/spi.c             [只读引用]

BLOCKED (越界修改):
- 其他HAL文件 (uart/i2c/gpio/pwm)
- 其他驱动文件
- 任何非相关文件
```

**Task 007 (气压计驱动) 允许修改范围**:
```
ALLOWED:
- firmware/stm32/drivers/lps22hb.h     [新建]
- firmware/stm32/drivers/lps22hb.c     [新建]

READONLY (引用):
- firmware/stm32/hal/i2c.h             [只读引用]
- firmware/stm32/hal/i2c.c             [只读引用]

BLOCKED (越界修改):
- 其他HAL文件 (uart/spi/gpio/pwm)
- 其他驱动文件
- 任何非相关文件
```

**Task 008 (磁力计驱动) 允许修改范围**:
```
ALLOWED:
- firmware/stm32/drivers/qmc5883p.h    [新建]
- firmware/stm32/drivers/qmc5883p.c    [新建]

READONLY (引用):
- firmware/stm32/hal/i2c.h             [只读引用]
- firmware/stm32/hal/i2c.c             [只读引用]

BLOCKED (越界修改):
- 其他HAL文件 (uart/spi/gpio/pwm)
- 其他驱动文件
- 任何非相关文件
```

**依赖检查规则**:
- 检查是否引入未在任务文档`依赖关系`中声明的新头文件
- 检查是否调用未声明的外部函数
- 检查是否修改与任务无关的文件

**拦截动作**:
- 修改文件不在`相关文件`列表中 → **阻止修改**，通知Worker
- 引入未声明依赖 → **阻止修改**，要求更新任务文档并重新审核
- 修改与任务无关的文件 → **阻止修改**，要求创建新任务

---

### Pre-merge Hook (合并前拦截)

**触发条件**: PR合并到main分支前

**检查清单**:

#### 1. 流程合规性
- [ ] Reviewer审批通过 (有审核报告)
- [ ] 修改在任务范围内
- [ ] 无新引入的未声明依赖

#### 2. 代码质量
- [ ] 代码编译通过，无警告
- [ ] 代码符合项目HAL风格规范
- [ ] WHO_AM_I/CHIP_ID读取验证通过

#### 3. 完成标准验证

**Task 006 完成标准**:
- [ ] `icm42688_read_id()` 返回 0x47
- [ ] 陀螺仪数据范围符合配置的量程
- [ ] 加速度计数据范围符合配置的量程
- [ ] 数据读取使用正确的大端序

**Task 007 完成标准**:
- [ ] `lps22hb_read_id()` 返回 0xB1
- [ ] 气压数据转换为合理范围 (950-1050 hPa)
- [ ] 温度数据转换为合理范围 (室温)
- [ ] 24位气压数据符号扩展正确

**Task 008 完成标准**:
- [ ] `qmc5883p_read_id()` 能正确读取CHIP_ID
- [ ] 磁场数据在合理范围 (0.3-0.6 Gauss)
- [ ] 三轴数据随传感器方向变化
- [ ] 数据读取使用正确的小端序

#### 4. 技术债务
- [ ] 新技术债务已记录到 `docs/exec-plans/tech-debt-tracker.md`
- [ ] PR描述清晰完整

**拦截动作**:
- 未完成标准未达标 → **阻止合并**，返回修改
- 无Reviewer审批 → **阻止合并**，要求审核
- 技术债务未记录 → **阻止合并**，要求记录

---

## Phase 2 特殊监控点

### 高风险监控项

| 风险 | 监控点 | 检测方法 | 响应动作 |
|------|--------|----------|----------|
| Task 008 CHIP_ID未确认 | Worker实现 | 检查代码是否强制检查特定值 | 要求改为自适应读取 |
| I2C1总线冲突 | Task 007 & 008 | 检查是否同时修改I2C HAL | 阻止并行执行，改为串行 |
| 字节序错误 | 数据转换代码 | 检查大小端转换逻辑 | 要求修正并重新审核 |
| 24位符号扩展错误 | Task 007气压数据 | 检查24位有符号数处理 | 要求修正并重新审核 |

### Worker自检要求

每个Worker在提交PR前必须确认:

**Task 006 (IMU)**:
- [ ] 已验证WHO_AM_I读取返回0x47
- [ ] 已确认使用大端序读取传感器数据
- [ ] 已测试burst read连续读取功能

**Task 007 (气压计)**:
- [ ] 已验证WHO_AM_I读取返回0xB1
- [ ] 已确认24位气压数据的符号扩展正确
- [ ] 已启用BDU确保数据一致性

**Task 008 (磁力计)**:
- [ ] 已读取CHIP_ID并记录实际值
- [ ] 已确认使用小端序读取传感器数据
- [ ] 已实现自适应CHIP_ID验证逻辑

---

## Agent分配状态

| 任务 | Agent类型 | 优先级 | 状态 |
|------|-----------|--------|------|
| Task 006: IMU驱动 | stm32-embedded-engineer | P0 | 待分配 |
| Task 007: 气压计驱动 | stm32-embedded-engineer | P1 | 待分配 |
| Task 008: 磁力计驱动 | stm32-embedded-engineer | P1 | 待分配 |

**执行建议**:
1. 首先分配Task 006 (IMU) - 6轴数据核心
2. Task 006完成后，可并行分配Task 007和008
3. 或串行执行：006 -> 007 -> 008

---

## 参考文档

- 任务文档:
  - `docs/exec-plans/active/task-006-imu-driver.md`
  - `docs/exec-plans/active/task-007-barometer-driver.md`
  - `docs/exec-plans/active/task-008-magnetometer-driver.md`
- Agent分配: `ralph-loop-session/agent-assignment.md`
- 本轮计划: `ralph-loop-session/plan.md`
- Harness规范: `RALPH-HARNESS.md`

---

*Hook配置版本: 2.0*
*配置日期: 2026-03-18*
*配置人: Harness-Architect*
