# Ralph-Loop Harness v2.1 - 改进版

> **版本**: 2.1
> **更新日期**: 2026-03-18
> **改进重点**: Hook → Checklist 转换，明确阶段Gate，状态可视化

---

## 1. 核心理念转变

### 1.1 从 "Hook拦截" 到 "Checklist验证"

v2.0 的 Hook 是概念性的，v2.1 改为**显式检查清单**：

| 阶段 | v2.0 (Hook) | v2.1 (Checklist) |
|------|-------------|------------------|
| Worker开始前 | "EnterWorktree Hook拦截" | Harness-Architect 检查分支命名、任务绑定 |
| Worker实施中 | "Edit/Write Hook拦截" | Worker 自检 + Reviewer 抽查 |
| 合并前 | "Pre-merge Hook拦截" | Reviewer 审核 + Harness-Architect 最终检查 |

### 1.2 状态可视化

新增状态文件机制，所有 Agent 可读取：

```
ralph-loop-session/
├── state.md              # 当前状态快照
├── phase-*.md            # 各阶段记录
└── worker-*/
    ├── status.md         # Worker 状态
    └── log.md            # 执行日志
```

---

## 2. 六阶段流程 (v2.1)

```
┌─────────────────────────────────────────────────────────────────┐
│                     Ralph-Loop v2.1 流程                         │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌─────────┐   ┌─────────┐   ┌─────────┐   ┌─────────┐         │
│  │Phase 1  │──→│Phase 2  │──→│Phase 3  │──→│Phase 4  │         │
│  │读取上下文│   │制定计划 │   │计划审核 │   │任务创建 │         │
│  └─────────┘   └─────────┘   └─────────┘   └─────────┘         │
│                                                  │              │
│  ┌─────────┐   ┌─────────┐   ┌─────────┐        │              │
│  │Phase 7  │←──│Phase 6  │←──│Phase 5  │←───────┘              │
│  │收尾总结 │   │执行监控 │   │任务分配 │                       │
│  └─────────┘   └─────────┘   └─────────┘                       │
│                                                                  │
│  Harness-Architect 全程监控，每个 Phase 有明确的 Gate            │
└─────────────────────────────────────────────────────────────────┘
```

---

## 3. 详细阶段定义

### Phase 1: 读取上下文 (Context Loading)

**执行者**: Orchestrator (你)

**读取清单**:
- [ ] CLAUDE.md - 架构地图
- [ ] RALPH-HARNESS.md - 流程规范
- [ ] docs/user-intent.md - 用户意图
- [ ] docs/exec-plans/active/plan.md - 当前计划
- [ ] docs/exec-plans/tech-debt-tracker.md - 技术债务
- [ ] ralph-loop-session/progress.md - 上一轮进度 (如有)

**输出**: `ralph-loop-session/phase-1-context.md`

**Gate**: 所有文档读取完成，技术债务已分类

---

### Phase 2: 制定计划 (Planning)

**执行者**: Leader (team-orchestrator)

**任务**:
1. 根据上下文制定本轮目标 (1-3句话)
2. 识别本轮要解决的技术债务
3. 确定本轮交付物

**输出**: `docs/exec-plans/active/plan.md` (更新)

**格式**:
```markdown
# 计划 - YYYY-MM-DD 第N轮

## 本轮目标
[1-3句话描述]

## 技术债务处理
- [ ] 债务1 [P0/P1/P2]

## 交付物
- [ ] 交付物1

## 风险评估
[可能影响本轮的问题]
```

**Gate**: 目标明确，交付物可验证

---

### Phase 3: 计划审核 (Plan Review)

**执行者**: Reviewer (code-reviewer)

**Checklist**:
- [ ] 目标是否符合 user-intent?
- [ ] 交付物是否可验证?
- [ ] 是否考虑了技术债务?
- [ ] 风险评估是否充分?

**输出**: `ralph-loop-session/phase-3-review.md`

**审核结果**:
- APPROVED → 进入 Phase 4
- NEEDS_REVISION → 返回 Phase 2

**Gate**: Reviewer 明确批准

---

### Phase 4: 任务创建 (Task Creation)

**执行者**: Leader (team-orchestrator)

**原则**:
- 每个任务 1-3 个文件
- 每个任务 30-60 分钟完成
- 任务之间尽量减少依赖

