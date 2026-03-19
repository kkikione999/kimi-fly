# Hook 配置 - Ralph-loop v2.0

> **版本**: 2.0
> **创建日期**: 2026-03-19
> **负责人**: Harness-Architect
> **作用**: 定义3个关键Hook点的验证规则

---

## 概述

Hook机制在Ralph-loop v2.0中负责在关键节点拦截操作，验证是否符合Harness规范。

| Hook点 | 触发时机 | 作用 |
|--------|----------|------|
| **EnterWorktree** | Worker创建工作区前 | 验证分支命名、代码基础、任务绑定 |
| **Edit/Write** | Worker修改文件前 | 验证文件范围、依赖声明、越界检测 |
| **Pre-merge** | PR合并前最终检查 | 验证审批流程、技术债务、测试状态 |

---

## 1. EnterWorktree Hook

### 1.1 触发时机
Worker调用 `EnterWorktree` 工具创建工作区时触发。

### 1.2 验证规则

| 检查项 | 规则 | 失败动作 |
|--------|------|----------|
| **分支命名规范** | 必须符合 `task-{NNN}-{description}` 格式 | 拒绝创建，提示正确格式 |
| **代码基础版本** | 必须基于 `main` 分支最新 commit | 拒绝创建，要求先更新 |
| **任务绑定验证** | Worker必须已读取对应的 `task-{NNN}.md` 文件 | 拒绝创建，要求先读取任务文档 |
| **工作区隔离** | 工作区必须在 `.claude/worktrees/` 目录下 | 拒绝创建，使用标准路径 |

### 1.3 分支命名规范

```
格式: task-{NNN}-{description}

示例:
- task-001-gpio-hal
- task-015-esp8266-wifi-config
- task-023-flight-debug

规则:
- NNN: 3位数字，与task文档编号一致
- description: 小写，用连字符分隔
- 不允许: 特殊字符、空格、大写字母
```

### 1.4 验证流程

```
1. 解析分支名 -> 提取 task 编号
2. 检查 task-{NNN}.md 是否存在
3. 检查 Worker 是否已读取该任务文档
4. 检查 base commit 是否为 main 最新
5. 全部通过 -> 允许创建
6. 任一失败 -> 拒绝并返回错误信息
```

---

## 2. Edit/Write Hook

### 2.1 触发时机
Worker调用 `Edit` 或 `Write` 工具修改文件时触发。

### 2.2 验证规则

| 检查项 | 规则 | 严重级别 | 失败动作 |
|--------|------|----------|----------|
| **文件范围检查** | 只能修改 `task-*.md` 中 `相关文件` 列表内的文件 | Critical | 阻止修改，提示越界 |
| **依赖检查** | 不能引入未在 `依赖关系` 中声明的新依赖 | High | 阻止修改，要求声明依赖 |
| **禁止修改Harness文档** | 不能修改 `RALPH-HARNESS.md`, `CLAUDE.md` 等流程文档 | Critical | 阻止修改，通知Harness-Architect |
| **禁止修改其他任务** | 不能修改其他 `task-*.md` 文件 | High | 阻止修改，要求通过Leader协调 |
| **禁止修改硬件真源** | 不能修改 `hardware-docs/pinout.md` | Critical | 阻止修改，硬件变更需人工审核 |

### 2.3 文件范围检查逻辑

```python
# 伪代码
allowed_files = read_task_related_files(task_id)
prohibited_patterns = [
    "RALPH-HARNESS.md",
    "CLAUDE.md",
    "docs/hooks-config*.md",
    "hardware-docs/pinout.md",
    "docs/exec-plans/active/task-*.md"  # 其他任务文档
]

def validate_edit(file_path, task_id):
    # 检查是否在允许列表
    if file_path not in allowed_files:
        return BLOCK, "文件不在任务相关文件列表中"

    # 检查是否命中禁止模式
    for pattern in prohibited_patterns:
        if matches(file_path, pattern):
            return BLOCK, f"禁止修改: {pattern}"

    return ALLOW
```

### 2.4 依赖检查逻辑

```python
# 伪代码
declared_deps = read_task_dependencies(task_id)

def validate_dependencies(file_path, new_content, task_id):
    # 解析新内容中的 #include
    new_includes = extract_includes(new_content)

    # 检查每个include是否在声明的依赖中
    for include in new_includes:
        if not is_in_dependencies(include, declared_deps):
            return BLOCK, f"未声明的依赖: {include}"

    return ALLOW
```

