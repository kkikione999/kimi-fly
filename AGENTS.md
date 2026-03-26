# 硬件开发强约束

> **项目**: 无人机WiFi飞行控制器

---

## AI 工具分派

只要任务不是纯文案, 而是和这个仓库的无人机开发/调试有关, agent 不能只停留在“选芯片 skill”, 还必须先判断应该调用哪条开发调试工具链。

- 只要任务涉及无人机飞控开发、台架/系绳调试、编译、烧录、串口抓日志、飞行日志复盘、STM32 <-> ESP32 联调、或“现在该用什么工具”, 必须先加载 `.agents/skills/drone-debug`
- `drone-debug` 负责先确认仓库里真实存在的工具和代码路径, 再继续分派到 `.agents/skills/stm32-dev` / `.agents/skills/esp32-c3-idf`
- 如果 `README.md`、`FLIGHT_DEBUG_STATUS.md`、旧注释、旧会话记录里提到某个脚本或命令, 但仓库当前文件树里不存在, 必须先把它判定为 stale, 再选择真实存在的工具
- 验证工具存在时优先用:
  - `rg --files tools`
  - `python3 -m platformio --version`
  - `source /Users/ll/esp/esp-idf/export.sh >/dev/null 2>&1 && idf.py --version`
  - `which openocd arm-none-eabi-gdb minicom`

---

## 实机确认事实

以下信息来自用户说明或 2026-03-24 的实机台架验证，不能仅凭源码可靠推导。后续 agent 必须默认相信这些事实，除非用户明确说硬件又改了。

- 机体坐标系: `+X = 前方`, `+Y = 左方`, `+Z = 上方`
- 电机真实位置与旋向:
  - `Motor1 / PA8 = FrontLeft / CCW`
  - `Motor2 / PA11 = RearLeft / CW`
  - `Motor3 / PB1 = RearRight / CCW`
  - `Motor4 / PB10 = FrontRight / CW`
- IMU 到机体系的运行时映射:
  - `body +X = IMU +Y`
  - `body +Y = IMU -X`
  - `body +Z = IMU +Z`
- 当前台架约束状态:
  - 无人机左右两侧各有一根绳子连接到外部支架
  - 两根绳子都有较大余量, 允许无人机起飞并在一定范围内运动, 不是刚性锁死
  - 这是用于调试的限位飞行状态, 不能把它当成完全落地固定或完全自由飞行
- 静止状态姿态:
  - 无人机在静止时机体是平的, 可视为近似水平
- 不要再从旧飞机代码反推电机布局或 IMU 朝向来覆盖这些实机结论。

---

## 硬件知识检索强约束


### 1. 哪些任务必须先调用 skill

处理以下任务时, 不能直接开始写代码、改寄存器、改初始化顺序、改引脚复用或改外设配置, 必须先加载对应 skill:

| 任务类型 | 必用 skill | 说明 |
|----------|------------|------|
| 无人机飞控开发、台架/系绳调试、编译/烧录、串口抓日志、飞行日志复盘、联调时判断该用什么工具 | `.agents/skills/drone-debug` | 先分派真实工具链, 再决定是否继续加载 STM32 / ESP32 skill |
| STM32、HAL/LL、GPIO、PWM、UART、I2C、SPI、ADC、EXTI、时钟、外设初始化 | `.agents/skills/stm32-dev` | 适用于所有 STM32F411 硬件敏感任务 |
| IMU (`ICM-42688-P`)、气压计 (`LPS22HBTR`)、磁力计 (`QMC5883P`) 驱动或配置 | `.agents/skills/stm32-dev` | 先查板级真连线, 再查芯片知识与寄存器 |
| 电机、电调、PWM 输出、定时器通道、混控所依赖的电机位置/旋向 | `.agents/skills/stm32-dev` | 电机身份必须绑定真实物理位置, 不能按通道顺序臆测 |
| 引脚分配、总线映射、片选、中断脚、外设模式、地址、时序、坐标系 | `.agents/skills/stm32-dev` | 先确认 `pinout.md` 与实机结论 |
| ESP32-C3、ESP-IDF、UART 桥接、WiFi 通信、GPIO、boot/reset、ESP32 侧外设配置 | `.agents/skills/esp32-c3-idf` | ESP32 任务一律走 ESP32 skill |
| 同时涉及 STM32 与 ESP32 的跨芯片链路任务 | `.agents/skills/stm32-dev` + `.agents/skills/esp32-c3-idf` | 两边都要完成各自检索, 不能只查一侧 |

