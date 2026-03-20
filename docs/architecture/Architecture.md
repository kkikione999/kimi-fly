# 系统架构

> 生成日期: 2026-03-18

## 硬件平台

- **飞控**: STM32F411CEU6 (Cortex-M4, 100MHz)
- **通信**: ESP32-C3 (WiFi STA)
- **传感器**: ICM-42688-P (IMU), LPS22HBTR (气压计), QMC5883P (磁力计)
- **动力**: 空心杯电机 × 4

## 软件栈

| 组件 | 选择 | 理由 |
|------|------|------|
| STM32 RTOS | FreeRTOS | 统一任务调度 |
| STM32 HAL | HAL库 | 开发效率 |
| ESP32 SDK | ESP-IDF | 原生WiFi性能 |

## 数据流

```
控制终端 ──WiFi──► ESP32-C3 ──UART2──► STM32 ──PWM──► 电机
                      ▲                    │
                      └────遥测数据─────────┘
```

## 模块职责

### ESP32-C3 (通信转发)
- WiFi STA连接管理 (ESP-IDF)
- TCP Server监听 (ESP-IDF)
- **AI编写**: 协议帧解析、心跳保活、UART转发

### STM32 (飞控核心)
- HAL层: GPIO/PWM/I2C/SPI/UART
- **AI编写**:
  - 命令分发器 (解析控制指令)
  - PID控制器 (角度/角速度双环)
  - AHRS (Mahony姿态解算)
  - 遥测上报 (状态回传)

## 协议简述

- **传输**: TCP over WiFi
- **帧格式**: [SOF:1] [CMD:1] [LEN:1] [PAYLOAD:N] [CRC:1] [EOF:1]
- **典型命令**: THROTTLE, PITCH, ROLL, YAW
- **遥测**: 姿态角、电池电压、飞行状态

## 代码目录

```
firmware/
├── stm32/
│   ├── main/          # 飞控主程序
│   ├── drivers/       # 传感器驱动
│   ├── control/       # PID/AHRS算法
│   └── hal/           # HAL封装
├── esp32c3/
│   ├── main/          # WiFi通信程序
│   ├── protocol/      # 协议解析
│   └── wifi/          # WiFi连接管理
└── shared/
    └── protocol.h     # 双端共用协议定义
```
