---
name: hardware_findings_task203
description: Verified hardware configuration differences from documentation found during Task 203 sensor validation
type: project
---

ICM-42688-P I2C address is 0x69 (not 0x68). AD0 pin is pulled HIGH on this drone hardware.

LPS22HBTR socket has LPS22HH installed (WHO_AM_I=0xB3 vs expected 0xB1). LPS22HH is a compatible variant but has different register bit layout for BDU and ODR.

ST-Link VCP port `/dev/cu.usbmodem212403` connects to USART1 (PA9/PA10) on STM32.
ESP32-C3 debug port is `/dev/cu.usbmodem212301`.

**Why:** Discovered during live sensor testing - I2C scan and WHO_AM_I reads revealed discrepancies.

**How to apply:** Always use 0x69 for ICM-42688-P address in this project. Accept WHO_AM_I 0xB1 OR 0xB3 for the barometer. Use LPS22HH datasheet for register configuration.
