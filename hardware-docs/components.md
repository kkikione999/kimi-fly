# 元器件清单 - 软件开发参考

> **版本**: 基于电路图 v2026-03-11
> **用途**: 软件开发所需的硬件元器件关键信息

---

## 主控板 (SCH_主控_1-P1)

### U9 - STM32F411CEU6 (主控MCU)

| 属性 | 值 |
|------|-----|
| 型号 | STM32F411CEU6 |
| 封装 | UFQFPN48 |
| 架构 | ARM Cortex-M4 |
| 主频 | 100MHz |
| Flash | 512KB |
| SRAM | 128KB |
| 工作电压 | 1.7V - 3.6V |

**时钟:**
- X2: 8MHz 外部晶振 (HSE输入: PH0-OSC_IN / PH1-OSC_OUT)

**电源引脚:**
- VDD: 引脚24, 36, 48 (接3V3_D)
- VSS: 引脚23, 35, 47 (接地)
- VDDA/VREF+: 引脚9 (模拟电源)
- VSSA/VREF-: 引脚8 (模拟地)
- VCAP1: 引脚22 (去耦电容)
- VBAT: 引脚1 (电池备份)

**关键GPIO分配:**

| 功能 | GPIO | 模式 | 说明 |
|------|------|------|------|
| SWDIO | PA13 | 复用 | 调试数据 |
| SWCLK | PA14 | 复用 | 调试时钟 |
| UART1_TX | PA9 | 复用 | 调试串口发送 |
| UART1_RX | PA10 | 复用 | 调试串口接收 |
| UART2_TX | PA2 | 复用 | WiFi模块发送 (STM_TXD2) |
| UART2_RX | PA3 | 复用 | WiFi模块接收 (STM_RXD2) |
| I2C1_SCL | PB6 | 开漏 | 传感器时钟 (STMI2C_SCL1) |
| I2C1_SDA | PB7 | 开漏 | 传感器数据 (STMI2C_SDA1) |
| SPI3_SCK | PB3 | 复用 | IMU时钟 (STM_SPI_CLK3) |
| SPI3_MISO | PB4 | 复用 | IMU输入 (STM_SPI_MISO3) |
| SPI3_MOSI | PB5 | 复用 | IMU输出 (STM_SPI_MOSI3) |
| SPI3_NSS | PA15 | 复用 | IMU片选 (STM_SPI_NSS3) |
| BOOT0 | 引脚44 | 输入 | 启动模式选择 |
| NRST | 引脚7 | 输入 | 外部复位 |

---

### U10 - ESP32-C3 (WiFi模块)

| 属性 | 值 |
|------|-----|
| 型号 | ESP32-C3 或 ESP-12F兼容模块 |
| 功能 | 2.4GHz WiFi |
| 接口 | UART + GPIO |
| 电源 | 3V3_RF |

**引脚连接:**

| 模块引脚 | 连接 | 说明 |
|----------|------|------|
| TXD0 | STM_RXD2 (PA3) | 模块发送 → STM接收 |
| RXD0 | STM_TXD2 (PA2) | STM发送 → 模块接收 |
| IO0 | BOOT_IO9 | 启动/工作模式控制 |
| EN | R73上拉到3V3_RF | 模块使能 |
| 3V3 | 3V3_RF | 电源 |
| GND | GND | 地 |

**WiFi配置:**
- SSID: `whc`
- 密码: `12345678`
- 模式: STA (Station模式)

**波特率:**
- 默认AT指令波特率: 115200

---

### U15 - LPS22HBTR (气压计)

| 属性 | 值 |
|------|-----|
| 型号 | LPS22HBTR |
| 功能 | 气压/温度传感器 |
| 接口 | I2C/SPI (本项目使用I2C) |
| 电源 | 3V3_IMU |

**引脚连接:**

| 引脚 | 连接 | 说明 |
|------|------|------|
| VDD_IO | 3V3_IMU | IO电源 |
| SCL/SPC | STMI2C_SCL1 (PB6) | I2C时钟 |
| SDA/SDI/SDO | STMI2C_SDA1 (PB7) | I2C数据 |
| CS | 3V3_IMU (R51=0Ω) | I2C模式 (CS接高) |
| INT_DRDY | BARO_INT | 中断/数据就绪 |
| GND | GND | 地 |

**I2C地址:**
- SA0=GND: 0x5C (写: 0xB8, 读: 0xB9)
- SA0=VCC: 0x5D (写: 0xBA, 读: 0xBB)
- 电路图中SA0通过R52(0Ω)接GND，地址为 **0x5C**

