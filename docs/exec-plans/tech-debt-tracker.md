# Technical Debt Tracker

持续追踪的技术债务。供定期清理 Agent 使用。

## Format

每项债务记录：
- **ID**: 唯一标识符 (DEBT-XXX)
- **Description**: 债务描述
- **File/Location**: 相关文件
- **Created**: 创建日期
- **Severity**: Critical / High / Medium / Low
- **Category**: Architecture / Code / Test / Doc / Tooling
- **Effort**: 预估修复工作量 (hours)
- **Status**: Open / In Progress / Resolved

## Active Debt

<!-- 新债务添加在此处 -->

## Resolved Debt

<!-- 已解决债务移动至此，保留历史记录 -->

## Cleanup Process

每周运行清理流程：

1. **扫描**: 代码扫描 Agent 检测偏离规范的模式
2. **评估**: 评估新发现债务的严重性和工作量
3. **排序**: 优先处理 Critical 和 High 债务
4. **修复**: 创建小 PR 修复债务
5. **验证**: 测试确保修复不引入新问题
6. **更新**: 移动已解决债务到 Resolved 部分

## Categories

### Architecture
- 模块边界违反
- 循环依赖
- 抽象泄漏

### Code
- 重复代码
- 复杂函数（圈复杂度高）
- 魔法数字
- 未处理的错误码

### Test
- 缺失的测试覆盖
- 脆弱的测试
- 慢的测试

### Doc
- 过时的文档
- 缺失的 API 文档
- 不一致的注释

### Tooling
- 缺失的自动化检查
- 缓慢的 CI
- 缺失的工具

---

*债务是正常开发的一部分。关键是可见和持续偿还。*