**输出**: `docs/exec-plans/active/task-{NNN}-{desc}.md`

**任务文档模板 (v2.1 改进)**:
```markdown
# Task {NNN}: {任务名称}

## 目标
一句话描述。

## 背景
### 相关代码
- 文件: `/path/to/file` - 关键内容

### 依赖关系
- 前置任务: Task {XXX} / 无
- 外部依赖: [库名]

### 硬件信息
- 涉及引脚: [引脚名]
- 参考: `hardware-docs/pinout.md` 第 X 节

## 具体修改要求

### 文件 1: `/path/to/file1`
1. 实现 [功能A]
2. 添加 [功能B]

## 完成标准 (必须可验证)
- [ ] 标准1: [如何验证]
- [ ] 标准2: [如何验证]
- [ ] 代码编译通过
- [ ] Reviewer 审核通过

## 相关文件 (范围边界)
- `/path/to/file1`
- `/path/to/file2`
- `/path/to/test_file`

## 注意事项
- [特别说明]

---
## Worker 执行区 (执行时填写)
- [ ] 已读取任务文档
- [ ] 已创建 worktree 分支
- [ ] 代码实施完成
- [ ] 自检通过
- [ ] PR 已创建
- [ ] Reviewer 审核通过
- [ ] 已合并到 main
```

**Gate**: 所有任务文档创建完成，Reviewer 审核通过

---

### Phase 5: Agent 分配与启动 (Assignment)

**执行者**: Harness-Architect + Leader

**步骤**:
1. Harness-Architect 分析每个任务，推荐 Agent 类型
2. Leader 根据推荐创建 Worker

**Agent 类型指南**:

| 任务类型 | 推荐 Agent | 理由 |
|----------|-----------|------|
| STM32 HAL/LL 驱动 | stm32-embedded-engineer | 熟悉 HAL/LL 库 |
| ESP32-C3 WiFi/协议 | esp32-c3-autonomous-engineer | 熟悉 ESP 开发 |
| 测试用例编写 | embedded-test-engineer | 专注测试设计 |
| 代码重构/优化 | code-simplifier | 代码质量专家 |

**输出**: `ralph-loop-session/agent-assignment.md`

```markdown
# Agent 分配

## Harness-Architect 分析
任务001: 推荐 stm32-embedded-engineer (GPIO/PWM)
任务002: 推荐 stm32-embedded-engineer (UART)

## 实际分配
- Worker-1 (stm32-embedded-engineer): Task 001, Task 002
- Worker-2 (esp32-c3-autonomous-engineer): Task 003
```

**Gate**: 每个任务都有明确的 Worker

---

### Phase 6: 任务执行与监控 (Execution)

**执行者**: Workers + Reviewer + Harness-Architect

#### 6.1 Worker 执行流程

```
1. 读取任务文档 (task-{NNN}.md)
2. 创建 worktree: EnterWorktree
   - 分支名: task-{NNN}-{short-desc}
3. 实施代码 (修改必须在"相关文件"列表内)
4. 自检:
   - [ ] 编译通过
   - [ ] 代码风格一致
   - [ ] 无新警告
5. 更新任务文档 "Worker 执行区"
6. 提交 PR
7. 通知 Reviewer
```

#### 6.2 Reviewer 审核流程

```
收到审核请求 → 读取 PR 描述 → 检查代码 → 输出审核报告
```

**审核 Checklist**:
- [ ] 修改在任务范围内
- [ ] 无未声明的新依赖
- [ ] 代码风格符合项目规范
- [ ] 有适当的错误处理
- [ ] 测试覆盖修改点 (如有)

**审核结果**:
- APPROVED → Worker 合并
- NEEDS_REVISION → Worker 修改

#### 6.3 Harness-Architect 监控

**触发时机**:
- Worker 创建 worktree 后
- Worker 提交 PR 后
- 合并到 main 前

**监控 Checklist**:
- [ ] 分支命名符合规范
- [ ] 修改文件在任务范围内
- [ ] 技术债务已记录 (如有)
- [ ] 流程未偏离

**偏差处理**:
1. 发现偏差 → 记录到 `docs/harness-deviation.md`
2. 分析根本原因
3. 通知相关 Agent 纠正
4. 严重偏差时暂停本轮

