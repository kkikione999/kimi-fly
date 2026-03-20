# Tech Debt Ledger

> Sync of `docs/plans/tech-debt-tracker.md` — 2026-03-19

---

## Open

| ID | Issue | Priority | Notes |
|----|-------|----------|-------|
| TD-001 | STM32–ESP32 UART physical wiring unconfirmed | P1 | Both RX=0 in test. Expected cross: STM32_TX(PA2)→ESP32_RX(GPIO1), ESP32_TX(GPIO0)→STM32_RX(PA3). Task 304 adds SW diagnostic. |

---

## Resolved

| ID | Issue | Resolution |
|----|-------|------------|
| TD-R01 | ICM42688 I2C address 0x68 (should be 0x69) | Fixed in Task 301 — icm42688.h addr→0x69U |
| TD-R02 | LPS22HB pressure 283hPa (SPI multi-byte read broken) | Fixed in Task 302 — use 0xC0\|reg for auto-increment |
| TD-R03 | QMC5883P Y/Z axes read 0 | Fixed in Task 303 — added SET op + 10ms + 50ms DRDY poll |
| TD-R04 | GPIO enum naming conflict (GPIO_MODE_* vs HAL) | Fixed Task 201 — renamed to HAL_GPIO_MODE_* |
| TD-R05 | uart.h `hal_state_t` typo | Fixed Task 202 — changed to `hal_status_t` |
| TD-R06 | HAL layer STM32Cube compatibility | Fixed — hal_common.h uses USE_HAL_DRIVER conditional |
