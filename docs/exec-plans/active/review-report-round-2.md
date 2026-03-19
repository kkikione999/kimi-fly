# Round 2 计划审核报告

> **审核日期**: 2026-03-19
> **审核人**: Code Reviewer (Ralph-loop v2.0)
> **审核范围**: plan-round-2.md + task-201~205.md

---

## 审核状态

**状态**: APPROVED (附条件通过)

**总体评价**: Round 2 计划整体结构清晰，任务划分合理，完成标准基本可验证。发现若干细节问题需要执行时注意，但不影响整体执行。

---

## 逐任务审核

### Task 201: STM32 固件编译和烧录验证

**状态**: APPROVED

**优点**:
- 任务边界清晰：专注于 STM32 编译和烧录
- 完成标准明确且可验证（编译成功、烧录成功、串口输出、LED指示）
- 相关文件列表完整（platformio.ini 已存在且配置正确）

**发现的问题**:
1. **相关文件列表不完整**: 实际代码中 `firmware/stm32/main/sensor_test.c` 已存在，但任务文档中未提及
2. **CMakeLists.txt 可能不需要**: 项目已使用 PlatformIO，CMakeLists.txt 是可选的

**建议**:
- 如 sensor_test.c 已存在，Task 203 可能需要调整范围
- 确认是否需要同时维护 Makefile 和 PlatformIO 两种构建方式

---

### Task 202: ESP32 固件编译和烧录验证

**状态**: NEEDS_REVISION (轻微)

**问题**:
1. **ESP32 代码状态不明确**: 现有 `firmware/esp32/main/main.c` 只有基础 UART 功能，**缺少 WiFi STA 实现**
2. **任务范围过大**: 文档要求实现 WiFi STA + UART 透传 + TCP 服务器，这超出了"编译烧录验证"的范围
3. **相关文件与实际不符**: `wifi_sta.cpp`, `uart_bridge.cpp`, `wifi_config.h` 目前不存在

**建议修改**:
```markdown
## 具体修改要求

### 文件 1: `firmware/esp32/platformio.ini` (如不存在则创建)
[原有内容]

### 文件 2: `firmware/esp32/src/wifi_sta.cpp` (创建)
1. 实现 WiFi STA 模式连接 `whc` 网络
2. 实现自动重连机制
3. 连接成功后输出 IP 地址到串口

### 文件 3: `firmware/esp32/src/main.cpp` (修改或创建)
1. 整合现有 UART 代码
2. 添加 WiFi 初始化和状态输出
```

**依赖风险**: Task 202 实际包含代码开发工作，可能需要更多时间。

---

### Task 203: 传感器读取验证

**状态**: APPROVED (附条件)

**优点**:
- 完成标准非常详细，包含数据合理性验证
- 错误处理考虑周全（重试机制）

**发现的问题**:
1. **代码已存在**: `firmware/stm32/main/sensor_test.c` 已完整实现，此任务可能主要是"验证"而非"开发"
2. **任务范围需调整**: 如代码已存在，任务应改为"运行传感器测试并验证输出"

**建议**:
- 确认 sensor_test.c 是否需要修改，还是直接运行验证
- 如无需修改，任务可简化为：编译测试固件 -> 烧录 -> 运行 -> 验证输出

---

### Task 204: STM32-ESP32 串口通信验证

**状态**: APPROVED

**优点**:
- 协议定义清晰（帧格式、CRC、波特率）
- 验证方法具体（万用表、逻辑分析仪、命令响应）

**发现的问题**:
1. **波特率不一致风险**:
   - STM32 调试串口: 460800 baud (platformio.ini)
   - STM32-ESP32 通信: 115200 baud (task-204.md)
   - 文档中未明确区分"调试串口"和"通信串口"

2. **引脚定义需确认**: 需验证 STM32 USART2 (PA2/PA3) 是否已在 HAL 层正确配置

**建议**:
- 在文档中明确标注两种串口的用途和波特率
- 执行前验证 `firmware/stm32/hal/uart.c` 中 USART2 配置

---

