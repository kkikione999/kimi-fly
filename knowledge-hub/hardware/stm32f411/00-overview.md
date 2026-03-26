# STM32F411CEU6 Overview

## Purpose

This file is the AI-first STM32 summary for this repository.
Read it before using the STM32 datasheet, reference manual, or generated chunks.

## Project Truth First

These facts come from `hardware-docs/pinout.md` and must override stale code comments:

- MCU: `STM32F411CEU6`
- Debug UART: `USART1 @ 460800, 8N1` on `PA9/PA10`
- WiFi bridge UART: `USART2 @ 115200, 8N1` on `PA2/PA3`
- Motors:
  - `PA8 = TIM1_CH1 = Motor1 = FrontLeft = CCW`
  - `PA11 = TIM1_CH4 = Motor2 = RearLeft = CW`
  - `PB1 = TIM3_CH4 = Motor3 = RearRight = CCW`
  - `PB10 = TIM2_CH3 = Motor4 = FrontRight = CW`
- Sensor buses:
  - `I2C1 = PB6/PB7` for `ICM-42688-P` and `QMC5883P`
  - `SPI3 = PB3/PB4/PB5` with `PA15` CS for `LPS22HBTR`
- Battery ADC:
  - `PB0 = ADC1_IN8`
- External interrupt inputs:
  - `PC13 = ICM-42688-P INT1`
  - `PB9 = LPS22HBTR INT_DRDY`

## Document Map

- Datasheet: package, electrical limits, alternate functions
- Reference manual: peripheral registers and behavior
- Programming manual: Cortex-M4 core behavior, faults, NVIC, SysTick

## Recommended Lookup Order

1. `hardware-docs/pinout.md`
2. STM32 datasheet overview and electrical limits
3. STM32 reference manual for peripheral register details
4. STM32 programming manual for core-level behavior
5. Generated chunks under `knowledge-hub/extracted/chunks/stm32f411/`
