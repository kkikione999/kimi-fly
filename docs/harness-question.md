# Claude Code Agent Team 配置指南

## 当前配置概览

你已经在 `.claude/agents/` 目录下配置了 4 个专业 Agent：

| Agent | 用途 | 颜色 | 模型 |
|-------|------|------|------|
| `harness-architect` | 多 Agent 架构设计与流程编排 | orange | sonnet |
| `stm32-embedded-engineer` | STM32 嵌入式开发 | green | sonnet |
| `esp32-c3-autonomous-engineer` | ESP32-C3 固件开发 | green | sonnet |
| `embedded-test-engineer` | 嵌入式测试 | purple | sonnet |

## Agent 配置文件结构

每个 Agent 配置文件（`.md`）包含以下部分：

```markdown
---
name: agent-name
description: "使用场景描述，包含 <example> 示例"
model: sonnet|opus|haiku
color: purple|green|blue|orange|red
memory: project|session|none
---

# Agent 角色定义
...

# Persistent Agent Memory
...
```

### 字段说明

- **name**: Agent 的唯一标识符
- **description**: 触发条件描述，可包含 `<example>` 标签说明使用场景
- **model**: 使用的 Claude 模型（sonnet/opus/haiku）
- **color**: 终端显示颜色
- **memory**: 记忆持久化级别
  - `project`: 跨会话持久化
  - `session`: 仅当前会话
  - `none`: 无记忆

## 如何创建新 Agent

1. 在 `.claude/agents/` 目录创建新的 `.md` 文件
2. 按照上述结构编写配置
3. 无需重启 Claude Code，配置立即生效

## 如何使用 Agent Team

在主会话中，Claude Code 会自动根据任务类型选择合适的 Agent。你也可以明确要求：

```
使用 stm32-embedded-engineer 来配置这个项目
```

或通过 Skill 调用：

```
/ralph-loop
```

## 已启用的插件

在 `.claude/settings.json` 中配置：

- `ralph-loop@claude-plugins-official` - 开发流程循环
- `code-review@claude-plugins-official` - 代码审查
- `superpowers@claude-plugins-official` - 增强功能

## 建议添加的 Agent

基于你的项目（kimi-fly，包含"机身"和"主控"目录），建议考虑添加：

1. **mechanical-designer** - 机械结构设计
2. **flight-control-engineer** - 飞控算法工程师
3. **pcb-designer** - PCB 布局设计
4. **system-integrator** - 系统集成

需要我帮你创建这些 Agent 配置吗？
