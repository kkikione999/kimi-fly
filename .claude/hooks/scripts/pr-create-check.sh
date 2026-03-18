#!/bin/bash
# Ralph-loop PR Creation Check
# Worker 完成任务后创建 PR 前的检查

set -e

BRANCH_NAME=$(git rev-parse --abbrev-ref HEAD)

echo "[Ralph-loop Hook] PR Creation Check"
echo "  Branch: $BRANCH_NAME"

# 检查分支格式
if [[ ! "$BRANCH_NAME" =~ ^task-[0-9]{3}- ]]; then
    echo "[HOOK ERROR] 分支名不符合任务分支格式!" >&2
    exit 1
fi

# 检查是否有未提交的更改
if ! git diff-index --quiet HEAD --; then
    echo "[HOOK ERROR] 有未提交的更改，请先提交" >&2
    git status --short >&2
    exit 1
fi

# 检查是否已推送到远程
if ! git branch -r | grep -q "$BRANCH_NAME"; then
    echo "[HOOK WARNING] 分支尚未推送到 origin" >&2
    echo "  请先执行: git push origin $BRANCH_NAME" >&2
fi

# 检查是否有 PR 模板
PR_TEMPLATE=".github/pull_request_template.md"
if [ -f "$PR_TEMPLATE" ]; then
    echo "[Ralph-loop Hook] 发现 PR 模板，请按模板填写"
fi

echo "[Ralph-loop Hook] ✓ PR 创建检查通过"
echo "  下一步: 使用 gh pr create 创建 PR"
exit 0
