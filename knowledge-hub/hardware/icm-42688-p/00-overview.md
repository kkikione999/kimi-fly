# ICM-42688-P Overview

## Purpose

This file is the AI-first summary for `ICM-42688-P`.
Use it before reading the register notes or raw chunks.

## Project Truth First

These facts come from the project SSOT and bench validation, not from old code:

- Device: `ICM-42688-P`
- Bus: `I2C1`
- Address: `0x69` (`AD0 = VCC`)
- STM32 pins: `PB6 = I2C1_SCL`, `PB7 = I2C1_SDA`, `PC13 = INT1`
- Body mapping at runtime:
  - `body +X = IMU +Y`
  - `body +Y = IMU -X`
  - `body +Z = IMU +Z`
- Source of truth: `hardware-docs/pinout.md`

Do not trust older comments that still say `0x68`.

## Device Summary

- 6-axis IMU: 3-axis gyro + 3-axis accel
- FIFO: `2 KB`
- Interfaces supported by the chip: `I3C`, `I2C`, `SPI`
- This project currently uses: `I2C`
- WHO_AM_I value: `0x47`
- Operating supply range from datasheet: `1.71V to 3.6V`

## High-Value Datasheet Facts

- The chip powers up with accel and gyro off. Source: `ICM-42688-P_datasheet.pdf` p.63, p.75
- `WHO_AM_I` is `0x47` and is not the same as the I2C slave address. Source: `ICM-42688-P_datasheet.pdf` p.87
- Default UI output config after reset is `GYRO_CONFIG0 = 0x06` and `ACCEL_CONFIG0 = 0x06`, which means default ODR `1kHz` and default FSR `±2000 dps` / `±16g`. Source: `ICM-42688-P_datasheet.pdf` p.76-p.77
- Temperature conversion from sensor registers is:
  - `temp_c = raw_temp / 132.48 + 25`
  - Source: `ICM-42688-P_datasheet.pdf` p.24
- Typical startup times:
  - accel valid data from sleep: `10 ms`
  - gyro drive ready after enable: `30 ms`
  - Source: `ICM-42688-P_datasheet.pdf` p.11-p.12

## Driver-Relevant Risks

- Old project files still contain `0x68` in comments and test strings. Treat those as stale until fixed.
- Datasheet use note says the part boots in SPI mode and documents explicit interface settings for I2C/SPI/I3C. Source: `ICM-42688-P_datasheet.pdf` p.57
- Datasheet use note says `INT_CONFIG1.INT_ASYNC_RESET` should be changed from default `1` to `0` for proper INT pin behavior. Source: `ICM-42688-P_datasheet.pdf` p.58

## Recommended Default Operating Point For This Project

For initial flight-controller bring-up:

- Gyro mode: `Low Noise`
- Accel mode: `Low Noise`
- Gyro range: `±2000 dps`
- Accel range: `±8g`
- Gyro ODR: `1 kHz`
- Accel ODR: `1 kHz`
- FIFO: disabled at first bring-up
- Data path: direct sensor-register burst reads before enabling FIFO/interrupt complexity

This matches the current driver intent while keeping the setup simple.
