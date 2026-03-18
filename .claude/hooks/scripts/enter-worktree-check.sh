#!/bin/bash
# Ralph-loop EnterWorktree Hook
# 检查分支命名规范和任务绑定

set -e

BRANCH_NAME="$1"
TASK_DOC="$2"

echo "[Ralph-loop Hook] EnterWorktree Check"
echo "  Branch: $BRANCH_NAME"
echo "  Task Doc: $TASK_DOC"

# 检查分支名格式: task-{NNN}-{description}
if [[ ! "$BRANCH_NAME" =~ ^task-[0-9]{3}-[a-z0-9-]+$ ]]; then
    echo "[HOOK ERROR] 分支名不符合规范!" >&2
    echo "  期望格式: task-{NNN}-{description}" >&2
    echo "  实际: $BRANCH_NAME" >&2
    exit 1
fi

# 提取任务编号
TASK_NUM=$(echo "$BRANCH_NAME" | grep -oE '[0-9]{3}')

# 检查任务文档是否存在
if [ ! -f "docs/exec-plans/active/task-${TASK_NUM}-*.md" ]; then
    echo "[HOOK ERROR] 未找到对应的任务文档!" >&2
    echo "  期望: docs/exec-plans/active/task-${TASK_NUM}-*.md" >&2
    exit 1
fi

# 检查是否基于 main 分支
CURRENT_BASE=$(git merge-base HEAD main 2>/dev/null || echo "unknown")
MAIN_HEAD=$(git rev-parse main 2>/dev/null || echo "unknown")

if [ "$CURRENT_BASE" != "$MAIN_HEAD" ]; then
    echo "[HOOK WARNING] 当前分支可能不是基于最新 main 分支创建" >&2
fi

echo "[Ralph-loop Hook] ✓ EnterWorktree 检查通过"
exit 0
