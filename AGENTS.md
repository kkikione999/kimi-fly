# Agent Index - 可用Agent列表

> **项目**: 无人机WiFi飞行控制器
> **Harness**: RALPH-HARNESS.md

---

## Ralph-loop 核心 Agent

| Agent | 角色 | 职责 | 触发时机 |
|-------|------|------|----------|
| **team-orchestrator** | Leader | 编排、计划、任务分配 | 每轮首先启动 |
| **code-reviewer** | Reviewer | 审核计划、任务、代码、PR | 计划制定后 |
| **harness-architect** | Architect | 流程监控、架构设计 | Leader之后启动 |

---

## 专业 Worker Agent

| Agent | 专长 | 适用任务 |
|-------|------|----------|
| **stm32-embedded-engineer** | STM32 HAL/LL驱动 | GPIO/PWM/UART/I2C/SPI开发 |
| **esp32-c3-autonomous-engineer** | ESP32-C3/WiFi | WiFi模块、通信协议 |
| **embedded-test-engineer** | 嵌入式测试 | 单元测试、HIL测试 |
| **code-simplifier** | 代码优化 | 重构、简化 |

---

## 辅助 Agent

| Agent | 用途 |
|-------|------|
| **plan-reviewer** | 计划审查（通用） |
| **code-structure-analyst** | 代码结构分析 |
| **documentation-reviewer** | 文档审查 |
| **quality-inspector** | 质量检查 |

---

## Agent 选择指南

### HAL层开发
- GPIO → `stm32-embedded-engineer`
- PWM → `stm32-embedded-engineer`
- UART → `stm32-embedded-engineer`
- I2C → `stm32-embedded-engineer`
- SPI → `stm32-embedded-engineer`

### 传感器驱动
- IMU (ICM-42688-P) → `stm32-embedded-engineer`
- 气压计 (LPS22HBTR) → `stm32-embedded-engineer`
- 磁力计 (QMC5883P) → `stm32-embedded-engineer`

### 通信模块
- ESP32-C3 WiFi → `esp32-c3-autonomous-engineer`
- 控制协议 → `esp32-c3-autonomous-engineer`

### 测试
- 单元测试 → `embedded-test-engineer`
- 集成测试 → `embedded-test-engineer`

---

## Agent 定义位置

所有Agent定义位于: `.claude/agents/`

```
.claude/agents/
├── team-orchestrator.md       # Leader
├── code-reviewer.md           # Reviewer
├── harness-architect.md       # Architect
├── stm32-embedded-engineer.md # STM32专家
├── esp32-c3-autonomous-engineer.md # ESP32专家
├── embedded-test-engineer.md  # 测试专家
├── code-simplifier.md         # 代码简化
└── plan-reviewer.md           # 计划审查
```

---

## 快速参考

```bash
# Leader - 编排任务
/team-orchestrator

# Reviewer - 审核代码
/code-reviewer

# STM32开发
/stm32-embedded-engineer

# ESP32开发
/esp32-c3-autonomous-engineer

# 启动完整Ralph-loop
/ralph-loop
```
