# STM32F411CEU6 Register Lookup Guide

This file is intentionally lightweight for now.
Use it as a map into the generated chunk set instead of re-reading whole PDFs.

## High-Value Areas For This Project

- `RCC`: peripheral clock enables and resets
- `GPIO`: alternate function setup, pull configuration, output type, speed
- `TIM1/TIM2/TIM3`: PWM generation for motors
- `USART1/USART2`: debug and WiFi bridge
- `I2C1`: IMU and magnetometer
- `SPI3`: barometer
- `ADC1`: battery voltage path
- `EXTI/NVIC`: sensor interrupts

## Generated Chunk Paths

- Datasheet chunks: `knowledge-hub/extracted/chunks/stm32f411/stm32f411-datasheet/`
- Reference manual chunks: `knowledge-hub/extracted/chunks/stm32f411/stm32f411-reference-manual/`
- Programming manual chunks: `knowledge-hub/extracted/chunks/stm32f411/stm32f411-programming-manual/`

## Search Examples

```bash
python3 scripts/search_hardware_knowledge.py "stm32f411 tim1 pwm" --chip stm32f411
python3 scripts/search_hardware_knowledge.py "usart2 baud oversampling" --chip stm32f411
```
