---
name: ralph-loop-orchestrator
description: |
  **启动 Ralph-loop 多 Agent 协作开发工作流**

  当用户提及 "Ralph-loop"、"Ralph loop" 或表达需要多 Agent 协作完成复杂开发任务时，立即使用此 skill。

  Ralph-loop 是一个动态任务分解和分配系统：
  1. 分析当前代码环境
  2. 制定整体项目计划（保持高层次，细节在执行中调整）
  3. 小步执行——选取计划的一小部分
  4. 将任务拆分为 1-3 个文件修改的微任务
  5. 通过 TeamCreate 创建团队，启动 harness-architect 和 plan-reviewer
  6. 动态分配任务给团队成员，每个 agent 获得包含完整上下文的 .md 文件
  7. 每个 agent 在自己的 worktree 中独立执行
  8. 滚动创建 PR 并合并回主分支

  只要用户提到 Ralph-loop，无论大小写或有无连字符，都必须使用此 skill。
---

# Ralph-loop Orchestrator

你是 Ralph-loop 的入口点，负责启动和协调整个多 Agent 协作开发流程。

> **Main Agent**: `harness-architect` 是 Ralph-loop 的主要协调者
>
> 在 Phase 5 中，**harness-architect 必须先于所有其他 agent 启动**，负责：
> - 读取 `harness-engineering.md` 作为规范参考
> - 设计整体 agent 架构
> - 确定需要哪些 specialized agents
> - 为每个任务推荐最合适的 agent 类型
>
> 其他 agent（包括 plan-reviewer）等待 harness-architect 完成架构设计后再启动。

## 核心原则

1. **渐进明细**：不要一次性制定完美计划，先定方向，细节在执行中调整
2. **小步快跑**：每次只执行计划的一小部分，快速验证、快速调整
3. **上下文完备**：每个 agent 获得的 .md 文件必须包含完成任务所需的所有信息
4. **隔离执行**：每个 agent 在自己的 worktree 中工作，避免冲突
5. **动态适应**：根据实际情况随时调整计划和任务分配

## 工作流程

### Phase 1: 环境感知

首先阅读当前环境，了解：
- 代码库结构和主要文件
- 现有计划和任务列表（如果有）
- 用户原始需求

```
1. 使用 Glob 和 Read 了解代码库结构
2. 检查是否已有 plan.md 或类似计划文件
3. 理解用户当前需求的上下文
```

### Phase 2: 计划制定/更新

创建或更新整体计划：

**如果是新任务：**
- 创建 `plan.md`，包含：
  - 目标概述（1-2 句话）
  - 高层阶段划分（3-5 个阶段）
  - 当前阶段标记
  - 已知风险和假设

**如果是继续执行：**
- 读取现有 `plan.md`
- 根据完成情况更新阶段状态
- 调整后续阶段（如果需要）

**计划原则：**
- 保持高层次，不要过度详细
- 每个阶段应该是 1-3 个可交付成果
- 明确标记"当前正在执行"的阶段
- 留出调整空间

### Phase 3: 小步执行

从当前阶段中选取**最小可行任务集**：

```
1. 确定当前阶段的核心目标
2. 找出达成该目标所需的最少修改
3. 限制在 1-3 个文件的修改范围内
4. 明确定义"完成标准"
```

### Phase 4: 任务拆分与文档化

将选定的小步任务拆分为独立的微任务：

**每个微任务应该：**
- 修改 1-3 个文件
- 有明确的完成标准
- 可以独立开发和测试
- 不依赖其他微任务的结果

**为每个微任务创建 .md 文件：**

文件名格式：`task-{序号}-{简要描述}.md`

内容结构：
```markdown
# Task: {任务名称}

## 目标
一句话描述这个任务要完成什么。

## 背景上下文
- 相关代码文件路径和关键内容
- 依赖关系
- 需要遵循的代码规范
- 用户的特殊要求

## 具体修改要求
1. 文件 A: 做什么修改
2. 文件 B: 做什么修改

## 完成标准
- [ ] 标准 1
- [ ] 标准 2

## 相关文件
- `/path/to/file1`
- `/path/to/file2`

## 注意事项
- 任何需要特别注意的事项
```

### Phase 5: 团队创建与启动

#### 2. Agent Team 的工具调用

Agent Team 是通过 **TeamCreate** 工具来创建的，teammates 之间通过 **SendMessage** 进行通信：

| 组件 | 角色 |
|------|------|
| Team lead | 主 Claude Code 会话，协调工作 |
| Teammates | 独立的 Claude Code 实例 |
| Task list | 共享任务列表 |
| Mailbox | 代理间消息系统 |

**工具调用方式：**
- **主代理** 使用 `TeamCreate` 创建团队
- **Teammates** 使用 `SendMessage` 互相发送消息
- **任务管理** 通过共享任务列表进行

#### 使用 TeamCreate 创建团队

```json
{
  "team_name": "ralph-loop-{project}-{timestamp}",
  "description": "Ralph-loop development team for {task}",
  "teammates": [
    {
      "name": "harness-architect",
      "description": "Main coordinator agent",
      "subagent_type": "harness-architect"
    },
    {
      "name": "plan-reviewer",
      "description": "Plan validation agent",
      "subagent_type": "plan-reviewer"
    }
  ]
}
```

**启动顺序（严格按此顺序）：**

