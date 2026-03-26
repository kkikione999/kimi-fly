# STM32F411CEU6 Bring-Up Lookup Guide

This file gives the preferred lookup path for STM32 bring-up work in this repository.

## Recipe 1: New Peripheral Bring-Up

1. Confirm actual board wiring in `hardware-docs/pinout.md`.
2. Check the STM32 datasheet for the pin alternate function and electrical constraints.
3. Check the STM32 reference manual for register sequencing.
4. Search generated chunks before opening the full PDF.
5. Only then patch HAL or low-level driver code.

## Recipe 2: Motor Output Work

1. Confirm the motor mapping in `hardware-docs/pinout.md`.
2. Search for `tim1`, `tim2`, or `tim3` in STM32 chunks.
3. Verify timer channel mode, preload behavior, and counter clock assumptions.
4. Keep motor identity tied to physical location, not timer channel order.

## Recipe 3: Sensor Bus Work

1. Confirm I2C1 or SPI3 pin usage in `hardware-docs/pinout.md`.
2. Search STM32 chunks for the exact peripheral and feature you are changing.
3. Cross-check with chip-specific knowledge files under `knowledge-hub/hardware/`.
