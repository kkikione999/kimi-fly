# ICM-42688-P Bring-Up Recipes

This file is intentionally opinionated for this repository.

## Recipe 1: Minimal Safe Bring-Up

Goal: prove the part is wired correctly and returns stable accel/gyro data.

1. Use project truth from `hardware-docs/pinout.md`.
2. Open `I2C1` and target address `0x69`.
3. Read `WHO_AM_I (0x75)` and require `0x47`.
4. Trigger software reset through `DEVICE_CONFIG.SOFT_RESET_CONFIG`.
5. Wait at least `1 ms`.
6. Enable accel and gyro in `PWR_MGMT0` with both in `Low Noise`.
7. Wait at least `10 ms` before first data read. For gyro stability, budget `45 ms`.
8. Program:
   - `GYRO_CONFIG0 = 0x06` for `±2000 dps, 1kHz`
   - `ACCEL_CONFIG0 = 0x26` for `±8g, 1kHz`
9. Burst-read 14 bytes starting at `TEMP_DATA1 (0x1D)`.
10. Apply the runtime axis mapping from the project SSOT before feeding AHRS.

Notes:

- `ACCEL_CONFIG0 = 0x26` comes from `FS_SEL = 001` and `ODR = 0110`
- Do not add FIFO or interrupt handling until this path is confirmed

## Recipe 2: Data-Ready Interrupt Bring-Up

Goal: add `INT1 -> PC13` only after polling reads work.

1. Confirm polling reads are stable first.
2. Configure `INT_CONFIG` for the required polarity/drive mode for the board.
3. Configure interrupt source registers so `UI_DRDY_INT1_EN` is enabled.
4. Change `INT_CONFIG1.INT_ASYNC_RESET` from default `1` to `0`.
5. Read `INT_STATUS` and use `DATA_RDY_INT`.

Warnings:

- `INT_STATUS` is read-to-clear.
- If interrupt behavior looks wrong, check the datasheet use note on `INT_ASYNC_RESET`.

## Recipe 3: FIFO Bring-Up

Goal: lower MCU bus traffic after the basic path is stable.

1. Start in bypass mode.
2. Enable FIFO contents explicitly.
3. Use `FIFO_COUNTH/FIFO_COUNTL` before reading `FIFO_DATA`.
4. Use `SIGNAL_PATH_RESET.FIFO_FLUSH` when recovering from bad FIFO state.
5. Keep in mind:
   - FIFO is `2 KB`
   - 20-bit FIFO mode constrains valid FSR operation

Warnings:

- FIFO adds recovery complexity.
- Do not use FIFO as the first driver milestone.

## Recipe 4: Converting Raw Data

- Gyro physical units:
  - `dps = raw / lsb_per_dps`
- Accel physical units:
  - `g = raw / lsb_per_g`
- Temperature:
  - `temp_c = raw_temp / 132.48 + 25`

## Project-Specific Integration Reminder

Driver output axes are not body axes.
Before attitude estimation or control:

- `body_x = +imu_y`
- `body_y = -imu_x`
- `body_z = +imu_z`

This mapping is bench-verified project truth and must not be re-inferred from old code.
