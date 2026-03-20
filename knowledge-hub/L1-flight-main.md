# L1 — STM32 Flight Main

> Path: `firmware/stm32/main/` | Language: C | Status: ✅ Complete

---

## Files

| File | Role |
|------|------|
| `flight_main.h/.c` | 1kHz main control loop integrating all subsystems |
| `flight_entry.c` | PlatformIO entry point for `env:flight` (calls `flight_main_run()`) |
| `flight_compat.h` | Compatibility shims (⚠️ intent unclear) |
| `sensor_test.h/.c` | Hardware validation test suite (sensors, UART, motors) |
| `uart_comm_test.c` | UART loopback / echo test for ESP32 comms verification |
| `verify_hardware.c` | One-shot hardware bring-up verification (WHO_AM_I checks, etc.) |

`firmware/stm32/src/main.c` — PlatformIO entry for `env:stm32f411` (debug builds with USE_HAL_DRIVER).

---

## Main Loop (flight_main.h)

System states:
```c
SYS_STATE_INIT → SYS_STATE_CALIBRATION → SYS_STATE_STANDBY ↔ SYS_STATE_ACTIVE
                                                                    ↓ error
                                                               SYS_STATE_ERROR
```

Timing:
```c
MAIN_LOOP_RATE_HZ       1000    // 1kHz main loop
WIFI_TASK_INTERVAL_MS   5       // 200Hz WiFi processing
TELEMETRY_INTERVAL_MS   20      // 50Hz telemetry TX
STATUS_INTERVAL_MS      1000    // 1Hz status broadcast
SENSOR_INIT_RETRY_MAX   3       // retries on init failure
CALIBRATION_SAMPLES     1000    // gyro/accel bias samples
```

Error codes:
```c
ERR_NONE, ERR_IMU_INIT, ERR_BARO_INIT, ERR_MAG_INIT,
ERR_COMM_INIT, ERR_FLIGHT_CTRL_INIT
```

Public API:
```c
hal_status_t flight_main_init(void);
void         flight_main_run(void);     // never returns
system_state_t flight_main_get_state(void);
```

---

## Startup Sequence

```
1. HAL init (clocks, GPIO, UART, I2C, SPI, PWM)
2. Sensor init with retry (ICM42688, LPS22HB, QMC5883P)
3. WiFi command handler init (UART2 to ESP32)
4. Sensor calibration (1000-sample gyro/accel bias)
5. Flight controller init
6. → SYS_STATE_STANDBY
7. Main loop @ 1kHz:
   a. Read IMU (ICM42688)
   b. AHRS update (Mahony filter)
   c. Flight controller update (PID cascade)
   d. Motor output
   e. Every 5ms: process WiFi commands
   f. Every 20ms: send telemetry
   g. Every 1000ms: send status
```

---

## Test / Diagnostic Builds

- `sensor_test.c` — cycles through all sensors, prints WHO_AM_I and live data to UART1 (debug console)
- `uart_comm_test.c` — sends known frames on USART2, checks loopback to verify ESP32 wiring
- `verify_hardware.c` — one-time WHO_AM_I pass/fail for all three sensors, used in CI/bring-up
