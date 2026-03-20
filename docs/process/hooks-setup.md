# Ralph-loop Hooks 配置说明

> **版本**: 1.0
> **创建日期**: 2026-03-18
> **状态**: ✅ 已配置完成

---

## 配置概览

Ralph-loop v2.0 的 Hook 机制现已配置完成，使用 **Claude Code 原生 Hook 系统**。

### 配置文件位置

```
.claude/settings.json              # Claude Code 主配置
.claude/hooks/scripts/             # Hook 脚本目录
    ├── enter-worktree-check.sh    # EnterWorktree Hook
    ├── edit-check.sh              # Edit/Write Hook
    ├── pre-merge-check.sh         # Pre-merge Hook
    └── pr-create-check.sh         # PR 创建检查
```

---

## Hook 类型与功能

### 1. PreToolUse Hooks (工具使用前)

| 匹配器 | 脚本 | 功能 |
|--------|------|------|
| `EnterWorktree` | `enter-worktree-check.sh` | 检查分支命名规范 `task-{NNN}-{desc}` |
| `Edit\|Write` | `edit-check.sh` | 检查文件是否在任务范围内 |
| `Bash` | Prompt | 检查 git merge/push 操作 |

**工作原理**:
```
Agent 调用工具 ──▶ Claude 匹配 Hook ──▶ 执行脚本/提示 ──▶ 通过/阻止
```

### 2. PostToolUse Hooks (工具使用后)

| 匹配器 | 类型 | 功能 |
|--------|------|------|
| `Edit\|Write` | Prompt | 提醒创建 PR |

### 3. SessionStart Hooks (会话开始)

| 匹配器 | 类型 | 功能 |
|--------|------|------|
| `*` | Prompt | 检查项目状态 |

### 4. Stop Hooks (会话结束)

| 匹配器 | 类型 | 功能 |
|--------|------|------|
| `*` | Command | 记录会话结束 |

---

## Hook 详细说明

### EnterWorktree Hook

**触发时机**: Agent 调用 `EnterWorktree` 工具创建工作区时

**检查内容**:
- 分支名是否符合 `task-{NNN}-{description}` 格式
- 对应的任务文档是否存在 (`docs/plans/active/task-{NNN}-*.md`)
- 是否基于最新 main 分支

**失败处理**:
```
[HOOK ERROR] 分支名不符合规范!
  期望格式: task-{NNN}-{description}
  实际: <branch-name>
```

---

### Edit/Write Hook

**触发时机**: Agent 调用 `Edit` 或 `Write` 工具修改文件时

**检查内容**:
- 修改的文件是否在对应任务的 "相关文件" 列表中
- 是否引入未声明的新依赖
- 是否修改与任务无关的文件

**注意**:
- 非任务分支（如 main）跳过此检查
- 新文件创建给予警告但允许

---

### Pre-merge Hook

**触发时机**: Agent 调用 `Bash` 工具执行 git merge/push 时

**检查内容**:
- 是否已获得 Reviewer 的 `APPROVED` 状态
- 技术债务是否已记录

**关键文件**:
```
.harness/pr-review-status.md
```

**失败处理**:
```
[HOOK ERROR] 此分支未通过 Reviewer 审批!
  请先提交 PR 并通过 Reviewer 审查
```

---

## PR 审查流程

### 1. Worker 创建 PR

```bash
# 1. 完成任务后推送到远程
git push origin task-003-uart-hal

# 2. 创建 PR
gh pr create --title "[Task 003] UART HAL Implementation" \
             --body "实现 USART2 HAL 用于 ESP32-C3 通信"

# 3. 更新 PR 状态文件
echo "| #4 | task-003-uart-hal | Task 003 | 🟡 PENDING | ..." >> pr-review-status.md
```

### 2. Reviewer 审查

```bash
# 审查代码后更新状态
echo "| #4 | task-003-uart-hal | Task 003 | 🟢 APPROVED | code-reviewer | 代码 OK" >> pr-review-status.md
```

### 3. 合并到 main

```bash
# Hook 会自动检查 pr-review-status.md 中的 APPROVED 状态
git merge task-003-uart-hal
```

---

## 验证 Hook 工作

### 测试 EnterWorktree Hook

```bash
# 尝试创建不规范的分支名
EnterWorktree name="bad-branch-name"
# 预期: Hook 阻止并提示格式错误
```

### 测试 Edit Hook

```bash
# 在 task-003 分支中修改非任务文件
Edit file_path="docs/unrelated-file.md"
# 预期: Hook 警告此文件不在任务范围内
```

### 测试 Pre-merge Hook

```bash
# 未经过审查尝试合并
Bash command="git merge task-003-uart-hal"
# 预期: Hook 阻止并要求 Reviewer 审批
```

---

## 故障排除

### Hook 未触发

**检查项**:
1. `.claude/settings.json` 是否存在且格式正确
2. Hook 脚本是否有执行权限 (`chmod +x`)
3. 脚本路径是否为绝对路径

### Hook 脚本错误

**调试方法**:
```bash
# 手动运行脚本测试
bash /Users/ll/kimi-fly/.claude/hooks/scripts/enter-worktree-check.sh task-003-uart-hal
```

### PR 状态未更新

**检查项**:
1. `.harness/pr-review-status.md` 是否存在
2. Reviewer 是否正确记录了 APPROVED 状态
3. 分支名是否与状态文件中的匹配

---

## 配置修改

### 添加新 Hook

编辑 `.claude/settings.json`:

```json
{
  "PreToolUse": [
    {
      "matcher": "Agent",
      "hooks": [
        {
          "type": "command",
          "command": "bash /path/to/new-hook.sh",
          "timeout": 30
        }
      ]
    }
  ]
}
```

### 禁用特定 Hook

将对应 hooks 数组设为空:

```json
{
  "PreToolUse": [
    {
      "matcher": "Edit",
      "hooks": []  // 禁用此 Hook
    }
  ]
}
```

---

## 注意事项

1. **Hook 不是绝对安全**: 经验丰富的 Agent 可能绕过检查
2. **人工审查仍需要**: Hook 是辅助工具，不能替代 Reviewer
3. **保持脚本更新**: 任务文档格式变更时需同步更新脚本
4. **定期检查日志**: 查看 Hook 触发记录，发现流程问题

---

## 相关文档

- `RALPH-HARNESS.md` - Harness 流程规范
- `docs/process/harness-deviation.md` - 流程偏差记录
- `.harness/pr-review-status.md` - PR 审查状态

---

*Ralph-loop Hooks v1.0 - 配置完成*
