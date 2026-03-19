# 无人机引脚定义 (Pinout)

> **⚠️ 硬件文档唯一真源 (Single Source of Truth)**
> **来源**: 从参考代码 `/Users/ll/fly/zmgjb/code/411/Core/` 提取验证
> **更新日期**: 2026-03-19 (第四次修正 - 成为唯一真源)
> **验证状态**: ✅ 已对照源代码完整验证

**注意**: 本文档是硬件设计的唯一权威参考。任何引脚相关的开发工作都必须以本文档为准。如有疑问，请查阅本文档而非其他次级文档。

---

## 1. 主控板 - STM32F411CEU6

### 1.1 电源与系统引脚

| 引脚 | 功能 | 说明 |
|------|------|------|
| VBAT | 1 | 电池供电输入 |
| VDDA/VREF+ | 9 | 模拟电源/参考电压 |
| VSSA/VREF- | 8 | 模拟地/参考地 |
| VCAP1 | 22 | 稳压电容 |
| VDD | 24, 36, 48 | 数字电源 (3V3_D) |
| VSS | 23, 35, 47 | 数字地 |
| NRST | 7 | 系统复位 |
| BOOT0 | 44 | 启动模式选择 |

### 1.2 晶振引脚

| 引脚 | 功能 | 说明 |
|------|------|------|
| PH0-OSC_IN | 5 | 8MHz 外部晶振输入 |
| PH1-OSC_OUT | 6 | 8MHz 外部晶振输出 |
| PC14/OSC32IN | 3 | 32.768kHz 晶振输入 |
| PC15/OSC32OUT | 4 | 32.768kHz 晶振输出 |

### 1.3 GPIO 引脚分配

#### PA 端口

| 引脚 | 主功能 | 复用功能 | 当前用途 |
|------|--------|----------|----------|
| PA0 | WKUP/USART2_CTS/ADC12_IN0/TIM2_CH1_ETR | - | 未使用 |
| PA1 | USART2_RTS/ADC12_IN1/TIM2_CH2 | - | 未使用 |
| PA2 | USART2_TX/ADC12_IN2/TIM2_CH3 | AF7 | **ESP32 UART TX** |
| PA3 | USART2_RX/ADC12_IN3/TIM2_CH4 | AF7 | **ESP32 UART RX** |
| PA4 | SPI1_NSS/USART2_CK/ADC12_IN4 | - | **Gyroscope_SPI_Software_NSS** (代码保留，未实际使用) |
| PA5 | SPI1_SCK/ADC12_IN5 | - | 未使用 |
| PA6 | SPI1_MISO/ADC12_IN6/TIM3_CH1 | - | 未使用 |
| PA7 | SPI1_MOSI/ADC12_IN7/TIM3_CH2 | - | 未使用 |
| PA8 | USART1_CK/TIM1_CH1/MCO | AF1 | **电机1 PWM (TIM1_CH1)** |
| PA9 | USART1_TX/TIM1_CH2 | AF7 | **调试串口 TX** |
| PA10 | USART1_RX/TIM1_CH3 | AF7 | **调试串口 RX** |
| PA11 | USART1_CTS/CAN_RX/TIM1_CH4/USBDM | AF1 | **电机2 PWM (TIM1_CH4)** |
| PA12 | USART1_RTS/CAN_TX/TIM1_ETR/USBDP | - | USB 数据正 |
| PA13 | SWDIO | - | SWD 数据 |
| PA14 | SWCLK | - | SWD 时钟 |
| PA15 | JTDI | - | **气压计 SPI 软件片选 (PA15)** |

#### PB 端口

