# Design Documents Index

设计文档目录。所有重大设计决策必须记录在此。

## 文档列表

| 文档 | 状态 | 最后更新 | 描述 |
|------|------|----------|------|
| [core-beliefs.md](./core-beliefs.md) | Active | 2026-03-14 | Agent-first 核心理念 |

## 文档状态

- **Active**: 当前有效，所有 Agent 必须遵循
- **Draft**: 草案阶段，征求意见中
- **Deprecated**: 已过时，待更新
- **Archived**: 历史参考，不适用于当前代码

## 添加新文档

1. 在此目录创建 `.md` 文件
2. 更新上表
3. 在 AGENTS.md 中添加链接（如适用）

## 审查流程

设计文档必须经过 review 才能标记为 Active：

1. 作者起草文档
2. harness-architect 审查结构和一致性
3. 相关领域 Agent 审查技术准确性
4. 更新状态为 Active
