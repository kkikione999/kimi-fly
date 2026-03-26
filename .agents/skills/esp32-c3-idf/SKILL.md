---
name: esp32-c3-idf
description: >
  ESP32-C3 + ESP-IDF 开发技能。只要任务涉及 ESP32-C3、ESP-IDF、UART/WiFi 桥接、GPIO、boot/reset、sdkconfig、Kconfig、ESP32 侧外设配置或板级连线，就必须使用此技能。进入实现前必须先核对 `hardware-docs/pinout.md` 与 `knowledge-hub/` 中的 ESP32-C3 资料，运行本地硬件检索命令并记录来源；未完成检索前不得修改 ESP32 启动行为、GPIO 用途、UART 连线假设或硬件相关配置。
compatibility: "Requires bash tool. Works best with context7 MCP connected for live ESP-IDF API lookups."
---

# ESP32-C3 + ESP-IDF 开发技能

## 仓库硬门禁

- 在这个仓库里, 先确认板级真相和 knowledge-hub, 再改 ESP32 代码或配置。
- `hardware-docs/pinout.md` 里的 STM32 <-> ESP32 连线高于旧代码和旧注释。
- 如果任务同时改动 STM32 侧串口、复位或协议边界, 额外加载 `stm32-dev`。
- 在检索完成前, 不得修改:
  - `UART` 引脚用途和桥接方向
  - `GPIO0/GPIO1` 相关假设
  - boot/reset 行为
  - `sdkconfig` / Kconfig 中依赖硬件事实的配置
  - 任何依赖板级连线的 GPIO/外设初始化

## 1. 本仓库中哪些任务必须使用本 skill

| 任务 | 是否必须 | 至少要查 |
|------|----------|----------|
| UART 桥接、控制协议收发、串口中断/FIFO | 必须 | `hardware-docs/pinout.md` + `esp32-c3` |
| boot/reset/下载模式/GPIO strap 排查 | 必须 | `hardware-docs/pinout.md` + `esp32-c3` |
| GPIO/UART/SPI/I2C 等 ESP32 侧外设配置 | 必须 | `esp32-c3` |
| `sdkconfig` / Kconfig / 启动行为修改 | 必须 | `esp32-c3` |
| 纯上层协议逻辑且不依赖硬件假设 | 可选 | 先确认不触碰硬件真相 |

## 2. 强制检索顺序

```bash
sed -n '1,220p' hardware-docs/pinout.md
sed -n '1,220p' knowledge-hub/README.md
sed -n '1,220p' knowledge-hub/hardware/esp32-c3/00-overview.md
sed -n '1,220p' knowledge-hub/hardware/esp32-c3/10-registers.md
sed -n '1,220p' knowledge-hub/hardware/esp32-c3/20-bringup-recipes.md
```

然后至少跑一组硬件检索:

```bash
python3 scripts/search_hardware_knowledge.py "esp32-c3 uart fifo interrupt" --chip esp32-c3
python3 scripts/search_hardware_knowledge.py "esp32-c3 gpio0 gpio1 strapping" --chip esp32-c3
python3 scripts/search_hardware_knowledge.py "esp32-c3 reset boot mode" --chip esp32-c3
```

从检索结果里打开命中的 chunk 文件。只有 chunk 不够时, 才去翻 `hardware-docs/` 下原始 PDF。

如果 manifest 缺失, 先构建:

```bash
python3 scripts/build_hardware_knowledge.py --chip esp32-c3 --clean
```

## 3. 本仓库已确认的板级事实

- 角色: WiFi bridge / control communications
- STM32 `PA2 (USART2_TX)` -> ESP32-C3 `GPIO1`
- STM32 `PA3 (USART2_RX)` <- ESP32-C3 `GPIO0`
- `EN` 拉高
- `RST` 与 STM32 `NRST` 共享

这些事实来自 `hardware-docs/pinout.md`。如果旧代码、旧注释、网上示例与此冲突, 本仓库以这里为准。

## 4. 按任务类型的最低检索要求

### 4.1 UART 桥接 / 串口中断

至少确认:

- `GPIO1` 是接收, `GPIO0` 是发送
- 桥接另一侧是 STM32 `USART2`
- UART/FIFO/中断行为来自 ESP32-C3 资料而不是猜测

推荐检索:

```bash
python3 scripts/search_hardware_knowledge.py "esp32-c3 uart fifo interrupt gpio0 gpio1" --chip esp32-c3
```

### 4.2 boot / reset / strap

至少确认:

- 当前板级 reset 连接方式
- `GPIO0/GPIO1` 是否涉及启动相关约束
- 修改不会破坏下载/启动路径

推荐检索:

```bash
python3 scripts/search_hardware_knowledge.py "esp32-c3 gpio0 gpio1 boot strap reset" --chip esp32-c3
```

### 4.3 GPIO / 外设配置

至少确认:

- 该 GPIO 是否已被板级连线占用
- IO mux / GPIO matrix / peripheral route 的限制
- 若与 STM32 串口桥或 reset 共用, 先同时核对 `pinout.md`

推荐检索:

```bash
python3 scripts/search_hardware_knowledge.py "esp32-c3 io mux gpio matrix uart" --chip esp32-c3
```

## 5. 引用格式

硬件敏感任务至少写出这些依据:

```text
Board truth: hardware-docs/pinout.md -> PA2(USART2_TX) -> GPIO1, PA3(USART2_RX) <- GPIO0
Project note: knowledge-hub/hardware/esp32-c3/00-overview.md -> reset shared with STM32 NRST
Datasheet/TRM: knowledge-hub/extracted/chunks/esp32-c3/<document>/pXXXX.md (hardware-docs/ESP32-C3_datasheet.pdf or TRM page)
```

规则:

- 只要桥接方向、GPIO 用途、boot/reset 假设发生变化, 就必须带来源
- 如果改动跨到 STM32 侧, 同时补 STM32 引用
- 没有来源支撑的硬件改动, 不应落代码

## 6. 在硬件检索之后再查 ESP-IDF API

- 先确认板级真相和芯片行为, 再查 ESP-IDF API
- 检查当前版本:

```bash
idf.py --version
```

- 首次建工程或目标不确定时:

```bash
idf.py set-target esp32c3
```

注意: `set-target` 会重置 `sdkconfig` 和 `build/` 目录, 不能盲跑。

需要具体 API / 配置模式时, 再查:

- `references/wifi.md`
- `references/ble.md`
- `references/partitions.md`
