# INT_CONFIG and FIFO_CONFIG

- Chip: `ICM-42688-P`
- Chip Slug: `icm-42688-p`
- Document: `icm-42688-p-datasheet`
- Document Kind: `datasheet`
- Source PDF: `hardware-docs/ICM-42688-P_datasheet.pdf`
- Page: `64`
- Tags: `interrupts, fifo_config, int_config`

## Curated Summary

INT_CONFIG controls polarity, drive, and pulse/latched behavior. FIFO_CONFIG selects bypass, stream, or stop-on-full FIFO mode.

## Extracted Page Text

ICM-42688-P
Page 64 of 109
Document Number: DS-000347
Revision: 1.2
14.3 INT_CONFIG
Name: INT_CONFIG
Address: 20 (14h)
Serial IF: R/W
Reset value: 0x00
Clock Domain: SCLK_UI
BIT NAME FUNCTION
7:6 - Reserved
5 INT2_MODE
INT2 interrupt mode
0: Pulsed mode
1: Latched mode
4 INT2_DRIVE_CIRCUIT
INT2 drive circuit
0: Open drain
1: Push pull
3 INT2_POLARITY
INT2 interrupt polarity
0: Active low (default)
1: Active high
2 INT1_MODE
INT1 interrupt mode
0: Pulsed mode
1: Latched mode
1 INT1_DRIVE_CIRCUIT
INT1 drive circuit
0: Open drain
1: Push pull
0 INT1_POLARITY
INT1 interrupt polarity
0: Active low (default)
1: Active high
14.4 FIFO_CONFIG
Name: FIFO_CONFIG
Address: 22 (16h)
Serial IF: R/W
Reset value: 0x00
Clock Domain: SCLK_UI
BIT NAME FUNCTION
7:6 FIFO_MODE 00: Bypass Mode (default)
01: Stream-to-FIFO Mode
10: STOP-on-FULL Mode
11: STOP-on-FULL Mode
5:0 - Reserved
14.5 TEMP_DATA1
Name: TEMP_DATA1
Address: 29 (1Dh)
Serial IF: SYNCR
Reset value: 0x80
Clock Domain: SCLK_UI
BIT NAME FUNCTION
7:0 TEMP_DATA[15:8] Upper byte of temperature data