#### 6.4 Leader 动态调度

```
Worker 完成任务 → 通知 Leader → Leader 分配新任务
```

**Gate**: 所有任务完成，或本轮时间耗尽

---

### Phase 7: 收尾总结 (Closure)

**执行者**: Leader

**任务**:
1. 更新 `docs/exec-plans/active/plan.md` (标记完成)
2. 更新 `docs/exec-plans/tech-debt-tracker.md` (新债务)
3. 写入 `ralph-loop-session/progress.md`

**progress.md 格式**:
```markdown
# 进度记录 - YYYY-MM-DD 第N轮

## 完成内容
- [x] 任务1
- [x] 任务2

## 新增技术债务
- [ ] 债务描述 [P1] - 来源: Task XXX

## 遇到的问题
[问题描述及解决方案]

## 下一轮重点
[下一步计划]
```

**Gate**: 文档更新完成

---

## 4. 状态可视化机制

### 4.1 实时状态文件

`ralph-loop-session/state.md` (每次状态变更更新):

```markdown
# Ralph-Loop 状态快照

**更新时间**: YYYY-MM-DD HH:MM:SS
**当前阶段**: Phase 6 - 任务执行
**本轮**: 第N轮

## 任务状态

| 任务 | Worker | 状态 | 进度 |
|------|--------|------|------|
| 001 | stm32-eng | 进行中 | 80% |
| 002 | esp32-eng | 等待中 | - |

## 活跃 Agent
- Leader: active
- Reviewer: waiting
- Harness-Architect: monitoring
- Worker-1: coding
- Worker-2: idle

## 阻塞项
无 / [描述]
```

### 4.2 Worker 状态

`ralph-loop-session/worker-{name}/status.md`:

```markdown
# Worker: {name}

**当前任务**: Task {NNN}
**状态**: coding / reviewing / waiting / error
**开始时间**: HH:MM:SS
**预计完成**: HH:MM:SS

## 日志
- HH:MM - 开始任务
- HH:MM - 创建 worktree
- HH:MM - 遇到 [问题]
```

---

## 5. 技术债务管理 (v2.1 细化)

### 5.1 记录时机

必须立即记录:
- Worker 发现任务定义不清
- 代码中发现的设计缺陷
- 无法立即解决的警告
- 测试覆盖不足

### 5.2 分类处理

| 优先级 | 处理时限 | 责任方 |
|--------|----------|--------|
| P0 - Blocker | 立即 | Leader 重新规划 |
| P1 - Warning | 本轮或下轮 | Leader 安排任务 |
| P2 - Tech Debt | 后续迭代 | 归档处理 |
| P3 - Nice to have | 有时间再处理 | 可选 |

### 5.3 技术债务模板

```markdown
## 待处理

- [ ] YYYY-MM-DD - {简要描述} - 来源: {任务/轮次} - 优先级: P1
  - 详情: [具体问题]
  - 影响: [如果不修复会怎样]
  - 建议方案: [如何解决]

## 已解决

- [x] YYYY-MM-DD - {描述} - 来源: {任务} - 解决: YYYY-MM-DD - 方案: {如何解决}
```

---

## 6. 边界情况处理

### 6.1 Worker 长时间无响应 (>30分钟)

1. Harness-Architect 标记状态为 "可能卡住"
2. 检查 Worker 状态文件
3. 尝试唤醒 Worker
4. 无响应则重新分配任务

### 6.2 任务执行中发现新依赖

**简单依赖** (如缺少头文件):
- Worker 自行添加
- PR 中注明
- 通知 Leader 更新任务模板

**复杂依赖** (如需要新外设驱动):
- Worker 停止工作
- 上报 Leader
- Leader 评估是否插入新任务
- 更新计划

### 6.3 计划需要大幅调整

1. Harness-Architect 暂停当前任务
2. Leader 评估影响
3. 更新 plan.md，说明调整原因
4. 重新进入 Phase 3 (审核)
5. 重新分配任务

### 6.4 Reviewer 与 Worker 意见冲突

1. 双方陈述理由
2. Harness-Architect 评估
3. 如无法达成一致，上报给用户

---

## 7. Agent 定义 (v2.1)

### 7.1 Agent 配置文件

`.claude/agents/{agent-name}.json`:

