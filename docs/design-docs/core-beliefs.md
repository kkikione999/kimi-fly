# Core Beliefs: kimi-fly Agent-First Principles

kimi-fly 项目的核心理念，适配 Harness Engineering 到嵌入式无人机开发。

## 1. Humans Steer, Agents Execute

人类负责：
- 设定目标和优先级
- 定义验收标准
- 做出判断性决策（安全、架构）

Agent 负责：
- 编写所有代码
- 执行测试
- 生成文档
- 处理反馈循环

## 2. Repository is the System of Record

所有知识必须在仓库中：

- 代码和注释
- 设计文档 (`docs/design-docs/`)
- 执行计划 (`docs/exec-plans/`)
- 硬件规格和接口定义

不在仓库中的知识不存在。

## 3. Progressive Disclosure

信息分层，避免过载：

```
AGENTS.md (100 lines) → CLAUDE.md → detailed docs → code
```

Agent 从地图开始，按需深入。

## 4. Mechanical Enforcement over Manual Review

优先使用自动化：

- Linter 检查代码风格
- 静态分析检查安全规则
- CI 检查构建和测试
- 自定义规则检查架构合规

不要依赖人工审查来捕捉模式问题。

## 5. Fail Fast, Fail Safe

嵌入式系统的安全原则：

- 所有边界条件必须检查
- 使用断言捕获编程错误
- 硬件访问必须验证
- 故障有明确的降级路径

```c
// Good: 快速失败，明确错误
int rc = sensor_init(&sensor);
if (rc < 0) {
    LOG_ERR("Sensor init failed: %d", rc);
    enter_safe_mode();
    return rc;
}

// Bad: 继续执行，未定义行为
sensor_init(&sensor);  // 忽略返回值
use_sensor(&sensor);   // 可能操作未初始化的硬件
```

## 6. Parse, Don't Validate

在边界解析数据，内部使用强类型：

```c
// Good: 解析为类型，后续代码安全
struct imu_data_t {
    float accel[3];  // m/s^2
    float gyro[3];   // rad/s
};

int imu_parse(const uint8_t *raw, struct imu_data_t *out);

// Bad: 原始字节到处传递，重复验证
void process_imu(const uint8_t *raw);
```

## 7. Small Changes, Fast Iteration

高吞吐量的合并哲学：

- PR 小而专注
- 测试失败快速重试，不阻塞
- 修正便宜，等待昂贵
- 技术债务持续偿还，不累积

## 8. Agent Legibility First

代码优化目标：Agent 能理解

- 清晰的命名 > 简洁的命名
- 显式的依赖 > 隐式的约定
- 简单的结构 > 聪明的优化
- 一致的模式 > 多样化的风格

## 9. Continuous Cleanup

技术债务像高息贷款：

- 定期扫描代码库
- 自动检测偏离规范的模式
- 小步重构，持续进行
- 记录债务到 `tech-debt-tracker.md`

## 10. Safety First

无人机是物理系统，安全第一：

- 所有飞行控制代码必须经过 HIL 测试
- 电机控制有硬件级超时保护
- 通信丢失触发自动降落
- 传感器故障有冗余和降级

---

*这些原则指导所有技术决策。当不确定时，回到这些原则。*
