# DEVICE_CONFIG

- Chip: `ICM-42688-P`
- Chip Slug: `icm-42688-p`
- Document: `icm-42688-p-datasheet`
- Document Kind: `datasheet`
- Source PDF: `hardware-docs/ICM-42688-P_datasheet.pdf`
- Page: `63`
- Tags: `reset, device_config, startup`

## Curated Summary

DEVICE_CONFIG.SOFT_RESET_CONFIG triggers software reset. After writing 1, wait at least 1 ms before any other register access.

## Extracted Page Text

ICM-42688-P
Page 63 of 109
Document Number: DS-000347
Revision: 1.2
14 USER BANK 0 REGISTER MAP – DESCRIPTIONS
This section describes the function and contents of each register within USR Bank 0.
Note: The device powers up in sleep mode.
14.1 DEVICE_CONFIG
Name: DEVICE_CONFIG
Address: 17 (11h)
Serial IF: R/W
Reset value: 0x00
Clock Domain: SCLK_UI
BIT NAME FUNCTION
7:5 - Reserved
4 SPI_MODE
SPI mode selection
0: Mode 0 and Mode 3 (default)
1: Mode 1 and Mode 2
3:1 - Reserved
0 SOFT_RESET_CONFIG Software reset configuration
0: Normal (default)
1: Enable reset
After writing 1 to this bitfield, wait 1ms for soft reset to be effective, before
attempting any other register access
14.2 DRIVE_CONFIG
Name: DRIVE_CONFIG
Address: 19 (13h)
Serial IF: R/W
Reset value: 0x05
Clock Domain: SCLK_UI
BIT NAME FUNCTION
7:6 - Reserved
5:3 I2C_SLEW_RATE
Controls slew rate for output pin 14 in I 2C mode only
000: 20ns-60ns
001: 12ns-36ns
010: 6ns-18ns
011: 4ns-12ns
100: 2ns-6ns
101: < 2ns
110: Reserved
111: Reserved
2:0 SPI_SLEW_RATE
Controls slew rate for output pin 14 in SPI or I3C SM mode, and for all other
output pins
000: 20ns-60ns
001: 12ns-36ns
010: 6ns-18ns
011: 4ns-12ns
100: 2ns-6ns
101: < 2ns
110: Reserved
111: Reserved
