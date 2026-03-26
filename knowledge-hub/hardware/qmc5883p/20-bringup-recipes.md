# QMC5883P Bring-Up Lookup Guide

1. Confirm `I2C1` wiring and address `0x2C` in `hardware-docs/pinout.md`.
2. Search the generated datasheet chunks for configuration and measurement registers.
3. Keep magnetometer-specific calibration separate from raw sensor bring-up.