### 2. 硬件任务统一检索顺序

进入实现前, 必须按以下顺序完成检索:

1. `hardware-docs/pinout.md`
2. `knowledge-hub/README.md`
3. `knowledge-hub/hardware/<chip>/00-overview.md`
4. `knowledge-hub/hardware/<chip>/10-registers.md`
5. `knowledge-hub/hardware/<chip>/20-bringup-recipes.md`
6. `python3 scripts/search_hardware_knowledge.py "<query>" --chip <chip>`
7. 打开命中的 `knowledge-hub/extracted/chunks/<chip>/<document>/...`
8. 如果前述资料仍不足, 再打开 `hardware-docs/` 下原始 PDF

适用 chip slug:

- `stm32f411`
- `esp32-c3`
- `icm-42688-p`
- `lps22hbtr`
- `qmc5883p`

### 3. 检索时必须确认的事实

在开始编码前, agent 必须明确确认与当前任务相关的以下事实:

- 引脚与总线: 引脚号、外设实例、AF/模式、片选、中断脚、上下游连接对象
- 寄存器与地址: 外设寄存器、传感器寄存器、7-bit 地址、默认值、`WHO_AM_I`
- 时序与模式: SPI mode、I2C 时序、PWM 频率、启动等待时间、复位顺序、数据就绪行为
- 坐标系与物理映射: IMU 到机体系映射、电机真实位置、旋向、台架限制状态
- 芯片约束: boot/reset/strap、电气限制、总线共享关系、初始化前置条件

如果这些事实里有任何一项没有查清, agent 只能继续检索, 不能直接写驱动或改配置。

### 4. 禁止事项
- 在完成检索前, 不能修改:
  - 传感器地址、寄存器常量、初始化序列
  - GPIO/AF/时钟/I2C/SPI/UART/ADC/EXTI 配置
  - 电机通道映射、旋向、PWM 频率、混控输入映射
  - ESP32 boot/reset/UART/GPIO 相关配置
- 在确认文件真实存在前, 不能直接照抄旧文档里的脚本名或命令名当成当前工具链
- 在确认当前代码路径前, 不能把 `firmware/esp32/platformio.ini` / `firmware/esp32/src/main.cpp` 自动当成 ESP32 当前主线; 先核对是否应走 `ESP-IDF` 的 `firmware/esp32/main/` + `sdkconfig` + `CMakeLists.txt`

原则:

- 只要改动依赖某个硬件事实, 就必须能指出该事实来自哪里
- 引脚、电机位置、坐标系这类板级真相, 必须优先引用 `pinout.md`

### 5. 对应 skill 选择
- 无人机飞控调试、台架联调、串口抓日志、编译烧录、飞行日志复盘、判断当前该跑什么工具 → `.agents/skills/drone-debug`
- STM32 HAL/LL、引脚、时钟、GPIO、PWM、I2C、SPI、UART、ADC、EXTI → `.agents/skills/stm32-dev`
- ICM-42688-P、LPS22HBTR、QMC5883P、传感器总线、坐标系、寄存器地址 → `.agents/skills/stm32-dev`
- 电机位置、旋向、定时器通道、混控依赖的物理映射 → `.agents/skills/stm32-dev`
- ESP32-C3、ESP-IDF、WiFi 桥接、UART/reset/boot/GPIO、ESP32 侧外设配置 → `.agents/skills/esp32-c3-idf`
- 跨 STM32 <-> ESP32 链路 → 两个 skill 都要使用