### Task 205: WiFi 控制命令测试

**状态**: NEEDS_REVISION

**严重问题**:
1. **前置条件不满足**: Task 205 依赖 Task 202，但 Task 202 缺少 TCP 服务器实现
2. **代码不存在**: `firmware/esp32/src/tcp_server.cpp` 需要创建
3. **任务范围过大**: 包含 WiFi 连接、TCP 服务器、地面站修改，可能应拆分为 2-3 个任务

**建议拆分**:
```
Task 202: ESP32 WiFi STA 模式实现
Task 205a: ESP32 TCP 服务器实现
Task 205b: 地面站 WiFi 模式支持
Task 205c: 端到端控制命令测试
```

**或调整 Task 202 范围**:
将 WiFi STA + TCP 服务器实现移到 Task 202，使 Task 205 专注于测试验证。

---

## 问题汇总

| 优先级 | 问题 | 影响 | 建议 |
|--------|------|------|------|
| 高 | Task 202 范围与实际代码状态不匹配 | 可能延期 | 明确是否包含开发工作 |
| 高 | Task 205 依赖的代码不存在 | 无法执行 | 拆分任务或调整 Task 202 |
| 中 | 串口波特率定义分散 | 配置错误风险 | 统一在硬件文档中定义 |
| 低 | Task 203 代码已存在 | 任务范围需调整 | 改为验证任务 |

---

## 建议

### 1. 任务范围调整建议

**方案 A - 最小改动**:
- Task 202: 明确包含"实现 WiFi STA 基础连接"（不含 TCP 服务器）
- Task 205: 拆分为 TCP 服务器实现 + 测试验证两个子任务

**方案 B - 重新平衡**:
- Task 202: 仅做编译烧录现有代码
- 新增 Task 206: ESP32 WiFi + TCP 功能开发
- Task 205: 专注于测试验证

### 2. 文档改进建议

1. **添加硬件连接检查清单**:
   ```markdown
   ## 硬件连接检查
   - [ ] ST-Link 连接: SWDIO(PA13), SWCLK(PA14), GND
   - [ ] USB-TTL 连接: PA9(TX), PA10(RX), GND (调试)
   - [ ] STM32-ESP32 连接: PA2(TX)->ESP_RX, PA3(RX)->ESP_TX, GND
   - [ ] WiFi 网络 `whc` 可用
   ```

2. **明确波特率定义**:
   | 用途 | 引脚 | 波特率 |
   |------|------|--------|
   | 调试输出 | PA9/PA10 | 460800 |
   | STM32-ESP32 | PA2/PA3 | 115200 |
   | ESP32 调试 | USB | 115200 |

### 3. Hook 范围确认

根据代码现状，实际 Hook 范围应为:

| 任务 | 实际相关文件 |
|------|-------------|
| 201 | `firmware/stm32/platformio.ini` (已存在) |
| 202 | `firmware/esp32/platformio.ini`, `src/wifi_sta.cpp`, `src/main.cpp` |
| 203 | `firmware/stm32/main/sensor_test.c` (已存在，可能只读) |
| 204 | `firmware/stm32/comm/protocol.c`, `firmware/stm32/main/flight_main.c` |
| 205 | `firmware/esp32/src/tcp_server.cpp`, `tools/ground_station/ground_station.py` |

---

## 执行建议

1. **Task 201**: 可直接执行，platformio.ini 已配置正确
2. **Task 202**: 先确认现有 ESP32 代码状态，如需开发 WiFi 功能，评估时间
3. **Task 203**: 检查 sensor_test.c 是否可直接编译运行
4. **Task 204**: 执行前验证 USART2 配置
5. **Task 205**: 建议暂停，待 Task 202 完成后再细化

---

## 审核结论

**计划整体通过，但 Task 202 和 Task 205 需要范围调整。**

建议 Harness-Architect 确认:
1. Task 202 是否包含 WiFi 功能开发？
2. Task 205 是否需要拆分为开发和测试两个阶段？

确认后即可进入执行阶段。

---

*审核完成 - Code Reviewer*
