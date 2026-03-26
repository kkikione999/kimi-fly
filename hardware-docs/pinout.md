# 无人机引脚真源 (Pinout SSOT)

> **用途**: 当前项目硬件引脚与板级连线唯一真源
> **范围**: 仅保留当前开发真正需要的引脚、外设、连线和电机映射
> **最后核对**: 2026-03-26
> **裁剪原则**: 已移除 `STM32F103/ST-Link`、USB Hub、充电芯片、连接器、LED 明细、DMA/中断软件配置、冗长修订附录

**重要约束**
- 机体坐标系: `+X = 前方`, `+Y = 左方`, `+Z = 上方`
- 电机真实位置与旋向:
  - `Motor1 / PA8 = FrontLeft / CCW`
  - `Motor2 / PA11 = RearLeft / CW`
  - `Motor3 / PB1 = RearRight / CCW`
  - `Motor4 / PB10 = FrontRight / CW`
- IMU 到机体系运行时映射:
  - `body +X = IMU +Y`
  - `body +Y = IMU -X`
  - `body +Z = IMU +Z`
- 上述结论以实机验证为准，不再从旧代码反推覆盖。

---

## 1. STM32F411CEU6 当前实际使用引脚

| 引脚 | 外设/模式 | 当前用途 | 连接对象 |
|------|-----------|----------|----------|
| PA2 | USART2_TX | WiFi 串口发送 | ESP32-C3 GPIO1 |
| PA3 | USART2_RX | WiFi 串口接收 | ESP32-C3 GPIO0 |
| PA8 | TIM1_CH1 | Motor1 PWM | 前左电机 |
| PA9 | USART1_TX | 调试串口 TX | PC / 调试器 |
| PA10 | USART1_RX | 调试串口 RX | PC / 调试器 |
| PA11 | TIM1_CH4 | Motor2 PWM | 后左电机 |
| PA13 | SWDIO | SWD 调试 | 调试接口 |
| PA14 | SWCLK | SWD 调试 | 调试接口 |
| PA15 | GPIO 输出 | 气压计 SPI 软件片选 | LPS22HBTR CS |
| PB0 | ADC1_IN8 | 电池电压检测 | 电池采样网络 |
| PB1 | TIM3_CH4 | Motor3 PWM | 后右电机 |
| PB3 | SPI3_SCK | 气压计 SPI 时钟 | LPS22HBTR |
| PB4 | SPI3_MISO | 气压计 SPI 读数据 | LPS22HBTR |
| PB5 | SPI3_MOSI | 气压计 SPI 写数据 | LPS22HBTR |
| PB6 | I2C1_SCL | 传感器 I2C 时钟 | ICM-42688-P / QMC5883P |
| PB7 | I2C1_SDA | 传感器 I2C 数据 | ICM-42688-P / QMC5883P |
| PB9 | GPIO / EXTI | 气压计数据就绪中断 | LPS22HBTR INT_DRDY |
| PB10 | TIM2_CH3 | Motor4 PWM | 前右电机 |
| PC13 | GPIO / EXTI | IMU 中断输入 | ICM-42688-P INT1 |
| NRST | 复位 | 主控复位 | 板级复位网络 / ESP32 RST |

---

## 2. 目标芯片连线

### 2.1 ICM-42688-P

| 芯片引脚 | 功能 | STM32 连接 | 说明 |
|----------|------|------------|------|
| SCL | I2C 时钟 | PB6 | I2C1_SCL |
| SDA | I2C 数据 | PB7 | I2C1_SDA |
| INT1 | 中断 | PC13 | IMU 数据/状态中断 |
| VDDIO | IO 电源 | 3V3_IMU | - |
| VDD | 电源 | 3V3_IMU | - |

- 接口: `I2C1`
- 7-bit 地址: `0x69` (`AD0 = VCC`)
- 运行时机体映射:
  - `body +X (前) = IMU +Y`
  - `body +Y (左) = IMU -X`
  - `body +Z (上) = IMU +Z`

### 2.2 LPS22HBTR

