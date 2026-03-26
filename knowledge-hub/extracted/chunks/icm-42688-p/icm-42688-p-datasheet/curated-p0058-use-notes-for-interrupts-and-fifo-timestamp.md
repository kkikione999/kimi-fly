# Use notes for interrupts and FIFO timestamp

- Chip: `ICM-42688-P`
- Chip Slug: `icm-42688-p`
- Document: `icm-42688-p-datasheet`
- Document Kind: `datasheet`
- Source PDF: `hardware-docs/ICM-42688-P_datasheet.pdf`
- Page: `58`
- Tags: `interrupts, int_async_reset, fifo, timestamp`

## Curated Summary

INT_CONFIG1.INT_ASYNC_RESET should be changed from default 1 to 0 for proper INT pin behavior. FIFO timestamp values require scaling depending on clock mode.

## Extracted Page Text

ICM-42688-P
Page 58 of 109
Document Number: DS-000347
Revision: 1.2
12.6 INT_ASYNC_RESET CONFIGURATION
For register INT_CONFIG1 (bank 0 register 0x64) bit 4 INT_ASYNC_RESET, user should change setting to 0 from default setting o f 1,
for proper INT1 and INT2 pin operation.
12.7 FIFO TIMESTAMP INTERVAL SCALING
When RTC_MODE =1 (bank 0 register 0x4D bit2) and register INTF_CONFIG5 (bank 1 register 0x7B) bit 2:1 (PIN9_FUNCTION) is set to
10 for CLKIN input;
THEN
Timestamp interval reported in FIFO requires scaling by a factor of 32.768/30.   For example when ODR = 1kHz, the true timestamp
interval should be 1000µs.  But the value in FIFO toggles between 915 and 916µs.  After scaling 915.5 * 32.768/30 = 1000µs.
ELSE
Timestamp interval reported in FIFO requires scaling by a factor of 32/30.   For example when ODR = 1kHz, the true timestamp
interval should be 1000µs.  But the value in FIFO toggles between 937 and 938µs.  After scaling 937.5 * 32/30 = 1000µs.
12.8 SUPPLEMENTARY INFORMATION FOR FIFO_HOLD_LAST_DATA_EN
This section contains supplementary information for using register field FIFO_HOLD _LAST_DATA_EN (bit 7) of register
INTF_CONFIG0 (address 0x4C, bank 0) .
The following table shows the values in FIFO:
FIFO_HOLD_LAST_DATA_EN 16-bit FIFO
Packet 20-bit FIFO Packet
0  (Insert Invalid code) Valid sample
All values in:
{-32766 to
+32767}
Gyro:  All Even numbers in {-524256 to
+524286}
Example:  {-524256, -524254, -524252, -524250
…..+524284, +524286}
Accel:  Every Other Even number in {-524256 to
+524284}
Example:  {-524256, -524252, -524248, -524244
…..+524280, +524284}
Invalid sample -32768 -524288
1  (“copy last valid” mode: No
invalid code insertion)
Valid sample
All values in:
{-32768 to
+32767}
Gyro:  All Even numbers in {-524288 to
+524286}
Example:  {-524288, -524286, -524284, -524282
…..+524284, +524286 }
Accel:  Every Other Even number in {-524288 to
+524284 }
Example: {-524288,  -524284, -524280
…..+524280, +524284}
Invalid sample Copy last valid sample
The following table shows the values in sense registers on reset:
FIFO_HOLD_LAST_DATA_EN = 0 FIFO_HOLD_LAST_DATA_EN = 1
Power On Reset till
First Sample Accel/Gyro/Temperature Sensor = -32768 Accel/Gyro/Temperature Sensor = 0
The following table shows the values in sense registers after first sample is received.  As shown in table, the combination of
FIFO_HOLD_LAST_DATA_EN and FSYNC Tag determine the range of values read for valid samples and invalid samples.
