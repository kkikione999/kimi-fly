# Agent Team 使用指南

如何在 kimi-fly 项目中使用 Agent 团队进行开发。

## 核心工作流: Ralph-loop

```
计划(Plan) → 执行(Execute) → 审查(Review) → 合并(Merge)
     ↑___________________________________________|
```

## 启动开发任务

### 1. 创建执行计划

对于复杂任务，先创建执行计划：

```markdown
# Execution Plan: [任务名称]

## Objective
[清晰的目标描述]

## Scope
- [ ] 包含的功能
- [ ] 明确排除的功能

## Tasks
1. [ ] 任务1 (预估: X小时)
2. [ ] 任务2 (预估: X小时)

## Dependencies
- 依赖项1
- 依赖项2

## Success Criteria
- [ ] 可验证的完成标准

## Risks
- 风险1: 缓解措施
```

保存到 `docs/exec-plans/active/[plan-name].md`

### 2. 审查计划

使用 plan-reviewer Agent 审查计划：

```
/ralph-loop
```

或明确调用：

```
使用 plan-reviewer 审查这个计划
```

### 3. 执行计划

根据任务类型选择合适的 Agent：

| 任务类型 | Agent |
|----------|-------|
| STM32 固件开发 | stm32-embedded-engineer |
| ESP32-C3 固件 | esp32-c3-autonomous-engineer |
| 测试代码 | embedded-test-engineer |
| 架构决策 | harness-architect |

### 4. 代码审查

所有代码必须经过审查：

- Agent 自审查（使用 code-review skill）
- 相关领域 Agent 审查
- 自动化检查（lint, test, build）

### 5. 合并

通过所有检查后合并。小问题可以后续修复，不要阻塞进度。

## Agent 协作模式

### 模式1: 顺序执行

适合有明确依赖关系的任务：

```
harness-architect 设计 → stm32-embedded-engineer 实现 → embedded-test-engineer 测试
```

### 模式2: 并行开发

适合独立模块：

```
stm32-embedded-engineer (传感器驱动)
        ↓
embedded-test-engineer (HIL测试)
        ↓
esp32-c3-autonomous-engineer (无线通信)
```

### 模式3: 专家咨询

遇到问题时咨询专家 Agent：

```
stm32-embedded-engineer: 实现中遇到 DMA 冲突问题
harness-architect: 分析并提供解决方案
```

## 最佳实践

### DO

- 从小任务开始，逐步建立信任
- 明确指定使用哪个 Agent
- 提供足够的上下文（硬件型号、接口定义、约束条件）
- 及时审查和反馈
- 将知识写回文档

### DON'T

- 不要在一个提示中混合多个不相关的任务
- 不要让 Agent 猜测硬件配置
- 不要忽视 Agent 提出的风险警告
- 不要在计划中省略验收标准

## 故障排除

### Agent 不理解任务

- 检查是否阅读了 `CLAUDE.md`
- 提供更多上下文和示例
- 分解为更小的子任务

### 代码不符合规范

- 引用 `CLAUDE.md` 的具体条款
- 提供符合规范的示例代码
- 让 Agent 使用 linter 自检

### 遇到阻塞

1. 在 `docs/harness-question.md` 中记录问题
2. 咨询 harness-architect
3. 如果涉及硬件，提供原理图参考

## 示例会话

### 示例1: 新驱动开发

```
用户: 为 MPU6050 实现 I2C 驱动

harness-architect: 我来协调这个任务。
1. 首先创建执行计划
2. 分配 stm32-embedded-engineer 实现
3. embedded-test-engineer 编写测试

[创建计划...]
[执行...]
[审查...]
```

### 示例2: 调试问题

```
用户: 电机 PWM 输出不稳定

stm32-embedded-engineer: 我来分析这个问题。
1. 检查时钟配置
2. 检查 DMA 设置
3. 检查中断优先级

[诊断...]

发现: TIM2 中断被阻塞
建议: 提高 TIM2 优先级或缩短 ISR
```

## 持续改进

定期（每周或每迭代）回顾：

1. 哪些 Agent 协作模式有效？
2. 哪些文档需要更新？
3. 有哪些重复问题可以工具化？

将改进点记录到 `docs/exec-plans/tech-debt-tracker.md`

---

*Agent 团队是 force multiplier。投资于流程和文档，回报会 compound。*
