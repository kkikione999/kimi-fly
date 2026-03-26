# INT_STATUS2/3 and SIGNAL_PATH_RESET

- Chip: `ICM-42688-P`
- Chip Slug: `icm-42688-p`
- Document: `icm-42688-p-datasheet`
- Document Kind: `datasheet`
- Source PDF: `hardware-docs/ICM-42688-P_datasheet.pdf`
- Page: `72`
- Tags: `signal_path_reset, fifo_flush, wom, apex`

## Curated Summary

SIGNAL_PATH_RESET provides FIFO flush and signal-path reset helpers. INT_STATUS2/3 expose WOM and APEX-related flags.

## Extracted Page Text

ICM-42688-P
Page 72 of 109
Document Number: DS-000347
Revision: 1.2
14.31 INT_STATUS2
Name: INT_STATUS2
Address: 55 (37h)
Serial IF: R/C
Reset value: 0x00
Clock Domain: SCLK_UI
BIT NAME FUNCTION
7:4 - Reserved
3 SMD_INT Significant Motion Detection Interrupt, clears on read
2 WOM_Z_INT Wake on Motion Interrupt on Z-axis, clears on read
1 WOM_Y_INT Wake on Motion Interrupt on Y-axis, clears on read
0 WOM_X_INT Wake on Motion Interrupt on X-axis, clears on read
14.32 INT_STATUS3
Name: INT_STATUS3
Address: 56 (38h)
Serial IF: R/C
Reset value: 0x00
Clock Domain: SCLK_UI
BIT NAME FUNCTION
7:6 - Reserved
5 STEP_DET_INT Step Detection Interrupt, clears on read
4 STEP_CNT_OVF_INT Step Count Overflow Interrupt, clears on read
3 TILT_DET_INT Tilt Detection Interrupt, clears on read
2 WAKE_INT Wake Event Interrupt, clears on read
1 SLEEP_INT Sleep Event Interrupt, clears on read
0 TAP_DET_INT Tap Detection Interrupt, clears on read
14.33 SIGNAL_PATH_RESET
Name: SIGNAL_PATH_RESET
Address: 75 (4Bh)
Serial IF: W/C
Reset value: 0x00
Clock Domain: SCLK_UI
BIT NAME FUNCTION
7 - Reserved
6 DMP_INIT_EN When this bit is set to 1, the DMP is enabled
5 DMP_MEM_RESET_EN When this bit is set to 1, the DMP memory is reset
4 - Reserved
3 ABORT_AND_RESET When this bit is set to 1, the signal path is reset by restarting the ODR
counter and signal path controls
2 TMST_STROBE When this bit is set to 1, the time stamp counter is latched into the time
stamp register. This is a write on clear bit.
1 FIFO_FLUSH When set to 1, FIFO will get flushed.
0 - Reserved
