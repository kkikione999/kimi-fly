# Agent Assignment - Ralph-loop v2.0

> **创建日期**: 2026-03-19
> **负责人**: Harness-Architect
> **状态**: 待 Leader 确认

---

## 任务分析

### Task 201: STM32 固件编译和烧录验证

| 属性 | 内容 |
|------|------|
| **目标** | 编译 STM32 飞控固件并通过 ST-Link 烧录到目标板 |
| **复杂度** | 中等 |
| **领域** | STM32 嵌入式开发 |
| **涉及技术** | PlatformIO, STM32Cube, ST-Link, ARM GCC |
| **相关文件** | `firmware/stm32/platformio.ini`, `CMakeLists.txt`, `Makefile` |
| **硬件平台** | STM32F411CEU6 |

**任务特点分析**:
- 需要配置 STM32 编译环境
- 涉及 PlatformIO 或 CMake 构建系统
- 需要了解 STM32 HAL 库结构
- 需要配置 ST-Link 烧录参数

**推荐 Agent**: `stm32-embedded-engineer`

**推荐理由**:
1. 任务核心是 STM32 固件编译和烧录
2. 需要熟悉 STM32Cube/PlatformIO 工具链
3. 涉及 STM32F411CEU6 特定配置
4. 该 Agent 专门处理 STM32 HAL/驱动/固件开发

---

### Task 202: ESP32 固件编译和烧录验证

| 属性 | 内容 |
|------|------|
| **目标** | 编译 ESP32-C3 WiFi 通信固件并烧录，验证 WiFi STA 模式 |
| **复杂度** | 中等 |
| **领域** | ESP32-C3 / WiFi 通信开发 |
| **涉及技术** | ESP-IDF/Arduino-ESP32, esptool, WiFi STA |
| **相关文件** | `firmware/esp32/platformio.ini`, `main.cpp`, `wifi_config.h` |
| **硬件平台** | ESP32-C3 |

**任务特点分析**:
- 需要配置 ESP32-C3 编译环境
- 涉及 WiFi STA 模式配置
- 需要实现 UART 透传功能
- 如固件不存在，需要实现基础功能

**推荐 Agent**: `esp32-c3-autonomous-engineer`

**推荐理由**:
1. 任务核心是 ESP32-C3 WiFi 开发
2. 需要配置 WiFi STA 模式
3. 涉及 ESP32-C3 特定工具链 (ESP-IDF/Arduino)
4. 该 Agent 专门处理 ESP32-C3/WiFi/通信开发

---

## Agent 分配建议

| 任务 | 推荐 Agent | 优先级 | 预估耗时 |
|------|------------|--------|----------|
| Task 201 | stm32-embedded-engineer | P0 | 30-60 min |
| Task 202 | esp32-c3-autonomous-engineer | P0 | 30-60 min |

---

## 并行化机会

**Tasks 201 和 202 可以并行执行**，原因如下：

1. **无依赖关系**: 两个任务的前置任务都是"无"
2. **独立工作区**: STM32 和 ESP32 固件位于不同目录
   - Task 201: `firmware/stm32/`
   - Task 202: `firmware/esp32/`
3. **独立硬件**: 两个任务分别操作不同的硬件模块
4. **独立工具链**: 使用不同的编译器和烧录工具

**建议执行顺序**:
```
同时启动:
├─ Worker 1 (stm32-embedded-engineer) -> Task 201
└─ Worker 2 (esp32-c3-autonomous-engineer) -> Task 202
```

---

## 风险缓解

| 潜在风险 | 缓解策略 |
|----------|----------|
| **Task 202 固件不存在** | 任务描述中提到"如固件不存在，需要先实现基础 WiFi STA + UART 透传功能"，可能需要增加任务范围或拆分任务 |
| **PlatformIO 环境未安装** | Worker 应在开始前检查环境，如缺失需记录到技术债务 |
| **WiFi 网络不可用** | Task 202 的标准 3 可能无法验证，需标记为条件性通过 |
| **ST-Link 连接问题** | Task 201 需要硬件连接，如失败需记录具体错误信息 |

---

## 备用方案

如推荐的 Agent 不可用，可考虑：

| 任务 | 首选 Agent | 备选 Agent | 备选理由 |
|------|------------|------------|----------|
| Task 201 | stm32-embedded-engineer | code-simplifier | 构建配置属于通用工程任务 |
| Task 202 | esp32-c3-autonomous-engineer | code-simplifier | 如仅需配置编译环境 |

---

## 下一步行动

1. **Leader 确认**: 请 Leader 确认 Agent 分配方案
2. **创建 Agent 团队**: 根据确认结果创建对应 Agent
3. **分配任务**: 将 Task 201 和 Task 202 分配给相应 Worker
4. **并行执行**: 同时启动两个任务的执行

---

## 参考文档

- Task 201: `docs/exec-plans/active/task-201.md`
- Task 202: `docs/exec-plans/active/task-202.md`
- Harness 流程: `RALPH-HARNESS.md`
- Hook 配置: `docs/hooks-config-v2.0.md`
