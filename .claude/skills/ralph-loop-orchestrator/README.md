# Ralph-loop Orchestrator

一个用于启动多 Agent 协作开发工作流的 Claude skill。

## 功能

当用户提及 "Ralph-loop" 时，此 skill 会：

1. **分析环境** - 读取当前代码库结构和上下文
2. **制定计划** - 创建高层次的项目计划（plan.md）
3. **小步执行** - 每次只执行计划的一小部分
4. **任务拆分** - 将工作拆分为 1-3 个文件的微任务
5. **团队创建** - 使用 TeamCreate 创建开发团队
6. **启动核心 Agent** - 首先启动 harness-architect 和 plan-reviewer
7. **动态分配** - 根据任务类型分配给合适的 agent
8. **滚动 PR** - 每个 agent 独立创建 PR 并合并

## 工作流程

```
用户说 "Ralph-loop"
       ↓
读取环境 + 制定/更新 plan.md
       ↓
选取当前阶段的小步任务
       ↓
拆分为 task-*.md 文件
       ↓
TeamCreate 创建团队
       ↓
启动 harness-architect + plan-reviewer
       ↓
动态分配任务给 agents
       ↓
Agents 在各自 worktree 中执行
       ↓
滚动 PR → 审查 → 合并
       ↓
重复直到完成
```

## 文件结构

```
ralph-loop-session/
├── plan.md              # 整体计划（高层次）
├── tasks/               # 微任务定义
│   ├── task-001-xxx.md
│   └── task-002-xxx.md
└── completed/           # 已完成任务记录
```

## 使用

直接在对话中说：
- "Ralph-loop: 帮我实现用户认证功能"
- "用 Ralph-loop 来重构这个模块"
- "继续 Ralph-loop"

## Agent 角色

### 必需启动的 Agent

1. **harness-architect**
   - 设计 agent 架构
   - 确定需要哪些 specialized agents

2. **plan-reviewer**
   - 审查计划合理性
   - 识别风险和依赖

### 动态分配的 Agent

根据具体任务类型，可能包括：
- 前端开发 agent
- 后端开发 agent
- 测试 agent
- 代码审查 agent
- 等等

## 原则

1. **渐进明细** - 先定方向，细节在执行中调整
2. **小步快跑** - 每次 1-3 个文件，快速验证
3. **上下文完备** - 每个 agent 获得完整上下文
4. **隔离执行** - 每个 agent 有自己的 worktree
5. **动态适应** - 根据实际情况调整计划

## 模板文件

- `references/plan-template.md` - 计划模板
- `references/task-template.md` - 任务模板

## 测试

测试用例位于 `evals/evals.json`

## 作者

Ralph-loop 团队
