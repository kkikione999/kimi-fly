# 无人机引脚定义 (Pinout)

> 从电路图提取的引脚定义文档
> 源文件: SCH_机身_1-P1_2026-03-11.pdf, SCH_主控_1-P1_2026-03-11.pdf

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
| PA0 | WKUP/USART2_CTS/ADC12_IN0/TIM2_CH1_ETR | - | 电池电压采样 (V_BAT) |
| PA1 | USART2_RTS/ADC12_IN1/TIM2_CH2 | - | - |
| PA2 | USART2_TX/ADC12_IN2/TIM2_CH3 | - | - |
| PA3 | USART2_RX/ADC12_IN3/TIM2_CH4 | - | - |
| PA4 | SPI1_NSS/USART2_CK/ADC12_IN4 | - | SPI1_NSS |
| PA5 | SPI1_SCK/ADC12_IN5 | - | SPI1_SCK |
| PA6 | SPI1_MISO/ADC12_IN6/TIM3_CH1 | - | SPI1_MISO |
| PA7 | SPI1_MOSI/ADC12_IN7/TIM3_CH2 | - | SPI1_MOSI |
| PA8 | USART1_CK/TIM1_CH1/MCO | - | - |
| PA9 | USART1_TX/TIM1_CH2 | - | - |
| PA10 | USART1_RX/TIM1_CH3 | - | - |
| PA11 | USART1_CTS/CAN_RX/TIM1_CH4/USBDM | USB_DM | USB 数据负 |
| PA12 | USART1_RTS/CAN_TX/TIM1_ETR/USBDP | USB_DP | USB 数据正 |
| PA13 | SWDIO | - | SWD 数据 |
| PA14 | SWCLK | - | SWD 时钟 |
| PA15 | JTDI | - | - |

#### PB 端口

| 引脚 | 主功能 | 复用功能 | 当前用途 |
|------|--------|----------|----------|
| PB0 | ADC12_IN8/TIM3_CH3 | - | - |
| PB1 | ADC12_IN9/TIM3_CH4 | - | - |
| PB2 | BOOT1 | - | - |
| PB3 | JTDO | - | - |
| PB4 | NJTRST | - | - |
| PB5 | I2C1_SMBA | - | - |
| PB6 | I2C1_SCL/TIM4_CH1 | - | I2C1_SCL (STMI2C_SCL1) |
| PB7 | I2C1_SDA/TIM4_CH2 | - | I2C1_SDA (STMI2C_SDA1) |
| PB8 | TIM4_CH3 | - | - |
| PB9 | TIM4_CH4 | - | - |
| PB10 | I2C2_SCL/USART3_TX | - | - |
| PB12 | SPI2_NSS/I2C2_SMBA/USART3_CK/TIM1_BKIN | - | - |
| PB13 | SPI2_SCK/USART3_CTS/TIM1_CH1N | - | - |
| PB14 | SPI2_MISO/USART3_RTS/TIM1_CH2N | - | - |
| PB15 | SPI2_MOSI/TIM1_CH3N | - | - |

#### PC 端口

| 引脚 | 功能 | 说明 |
|------|------|------|
| PC13 | TAMPER-RTC | RTC/唤醒 |
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

### 3.1 IMU - ICM-42688-P (SPI 接口)

| ICM 引脚 | 功能 | STM32 连接 | 说明 |
|----------|------|------------|------|
| AP_SDO/AP_AD0 | 1 | - | SPI 数据出/地址位0 |
| INT1/INT | 4 | - | 中断1 (IMU_INT1) |
| VDDIO | 5 | 3V3_IMU | IO 电源 |
| VDD | 8 | 3V3_IMU | 电源 |
| INT2/FSYNC/CLKIN | 9 | - | 中断2/同步 |
| AP_CS | 12 | STM_SPI_NSS3 | SPI 片选 |
| AP_SCL/AP_SCLK | 13 | STM_SPI_CLK3 | SPI 时钟 |
| AP_SDA/AP_SDIO/AP_SDI | 14 | STM_SPI_MOSI3 | SPI 数据入 |
| SDO/SA0 | 5 | STM_SPI_MISO3 | SPI 数据出/地址 |

### 3.2 气压计 - LPS22HBTR (I2C 接口)

| LPS22 引脚 | 功能 | STM32 连接 | 说明 |
|------------|------|------------|------|
| VDD_IO | 1 | 3V3_IMU | IO 电源 |
| SCL/SPC | 2 | STMI2C_SCL1 | I2C 时钟 |
| SDA/SDI/SDO | 4 | STMI2C_SDA1 | I2C 数据 |
| SDO/SA0 | 5 | - | 地址选择 |
| CS | 6 | 3V3_IMU | 片选 (I2C 模式拉高) |
| INT_DRDY | 7 | BARO_INT | 中断/数据就绪 |
| VDD | 10 | 3V3_IMU | 电源 |

