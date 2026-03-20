# L1 — STM32 HAL Layer

> Path: `firmware/stm32/hal/` | Language: C | Status: ✅ Complete

---

## Purpose

Custom HAL wrappers for STM32F411CEU6 peripherals. Two build modes:
- `env:flight` — standalone (no STM32Cube HAL stdlib, custom register access)
- `env:stm32f411` — with `USE_HAL_DRIVER` (uses STM32Cube HAL)

Headers use `#ifndef USE_HAL_DRIVER` guards to avoid enum conflicts with STM32 HAL.

---

## Files

| File | Role |
|------|------|
| `hal_common.h` | Shared types (`hal_status_t`, `HAL_OK`, `HAL_ERROR`), include gate for `stm32f4xx_hal.h` |
| `gpio.h/.c` | GPIO init, read, write, toggle. Enums: `gpio_port_t`, `HAL_GPIO_PIN_*`, `HAL_GPIO_MODE_*` |
| `uart.h/.c` | UART init, TX, RX, interrupt. USART2 @ PA2/PA3, 115200 baud (default) |
| `i2c.h/.c` | I2C1 master read/write. PB6=SCL, PB7=SDA, 400kHz fast mode |
| `spi.h/.c` | SPI3 master. PA15=CS(SW), PB3=SCK, PB4=MISO, PB5=MOSI, Mode0 |
| `pwm.h/.c` | TIM-based PWM. Motor API: `motor_set_throttle()`, `motors_init()` |

---

## Public API Summary

### gpio.h
```c
hal_status_t gpio_init(gpio_port_t port, uint16_t pin, HAL_GPIO_MODE_t mode, HAL_GPIO_PUPD_t pupd);
void         gpio_write(gpio_port_t port, uint16_t pin, uint8_t state);
uint8_t      gpio_read(gpio_port_t port, uint16_t pin);
void         gpio_toggle(gpio_port_t port, uint16_t pin);
```

### uart.h
```c
hal_status_t uart_init(uart_instance_t instance, uint32_t baudrate);
hal_status_t uart_transmit(uart_instance_t instance, const uint8_t *data, uint16_t len, uint32_t timeout);
hal_status_t uart_receive(uart_instance_t instance, uint8_t *data, uint16_t len, uint32_t timeout);
// Interrupt-driven RX buffer support
```
- Instance `UART_INSTANCE_2` → USART2 (PA2/PA3, ESP32 comms)
- Instance `UART_INSTANCE_1` implied USART1 (PA9/PA10, debug)

### i2c.h
```c
hal_status_t i2c_init(i2c_speed_t speed);  // I2C_SPEED_FAST=400kHz
hal_status_t i2c_write(uint8_t dev_addr, uint8_t reg, const uint8_t *data, uint16_t len);
hal_status_t i2c_read(uint8_t dev_addr, uint8_t reg, uint8_t *data, uint16_t len);
```
- Device addresses: ICM42688=0x69, QMC5883P=0x2C (defined in driver headers)

### spi.h
```c
hal_status_t spi_init(spi_periph_t periph, spi_cpol_t cpol, spi_cpha_t cpha, spi_baudrate_prescaler_t prescaler);
hal_status_t spi_transfer(spi_periph_t periph, const uint8_t *tx, uint8_t *rx, uint16_t len);
hal_status_t spi_cs_low(gpio_port_t port, uint16_t pin);
hal_status_t spi_cs_high(gpio_port_t port, uint16_t pin);
```
- SPI_PERIPH_3 used for barometer (LPS22HBTR)

### pwm.h — Motor API
```c
hal_status_t motors_init(void);
hal_status_t motor_set_throttle(motor_id_t motor, uint16_t throttle_1000); // 0–999
hal_status_t motors_set_all(uint16_t m1, uint16_t m2, uint16_t m3, uint16_t m4);
hal_status_t motors_stop_all(void);
```
Motor mapping:
| Motor | Timer | Pin |
|-------|-------|-----|
| MOTOR_1 | TIM1_CH1 | PA8 |
| MOTOR_2 | TIM1_CH4 | PA11 |
| MOTOR_3 | TIM3_CH4 | PB1 |
| MOTOR_4 | TIM2_CH3 | PB10 |

PWM: 42kHz, duty 0–1000 (0–100%)

---

## Known Issues / Constraints

- `env:flight` does NOT link STM32Cube HAL — all register access must be done via custom code or CMSIS.
- GPIO enum prefix changed from `GPIO_MODE_*` → `HAL_GPIO_MODE_*` to avoid collision (fixed in Task 201).
- `uart.h` previously had `hal_state_t` typo — fixed to `hal_status_t` (Task 202).
