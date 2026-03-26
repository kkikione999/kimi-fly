# ICM-42688-P Register Notes

This file keeps only the registers that are high value for this project.

## Banking

- `REG_BANK_SEL` at `0x76`
- Default bank after reset: `Bank 0`
- Value mapping:
  - `0`: Bank 0
  - `1`: Bank 1
  - `2`: Bank 2
  - `4`: Bank 4
- Source: `ICM-42688-P_datasheet.pdf` p.87

## Core Bring-Up Registers

### `DEVICE_CONFIG` (`0x11`, Bank 0)

- `SOFT_RESET_CONFIG[0] = 1` triggers software reset
- Wait at least `1 ms` before further register access
- Source: `ICM-42688-P_datasheet.pdf` p.63

### `INT_CONFIG` (`0x14`, Bank 0)

- Controls INT1/INT2 polarity, drive type, pulse/latched mode
- Important if you wire `INT1` to STM32 `PC13`
- Source: `ICM-42688-P_datasheet.pdf` p.64

### `FIFO_CONFIG` (`0x16`, Bank 0)

- `00`: bypass
- `01`: stream-to-FIFO
- `10` or `11`: stop-on-full
- Start with bypass during first driver bring-up
- Source: `ICM-42688-P_datasheet.pdf` p.64

### `INT_STATUS` (`0x2D`, Bank 0, read-to-clear)

- `bit4 RESET_DONE_INT`
- `bit3 DATA_RDY_INT`
- `bit2 FIFO_THS_INT`
- `bit1 FIFO_FULL_INT`
- Use carefully because read clears flags
- Source: `ICM-42688-P_datasheet.pdf` p.68

### `INT_STATUS2` (`0x37`, Bank 0, read-to-clear)

- Wake-on-motion and significant-motion status
- Not needed for initial flight bring-up
- Source: `ICM-42688-P_datasheet.pdf` p.72

### `INT_STATUS3` (`0x38`, Bank 0, read-to-clear)

- Step/tap/tilt/wake/sleep status from APEX features
- Ignore for initial flight bring-up
- Source: `ICM-42688-P_datasheet.pdf` p.72

### `SIGNAL_PATH_RESET` (`0x4B`, Bank 0, write-on-clear)

- `bit3 ABORT_AND_RESET`: restart ODR counter and signal path controls
- `bit2 TMST_STROBE`: latch timestamp
- `bit1 FIFO_FLUSH`: flush FIFO
- Useful recovery register when stream state is corrupted
- Source: `ICM-42688-P_datasheet.pdf` p.72

### `PWR_MGMT0` (`0x4E`, Bank 0)

- `TEMP_DIS[5]`: `0` keeps temp enabled
- `GYRO_MODE[3:2]`
  - `00`: off
  - `01`: standby
  - `11`: low noise
- `ACCEL_MODE[1:0]`
  - `00` / `01`: off
  - `10`: low power
  - `11`: low noise
- Datasheet timing notes:
  - after OFF -> another mode, do not issue register writes for `200 us`
  - gyro should stay on at least `45 ms`
- Source: `ICM-42688-P_datasheet.pdf` p.75

### `GYRO_CONFIG0` (`0x4F`, Bank 0)

- `GYRO_FS_SEL[7:5]`
  - `0`: `±2000 dps`
  - `1`: `±1000 dps`
  - `2`: `±500 dps`
  - `3`: `±250 dps`
  - `4`: `±125 dps`
  - `5`: `±62.5 dps`
  - `6`: `±31.25 dps`
  - `7`: `±15.625 dps`
- `GYRO_ODR[3:0]`
  - `1`: `32 kHz`
  - `2`: `16 kHz`
  - `3`: `8 kHz`
  - `4`: `4 kHz`
  - `5`: `2 kHz`
  - `6`: `1 kHz`
  - `7`: `200 Hz`
  - `8`: `100 Hz`
  - `9`: `50 Hz`
  - `10`: `25 Hz`
  - `11`: `12.5 Hz`
  - `15`: `500 Hz`
- Reset value: `0x06`
- Source: `ICM-42688-P_datasheet.pdf` p.76

### `ACCEL_CONFIG0` (`0x50`, Bank 0)

- `ACCEL_FS_SEL[7:5]`
  - `0`: `±16 g`
  - `1`: `±8 g`
  - `2`: `±4 g`
  - `3`: `±2 g`
- `ACCEL_ODR[3:0]`
  - `1`: `32 kHz` LN only
  - `2`: `16 kHz` LN only
  - `3`: `8 kHz` LN only
  - `4`: `4 kHz` LN only
  - `5`: `2 kHz` LN only
  - `6`: `1 kHz` LN only
  - `7`: `200 Hz` LP or LN
  - `8`: `100 Hz` LP or LN
  - `9`: `50 Hz` LP or LN
  - `10`: `25 Hz` LP or LN
  - `11`: `12.5 Hz` LP or LN
  - `12`: `6.25 Hz` LP only
  - `13`: `3.125 Hz` LP only
  - `14`: `1.5625 Hz` LP only
  - `15`: `500 Hz` LP or LN
- Reset value: `0x06`
- Source: `ICM-42688-P_datasheet.pdf` p.77

### `GYRO_ACCEL_CONFIG0` (`0x52`, Bank 0)

- Shared UI filter bandwidth control
- Default reset value: `0x11`
- Keep defaults until the basic data path is proven stable
- Source: `ICM-42688-P_datasheet.pdf` p.79

### `WHO_AM_I` (`0x75`, Bank 0)

- Read-only device ID
- Expected value: `0x47`
- This is not the same as I2C address `0x69`
- Source: `ICM-42688-P_datasheet.pdf` p.87

## Sensor Data Registers

- Burst read from `TEMP_DATA1` (`0x1D`) for 14 bytes:
  - temp
  - accel x/y/z
  - gyro x/y/z
- This is the simplest initial read path
- Source: `ICM-42688-P_datasheet.pdf` p.64-p.69

## Sensitivity Quick Reference

- Gyro LSB per dps:
  - `±2000`: `16.4`
  - `±1000`: `32.8`
  - `±500`: `65.5`
  - `±250`: `131`
- Accel LSB per g:
  - `±16g`: `2048`
  - `±8g`: `4096`
  - `±4g`: `8192`
  - `±2g`: `16384`
- Source: `ICM-42688-P_datasheet.pdf` p.11-p.12