| 芯片引脚 | 功能 | STM32 连接 | 说明 |
|----------|------|------------|------|
| CS | 片选 | PA15 | 软件 NSS |
| SCL/SPC | SPI 时钟 | PB3 | SPI3_SCK |
| SDA/SDI | 数据输入 | PB5 | SPI3_MOSI |
| SDO | 数据输出 | PB4 | SPI3_MISO |
| INT_DRDY | 数据就绪中断 | PB9 | EXTI 输入 |
| VDD_IO | IO 电源 | 3V3_IMU | - |
| VDD | 电源 | 3V3_IMU | - |

- 接口: `SPI3`
- 模式: `SPI Mode 0`

### 2.3 QMC5883P

| 芯片引脚 | 功能 | STM32 连接 | 说明 |
|----------|------|------------|------|
| SCK | I2C 时钟 | PB6 | 与 IMU 共享 I2C1_SCL |
| SDA | I2C 数据 | PB7 | 与 IMU 共享 I2C1_SDA |
| VDD | 电源 | 3V3_IMU | - |

- 接口: `I2C1`
- 7-bit 地址: `0x2C`

### 2.4 ESP32-C3

| ESP32-C3 引脚 | 功能 | STM32 连接 | 说明 |
|---------------|------|------------|------|
| GPIO1 | UART 接收 | PA2 (USART2_TX) | STM32 -> ESP32 |
| GPIO0 | UART 发送 | PA3 (USART2_RX) | ESP32 -> STM32 |
| 3V3 | 电源 | 3V3_RF | 3.3V |
| GND | 地 | GND | - |
| EN | 使能 | 3V3_RF | 高电平使能 |
| RST | 复位 | NRST | 与 STM32 共享复位 |

- 接口: `USART2`
- 串口参数: `115200, 8N1`

### 2.5 电池电压检测

| 功能 | STM32 引脚 | 配置 |
|------|------------|------|
| 电池采样输入 | PB0 (`ADC1_IN8`) | ADC1 通道 8 |

- 用途: 电池电压检测 / 低压保护 / 遥测上报
- 这是板级真连接的一部分，必须保留

---

## 3. 调试接口

| 功能 | STM32 引脚 | 说明 |
|------|------------|------|
| SWDIO | PA13 | SWD 数据 |
| SWCLK | PA14 | SWD 时钟 |
| Debug TX | PA9 (`USART1_TX`) | 调试串口输出 |
| Debug RX | PA10 (`USART1_RX`) | 调试串口输入 |

- 调试串口: `USART1 @ 460800, 8N1`

---

## 4. 电机输出

| 电机 | 定时器 | 通道 | 引脚 | 真机位置 | 旋向 | PWM 频率 |
|------|--------|------|------|----------|------|----------|
| Motor1 | TIM1 | CH1 | PA8 | 前左 (`FrontLeft`) | CCW | 42kHz |
| Motor2 | TIM1 | CH4 | PA11 | 后左 (`RearLeft`) | CW | 42kHz |
| Motor3 | TIM3 | CH4 | PB1 | 后右 (`RearRight`) | CCW | 42kHz |
| Motor4 | TIM2 | CH3 | PB10 | 前右 (`FrontRight`) | CW | 42kHz |

- 以上位置与旋向已做实机单电机验证
- 后续飞控混控、姿态方向和保护逻辑都以此表为准

---

## 5. 快速参考

### 5.1 外设总览

| 功能 | 外设 | 引脚/设备 | 关键参数 |
|------|------|-----------|----------|
| IMU | I2C1 | PB6 / PB7 + PC13 | `0x69` |
| Magnetometer | I2C1 | PB6 / PB7 | `0x2C` |
| Barometer | SPI3 | PA15 + PB3 / PB4 / PB5 + PB9 | `Mode 0` |
| WiFi Bridge | USART2 | PA2 / PA3 | `115200, 8N1` |
| Debug UART | USART1 | PA9 / PA10 | `460800, 8N1` |
| Battery ADC | ADC1 | PB0 (`IN8`) | 电池采样 |
| Motors | TIM1 / TIM2 / TIM3 | PA8 / PA11 / PB1 / PB10 | `42kHz` |

### 5.2 I2C1 设备

| 设备 | 地址 | 备注 |
|------|------|------|
| ICM-42688-P | `0x69` | AD0 拉高 |
| QMC5883P | `0x2C` | 固定使用 |

---

*文档定位: 当前项目引脚与连线唯一真源*
