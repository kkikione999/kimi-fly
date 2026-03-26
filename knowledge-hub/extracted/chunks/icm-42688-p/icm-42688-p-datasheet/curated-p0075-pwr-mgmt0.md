# PWR_MGMT0

- Chip: `ICM-42688-P`
- Chip Slug: `icm-42688-p`
- Document: `icm-42688-p-datasheet`
- Document Kind: `datasheet`
- Source PDF: `hardware-docs/ICM-42688-P_datasheet.pdf`
- Page: `75`
- Tags: `power, pwr_mgmt0, low_noise, low_power`

## Curated Summary

PWR_MGMT0 controls temp sensor enable plus gyro/accel modes. After OFF-to-ON transition, do not issue register writes for 200 us. Gyro should remain on at least 45 ms.

## Extracted Page Text

ICM-42688-P
Page 75 of 109
Document Number: DS-000347
Revision: 1.2
14.36 PWR_MGMT0
Name: PWR_MGMT0
Address: 78 (4Eh)
Serial IF: R/W
Reset value: 0x00
Clock Domain: SCLK_UI
BIT NAME FUNCTION
7:6 - Reserved
5 TEMP_DIS 0: Temperature sensor is enabled (default)
1: Temperature sensor is disabled
4 IDLE
If this bit is set to 1, the RC oscillator is powered on even if Accel and Gyro
are powered off.
Nominally this bit is set to 0, so when Accel and Gyro are powered off,
the chip will go to OFF state, since the RC oscillator will also be powered off
3:2 GYRO_MODE
00: Turns gyroscope off (default)
01: Places gyroscope in Standby Mode
10: Reserved
11: Places gyroscope in Low Noise (LN) Mode
Gyroscope needs to be kept ON for a minimum of 45ms. When transitioning
from OFF to any of the other modes, do not issue any register writes for
200µs.
1:0 ACCEL_MODE
00: Turns accelerometer off (default)
01: Turns accelerometer off
10: Places accelerometer in Low Power (LP) Mode
11: Places accelerometer in Low Noise (LN) Mode
When transitioning from OFF to any of the other modes, do not issue any
register writes for 200µs.
