#!/bin/bash
# Ralph-loop Edit/Write Hook
# 检查文件修改范围

set -e

FILE_PATH="$1"

echo "[Ralph-loop Hook] Edit/Write Check"
echo "  File: $FILE_PATH"

# 获取当前分支名
BRANCH_NAME=$(git rev-parse --abbrev-ref HEAD 2>/dev/null || echo "unknown")

# 如果不是任务分支，跳过检查
if [[ ! "$BRANCH_NAME" =~ ^task-[0-9]{3}- ]]; then
    echo "[Ralph-loop Hook] 非任务分支，跳过范围检查"
    exit 0
fi

# 提取任务编号
TASK_NUM=$(echo "$BRANCH_NAME" | grep -oE '[0-9]{3}')

# 查找任务文档
TASK_DOC=$(find docs/exec-plans/active -name "task-${TASK_NUM}-*.md" 2>/dev/null | head -1)

if [ -z "$TASK_DOC" ]; then
    echo "[HOOK ERROR] 未找到任务文档: task-${TASK_NUM}-*.md" >&2
    exit 1
fi

# 检查文件是否在相关文件列表中
if ! grep -q "$FILE_PATH" "$TASK_DOC" 2>/dev/null; then
    # 检查是否是允许的新文件创建
    if [ -f "$FILE_PATH" ]; then
        echo "[HOOK WARNING] 修改的文件不在任务文档的 '相关文件' 列表中" >&2
        echo "  文件: $FILE_PATH" >&2
        echo "  任务: $TASK_DOC" >&2
        echo "  请确认此修改在任务范围内，或更新任务文档" >&2
        # 暂时警告但不阻止，让 Worker 自行判断
    else
        echo "[Ralph-loop Hook] 创建新文件: $FILE_PATH"
        echo "  请确保在任务文档中记录此文件" >&2
    fi
fi

echo "[Ralph-loop Hook] ✓ Edit/Write 检查通过"
exit 0