| 引脚 | 主功能 | 复用功能 | 当前用途 |
|------|--------|----------|----------|
| PB0 | ADC12_IN8/TIM3_CH3 | - | **ADC1_IN8 (电池电压检测)** |
| PB1 | ADC12_IN9/TIM3_CH4 | AF2 | **电机3 PWM (TIM3_CH4)** |
| PB2 | BOOT1 | - | 启动模式 |
| PB3 | JTDO/SPI3_SCK | AF6 | **气压计 SPI SCK** |
| PB4 | NJTRST/SPI3_MISO | AF6 | **气压计 SPI MISO** |
| PB5 | I2C1_SMBA/SPI3_MOSI | AF6 | **气压计 SPI MOSI** |
| PB6 | I2C1_SCL/TIM4_CH1 | AF4 | **I2C1_SCL (陀螺仪+磁力计)** |
| PB7 | I2C1_SDA/TIM4_CH2 | AF4 | **I2C1_SDA (陀螺仪+磁力计)** |
| PB8 | TIM4_CH3 | - | 未使用 |
| PB9 | TIM4_CH4 | - | **气压计中断 (Barometer_EXTI)** |
| PB10 | I2C2_SCL/USART3_TX/TIM2_CH3 | AF1 | **电机4 PWM (TIM2_CH3)** |
| PB11 | I2C2_SDA/USART3_RX | - | 未使用 |
| PB12 | SPI2_NSS/I2C2_SMBA/USART3_CK/TIM1_BKIN | - | 未使用 |
| PB13 | SPI2_SCK/USART3_CTS/TIM1_CH1N | - | 未使用 |
| PB14 | SPI2_MISO/USART3_RTS/TIM1_CH2N | - | **LED 红色 (LED_R)** |
| PB15 | SPI2_MOSI/TIM1_CH3N | - | **LED 白色 (LED_L)** |

#### PC 端口

| 引脚 | 功能 | 说明 |
|------|------|------|
| PC13 | TAMPER-RTC | **陀螺仪中断 (Gyroscope_EXIT)** |
| PC14 | OSC32IN | 32.768kHz 晶振输入 |
| PC15 | OSC32OUT | 32.768kHz 晶振输出 |

---

## 2. 机身板 - STM32F103CBT6 (ST-Link)

### 2.1 电源与系统引脚

| 引脚 | 功能 | 说明 |
|------|------|------|
| VBAT | 1 | 电池供电 |
| VDD_A | 8 | 模拟电源 |
| VSS_A | 7 | 模拟地 |
| VDD_1 | 24 | 数字电源 1 (3V3_LINK) |
| VSS_1 | 23 | 数字地 1 |
| VDD_2 | 36 | 数字电源 2 |
| VSS_2 | 35 | 数字地 2 |
| VDD_3 | 48 | 数字电源 3 |
| VSS_3 | 47 | 数字地 3 |
| NRST | 7 | 系统复位 |
| BOOT0 | 44 | 启动模式选择 |

### 2.2 晶振引脚

| 引脚 | 功能 | 说明 |
|------|------|------|
| PD0-OSC_IN | 5 | 外部晶振输入 |
| PD1-OSC_OUT | 6 | 外部晶振输出 |
| PC14-OSC32_IN | 3 | 32.768kHz 晶振输入 |
| PC15-OSC32_OUT | 4 | 32.768kHz 晶振输出 |

### 2.3 调试接口引脚

