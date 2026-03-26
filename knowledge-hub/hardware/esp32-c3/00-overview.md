# ESP32-C3 Overview

## Purpose

This file is the AI-first ESP32-C3 summary for the repository.

## Project Truth First

- Role: WiFi bridge / control communications
- Connected to STM32 over `USART2`
- STM32 `PA2 (USART2_TX)` -> ESP32-C3 `GPIO1`
- STM32 `PA3 (USART2_RX)` <- ESP32-C3 `GPIO0`
- Power: `3V3_RF`
- Reset: shared with STM32 `NRST`

## Document Map

- Datasheet: package, IO, electrical data, bootstrapping basics
- TRM: peripheral registers and lower-level behavior

## Generated Chunk Paths

- Datasheet chunks: `knowledge-hub/extracted/chunks/esp32-c3/esp32-c3-datasheet/`
- TRM chunks: `knowledge-hub/extracted/chunks/esp32-c3/esp32-c3-trm/`
