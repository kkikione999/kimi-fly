# 修复计划 - Ralph-loop v2.0 流程修正

> **创建日期**: 2026-03-18
> **优先级**: P0 - Blocker
> **状态**: 待启动
> **目标**: 修复Ralph-loop流程文档和配置，明确固定核心团队与动态Worker的分工

---

## 问题摘要

### 发现的问题

1. **Agent角色混淆**: `code-reviewer` 和 `plan-reviewer` 混用
2. **缺少团队生命周期**: 未说明 TeamCreate/TeamDelete 的使用
3. **缺少任务分配机制**: 未明确 TaskCreate/TaskUpdate/Agent 的使用场景
4. **流程不完整**: 缺少本轮结束时的清理步骤

### 影响范围

- `SKILL.md` - 启动流程说明错误
- `RALPH-HARNESS.md` - 流程规范不完整
- 实际执行时角色混乱

---

## 修复任务清单

### Task F001: 修复 SKILL.md 角色定义

**负责人**: code-simplifier (或 general-purpose)

**相关文件**:
- `.claude/skills/ralph-loop-orchestrator/SKILL.md`

**修改内容**:
```markdown
## 修改点

1. 第32行: 修改 Agent 角色定义表
   - 原: Reviewer | code-reviewer
   - 改: Reviewer | plan-reviewer

2. 第69行: 修改 Phase 3 说明
   - 原: subagent_type: "code-reviewer"
   - 改: subagent_type: "plan-reviewer"

3. 第197行: 修改质量检查清单
   - 原: code-reviewer 是否已审核计划?
   - 改: plan-reviewer 是否已审核计划?

4. 增加"团队架构"章节:
   ### 固定核心团队 (TeamCreate创建)
   - Leader: team-orchestrator
   - Reviewer: plan-reviewer
   - Architect: harness-architect

   ### 动态Worker (Task分配)
   - code-reviewer (代码审查)
   - stm32-embedded-engineer (STM32开发)
   - esp32-c3-autonomous-engineer (ESP32开发)
   - embedded-test-engineer (测试)

5. 修改 Phase 0 和 Phase 9:
   - 增加 TeamCreate 启动步骤
   - 增加 TeamDelete 清理步骤

6. 修改 Phase 5 任务分配:
   - 明确 TaskCreate → TaskUpdate(owner) 流程
   - 或 Agent(team_name, subagent_type) 方式
```

**完成标准**:
- [ ] 所有 `code-reviewer` 改为 `plan-reviewer` (3处)
- [ ] 增加团队架构区分章节
- [ ] 增加 TeamCreate/TeamDelete 步骤
- [ ] 明确任务分配机制

**预估时间**: 30分钟

---

### Task F002: 修复 RALPH-HARNESS.md 流程规范

**负责人**: code-simplifier (或 general-purpose)

**相关文件**:
- `RALPH-HARNESS.md`

**修改内容**:
```markdown
## 修改点

1. 1.1 Agent角色定义:
   - 更新表格，区分"固定核心团队"和"动态Worker"
   - 增加 subagent_type 列

2. 2.1 单轮循环流程图:
   - 步骤0: TeamCreate 创建团队
   - 步骤9: TeamDelete 清理团队
   - 更新流程图ASCII图

3. 5.2 Reviewer工作流:
   - 拆分为两个小节:
     ### 5.2a plan-reviewer工作流 (计划审核)
     ### 5.2b code-reviewer工作流 (代码/PR审查)

4. 增加 5.5 本轮结束流程:
   - Leader更新进度
   - TeamDelete清理团队
   - 归档任务文档

5. 8.1 快速参考表:
   - 增加 TeamCreate/TeamDelete/TeamList
   - 增加 TaskCreate/TaskUpdate/TaskList
```

**完成标准**:
- [ ] Agent角色定义表更新
- [ ] 流程图增加TeamCreate/Delete
- [ ] Reviewer工作流拆分
- [ ] 增加本轮结束流程章节
- [ ] 快速参考表更新

**预估时间**: 45分钟

---

### Task F003: 创建团队生命周期文档

**负责人**: harness-architect

**相关文件**:
- `docs/team-lifecycle.md` (新建)