```json
{
  "name": "stm32-embedded-engineer",
  "type": "worker",
  "skills": ["stm32-hal", "embedded-c", "gpio", "pwm", "uart", "i2c", "spi"],
  "constraints": {
    "can_write_code": true,
    "can_create_tasks": false,
    "can_assign_tasks": false,
    "can_approve": false
  },
  "checklist": [
    "读取任务文档",
    "创建 worktree",
    "在范围内修改",
    "编译通过",
    "自检完成",
    "提交 PR"
  ]
}
```

### 7.2 内置 Agent 定义

| Agent | Type | 职责 | 禁止 |
|-------|------|------|------|
| team-orchestrator | leader | 编排、计划、分配 | 不写代码 |
| code-reviewer | reviewer | 审核计划、代码、PR | 不实施代码 |
| harness-architect | architect | 流程监控、架构设计 | 不直接实施 |
| stm32-embedded-engineer | worker | STM32 HAL/LL 开发 | 不制定计划 |
| esp32-c3-autonomous-engineer | worker | ESP32 WiFi/协议 | 不制定计划 |
| embedded-test-engineer | worker | 测试用例编写 | 不制定计划 |
| code-simplifier | worker | 代码优化重构 | 不制定计划 |

---

## 8. 启动命令

### 8.1 启动 Ralph-Loop

```bash
# 启动新循环
/ralph-loop

# Orchestrator 自动执行:
# 1. Phase 1: 读取上下文
# 2. Phase 2: 启动 Leader 制定计划
# 3. Phase 3: 启动 Reviewer 审核
# 4. Phase 4: 启动 Harness-Architect + 创建任务
# 5. Phase 5+: 分配 Workers 开始执行
```

### 8.2 查看状态

```bash
# 查看当前状态
cat ralph-loop-session/state.md

# 查看当前计划
cat docs/exec-plans/active/plan.md

# 查看技术债务
cat docs/exec-plans/tech-debt-tracker.md

# 查看进度历史
cat ralph-loop-session/progress.md
```

---

## 9. 附录: 快速参考

### 9.1 文件组织

```
/
├── CLAUDE.md                    # 架构地图 (入口)
├── RALPH-HARNESS.md            # 流程规范
├── RALPH-HARNESS-v2.1.md       # 本文件 - 改进版规范
├── docs/
│   ├── user-intent.md          # 用户意图
│   ├── harness-deviation.md    # 偏差记录
│   ├── exec-plans/
│   │   ├── active/
│   │   │   ├── plan.md         # 当前主计划
│   │   │   └── task-*.md       # 任务文档
│   │   └── tech-debt-tracker.md
├── ralph-loop-session/
│   ├── state.md                # 实时状态 (NEW)
│   ├── progress.md             # 进度记录
│   ├── phase-*.md              # 阶段记录 (NEW)
│   ├── agent-assignment.md     # Agent 分配
│   └── worker-*/               # Worker 工作区 (NEW)
│       ├── status.md
│       └── log.md
└── .claude/
    └── agents/                 # Agent 定义
```

### 9.2 检查清单汇总

**Phase Gate 检查清单**:
- Phase 1 → 2: 所有文档已读，技术债务已分类
- Phase 2 → 3: 目标明确，交付物可验证
- Phase 3 → 4: Reviewer 明确批准
- Phase 4 → 5: 任务文档创建完成，范围清晰
- Phase 5 → 6: 每个任务有 Worker
- Phase 6 → 7: 所有任务完成或时间耗尽
- Phase 7 → 结束: 文档更新完成

---

## 10. v2.0 → v2.1 变更摘要

| 方面 | v2.0 | v2.1 |
|------|------|------|
| Hook 机制 | 概念性 Hook | Checklist + 状态文件 |
| 状态可见性 | 隐式 | 显式 state.md |
| Phase 定义 | 模糊 | 明确的 7 个 Phase |
| Gate 控制 | 弱 | 每个 Phase 有明确 Gate |
| Worker 反馈 | 仅完成通知 | 实时状态 + 日志 |
| 偏差处理 | 记录即可 | 分析 + 纠正 + 预防 |
| 任务文档 | 静态 | 动态 (含 Worker 执行区) |

---

*Ralph-Loop Harness v2.1 - 让 AI 自主开发更可靠*
