---
name: stm32-dev
description: >
  STM32 + 传感器 + 电机硬件开发技能。只要任务涉及 STM32F411、HAL/LL/CMSIS/FreeRTOS、GPIO/PWM/UART/I2C/SPI/ADC/EXTI、ICM-42688-P、LPS22HBTR、QMC5883P、电机 PWM、引脚分配、外设配置、地址/寄存器/时序/坐标系，就必须使用此技能。进入实现前必须先核对 `hardware-docs/pinout.md` 与 `knowledge-hub/`，运行本地硬件检索命令并记录来源；未完成检索前不得修改驱动、寄存器配置、引脚初始化或硬件常量。
compatibility: "bash; gcc-arm-none-eabi; openocd; stlink-tools; minicom; context7 MCP (optional)"
---

# STM32 + 传感器开发技能

## 仓库硬门禁

- 对这个仓库来说, `hardware-docs/pinout.md` 和实机结论优先级最高, 高于旧代码、旧注释、旧测试文件和旧飞机经验。
- 只要任务涉及 STM32、IMU、气压计、磁力计、电机、引脚、外设配置, 先完成硬件检索, 再写代码。
- 在硬件检索完成前, 你只能做这些事:
  - 阅读源码并定位将被修改的文件
  - 梳理影响范围
  - 列出待确认事实
- 在硬件检索完成前, 不得修改:
  - 引脚/AF/时钟/外设初始化
  - 传感器地址、寄存器常量、初始化顺序
  - 电机通道、物理位置、旋向、PWM 频率
  - 中断极性、数据就绪模式、坐标轴映射
- 如果任务同时涉及 ESP32-C3, 额外加载 `esp32-c3-idf`。

## 1. 本仓库中哪些任务必须使用本 skill

| 任务 | 是否必须 | 需要查哪些芯片 |
|------|----------|----------------|
| STM32F411 GPIO / PWM / UART / I2C / SPI / ADC / EXTI / RCC / NVIC | 必须 | `stm32f411` |
| ICM-42688-P 驱动、INT1、中断、坐标映射、初始化 | 必须 | `icm-42688-p` + `stm32f411` |
| LPS22HBTR 驱动、SPI3、CS、DRDY | 必须 | `lps22hbtr` + `stm32f411` |
| QMC5883P 驱动、I2C1、量程/输出模式 | 必须 | `qmc5883p` + `stm32f411` |
| 电机 PWM、定时器通道、混控依赖的电机物理映射 | 必须 | `stm32f411` + `hardware-docs/pinout.md` |
| 仅纯算法且不改变任何硬件接口 | 可选 | 先确认确实不触碰硬件事实 |

## 2. 强制检索顺序

先读板级真源和总规则:

```bash
sed -n '1,220p' hardware-docs/pinout.md
sed -n '1,220p' knowledge-hub/README.md
sed -n '1,220p' knowledge-hub/hardware/stm32f411/00-overview.md
sed -n '1,220p' knowledge-hub/hardware/stm32f411/10-registers.md
sed -n '1,220p' knowledge-hub/hardware/stm32f411/20-bringup-recipes.md
```

按需追加传感器资料:

```bash
sed -n '1,220p' knowledge-hub/hardware/icm-42688-p/00-overview.md
sed -n '1,220p' knowledge-hub/hardware/icm-42688-p/10-registers.md
sed -n '1,220p' knowledge-hub/hardware/icm-42688-p/20-bringup-recipes.md

sed -n '1,220p' knowledge-hub/hardware/lps22hbtr/00-overview.md
sed -n '1,220p' knowledge-hub/hardware/lps22hbtr/10-registers.md
sed -n '1,220p' knowledge-hub/hardware/lps22hbtr/20-bringup-recipes.md

sed -n '1,220p' knowledge-hub/hardware/qmc5883p/00-overview.md
sed -n '1,220p' knowledge-hub/hardware/qmc5883p/10-registers.md
sed -n '1,220p' knowledge-hub/hardware/qmc5883p/20-bringup-recipes.md
```

然后运行本地 chunk 检索。优先检索, 不要先翻整本 PDF:

```bash
python3 scripts/search_hardware_knowledge.py "stm32f411 tim pwm gpio alternate function" --chip stm32f411
python3 scripts/search_hardware_knowledge.py "i2c1 spi3 usart2 exti" --chip stm32f411
python3 scripts/search_hardware_knowledge.py "who am i pwr_mgmt0 accel_config0 gyro_config0 int_config" --chip icm-42688-p
python3 scripts/search_hardware_knowledge.py "who am i ctrl_reg1 ctrl_reg2 press_out spi mode" --chip lps22hbtr
python3 scripts/search_hardware_knowledge.py "control measurement mode odr osr" --chip qmc5883p
```

从检索结果里打开命中的 chunk 文件。只有 chunk 不够时, 才去翻 `hardware-docs/` 下原始 PDF。

如果 `search_hardware_knowledge.py` 报 manifest 不存在, 先构建再继续:

```bash
python3 scripts/build_hardware_knowledge.py --chip stm32f411 --clean
python3 scripts/build_hardware_knowledge.py --chip icm-42688-p --clean
python3 scripts/build_hardware_knowledge.py --chip lps22hbtr --clean
python3 scripts/build_hardware_knowledge.py --chip qmc5883p --clean
```

## 3. 按任务类型的最低检索要求

### 3.1 引脚 / 外设配置

至少确认这些事实:

- 真实引脚和连接对象来自 `hardware-docs/pinout.md`
- STM32 外设实例、通道、AF、时钟门控、复位依赖来自 `stm32f411` 文档
- 总线模式和时序来自对应器件知识与 chunk

推荐检索:

```bash
python3 scripts/search_hardware_knowledge.py "gpio alternate function pull output type speed" --chip stm32f411
python3 scripts/search_hardware_knowledge.py "rcc clock enable reset" --chip stm32f411
```

### 3.2 电机 / PWM / 定时器

编码前必须明确:

- `Motor1/PA8 = FrontLeft/CCW`
- `Motor2/PA11 = RearLeft/CW`
- `Motor3/PB1 = RearRight/CCW`
- `Motor4/PB10 = FrontRight/CW`

这些身份来自实机, 不能按 `TIMx_CHy` 顺序重命名。

推荐检索:

```bash
python3 scripts/search_hardware_knowledge.py "tim1 pwm preload update event" --chip stm32f411
python3 scripts/search_hardware_knowledge.py "tim2 pwm preload update event" --chip stm32f411
python3 scripts/search_hardware_knowledge.py "tim3 pwm preload update event" --chip stm32f411
```

### 3.3 ICM-42688-P

编码前至少确认:

- 总线: `I2C1`
- 地址: `0x69`
- `WHO_AM_I = 0x47`
- 上电/复位等待时间
- `body +X = IMU +Y`, `body +Y = IMU -X`, `body +Z = IMU +Z`

推荐检索:

```bash
python3 scripts/search_hardware_knowledge.py "who am i pwr_mgmt0 accel_config0 gyro_config0 int_config int_async_reset" --chip icm-42688-p
```

### 3.4 LPS22HBTR

编码前至少确认:

- 总线: `SPI3`
- CS: `PA15`
- SPI mode: `Mode 0`
- `WHO_AM_I`
- 压力/温度输出寄存器

推荐检索:

```bash
python3 scripts/search_hardware_knowledge.py "who am i spi mode ctrl_reg1 ctrl_reg2 press_out temp_out" --chip lps22hbtr
```

### 3.5 QMC5883P

编码前至少确认:

- 总线: `I2C1`
- 地址: `0x2C`
- 控制寄存器、量程/OSR/ODR、数据输出寄存器

推荐检索:

```bash
python3 scripts/search_hardware_knowledge.py "who am i control measurement mode range odr osr data output" --chip qmc5883p
```

## 4. 引用格式

硬件敏感任务在计划、提交说明、代码解释或评审意见里, 至少要留下这三类依据:

1. `Board truth`
2. `Project hardware note`
3. `Datasheet/register source`

推荐写法:

```text
Board truth: hardware-docs/pinout.md -> PB6/PB7 = I2C1 for ICM-42688-P / QMC5883P
Project note: knowledge-hub/hardware/icm-42688-p/00-overview.md -> address = 0x69, body_x = +imu_y
Datasheet: knowledge-hub/extracted/chunks/icm-42688-p/icm-42688-p-datasheet/p0087.md (hardware-docs/ICM-42688-P_datasheet.pdf, p.87)
```

规则:

- 只要你改了某个硬件事实, 就必须能指出来源
- 引脚、电机位置、坐标系必须优先引用 `hardware-docs/pinout.md`
- 如果源码里有冲突常量或注释, 标记为 stale, 不要拿它做依据

## 5. 在硬件检索之后再查 API

硬件事实确认完之后, 如果还要写 HAL / LL / CMSIS / FreeRTOS 代码:

- 优先查 context7 MCP, 不要凭记忆猜 API
- 常见主题:
  - `HAL_TIM_PWM_Start`
  - `HAL_I2C_Mem_Read`
  - `HAL_SPI_TransmitReceive`
  - `HAL_UART_Transmit`
  - `HAL_GPIO_Init`
  - `xTaskCreate`

## 6. 常用仓库入口

- `knowledge-hub/hardware/stm32f411/00-overview.md`
- `knowledge-hub/hardware/stm32f411/10-registers.md`
- `knowledge-hub/hardware/stm32f411/20-bringup-recipes.md`
- `knowledge-hub/hardware/icm-42688-p/20-bringup-recipes.md`
- `knowledge-hub/hardware/lps22hbtr/20-bringup-recipes.md`
- `knowledge-hub/hardware/qmc5883p/20-bringup-recipes.md`
- `references/hal-api-patterns.md`
- `references/openocd-advanced.md`