**内容**:
```markdown
# Ralph-loop 团队生命周期

## 阶段1: 团队创建 (TeamCreate)

### 工具调用
TeamCreate(team_name="kimi-fly-iteration-N", description="...")

### 自动创建的Agent
1. team-orchestrator (Leader)
2. plan-reviewer (计划审核)
3. harness-architect (流程监控)

### 配置文件
~/.claude/teams/kimi-fly-iteration-N/config.json

## 阶段2: 任务执行

### 任务创建
TaskCreate(subject="...", description="...")

### 任务分配
TaskUpdate(taskId="N", owner="stm32-embedded-engineer")

### 或动态启动Agent
Agent(team_name="kimi-fly-iteration-N", subagent_type="stm32-embedded-engineer")

## 阶段3: 团队清理 (TeamDelete)

### 工具调用
TeamDelete(team_name="kimi-fly-iteration-N")

### 清理内容
- 删除团队配置文件
- 清理worktrees (可选)
- 结束所有团队成员

## 注意事项

1. 每轮循环必须TeamCreate创建新团队
2. 循环结束必须TeamDelete清理
3. 团队名称建议包含迭代号: "kimi-fly-iteration-3"
```

**完成标准**:
- [ ] 文档创建完成
- [ ] 包含完整的生命周期说明
- [ ] 包含工具调用示例

**预估时间**: 30分钟

---

### Task F004: 验证和测试修复

**负责人**: plan-reviewer

**相关文件**:
- `.claude/skills/ralph-loop-orchestrator/SKILL.md`
- `RALPH-HARNESS.md`
- `docs/team-lifecycle.md`

**验证内容**:
```markdown
## 检查清单

### SKILL.md
- [ ] 所有 code-reviewer 已改为 plan-reviewer
- [ ] 团队架构章节存在且清晰
- [ ] TeamCreate/TeamDelete 步骤存在
- [ ] 任务分配机制明确

### RALPH-HARNESS.md
- [ ] Agent角色定义表已更新
- [ ] 流程图包含TeamCreate/Delete
- [ ] Reviewer工作流已拆分
- [ ] 本轮结束流程章节存在

### team-lifecycle.md
- [ ] 文档结构清晰
- [ ] 包含所有工具调用示例
- [ ] 注意事项完整

### 一致性检查
- [ ] SKILL.md 和 RALPH-HARNESS.md 描述一致
- [ ] Agent名称一致 (plan-reviewer/code-reviewer)
- [ ] 流程步骤一致
```

**完成标准**:
- [ ] 所有检查项通过
- [ ] 无矛盾描述
- [ ] 输出审核报告

**预估时间**: 20分钟

---

## 修复执行计划

### 执行顺序

```
Task F001 (SKILL.md修复)
    │
    ▼
Task F002 (RALPH-HARNESS.md修复)
    │
    ▼
Task F003 (team-lifecycle.md创建)
    │
    ▼
Task F004 (验证和测试)
    │
    ▼
修复完成
```

### 依赖关系

- F004 依赖 F001, F002, F003 全部完成
- F001 和 F002 可并行执行
- F003 可独立执行

### 总预估时间

- F001: 30分钟
- F002: 45分钟
- F003: 30分钟
- F004: 20分钟
- **总计**: 约2小时（串行）/ 1小时（并行）

---

## 修复后验证

### 模拟执行测试

修复完成后，进行以下思维验证:

1. **启动流程**:
   ```
   /ralph-loop
   └──▶ TeamCreate(team_name="kimi-fly-iteration-4")
       └──▶ 自动创建: team-orchestrator, plan-reviewer, harness-architect
   ```

2. **任务分配**:
   ```
   Leader创建Task
   └──▶ TaskUpdate(owner="stm32-embedded-engineer")
       └──▶ Worker动态加入团队
   ```

3. **结束流程**:
   ```
   所有任务完成
   └──▶ TeamDelete(team_name="kimi-fly-iteration-4")
       └──▶ 清理团队配置
   ```

### 预期结果

- ✅ 角色不再混淆
- ✅ 流程完整闭环
- ✅ 文档自洽
- ✅ 可实际执行

---

## 风险与注意事项

### 风险1: 修改引入新问题

**缓解措施**:
- Task F004 验证阶段仔细检查
- 对比修改前后的逻辑一致性

### 风险2: 与其他文档冲突

**缓解措施**:
- 检查 `CLAUDE.md` 引用
- 检查 `docs/` 其他文档
- 保持术语一致

### 风险3: 实际执行仍有问题

**缓解措施**:
- 下一轮迭代严格按新流程执行
- 记录任何问题到 `harness-deviation.md`

---

## 修复验收标准

1. **文档完整性**: 所有修改点已完成
2. **逻辑一致性**: SKILL.md 和 RALPH-HARNESS.md 描述一致
3. **可执行性**: 修复后的流程可以实际运行
4. **Reviewer批准**: plan-reviewer 审核通过

---

## 下一步行动

1. **启动修复**: 分配 Task F001, F002, F003 给相应 Agent
2. **并行执行**: F001 和 F002 并行，F003 独立
3. **验证审查**: F004 验证所有修复
4. **合并发布**: 修复完成，更新主分支

---

*修复计划创建完成 - 等待启动*