**关键寄存器:**
- WHO_AM_I (0x0F): 预期值 0xB1

---

### U16 - QMC5883P (磁力计)

| 属性 | 值 |
|------|-----|
| 型号 | QMC5883P |
| 功能 | 3轴磁力计 |
| 接口 | I2C |
| 电源 | 3V3_IMU |

**引脚连接:**

| 引脚 | 连接 | 说明 |
|------|------|------|
| VDD | 3V3_IMU | 电源 |
| SCL | STMI2C_SCL1 (PB6) | I2C时钟 (共享) |
| SDA | STMI2C_SDA1 (PB7) | I2C数据 (共享) |
| GND | GND | 地 |

**I2C地址:**
- 固定地址: **0x0D** (写: 0x1A, 读: 0x1B)

**关键寄存器:**
- CHIP_ID (0x0D): 预期值 0xFF 或特定ID

---

### U17 - ICM-42688-P (6轴IMU)

| 属性 | 值 |
|------|-----|
| 型号 | ICM-42688-P |
| 功能 | 3轴陀螺仪 + 3轴加速度计 |
| 接口 | SPI/I2C (本项目使用SPI3) |
| 电源 | 3V3_IMU |
| 电源IO | 3V3_IMU |

**引脚连接:**

| 引脚 | 连接 | 说明 |
|------|------|------|
| VDD | 3V3_IMU | 核心电源 |
| VDDIO | 3V3_IMU | IO电源 |
| AP_CS | STM_SPI_NSS3 (PA15) | SPI片选 |
| AP_SCLK | STM_SPI_CLK3 (PB3) | SPI时钟 |
| AP_SDO/AP_AD0 | STM_SPI_MISO3 (PB4) | SPI数据入 |
| AP_SDI/AP_SDIO | STM_SPI_MOSI3 (PB5) | SPI数据出 |
| INT1/INT | IMU_INT1 | 中断1 |
| INT2/FSYNC | - | 中断2/帧同步 |
| GND | GND | 地 |

**SPI模式:**
- 模式: CPOL=0, CPHA=0 (Mode 0)
- 最大时钟: 24MHz

**关键寄存器:**
- WHO_AM_I (0x75): 预期值 **0x47**

---

### 电源芯片

#### U2 - XC6204B302MR (3V3_RF)

| 属性 | 值 |
|------|-----|
| 型号 | XC6204B302MR |
| 功能 | LDO稳压器 |
| 输入 | V_BAT |
| 输出 | 3V3_RF (3.0V) |
| 用途 | WiFi模块供电 |

**引脚:**
- VIN: 输入
- VSS: 地
- CE: 使能
- VOUT: 3V3_RF输出

#### U3 - XC6204B302MR (3V3_D)

| 属性 | 值 |
|------|-----|
| 输出 | 3V3_D (3.0V) |
| 用途 | STM32数字电路供电 |

#### U3 (第二个) - XC6204B302MR (3V3_IMU)

| 属性 | 值 |
|------|-----|
| 输出 | 3V3_IMU (3.0V) |
| 用途 | 传感器组供电 |

---

## 机身板 (SCH_机身_1-P1)

### U13 - STM32F103CBT6 (ST-Link调试器)

| 属性 | 值 |
|------|-----|
| 型号 | STM32F103CBT6 |
| 功能 | 板载ST-Link V2.1简化版 |
| 电源 | 3V3_LINK (来自USB) |

**与主控连接:**

| 信号 | 目标 | 说明 |
|------|------|------|
| TARGET_SWDIO | STM_SWDIO# | SWD数据 |
| TARGET_SCLK | STM_SWCLK# | SWD时钟 |
| TARGET_NRST | NRST# | 复位 |
| TARGET_TXD | STM_RXD1# | 调试UART发送 |
| TARGET_RXD | STM_TXD1# | 调试UART接收 |

### U11 - CH334R (USB HUB)

| 属性 | 值 |
|------|-----|
| 型号 | CH334R |
| 功能 | USB 2.0 HUB控制器 |
| 端口数 | 4口 |
| 上行 | USB-C (TYPE-C 16P) |

**连接关系:**
- DM1/DP1: 连接ST-Link (U13)
- 其他端口: 可用于外部USB设备

### U1 - TP4059 (充电管理)

