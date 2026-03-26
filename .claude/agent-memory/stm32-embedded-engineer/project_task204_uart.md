---
name: Task 204 UART communication status
description: STM32-ESP32 UART test status as of 2026-03-19 - firmware working, hardware link unverified
type: project
---

Task 204 tests bidirectional UART between STM32F411 (USART2 PA2/PA3) and ESP32-C3 (Serial1 GPIO5=TX/GPIO4=RX) at 115200 baud.

**Current status (2026-03-19)**: Both firmware sides running, hardware UART link RX=0 both directions.

**Why:** Hardware connection (physical wires between STM32 PA2/PA3 and ESP32 GPIO4/GPIO5) needs physical inspection. Both TX counters incrementing but neither RX counter increments.

**How to apply:** Next debugging step is physical wire inspection:
- STM32 PA2 (TX) → ESP32 GPIO4 (RX)
- STM32 PA3 (RX) → ESP32 GPIO5 (TX)
- Common GND required

Serial ports: STM32 debug = `/dev/cu.usbmodem212403` @ 460800, ESP32 debug = `/dev/cu.usbmodem212301` @ 115200
