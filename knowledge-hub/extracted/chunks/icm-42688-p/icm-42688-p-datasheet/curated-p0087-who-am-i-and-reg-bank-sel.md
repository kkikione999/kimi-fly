# WHO_AM_I and REG_BANK_SEL

- Chip: `ICM-42688-P`
- Chip Slug: `icm-42688-p`
- Document: `icm-42688-p-datasheet`
- Document Kind: `datasheet`
- Source PDF: `hardware-docs/ICM-42688-P_datasheet.pdf`
- Page: `87`
- Tags: `who_am_i, bank, identity`

## Curated Summary

WHO_AM_I reset value is 0x47. REG_BANK_SEL selects banks 0,1,2,4. WHO_AM_I is not the same as I2C slave address.

## Extracted Page Text

ICM-42688-P
Page 87 of 109
Document Number: DS-000347
Revision: 1.2
14.57 SELF_TEST_CONFIG
Name: SELF_TEST_CONFIG
Address: 112 (70h)
Serial IF: R/W
Reset value: 0x00
Clock Domain: SCLK_UI
BIT NAME FUNCTION
7 - Reserved
6 ACCEL_ST_POWER Set to 1 for accel self-test
Otherwise set to 0; Set to 0 after self-test is completed
5 EN_AZ_ST Enable Z-accel self-test
4 EN_AY_ST Enable Y-accel self-test
3 EN_AX_ST Enable X-accel self-test
2 EN_GZ_ST Enable Z-gyro self-test
1 EN_GY_ST Enable Y-gyro self-test
0 EN_GX_ST Enable X-gyro self-test
14.58 WHO_AM_I
Name: WHO_AM_I
Address: 117 (75h)
Serial IF: R
Reset value: 0x47
Clock Domain: SCLK_UI
BIT NAME FUNCTION
7:0 WHOAMI Register to indicate to user which device is being accessed
Description:
This register is used to verify the identity of the device. The contents of WHOAMI is an 8-bit device ID. The default value of the
register is 0x47. This is different from the I2C address of the device as seen on the slave I 2C controller by the applications processor.
14.59 REG_BANK_SEL
Note: This register is accessible from all register banks
Name: REG_BANK_SEL
Address: 118 (76h)
Serial IF: R/W
Reset value: 0x00
Clock Domain: ALL
BIT NAME FUNCTION
7:3 - Reserved
2:0 BANK_SEL
Register bank selection
000: Bank 0 (default)
001: Bank 1
010: Bank 2
011: Bank 3
100: Bank 4
101: Reserved
110: Reserved
111: Reserved