| 属性 | 值 |
|------|-----|
| 型号 | TP4059 |
| 功能 | 单节锂电池线性充电器 |
| 充电电流 | 可通过R1编程 (默认1.6kΩ) |
| 输入 | V_USB (5V) |
| 输出 | V_BAT |

**状态指示:**
- CHARGE_STATE: 充电状态输出

### 电机驱动MOSFET

| 位号 | 型号 | 控制信号 | 驱动电机 |
|------|------|----------|----------|
| Q1 | Si2302CDS-T1-GE3 | MOTOR4# | MOTOR4 |
| Q2 | Si2302CDS-T1-GE3 | MOTOR1# | MOTOR1 |
| Q3 | Si2302CDS-T1-GE3 | MOTOR2# | MOTOR2 |
| Q4 | Si2302CDS-T1-GE3 | MOTOR3# | MOTOR3 |

**类型:** N沟道MOSFET
**驱动方式:** 低侧开关 (源极接地，漏极接电机负极)

### U14 - ME6212C33M5G (3V3_LINK)

| 属性 | 值 |
|------|-----|
| 型号 | ME6212C33M5G |
| 功能 | LDO稳压器 |
| 输出 | 3V3_LINK (3.3V) |
| 用途 | ST-Link供电 |

### Q9 - XR25P01M (电源控制)

| 属性 | 值 |
|------|-----|
| 型号 | XR25P01M |
| 功能 | 双P沟道MOSFET |
| 用途 | 电池/USB电源切换 |

---

## 连接器

### FPC1/FPC2 - AFC05-S16FIA-00 (16Pin FPC)

**信号定义:**

| 引脚 | 信号 | 方向 | 说明 |
|------|------|------|------|
| 1 | - | - | - |
| 2 | - | - | - |
| 3 | STM_TXD1# | 出 | UART1 TX (调试) |
| 4 | STM_RXD1# | 入 | UART1 RX (调试) |
| 5 | MOTOR4# | 出 | 电机4 PWM |
| 6 | MOTOR3# | 出 | 电机3 PWM |
| 7 | MOTOR2# | 出 | 电机2 PWM |
| 8 | MOTOR1# | 出 | 电机1 PWM |
| 9 | NRST# | 入 | 复位 |
| 10 | SCREW5 | - | 固定 |
| 11 | SCREW6 | - | 固定 |
| 12 | SCREW7 | - | 固定 |
| 13 | SCREW8 | - | 固定 |
| 14 | POWER# | - | 电源控制 |
| 15 | UD3_P# | 双向 | USB D+ |
| 16 | UD3_N# | 双向 | USB D- |
| 17 | STM_SWDIO# | 双向 | SWD数据 |
| 18 | STM_SWCLK# | 出 | SWD时钟 |

### CN8-CN11 - M1250R-02P (电机接口)

| 连接器 | 电机 |
|--------|------|
| CN8 | MOTOR1 |
| CN9 | MOTOR2 |
| CN10 | MOTOR3 |
| CN11 | MOTOR4 |

---

## 指示灯LED

| 位号 | 颜色 | 控制信号 | 位置 |
|------|------|----------|------|
| LED2 | 红 | LED_R | 机身 |
| LED3 | 绿 | LED_G | 主控 |
| LED4 | 白 | LED_WL | 主控 |
| LED5 | 黄 | - | 机身 (充电指示) |

---

## 快速参考表

### I2C设备地址汇总

| 设备 | 型号 | 地址 | 备注 |
|------|------|------|------|
| 气压计 | LPS22HBTR | 0x5C | SA0=GND |
| 磁力计 | QMC5883P | 0x0D | 固定地址 |

### SPI设备片选汇总

| 设备 | 型号 | NSS引脚 | 说明 |
|------|------|---------|------|
| IMU | ICM-42688-P | PA15 | SPI3 |

### UART端口汇总

| UART | TX | RX | 连接设备 |
|------|----|----|----------|
| UART1 | PA9 | PA10 | ST-Link (调试) |
| UART2 | PA2 | PA3 | ESP32-C3 (WiFi) |

### 电源网络汇总

| 网络 | 电压 | 来源 | 负载 |
|------|------|------|------|
| 3V3_D | 3.0V | U3 | STM32 |
| 3V3_IMU | 3.0V | U3 | 传感器 |
| 3V3_RF | 3.0V | U2 | WiFi模块 |
| 3V3_LINK | 3.3V | U14 | ST-Link |
| V_BAT | 3.7-4.2V | 电池 | 电机 |
| V_USB | 5V | USB | 充电 |
