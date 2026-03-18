# Ralph-loop 团队生命周期

> **版本**: 1.0
> **创建日期**: 2026-03-18
> **适用版本**: Ralph-loop v2.0

---

## 目录

1. [概述](#概述)
2. [阶段1: 团队创建 (TeamCreate)](#阶段1-团队创建-teamcreate)
3. [阶段2: 任务执行](#阶段2-任务执行)
4. [阶段3: 团队清理 (TeamDelete)](#阶段3-团队清理-teamdelete)
5. [完整流程示例](#完整流程示例)
6. [注意事项](#注意事项)
7. [故障排除](#故障排除)
8. [相关文档](#相关文档)

---

## 概述

Ralph-loop 使用 Claude Code 的 Team 功能管理多Agent协作。每轮循环包含三个明确的阶段：

1. **团队创建** - 建立固定核心团队
2. **任务执行** - 动态分配Worker执行任务
3. **团队清理** - 结束本轮循环

```
┌─────────────────────────────────────────────────────────────────┐
│                     Ralph-loop 团队生命周期                      │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│   ┌─────────────┐     ┌─────────────┐     ┌─────────────┐      │
│   │  阶段1      │────→│  阶段2      │────→│  阶段3      │      │
│   │ 团队创建    │     │ 任务执行    │     │ 团队清理    │      │
│   │ TeamCreate  │     │ Task分配    │     │ TeamDelete  │      │
│   └─────────────┘     └─────────────┘     └─────────────┘      │
│        │                   │                   │               │
│        ▼                   ▼                   ▼               │
│   ┌─────────┐         ┌─────────┐         ┌─────────┐          │
│   │固定核心 │         │动态Worker│         │清理配置 │          │
│   │3个Agent│         │按需加入 │         │结束本轮 │          │
│   └─────────┘         └─────────┘         └─────────┘          │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

---

## 阶段1: 团队创建 (TeamCreate)

### 工具调用

```bash
TeamCreate(
  team_name="kimi-fly-iteration-N",
  description="本轮迭代目标..."
)
```

### 自动创建的固定核心团队

调用 `TeamCreate` 后，系统自动创建以下Agent：

| 角色 | subagent_type | 职责 |
|------|---------------|------|
| Leader | team-orchestrator | 制定计划、分配任务、跟踪进度 |
| Reviewer | plan-reviewer | 审核计划和任务文档 |
| Architect | harness-architect | 配置Hook、流程监控、架构设计 |

### 配置文件位置

```
~/.claude/teams/kimi-fly-iteration-N/
├── config.json          # 团队配置
└── tasks/               # 任务目录
```

### 最佳实践

- 团队名称建议包含迭代号：`kimi-fly-iteration-3`
- 描述应清晰说明本轮目标
- 一轮迭代一个团队，避免混淆

---

## 阶段2: 任务执行

### 2.1 创建任务

Leader使用 `TaskCreate` 定义工作单元：

```bash
TaskCreate(
  subject="实现 UART HAL",
  description="详细描述任务内容..."
)
```

### 2.2 分配任务给动态Worker

**方式1 - TaskUpdate** (推荐):

```bash
# 分配任务给特定Agent类型
TaskUpdate(
  taskId="4",
  owner="stm32-embedded-engineer"
)
```

**方式2 - Agent工具**:

```bash
# 动态启动Worker加入团队
Agent(
  team_name="kimi-fly-iteration-N",
  subagent_type="stm32-embedded-engineer",
  prompt="任务详情..."
)
```

### 2.3 动态Worker类型

| subagent_type | 用途 |
|---------------|------|
| code-reviewer | 代码/PR审查 |
| stm32-embedded-engineer | STM32 HAL/驱动开发 |
| esp32-c3-autonomous-engineer | ESP32-C3/WiFi开发 |
| embedded-test-engineer | 测试用例编写 |
| code-simplifier | 代码优化 |

### 2.4 Worker执行流程

```
┌─────────────────────────────────────────────────────────────────┐
│                        Worker 执行流程                           │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐         │
│  │1.EnterWorktree│→ │2.实施代码   │→ │3.提交PR     │         │
│  │  创建分支    │    │ Hook监控    │    │ 推送到远程  │         │
│  └─────────────┘    └─────────────┘    └──────┬──────┘         │
│                                               │                │
│                                               ▼                │
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐         │
│  │5.合并到main │←── │4.Reviewer   │←── │ 通知Reviewer│         │
│  │  结束任务    │    │ 代码审查    │    │ code-reviewer│         │
│  └─────────────┘    └─────────────┘    └─────────────┘         │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

1. **EnterWorktree** - 创建隔离工作区
2. **实施代码** - 在Hook监控下修改文件
3. **提交PR** - 推送到远程并创建Pull Request
4. **通知Reviewer** - code-reviewer审查代码
5. **合并** - 通过后合并到main

---

## 阶段3: 团队清理 (TeamDelete)

### 何时清理

所有任务完成后，Leader应执行清理。

### 工具调用

```bash
TeamDelete(team_name="kimi-fly-iteration-N")
```

### 清理内容

1. **删除团队配置**
   - 删除 `~/.claude/teams/{team-name}/config.json`
   - 清理团队成员状态

2. **清理Worktrees** (可选)
   - 检查 `.claude/worktrees/` 中的残留分支
   - 手动清理已合并的worktree

3. **归档任务**
   - 将 `task-*.md` 移动到 `exec-plans/completed/`
   - 更新 `progress.md` 记录本轮完成

### 最佳实践

- 每轮结束必须执行 TeamDelete
- 团队名称建议保留在文档中备查
- 清理前确认所有任务已合并

---

## 完整流程示例

```bash
# === 阶段1: 团队创建 ===
TeamCreate(team_name="kimi-fly-iteration-4", description="HAL层实现")

# 系统自动创建:
# - team-orchestrator (Leader)
# - plan-reviewer (Reviewer)
# - harness-architect (Architect)

# === 阶段2: 任务执行 ===
# Leader创建任务
TaskCreate(subject="UART HAL", description="...")

# 分配给Worker
TaskUpdate(taskId="1", owner="stm32-embedded-engineer")

# 或动态启动Worker
Agent(team_name="kimi-fly-iteration-4", subagent_type="stm32-embedded-engineer")

# Worker执行: EnterWorktree → Edit → PR → Review → Merge

# === 阶段3: 团队清理 ===
TeamDelete(team_name="kimi-fly-iteration-4")
```

---

## 注意事项

### 1. 固定核心团队 vs 动态Worker

| | 固定核心团队 | 动态Worker |
|---|---|---|
| 创建方式 | TeamCreate自动创建 | TaskUpdate分配或Agent启动 |
| 数量 | 每轮3个固定 | 按需动态加入 |
| 职责 | 计划、审核、监控 | 实际代码实施 |
| subagent_type | team-orchestrator/plan-reviewer/harness-architect | code-reviewer/stm32-embedded-engineer/etc |

### 2. 团队命名规范

```
{project}-iteration-{N}
```

例如:
- `kimi-fly-iteration-1`
- `kimi-fly-iteration-2`
- `kimi-fly-iteration-3`

### 3. 常见错误

- ❌ 忘记执行 TeamDelete 清理团队
- ❌ 混淆 plan-reviewer 和 code-reviewer 的职责
- ❌ 动态Worker直接修改计划文档
- ❌ 一轮迭代创建多个团队

---

## 故障排除

### 问题: TeamCreate 后看不到团队成员

**解决**: 使用 `TeamList()` 查看团队状态

### 问题: Worker 无法加入团队

**解决**: 确认 `team_name` 参数正确无误

### 问题: TeamDelete 后任务状态丢失

**解决**: 删除前确保已更新 `progress.md`

---

## 相关文档

| 文档 | 路径 | 说明 |
|------|------|------|
| Harness流程规范 | `RALPH-HARNESS.md` | Ralph-loop完整流程规范 |
| Skill定义 | `.claude/skills/ralph-loop-orchestrator/SKILL.md` | Skill详细定义 |
| 当前执行计划 | `docs/exec-plans/active/plan.md` | 本轮计划 |
| 技术债务 | `docs/exec-plans/tech-debt-tracker.md` | 待修复问题 |

---

*Ralph-loop 团队生命周期 v1.0*
