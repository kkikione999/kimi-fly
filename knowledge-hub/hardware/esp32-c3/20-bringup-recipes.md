# ESP32-C3 Bring-Up Lookup Guide

## Recipe 1: UART Bridge Debug

1. Confirm the STM32 and ESP32 pin pairing in `hardware-docs/pinout.md`.
2. Search ESP32-C3 datasheet/TRM chunks for UART TX/RX behavior.
3. Cross-check firmware assumptions in `firmware/esp32/`.

## Recipe 2: Boot Or Reset Investigation

1. Check reset and power wiring first.
2. Search ESP32-C3 chunks for reset and GPIO boot behavior.
3. Only then change startup code or serial download assumptions.
