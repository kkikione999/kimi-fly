# ACCEL_CONFIG0

- Chip: `ICM-42688-P`
- Chip Slug: `icm-42688-p`
- Document: `icm-42688-p-datasheet`
- Document Kind: `datasheet`
- Source PDF: `hardware-docs/ICM-42688-P_datasheet.pdf`
- Page: `77`
- Tags: `accel_config0, odr, fsr`

## Curated Summary

ACCEL_CONFIG0 holds accel full-scale range and UI ODR. Reset value 0x06 means ±16 g and 1 kHz. For ±8 g and 1 kHz, write 0x26.

## Extracted Page Text

ICM-42688-P
Page 77 of 109
Document Number: DS-000347
Revision: 1.2
14.38 ACCEL_CONFIG0
Name: ACCEL_CONFIG0
Address: 80 (50h)
Serial IF: R/W
Reset value: 0x06
Clock Domain: SCLK_UI
BIT NAME FUNCTION
7:5 ACCEL_FS_SEL
Full scale select for accelerometer UI interface output
000: ±16g (default)
001: ±8g
010: ±4g
011: ±2g
100: Reserved
101: Reserved
110: Reserved
111: Reserved
4 - Reserved
3:0 ACCEL_ODR
Accelerometer ODR selection for UI interface output
0000: Reserved
0001: 32kHz (LN mode)
0010: 16kHz (LN mode)
0011: 8kHz (LN mode)
0100: 4kHz (LN mode)
0101: 2kHz (LN mode)
0110: 1kHz (LN mode) (default)
0111: 200Hz (LP or LN mode)
1000: 100Hz (LP or LN mode)
1001: 50Hz (LP or LN mode)
1010: 25Hz (LP or LN mode)
1011: 12.5Hz (LP or LN mode)
1100: 6.25Hz (LP mode)
1101: 3.125Hz (LP mode)
1110: 1.5625Hz (LP mode)
1111: 500Hz (LP or LN mode)
