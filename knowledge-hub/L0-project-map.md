# L0 Project Map — Drone WiFi Flight Controller

> Generated: 2026-03-19 | Strategy: module-by-module | Files: 66 source (316 total, excl. build)

---

## Project Identity

| Field | Value |
|-------|-------|
| Goal | WiFi-controlled drone flight |
| MCU | STM32F411CEU6 (Cortex-M4, 100MHz) |
| WiFi | ESP32-C3 (STA mode, SSID=whc, PW=12345678) |
| Build | PlatformIO (STM32: stm32cube; ESP32: ESP-IDF + Arduino) |
| Phase | Phase 2–3 complete (sensors ✅, AHRS ✅, PID ✅, WiFi comms partial ⚠️) |

---

## Directory Structure

```
kimi-fly/
├── firmware/
│   ├── stm32/                  ← STM32F411 flight controller (main codebase)
│   │   ├── hal/                ← GPIO, UART, I2C, SPI, PWM HAL drivers
│   │   ├── drivers/            ← ICM42688 (IMU), LPS22HB (baro), QMC5883P (mag)
│   │   ├── algorithm/          ← AHRS (Mahony), PID, FlightCtrl, SensorCalib
│   │   ├── comm/               ← Binary protocol, WiFi command handler
│   │   ├── main/               ← flight_main.c (1kHz loop), sensor_test.c
│   │   └── src/                ← main.c (PlatformIO entry for debug builds)
│   ├── esp32/                  ← ESP32-C3 WiFi bridge (ESP-IDF)
│   │   └── main/               ← wifi_sta.c, tcp_server.c, main.c (UART bridge)
│   └── hal/                    ← (empty legacy stubs)
├── hardware-docs/
│   ├── pinout.md               ← ⚠️ SSOT — all pin assignments authoritative here
│   ├── components.md
│   └── datasheets/             ← ICM42688, LPS22HBTR, QMC5883P, STM32F411, ESP32-C3
├── docs/
│   ├── Architecture.md
│   ├── exec-plans/active/      ← Current task files (plan.md, task-3xx series)
│   ├── exec-plans/tech-debt-tracker.md
│   └── user-intent/user-intent.md
└── tools/
    ├── ground_station/         ← Python ground station (TCP client)
    └── simulation/             ← Drone dynamics SIL simulation
```

---

## Module Inventory

| Module | Path | Language | Status |
|--------|------|----------|--------|
| HAL | `firmware/stm32/hal/` | C | ✅ Complete |
| Sensors | `firmware/stm32/drivers/` | C | ✅ Complete (bugs fixed) |
| Algorithm | `firmware/stm32/algorithm/` | C | ✅ Complete |
| Comm | `firmware/stm32/comm/` | C | ⚠️ Protocol ready, UART wiring unverified |
| Flight Main | `firmware/stm32/main/` | C | ✅ Complete |
| ESP32 WiFi | `firmware/esp32/main/` | C (ESP-IDF) | ✅ TCP bridge ready |
| Ground Station | `tools/ground_station/` | Python | ✅ Reference impl |

---

## Build Environments

| Target | Config | Framework | Upload |
|--------|--------|-----------|--------|
| STM32 flight | `platformio.ini [env:flight]` | stm32cube (no HAL stdlib) | ST-Link |
| STM32 debug | `platformio.ini [env:stm32f411]` | stm32cube (USE_HAL_DRIVER) | ST-Link |
| ESP32 | `firmware/esp32/platformio.ini` | Arduino / ESP-IDF | esptool |

---

## Key Interfaces

| Interface | Pins | Peripheral | Devices |
|-----------|------|------------|---------|
| I2C1 | PB6 (SCL), PB7 (SDA) | I2C @ 400kHz | ICM-42688-P (0x69), QMC5883P (0x2C) |
| SPI3 | PA15 (CS), PB3 (SCK), PB4 (MISO), PB5 (MOSI) | SPI Mode0 | LPS22HBTR |
| USART2 | PA2 (TX), PA3 (RX) | 115200 baud | ESP32-C3 bridge |
| USART1 | PA9 (TX), PA10 (RX) | Debug uart | Debug console |
| TIM1_CH1 | PA8 | PWM 42kHz | Motor 1 |
| TIM1_CH4 | PA11 | PWM 42kHz | Motor 2 |
| TIM3_CH4 | PB1 | PWM 42kHz | Motor 3 |
| TIM2_CH3 | PB10 | PWM 42kHz | Motor 4 |
| ADC1_IN8 | PB0 | ADC | Battery voltage |

---

## Communication Protocol (STM32 ↔ ESP32 ↔ Ground Station)

```
Ground Station (Python, TCP:8888)
    │ TCP socket
    ▼
ESP32-C3 (TCP server, WiFi STA)
    │ UART 115200 baud (GPIO0=TX, GPIO1=RX)
    ▼
STM32 USART2 (PA2=TX, PA3=RX)
    │ Binary framing: 0xAA55 | len | cmd | payload | CRC16
    ▼
WiFi Command Handler → Flight Controller
```

---

## Active Work (as of 2026-03-19)

| Task | Subject | Status |
|------|---------|--------|
| Task 301 | ICM42688 I2C address fix (0x68→0x69) | ✅ Done |
| Task 302 | LPS22HB SPI auto-increment fix | ✅ Done |
| Task 303 | QMC5883P Y/Z axis zero fix | ✅ Done |
| Task 304 | UART diagnostic tool (STM32↔ESP32 wiring) | ⏳ Pending |

---

## Open Tech Debt

| ID | Issue | Priority |
|----|-------|----------|
| TD-001 | STM32–ESP32 UART physical wiring unconfirmed (both RX=0, suspected TX/RX not crossed) | P1 |
