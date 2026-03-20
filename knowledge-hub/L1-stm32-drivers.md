# L1 — STM32 Sensor Drivers

> Path: `firmware/stm32/drivers/` | Language: C | Status: ✅ Complete (bugs fixed 2026-03-19)

---

## Purpose

Hardware drivers for the three onboard sensors. All use the HAL layer; no direct register access outside driver files.

---

## Files

| File | Sensor | Interface |
|------|--------|-----------|
| `icm42688.h/.c` | ICM-42688-P — 6-axis IMU (accel + gyro) | I2C1 |
| `lps22hb.h/.c` | LPS22HBTR — barometer (pressure + temp) | SPI3 |
| `qmc5883p.h/.c` | QMC5883P — 3-axis magnetometer | I2C1 |

---

## ICM-42688-P (icm42688.h)

```
I2C addr: 0x69  (AD0 pulled to VCC — was 0x68, fixed Task 301)
WHO_AM_I: 0x47
```

Key data types:
```c
typedef struct {
    float accel_x, accel_y, accel_z;   // g
    float gyro_x, gyro_y, gyro_z;      // deg/s
    float temperature;                  // °C
} icm42688_data_t;
```

Public API:
```c
hal_status_t icm42688_init(void);
hal_status_t icm42688_read(icm42688_data_t *data);
hal_status_t icm42688_who_am_i(uint8_t *id);
```

Config: Accel ±16g, Gyro ±2000dps, ODR 1kHz (set in `icm42688_init`).

---

## LPS22HBTR (lps22hb.h)

```
SPI3: PA15=CS, PB3=SCK, PB4=MISO, PB5=MOSI — Mode 0
WHO_AM_I: 0xB3  (LPS22HH variant confirmed by hardware)
SPI auto-increment: bit6=1 on register address for multi-byte reads (fixed Task 302)
```

Key data types:
```c
typedef struct {
    float pressure;      // hPa
    float temperature;   // °C
} lps22hb_data_t;
```

Public API:
```c
hal_status_t lps22hb_init(void);
hal_status_t lps22hb_read(lps22hb_data_t *data);
hal_status_t lps22hb_who_am_i(uint8_t *id);
```

⚠️ Multi-byte SPI read must use `0xC0 | reg` (READ bit + AUTO_INC bit). Using `0x80 | reg` alone reads only first byte — causes wrong pressure (was 283 hPa, fixed Task 302).

---

## QMC5883P (qmc5883p.h)

```
I2C addr: 0x2C  (7-bit)
CHIP_ID reg: 0x00, default value 0x80
```

Key data types:
```c
typedef struct {
    int16_t mag_x, mag_y, mag_z;   // raw counts (16-bit signed, little-endian)
    float temperature;              // °C
} qmc5883p_data_t;
```

Public API:
```c
hal_status_t qmc5883p_init(void);
hal_status_t qmc5883p_read(qmc5883p_data_t *data);
hal_status_t qmc5883p_who_am_i(uint8_t *id);
```

⚠️ Init sequence must:
1. Write `CTRL2_SET` operation (SET/RESET pulse)
2. Wait 10ms
3. Poll DRDY bit up to 50ms before first read

Skipping this causes Y/Z axes to read 0 (was broken, fixed Task 303).

---

## Sensor Bus Sharing

Both ICM-42688-P and QMC5883P share I2C1 (PB6/PB7). LPS22HBTR is on SPI3. No bus conflicts if drivers are called sequentially (no concurrent access).
