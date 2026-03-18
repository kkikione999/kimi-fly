#!/bin/bash
# Ralph-loop Pre-merge Hook
# 检查 PR 是否可以合并到 main

set -e

echo "[Ralph-loop Hook] Pre-merge Check"

# 检查是否在 PR 审查状态文件中
REVIEW_STATUS_FILE="ralph-loop-session/pr-review-status.md"

if [ ! -f "$REVIEW_STATUS_FILE" ]; then
    echo "[HOOK ERROR] 未找到审查状态文件!" >&2
    echo "  期望文件: $REVIEW_STATUS_FILE" >&2
    echo "  请确保 Reviewer 已完成审查并记录结果" >&2
    exit 1
fi

# 获取当前分支
BRANCH_NAME=$(git rev-parse --abbrev-ref HEAD)

# 检查当前 PR 是否已通过审查
if ! grep -A 5 "$BRANCH_NAME" "$REVIEW_STATUS_FILE" | grep -q "APPROVED"; then
    echo "[HOOK ERROR] 此分支未通过 Reviewer 审批!" >&2
    echo "  分支: $BRANCH_NAME" >&2
    echo "  请先提交 PR 并通过 Reviewer 审查" >&2
    exit 1
fi

# 检查技术债务记录
TECH_DEBT_FILE="docs/exec-plans/tech-debt-tracker.md"
if [ -f "$TECH_DEBT_FILE" ]; then
    # 检查本轮是否产生了新的未记录债务
    NEW_DEBT=$(grep -c "$(date +%Y-%m)" "$TECH_DEBT_FILE" 2>/dev/null || echo "0")
    echo "[Ralph-loop Hook] 当前技术债务条目: $NEW_DEBT"
fi

echo "[Ralph-loop Hook] ✓ Pre-merge 检查通过"
echo "  Reviewer 审批: 通过"
echo "  可以安全合并到 main"
exit 0