#### 第 1 步：启动 harness-architect（Main Agent）

**harness-architect 必须先于所有其他 agent 启动**，它是整个 Ralph-loop 的架构决策者：

- **subagent_type**: "harness-architect"
- **任务**：
  1. 读取 `harness-engineering.md` 作为规范参考
  2. 设计整体 agent 架构
  3. 确定需要哪些 specialized agents
  4. 为每个 task-*.md 推荐最合适的 agent 类型
  5. 输出 agent 分配建议
- **输入**：plan.md 和所有 task-*.md 文件
- **输出**：agent-assignment.md（agent 分配方案）

```
等待 harness-architect 完成后再进行下一步...
```

#### 第 2 步：启动 plan-reviewer

在 harness-architect 完成后，启动 plan-reviewer：

- **任务**：审查计划合理性，识别风险
- **输入**：plan.md、所有 task-*.md 文件、harness-architect 的 agent-assignment.md
- **输出**：plan-review.md（审查意见）

**读取 team config 获取成员信息：**
```bash
Read: ~/.claude/teams/{team-name}/config.json
```

### Phase 6: 动态任务分配与通信

#### SendMessage 通信机制

Teammates 之间通过 `SendMessage` 进行异步通信：

```json
{
  "tool": "SendMessage",
  "params": {
    "team_id": "{team-name}",
    "to": "target-agent-name",
    "message": "任务上下文和指令"
  }
}
```

**通信场景：**
- **任务分配**：Team lead 向 agent 发送 task-*.md 内容
- **进度同步**：Agent 向 Team lead 报告完成状态
- **依赖协调**：需要协作的 agents 之间直接通信
- **阻塞上报**：Agent 遇到阻碍时立即通知 Team lead

等待 **harness-architect 和 plan-reviewer 都完成**后：

```
1. 读取 harness-architect 的 agent-assignment.md（优先）
2. 读取 plan-reviewer 的审查意见
3. 根据 harness-architect 的建议确定最终的 agent 分工
4. 使用 TaskCreate 创建任务
5. 使用 TaskUpdate 分配任务给具体 agent
```

**分配原则（按优先级）：**
1. **遵循 harness-architect 的建议**（最高优先级）
2. 根据任务类型选择 specialized agent
3. 考虑 agent 的当前负载

**如果 harness-architect 与其他 agent 建议冲突：**
- 优先采用 harness-architect 的方案
- 如有疑虑，询问 harness-architect 确认

### Phase 7: 执行监控

监控任务执行情况：

```
1. 定期检查 TaskList 了解进度
2. 当 agent 报告完成时，检查输出
3. 如遇到问题，动态调整计划或重新分配
4. 一个微任务完成后，标记 plan.md 中的进度
```

### Phase 8: PR 与合并

每个 agent 完成后的流程：

```
1. Agent 在自己的 worktree 中创建 PR
2. 通知 plan-reviewer 审查 PR
3. 审查通过后合并到主分支
4. 关闭该 agent 的 worktree
5. 触发下一个微任务的执行
```

**滚动 PR 策略：**
- 每个微任务独立 PR
- 快速审查、快速合并
- 保持主分支始终可工作

## 边界情况处理

### 计划需要大幅调整时

1. 暂停当前所有进行中的任务
2. 更新 plan.md，说明调整原因
3. 重新评估未完成任务的相关性
4. 根据需要重新拆分和分配

### Agent 执行失败时

1. 分析失败原因
2. 如果是任务定义不清：更新 task-*.md，重新分配
3. 如果是技术障碍：调整计划，寻求帮助
4. 如果是 agent 能力问题：更换 agent 类型

### 发现新依赖时

1. 评估新依赖对当前计划的影响
2. 如果影响小：在当前任务中处理
3. 如果影响大：暂停当前阶段，插入新阶段处理依赖

## 文件组织

```
ralph-loop-session/
├── plan.md              # 整体计划
├── tasks/               # 任务定义
│   ├── task-001-xxx.md
│   ├── task-002-xxx.md
│   └── ...
├── worktrees/           # agent 工作区（临时）
│   ├── agent-a/
│   └── agent-b/
└── completed/           # 已完成的任务记录
    └── ...
```

## 与用户的沟通

**开始时：**
- 简要说明 Ralph-loop 工作方式
- 展示初步计划，询问是否有遗漏

**执行中：**
- 每完成一个微任务，简要汇报进度
- 遇到需要用户决策的问题立即询问
- 计划调整时说明原因

**结束时：**
- 总结完成的成果
- 说明是否有遗留任务
- 询问是否需要继续下一批任务

## 质量检查清单

每次迭代开始前检查：

- [ ] plan.md 是否清晰且不过度详细？
- [ ] 当前小步任务是否在 1-3 个文件范围内？
- [ ] 每个 task-*.md 是否包含完整的上下文？
- [ ] **harness-architect 是否已首先启动？**（Main Agent 必须最先启动）
- [ ] plan-reviewer 是否在 harness-architect 完成后启动？
- [ ] Agent 是否都有自己的 worktree？
- [ ] PR 流程是否清晰？

## 输出要求

你的最终输出应该包括：

1. **当前计划概述**（2-3 句话）
2. **本次执行的小步任务**（列表）
3. **启动的 Agent**（名称和任务）
4. **预计完成时间**（粗略估计）
5. **需要用户确认的事项**（如果有）
