# Use notes for interface and filters

- Chip: `ICM-42688-P`
- Chip Slug: `icm-42688-p`
- Document: `icm-42688-p-datasheet`
- Document Kind: `datasheet`
- Source PDF: `hardware-docs/ICM-42688-P_datasheet.pdf`
- Page: `57`
- Tags: `i2c, spi, i3c, filters`

## Curated Summary

Datasheet includes explicit interface settings for I2C/I3C/SPI operation and notes that notch/anti-alias filters are for LN mode only.

## Extracted Page Text

ICM-42688-P
Page 57 of 109
Document Number: DS-000347
Revision: 1.2
12 USE NOTES
12.1 ACCELEROMETER MODE TRANSITIONS
When transitioning from accelerometer Low Power (LP) mode to accelerometer Low Noise (LN) mode, if ODR is 6.25Hz or lower,
software should change ODR to a value of 12.5Hz or higher, because accelerometer LN mode does not support ODR values belo w
12.5Hz.
When transitioning from accelerometer LN mode to accelerometer LP mode, if ODR is greater than 500Hz, software should change
ODR to a value of 500Hz or lower, because accelerometer LP mode does not support ODR values above 500Hz.
12.2 ACCELEROMETER LOW POWER (LP) MODE AVERAGING FILTER SETTING
Software drivers provided with the device use Averaging Filter setting of 16x.  This setting is recommended for meeting Andro id
noise requirements in LP mode, and to minimize accelerometer offset variation when transitioning from LP to Low Noise (LN) mode.
1x averaging filter can be used by following the setting configuration shown in section 14.38.
12.3 SETTINGS FOR I2C, I3CSM, AND SPI OPERATION
Upon bootup the device comes up in SPI mode.  The following settings s hould be used for I2C, I3CSM, and SPI operation.
Scenario 1: INT1/INT2 pins are used for interrupt assertion in I3CSM mode.
Register Field I2C Driver Setting I3CSM Driver Setting SPI Driver Setting
I3C_EN (bit 4, register INTF_CONFIG6, address 0x7C, bank 1) 1 1 1
I3C_SDR_EN (bit 0, register INTF_CONFIG6, address 0x7C, bank 1) 0 1 1
I3C_DDR_EN (bit 1, register INTF_CONFIG6, address 0x7C, bank 1) 0 0 1
I3C_BUS_MODE (bit 6, register INTF_CONFIG4, address 0x7A, bank 1) 0 0 0
I2C_SLEW_RATE (bits 5:3, register DRIVE_CONFIG, address 0x13, bank 0) 1 0 0
SPI_SLEW_RATE (bits 2:0, register DRIVE_CONFIG, address 0x13, bank 0) 1 5 5
Scenario 2: IBI is used for interrupt assertion in I3CSM mode.
Register Field I2C Driver Setting I3CSM Driver Setting SPI Driver Setting
I3C_EN (bit 4, register INTF_CONFIG6, address 0x7C, bank 1) 1 1 1
I3C_SDR_EN (bit 0, register INTF_CONFIG6, address 0x7C, bank 1) 0 1 1
I3C_DDR_EN (bit 1, register INTF_CONFIG6, address 0x7C, bank 1) 0 1 1
I3C_BUS_MODE (bit 6, register INTF_CONFIG4, address 0x7A, bank 1) 0 0 0
I2C_SLEW_RATE (bits 5:3, register DRIVE_CONFIG, address 0x13, bank 0) 1 0 0
SPI_SLEW_RATE (bits 2:0, register DRIVE_CONFIG, address 0x13, bank 0) 1 5 5
12.4 NOTCH FILTER AND ANTI-ALIAS FILTER OPERATION
Use of Notch Filter and Anti-Alias Filter is supported only for Low Noise (LN) mode operation. The host is responsible for keeping the
UI path in LN mode while Notch Filter and Anti-Alias Filter are turned on.
12.5 EXTERNAL CLOCK INPUT EFFECT ON ODR
ODR values supported by the device scale with external clock frequency, if external clock input is used.  The ODR values show n in the
datasheet are supported with external clock input frequency of 32kHz.  For any other external cl ock input frequency, these ODR
values will scale by a factor of (External clock value in kHz / 32).  For example, if an external clock frequency of 32.768kH z is used,
instead of ODR value of 500Hz, it will be 500 * (32.768 / 32) = 512Hz.
