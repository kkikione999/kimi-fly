# 元器件数据手册索引

> **创建日期**: 2026-03-18
> **用途**: 软件开发参考用元器件数据手册

---

## 已下载数据手册

### 主控芯片

| 元器件 | 文件 | 大小 | 说明 |
|--------|------|------|------|
| STM32F411CEU6 | `STM32F411CEU6_datasheet.pdf` | 2.1MB | 主控MCU数据手册 |
| STM32F103CBT6 | `STM32F103CBT6_datasheet.pdf` | 1.9MB | ST-Link调试器MCU |

### 通信模块

| 元器件 | 文件 | 大小 | 说明 |
|--------|------|------|------|
| ESP32-C3 | `ESP32-C3_datasheet.pdf` | 66KB | WiFi模块规格书 |
| ESP32-C3 AT | `ESP32-C3_AT_commands.pdf` | 8KB | AT指令集手册 |

### 传感器

| 元器件 | 文件 | 大小 | 说明 |
|--------|------|------|------|
| LPS22HBTR | `LPS22HBTR_datasheet.pdf` | 1.5MB | 气压计数据手册 |
| QMC5883P | `QMC5883P_datasheet.pdf` | 1.1MB | 磁力计数据手册 |

### 电源管理

| 元器件 | 文件 | 大小 | 说明 |
|--------|------|------|------|
| TP4059 | `TP4059_datasheet.pdf` | 687KB | 锂电池充电管理 |
| XC6204 | `XC6204_datasheet.pdf` | 5.0MB | LDO稳压器 (3.0V) |

### 外设/接口

| 元器件 | 文件 | 大小 | 说明 |
|--------|------|------|------|
| CH334R | `CH334R_datasheet.pdf` | 5KB | USB HUB控制器 |
| Si2302CDS | `Si2302CDS_datasheet.pdf` | 719KB | 电机驱动MOSFET |
| XR25P01M | `XR25P01M_datasheet.pdf` | 564KB | 电源控制双MOSFET |

### 电路图

| 文档 | 文件 | 大小 | 说明 |
|------|------|------|------|
| 机身电路 | `SCH_机身_1-P1_2026-03-11.pdf` | 215KB | 电机驱动、充电、USB-HUB |
| 主控电路 | `SCH_主控_1-P1_2026-03-11.pdf` | 246KB | MCU、传感器、WiFi |

---

## 需外部获取的数据手册

以下手册无法直接下载，请通过官方链接获取：

### IMU传感器

**ICM-42688-P** (6轴姿态传感器)
- 官方下载: https://invensense.tdk.com/products/motion-tracking/6-axis/icm-42688-p/
- 或搜索: "ICM-42688-P datasheet TDK"
- 关键信息:
  - WHO_AM_I寄存器(0x75): 0x47
  - SPI模式: CPOL=0, CPHA=0
  - 最大SPI时钟: 24MHz
  - 支持I2C/SPI接口

### 电源管理

**ME6212C33M5G** (LDO稳压器)
- 官方下载: https://www.micro-one.com.cn/ (南京微盟)
- 或搜索: "ME6212 datasheet 南京微盟"
- 关键信息:
  - 输入电压: 2.0V~6.0V
  - 输出电压: 3.3V (ME6212C33)
  - 最大输出电流: 300mA
  - 压差: 250mV@200mA

---

## 官方文档链接汇总

### ST Microelectronics
- STM32F411: https://www.st.com/en/microcontrollers-microprocessors/stm32f411.html
- STM32F103: https://www.st.com/en/microcontrollers-microprocessors/stm32f103.html
- LPS22HB: https://www.st.com/en/mems-and-sensors/lps22hb.html

### Espressif
- ESP32-C3: https://www.espressif.com/en/products/socs/esp32-c3
- ESP-AT指南: https://docs.espressif.com/projects/esp-at/en/latest/esp32c3/index.html

### TDK InvenSense
- ICM-42688-P: https://invensense.tdk.com/products/motion-tracking/6-axis/icm-42688-p/

### QST (矽睿科技)
- QMC5883P: http://www.qstcorp.com/

### WCH (沁恒微)
- CH334R: https://www.wch-ic.com/products/CH334.html

### Torex (特瑞仕)
- XC6204: https://www.torexsemi.com/series/xc6204/

---

## 快速查找表

### I2C设备地址

| 设备 | 地址 | 寄存器 |
|------|------|--------|
| LPS22HBTR | 0x5C/0x5D | WHO_AM_I=0xB1 |
| QMC5883P | 0x0D | CHIP_ID |

### SPI设备

| 设备 | 片选引脚 | 最大时钟 |
|------|----------|----------|
| ICM-42688-P | PA15 | 24MHz |

### 关键寄存器

| 设备 | 寄存器 | 地址 | 默认值 |
|------|--------|------|--------|
| ICM-42688-P | WHO_AM_I | 0x75 | 0x47 |
| LPS22HBTR | WHO_AM_I | 0x0F | 0xB1 |

---

## 使用说明

1. **软件开发优先参考**: `components.md` 和 `pinout.md`
2. **详细技术规格**: 查阅本目录下的PDF数据手册
3. **寄存器配置**: 参考对应芯片数据手册的Register Map章节
4. **电路连接**: 参考 `SCH_*.pdf` 电路图

---

*最后更新: 2026-03-18*