---

## 3. Pre-merge Hook

### 3.1 触发时机
PR准备合并到 `main` 分支前触发。

### 3.2 验证规则

| 检查项 | 规则 | 严重级别 | 失败动作 |
|--------|------|----------|----------|
| **Reviewer审批** | 必须通过 `code-reviewer` 或 `plan-reviewer` 审核 | Critical | 阻止合并，要求审核 |
| **测试检查** | 必须通过自测（单元测试/集成测试） | High | 阻止合并，要求修复测试 |
| **技术债务检查** | 如有新债务，必须记录到 `tech-debt-tracker.md` | Medium | 阻止合并，要求记录 |
| **代码规范** | 必须符合项目代码规范 | Medium | 要求修正 |
| **文档更新** | 如修改接口，必须更新相关文档 | Medium | 要求更新文档 |

### 3.3 合并检查清单

```markdown
## Pre-merge Checklist

- [ ] Reviewer审批通过
  - 审核人: [code-reviewer/plan-reviewer]
  - 审批状态: APPROVED / NEEDS_REVISION

- [ ] 测试通过
  - 单元测试: [通过/失败]
  - 集成测试: [通过/失败]
  - 硬件测试: [通过/跳过/失败]

- [ ] 技术债务记录
  - 新债务: [有/无]
  - 已记录到: `docs/exec-plans/tech-debt-tracker.md`

- [ ] 代码规范
  - 符合STM32 HAL风格
  - 无编译警告
  - 寄存器访问使用volatile

- [ ] 文档更新
  - 接口变更已更新文档
  - 新增功能已添加说明
```

### 3.4 合并流程

```
1. Worker提交PR
2. Pre-merge Hook触发
3. 检查Reviewer审批状态
4. 检查测试状态
5. 检查技术债务记录
6. 全部通过 -> 允许合并
7. 任一失败 -> 阻止合并，返回检查报告
8. 合并后 -> 归档任务文档到 completed/
```

---

## 4. 偏差处理流程

### 4.1 Hook触发偏差

当Hook检测到违规时:

1. **立即拦截**操作
2. **记录偏差**到 `docs/harness-deviation.md`
3. **通知相关Agent**:
   - Worker: 告知违规原因和正确做法
   - Leader: 如需要任务调整
   - Harness-Architect: 如需要流程改进
4. **分类处理**:
   - 简单偏差: Worker自行修正
   - 复杂偏差: Leader修订任务
   - 系统性偏差: Harness-Architect更新流程

### 4.2 偏差记录格式

```markdown
## Deviation Report - YYYY-MM-DD HH:MM

**Hook点**: [EnterWorktree/Edit/Write/Pre-merge]
**任务**: Task {NNN}
**Worker**: [agent-name]

**违规描述**:
[具体描述违规内容]

**期望行为**:
[应该怎么做]

**实际行为**:
[实际做了什么]

**根因分析**:
[为什么会出现这个偏差]

**纠正措施**:
[如何修正]

**预防措施**:
[如何防止再次发生]
```

---

## 5. Agent类型与Hook权限

| Agent类型 | EnterWorktree | Edit/Write | Pre-merge |
|-----------|---------------|------------|-----------|
| **stm32-embedded-engineer** | 需验证 | 需验证 | 需验证 |
| **esp32-c3-autonomous-engineer** | 需验证 | 需验证 | 需验证 |
| **embedded-test-engineer** | 需验证 | 需验证 | 需验证 |
| **code-simplifier** | 需验证 | 需验证 | 需验证 |
| **harness-architect** | 豁免 | 豁免(仅流程文档) | 豁免 |
| **plan-reviewer** | 豁免 | 豁免 | 豁免 |

---

## 6. 附录

### 6.1 快速参考

| 场景 | Hook点 | 处理 |
|------|--------|------|
| Worker创建分支 | EnterWorktree | 验证命名规范 |
| Worker修改代码 | Edit/Write | 验证文件范围 |
| Worker提交PR | Pre-merge | 验证审批和测试 |

### 6.2 相关文档

- Harness流程: `RALPH-HARNESS.md`
- 架构地图: `CLAUDE.md`
- 技术债务: `docs/exec-plans/tech-debt-tracker.md`
- 偏差记录: `docs/harness-deviation.md`