### 3.3 磁力计 - QMC5883P (I2C 接口)

| QMC5883 引脚 | 功能 | STM32 连接 | 说明 |
|--------------|------|------------|------|
| VDD | 2 | 3V3_IMU | 电源 |
| SCK | 1 | STMI2C_SCL1 | I2C 时钟 |
| SDA | 16 | STMI2C_SDA1 | I2C 数据 |

### 3.4 ESP8266 WiFi 模块

| ESP 引脚 | 功能 | 连接 | 说明 |
|----------|------|------|------|
| 3V3 | 4 | 3V3_RF | 电源 |
| GND | 3, 2, 12, 15 | GND | 地 |
| EN | 9 | 3V3_RF | 使能 |
| IO0 | 13 | BOOT_IO9 | 启动模式/下载 |
| IO1 | 14 | - | TXD0 |
| IO2 | 6 | - | - |
| IO3 | 7 | - | RXD0 |
| IO4 | 19 | - | - |
| IO5 | 20 | - | - |
| IO6 | 21 | - | - |
| IO7 | 22 | - | - |
| IO8 | 23 | - | - |
| IO9 | 24 | BOOT_IO9 | 启动模式 |
| IO10 | 17 | STM_SPI_NSS1 | SPI 片选 |
| IO18 | 27 | STM_SPI_CLK1 | SPI 时钟 |
| IO19 | 28 | STM_SPI_MOSI1 | SPI MOSI |
| RXD0 | 30 | STM_TXD2 | UART 接收 |
| TXD0 | 31 | STM_RXD2 | UART 发送 |
| RST | - | NRST | 复位 |

---

## 4. 电机输出

### 4.1 机身板电机接口

| 接口 | 引脚 | 功能 | 说明 |
|------|------|------|------|
| CN8 | 1, 2 | MOTOR1# | 电机1驱动 |
| CN9 | 1, 2 | MOTOR2# | 电机2驱动 |
| CN10 | 1, 2 | MOTOR3# | 电机3驱动 |
| CN11 | 1, 2 | MOTOR4# | 电机4驱动 |

### 4.2 驱动电路

| 元件 | 型号 | 功能 |
|------|------|------|
| Q1-Q4 | Si2302CDS-T1-GE3 | MOSFET 驱动 |
| D1-D4 | BAT54WS | 续流二极管 |

---

## 5. 电源管理

### 5.1 主控板电源

| 元件 | 型号 | 输入 | 输出 | 用途 |
|------|------|------|------|------|
| U2 | XC6204B302MR | V_BAT | 3V3_RF | 通信部分电源 |
| U3 | XC6204B302MR | V_BAT | 3V3_D | 数字部分电源 |
| - | XC6204B302MR | V_BAT | 3V3_IMU | IMU 电源 |

### 5.2 机身板电源

| 元件 | 型号 | 输入 | 输出 | 用途 |
|------|------|------|------|------|
| U1 | TP4059 | V_USB | V_BAT | 电池充电管理 |
| U14 | ME6212C33M5G | V_USB/V_BAT | 3V3_LINK | ST-Link 电源 |

---

## 6. USB 接口

### 6.1 USB Type-C (机身板)

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

### 6.2 USB Hub (CH334R)

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

## 7. LED 指示灯

### 7.1 主控板 LED

| LED | 颜色 | 控制引脚 | 说明 |
|-----|------|----------|------|
| LED3 | 绿色 (XL-3210UGC) | LED_G | 状态指示 |
| LED4 | 白色 (XL-3210UWC) | LED_WL/LED_WR | 状态指示 |
| LED2 | 红色 (XL-3210SURC) | - | 电源/错误 |

### 7.2 机身板 LED

| LED | 颜色 | 说明 |
|-----|------|------|
| LED5 | 黄色 (NCD0603Y2) | 充电状态 |

---

## 8. 连接器汇总

### 8.1 FPC 连接器

| 连接器 | 型号 | 引脚数 | 用途 |
|--------|------|--------|------|
| FPC1 | AFC05-S16FIA-00 | 16 | 机身-主控连接 |
| FPC2 | AFC05-S16FIA-00 | 16 | 扩展接口 |

### 8.2 电机连接器

| 连接器 | 型号 | 引脚数 | 用途 |
|--------|------|--------|------|
| CN8-CN11 | M1250R-02P | 2 | 电机接口 |

---

## 9. 信号汇总表

### 9.1 主控板-机身板连接 (FPC1)

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

### 9.2 调试接口 (ST-Link)

| 信号名 | 类型 | 说明 |
|--------|------|------|
| STM_SWCLK# | 输出 | 目标时钟 |
| STM_SWDIO# | 双向 | 目标数据 |
| STM_TXD1# | 输出 | 调试串口发送 |
| STM_RXD1# | 输入 | 调试串口接收 |
| NRST# | 开漏 | 目标复位 |

---

*文档生成时间: 2026-03-17*