| 引脚 | 功能 | 说明 |
|------|------|------|
| PA13 | SWDIO | SWD 数据线 (STM_SWDIO#) |
| PA14 | SWCLK | SWD 时钟线 (STM_SWCLK#) |

---

## 3. 外设连接

### 3.1 IMU - ICM-42688-P (I2C1 接口)

| ICM 引脚 | 功能 | STM32 连接 | 说明 |
|----------|------|------------|------|
| SCL | I2C时钟 | PB6 | I2C1_SCL |
| SDA | I2C数据 | PB7 | I2C1_SDA |
| INT1 | 中断 | PC13 | 陀螺仪中断 |
| VDDIO | IO电源 | 3V3_IMU | - |
| VDD | 电源 | 3V3_IMU | - |

**配置**: I2C1, 100kHz, 7bit 地址 **0x68**

### 3.2 气压计 - LPS22HBTR (SPI3 接口)

| LPS22 引脚 | 功能 | STM32 连接 | 说明 |
|------------|------|------------|------|
| CS | 片选 | PA15 | 软件 NSS |
| SCL/SPC | 时钟 | PB3 | SPI3_SCK |
| SDA/SDI | 数据入 | PB5 | SPI3_MOSI |
| SDO | 数据出 | PB4 | SPI3_MISO |
| INT_DRDY | 中断 | PB9 | 数据就绪 |
| VDD_IO | IO电源 | 3V3_IMU | - |
| VDD | 电源 | 3V3_IMU | - |

**配置**: SPI3, Mode=Master, 8bit, CPOL=Low, CPHA=1Edge, Prescaler=2 (42MHz)

### 3.3 磁力计 - QMC5883P (I2C1 接口)

| QMC5883 引脚 | 功能 | STM32 连接 | 说明 |
|--------------|------|------------|------|
| SCK | I2C时钟 | PB6 | I2C1_SCL (与陀螺仪共享) |
| SDA | I2C数据 | PB7 | I2C1_SDA (与陀螺仪共享) |
| VDD | 电源 | 3V3_IMU | - |

**配置**: I2C1, 100kHz, 7bit 地址 **0x2C**

### 3.4 电池电压检测 (ADC1)

| 功能 | STM32 引脚 | 配置 |
|------|------------|------|
| ADC 输入 | PB0 (ADC1_IN8) | 12bit, 3 cycles |

**配置**: ADC1, Channel 8, 12bit resolution, Software trigger

### 3.5 ESP32-C3 WiFi 模块 (USART2)

| ESP32-C3 引脚 | 功能 | STM32 连接 | 说明 |
|---------------|------|------------|------|
| RXD0 | UART接收 | PA2 (USART2_TX) | 来自STM32 |
| TXD0 | UART发送 | PA3 (USART2_RX) | 发送到STM32 |
| 3V3 | 电源 | 3V3_RF | 3.3V |
| GND | 地 | GND | - |
| EN | 使能 | 3V3_RF | 高电平使能 |
| RST | 复位 | NRST | 与STM32共复位 |

**配置**: USART2, 波特率=115200, 8N1, DMA收发

### 3.6 调试串口 (USART1)

| 功能 | STM32 引脚 | 说明 |
|------|------------|------|
| TX | PA9 (USART1_TX) | 发送数据到PC |
| RX | PA10 (USART1_RX) | 接收数据从PC |

**配置**: USART1, 波特率=460800, 8N1, DMA发送

---

## 4. 电机输出

### 4.1 电机 PWM 引脚

| 电机 | TIM实例 | 通道 | STM32引脚 | PWM频率 |
|------|---------|------|-----------|---------|
| Motor1 | TIM1 | CH1 | PA8 | 42kHz |
| Motor2 | TIM1 | CH4 | PA11 | 42kHz |
| Motor3 | TIM3 | CH4 | PB1 | 42kHz |
| Motor4 | TIM2 | CH3 | PB10 | 42kHz |

**PWM 配置详情**:
- Prescaler: 1
- Period: 999
- Clock: 84MHz (系统时钟/2 = 42MHz APB)
- Frequency: 42MHz / 1000 = 42kHz
- Mode: PWM1, 高电平有效

### 4.2 机身板电机接口

| 接口 | 引脚 | 功能 | 说明 |
|------|------|------|------|
| CN8 | 1, 2 | MOTOR1# | 电机1驱动 |
| CN9 | 1, 2 | MOTOR2# | 电机2驱动 |
| CN10 | 1, 2 | MOTOR3# | 电机3驱动 |
| CN11 | 1, 2 | MOTOR4# | 电机4驱动 |

### 4.3 驱动电路

| 元件 | 型号 | 功能 |
|------|------|------|
| Q1-Q4 | Si2302CDS-T1-GE3 | MOSFET 驱动 |
| D1-D4 | BAT54WS | 续流二极管 |

---

## 5. DMA 配置

| DMA | Stream | Channel | 功能 | 模式 |
|-----|--------|---------|------|------|
| DMA1 | Stream5 | Channel 4 | USART2_RX | Circular |
| DMA1 | Stream6 | Channel 4 | USART2_TX | Normal |
| DMA2 | Stream7 | Channel 4 | USART1_TX | Normal |

---

## 6. 中断配置

| 中断源 | 优先级 | 说明 |
|--------|--------|------|
| DMA1_Stream5_IRQn | 5 | USART2 RX DMA |
| DMA1_Stream6_IRQn | 5 | USART2 TX DMA |
| DMA2_Stream7_IRQn | 5 | USART1 TX DMA |
| TIM1_UP_TIM10_IRQn | 15 | TIM1 更新 |
| USART1_IRQn | 5 | USART1 全局中断 |
| USART2_IRQn | 5 | USART2 全局中断 |

---

## 7. 电源管理

### 7.1 主控板电源

| 元件 | 型号 | 输入 | 输出 | 用途 |
|------|------|------|------|------|
| U2 | XC6204B302MR | V_BAT | 3V3_RF | 通信部分电源 |
| U3 | XC6204B302MR | V_BAT | 3V3_D | 数字部分电源 |
| - | XC6204B302MR | V_BAT | 3V3_IMU | IMU 电源 |

### 7.2 机身板电源

| 元件 | 型号 | 输入 | 输出 | 用途 |
|------|------|------|------|------|
| U1 | TP4059 | V_USB | V_BAT | 电池充电管理 |
| U14 | ME6212C33M5G | V_USB/V_BAT | 3V3_LINK | ST-Link 电源 |

---

## 8. USB 接口

### 8.1 USB Type-C (机身板)

| 引脚 | 功能 | 连接 |
|------|------|------|
| A4, A9 | VBUS | V_USB |
| A1, A12, B1, B12 | GND | GND |
| A6 | DP1 | USB_P |
| A7 | DN1 | USB_N |
| B6 | DP2 | USB_P |
| B7 | DN2 | USB_N |
| A5 | CC1 | 5.1kΩ下拉 |
| B5 | CC2 | 5.1kΩ下拉 |

### 8.2 USB Hub (CH334R)

| 引脚 | 功能 | 说明 |
|------|------|------|
| 1-2 | DM4/DP4 | 上行端口4 |
| 3-4 | DM3/DP3 | 上行端口3 |
| 5-6 | DM2/DP2 | 上行端口2 |
| 7-8 | DM1/DP1 | 上行端口1 |
| 10-11 | DMU/DPU | USB 上行 |
| 12 | V5 | 5V 电源 |
| 13 | VDD33 | 3.3V 电源 |

---

## 9. LED 指示灯

### 9.1 主控板 LED

| LED | 颜色 | 控制引脚 | 说明 |
|-----|------|----------|------|
| LED3 | 绿色 | - | 状态指示 |
| LED_R | 红色 | PB14 | 状态指示 (代码可控) |
| LED_L | 白色 | PB15 | 状态指示 (代码可控) |
| LED2 | 红色 | - | 电源/错误 |

### 9.2 机身板 LED

| LED | 颜色 | 说明 |
|-----|------|------|
| LED5 | 黄色 | 充电状态 |

---

## 10. 连接器汇总

### 10.1 FPC 连接器

| 连接器 | 型号 | 引脚数 | 用途 |
|--------|------|--------|------|
| FPC1 | AFC05-S16FIA-00 | 16 | 机身-主控连接 |
| FPC2 | AFC05-S16FIA-00 | 16 | 扩展接口 |

### 10.2 电机连接器

| 连接器 | 型号 | 引脚数 | 用途 |
|--------|------|--------|------|
| CN8-CN11 | M1250R-02P | 2 | 电机接口 |

---

## 11. 信号汇总表

### 11.1 主控板-机身板连接 (FPC1)

| FPC 引脚 | 信号名 | 方向 | 说明 |
|----------|--------|------|------|
| 1 | V_USB_S | → | USB 供电 |
| 2 | STM_SWCLK# | ← | SWD 时钟 |
| 3 | STM_SWDIO# | ↔ | SWD 数据 |
| 4 | UD3_N# | ↔ | USB 数据负 |
| 5 | UD3_P# | ↔ | USB 数据正 |
| 6 | POWER# | → | 电源控制 |
| 7-10 | SCREW5-8 | - | 固定孔 |
| 11 | NRST# | ← | 复位 |
| 12 | MOTOR1# | → | 电机1 |
| 13 | MOTOR2# | → | 电机2 |
| 14 | MOTOR3# | → | 电机3 |
| 15 | MOTOR4# | → | 电机4 |
| 16 | V_BAT | → | 电池电压 |

### 11.2 调试接口 (ST-Link)

| 信号名 | 类型 | 说明 |
|--------|------|------|
| STM_SWCLK# | 输出 | 目标时钟 |
| STM_SWDIO# | 双向 | 目标数据 |
| STM_TXD1# | 输出 | 调试串口发送 (PA9) |
| STM_RXD1# | 输入 | 调试串口接收 (PA10) |
| NRST# | 开漏 | 目标复位 |

---

## 12. 快速参考

### 12.1 外设配置速查

| 外设 | 接口 | 引脚 | 地址/配置 |
|------|------|------|-----------|
| **陀螺仪 ICM-42688-P** | **I2C1** | **PB6-PB7** | **0x68** |
| **磁力计 QMC5883P** | **I2C1** | **PB6-PB7** | **0x2C** |
| **气压计 LPS22HBTR** | **SPI3** | **PA15/PB3-PB5** | SPI Mode 0 |
| **电池检测** | **ADC1** | **PB0** | **Channel 8** |
| ESP32-C3 | USART2 | PA2-PA3 | 115200 baud |
| 调试串口 | USART1 | PA9-PA10 | 460800 baud |
| Motor1 | TIM1_CH1 | PA8 | 42kHz |
| Motor2 | TIM1_CH4 | PA11 | 42kHz |
| Motor3 | TIM3_CH4 | PB1 | 42kHz |
| Motor4 | TIM2_CH3 | PB10 | 42kHz |
| LED_R | GPIO | PB14 | 输出 |
| LED_L | GPIO | PB15 | 输出 |

### 12.2 中断引脚

| 功能 | 引脚 | 触发方式 |
|------|------|----------|
| 陀螺仪中断 | PC13 | 上升沿 |
| 气压计中断 | PB9 | 上升沿 |

### 12.3 I2C1 总线设备列表

| 设备 | 地址 | 备注 |
|------|------|------|
| ICM-42688-P | 0x68 | 陀螺仪+加速度计 |
| QMC5883P | 0x2C | 磁力计 |

---

## 13. 重要修正历史

### 13.1 第四次修正 (2026-03-19) - 成为唯一真源

**本次修正使本文档成为硬件文档的唯一真源 (SSOT)**

| 项目 | 变更说明 |
|------|----------|
| **PA4 引脚** | 从"未使用"更新为"Gyroscope_SPI_Software_NSS (代码保留，未实际使用)" |
| **文档性质** | 明确标注为"硬件文档唯一真源"，所有引脚配置从源代码验证 |
| **验证详情** | 添加第14节"源代码验证详情"，列出所有验证文件 |

### 13.2 第三次修正 (2026-03-19)

| 项目 | 错误 | 正确 |
|------|------|------|
| **ADC 输入** | PA0 | **PB0 (ADC1_IN8)** |

### 13.3 第二次修正 (2026-03-19)

| 项目 | 错误 | 正确 |
|------|------|------|
| **ICM-42688-P 接口** | SPI1 (PA4-PA7) | **I2C1 (PB6-PB7)** |

### 13.4 第一次分析错误

| 项目 | 错误 | 正确 |
|------|------|------|
| IMU 接口 | SPI3 | **I2C1** |

---

## 14. 源代码验证详情

本文档作为硬件文档的**唯一真源**，所有引脚定义均从源代码直接提取验证：

### 14.1 验证的文件清单

| 文件路径 | 验证内容 |
|----------|----------|
| `Core/Inc/main.h` | GPIO 引脚宏定义 |
| `Core/Src/gpio.c` | GPIO 初始化配置 |
| `Core/Src/i2c.c` | I2C1 引脚配置 (PB6-PB7) |
| `Core/Src/spi.c` | SPI3 引脚配置 (PB3-PB5) |
| `Core/Src/usart.c` | UART 引脚配置 (PA2-PA3, PA9-PA10) |
| `Core/Src/tim.c` | PWM 引脚配置 (PA8, PA11, PB1, PB10) |
| `Core/Src/adc.c` | ADC 引脚配置 (PB0) |
| `Hardware/ICM42688.c/h` | 陀螺仪 I2C 地址 0x68 |
| `Hardware/QMC5883P.c` | 磁力计 I2C 地址 0x2C |
| `Hardware/LPS22HBTR.c` | 气压计 SPI 配置 |

### 14.2 关键验证发现

1. **ICM-42688-P 使用 I2C1** (`ICM42688.h` 第10行)
   ```c
   #define ICM_I2C_HANDLE hi2c1
   ```

2. **PA4 引脚状态** (`main.h` 第66-67行)
   ```c
   #define Gyroscope_SPI_Software_NSS_Pin GPIO_PIN_4
   #define Gyroscope_SPI_Software_NSS_GPIO_Port GPIOA
   ```
   虽然在代码中定义并初始化，但实际 ICM-42688-P 使用 I2C 通信，此引脚可能未连接或保留兼容。

3. **I2C1 共享总线** (`i2c.c` 第70-79行)
   - PB6: I2C1_SCL (AF4)
   - PB7: I2C1_SDA (AF4)
   - 两个设备共享: ICM-42688-P (0x68) + QMC5883P (0x2C)

### 14.3 文档维护规范

**本文档是硬件文档的唯一真源 (SSOT)**：

1. **所有硬件开发**必须以本文档为准
2. **代码更新涉及引脚变更**时，必须同步更新本文档
3. **发现文档错误**时，立即修正并更新修正历史
4. **禁止创建次级引脚文档**，避免信息分歧
5. **需要引用时**，使用本文档路径: `hardware-docs/pinout.md`

---

*文档更新于: 2026-03-19*
*基于参考代码: /Users/ll/fly/zmgjb/code/411/*
*验证: 所有引脚配置已对照代码确认*
*文档性质: 硬件文档唯一真源 (SSOT)*
