# GYRO_CONFIG0

- Chip: `ICM-42688-P`
- Chip Slug: `icm-42688-p`
- Document: `icm-42688-p-datasheet`
- Document Kind: `datasheet`
- Source PDF: `hardware-docs/ICM-42688-P_datasheet.pdf`
- Page: `76`
- Tags: `gyro_config0, odr, fsr`

## Curated Summary

GYRO_CONFIG0 holds gyro full-scale range and UI ODR. Reset value 0x06 means ±2000 dps and 1 kHz.

## Extracted Page Text

ICM-42688-P
Page 76 of 109
Document Number: DS-000347
Revision: 1.2
14.37 GYRO_CONFIG0
Name: GYRO_CONFIG0
Address: 79 (4Fh)
Serial IF: R/W
Reset value: 0x06
Clock Domain: SCLK_UI
BIT NAME FUNCTION
7:5 GYRO_FS_SEL
Full scale select for gyroscope UI interface output
000: ±2000dps (default)
001: ±1000dps
010: ±500dps
011: ±250dps
100: ±125dps
101: ±62.5dps
110: ±31.25dps
111: ±15.625dps
4 - Reserved
3:0 GYRO_ODR
Gyroscope ODR selection for UI interface output
0000: Reserved
0001: 32kHz
0010: 16kHz
0011: 8kHz
0100: 4kHz
0101: 2kHz
0110: 1kHz (default)
0111: 200Hz
1000: 100Hz
1001: 50Hz
1010: 25Hz
1011: 12.5Hz
1100: Reserved
1101: Reserved
1110: Reserved
1111: 500Hz
