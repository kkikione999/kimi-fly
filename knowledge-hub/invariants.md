# Business Invariants

> Extracted from hardware docs, user intent, and code review — 2026-03-19

---

## Hardware (Non-Negotiable)

1. **`hardware-docs/pinout.md` is the SSOT** — all pin assignments must match this file. Do not derive pin assignments from code; verify against pinout.md.

2. **Hardware is not faulty** — the user has stated: *"硬件无问题，已被验证过。如有问题一定是代码的问题。"* Always look for software/config errors first.

3. **ICM-42688-P I2C address is 0x69** — AD0 pulled to VCC. Using 0x68 will fail WHO_AM_I.

4. **LPS22HBTR SPI multi-byte read requires `0xC0 | reg`** — bit7=READ, bit6=AUTO_INC. Using `0x80 | reg` reads only first byte.

5. **QMC5883P requires SET operation on init** — must write CTRL2_SET + 10ms wait + 50ms DRDY poll. Skipping causes Y/Z axes to read 0.

---

## Architecture

6. **STM32 is flight controller, ESP32 is transparent bridge** — ESP32 must not interpret or filter protocol frames.

7. **1kHz main loop is the hard real-time constraint** — no blocking calls in the main flight loop. WiFi processing is limited to every 5ms slot.

8. **Motor throttle range is 0–999** — `MOTOR_MIN_THROTTLE=0`, `MOTOR_MAX_THROTTLE=999`, `MOTOR_IDLE_THROTTLE=50` (when armed). Never exceed 999.

9. **Motors only run when ARMED** — `SYS_STATE_ACTIVE`. Any error transitions to `SYS_STATE_ERROR` which stops motors.

---

## Development Process

10. **USB power = safe development mode** — under USB power, motors cannot generate enough thrust to fly. This is intentional for development safety.

11. **Build environments are separate** — `env:flight` does NOT have STM32Cube HAL stdlib; `env:stm32f411` does. Do not mix includes between environments.

12. **Protocol frame header is 0xAA55** — big-endian, 2 bytes. Any parser must check this before processing.
