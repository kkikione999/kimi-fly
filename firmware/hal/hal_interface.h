/*
 * Copyright (c) 2026 kimi-fly Project
 * SPDX-License-Identifier: MIT
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/**
 * @file hal_interface.h
 * @brief Hardware abstraction layer interface definitions
 *
 * This file defines the unified HAL interfaces using function pointers
 * to achieve platform abstraction. Platform-specific implementations
 * provide concrete function implementations.
 */

#ifndef HAL_INTERFACE_H
#define HAL_INTERFACE_H

#include "hal_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup HAL_GPIO_INTERFACE GPIO Interface
 * @{
 */

/**
 * @brief GPIO configuration structure
 */
typedef struct {
    uint32_t pin;               /* Pin number (platform-specific) */
    hal_gpio_dir_t dir;         /* Pin direction */
    hal_gpio_pull_t pull;       /* Pull-up/down configuration */
    hal_gpio_state_t init_val;  /* Initial output value (if output) */
} hal_gpio_cfg_t;

/**
 * @brief GPIO interface structure
 */
typedef struct {
    /**
     * @brief Initialize a GPIO pin
     *
     * @param cfg GPIO configuration
     * @param[out] handle Handle to initialized GPIO instance
     * @return 0 on success, negative error code on failure
     */
    int (*init)(const hal_gpio_cfg_t *cfg, hal_handle_t *handle);

    /**
     * @brief Set GPIO output state
     *
     * @param handle GPIO handle
     * @param state State to set (HAL_GPIO_LOW or HAL_GPIO_HIGH)
     * @return 0 on success, negative error code on failure
     */
    int (*set)(hal_handle_t handle, hal_gpio_state_t state);

    /**
     * @brief Get GPIO input state
     *
     * @param handle GPIO handle
     * @param[out] state Current pin state
     * @return 0 on success, negative error code on failure
     */
    int (*get)(hal_handle_t handle, hal_gpio_state_t *state);

    /**
     * @brief Toggle GPIO output state
     *
     * @param handle GPIO handle
     * @return 0 on success, negative error code on failure
     */
    int (*toggle)(hal_handle_t handle);

    /**
     * @brief Deinitialize GPIO pin
     *
     * @param handle GPIO handle
     * @return 0 on success, negative error code on failure
     */
    int (*deinit)(hal_handle_t handle);
} hal_gpio_interface_t;

/**
 * @brief Register GPIO interface implementation
 *
 * @param iface Pointer to GPIO interface implementation
 * @return 0 on success, negative error code on failure
 */
int hal_gpio_register(const hal_gpio_interface_t *iface);

/**
 * @brief Get registered GPIO interface
 *
 * @return Pointer to GPIO interface, or NULL if not registered
 */
const hal_gpio_interface_t *hal_gpio_get_interface(void);

/** @} */

/**
 * @defgroup HAL_UART_INTERFACE UART Interface
 * @{
 */

/**
 * @brief UART configuration structure
 */
typedef struct {
    uint32_t baudrate;              /* Baud rate (e.g., 115200) */
    hal_uart_data_bits_t data_bits; /* Number of data bits */
    hal_uart_stop_bits_t stop_bits; /* Number of stop bits */
    hal_uart_parity_t parity;       /* Parity mode */
} hal_uart_cfg_t;

/**
 * @brief UART interface structure
 */
typedef struct {
    /**
     * @brief Initialize UART peripheral
     *
     * @param port UART port number (platform-specific)
     * @param cfg UART configuration
     * @param[out] handle Handle to initialized UART instance
     * @return 0 on success, negative error code on failure
     */
    int (*init)(uint8_t port, const hal_uart_cfg_t *cfg, hal_handle_t *handle);

    /**
     * @brief Send data over UART
     *
     * @param handle UART handle
     * @param data Data buffer to send
     * @param len Number of bytes to send
     * @param timeout_ms Timeout in milliseconds (0 for non-blocking)
     * @return Number of bytes sent, or negative error code
     */
    int (*send)(hal_handle_t handle, const uint8_t *data, size_t len,
                uint32_t timeout_ms);

    /**
     * @brief Receive data from UART
     *
     * @param handle UART handle
     * @param[out] data Buffer to receive data
     * @param len Maximum number of bytes to receive
     * @param timeout_ms Timeout in milliseconds (0 for non-blocking)
     * @return Number of bytes received, or negative error code
     */
    int (*recv)(hal_handle_t handle, uint8_t *data, size_t len,
                uint32_t timeout_ms);

    /**
     * @brief Flush UART buffers
     *
     * @param handle UART handle
     * @return 0 on success, negative error code on failure
     */
    int (*flush)(hal_handle_t handle);

    /**
     * @brief Deinitialize UART peripheral
     *
     * @param handle UART handle
     * @return 0 on success, negative error code on failure
     */
    int (*deinit)(hal_handle_t handle);
} hal_uart_interface_t;

/**
 * @brief Register UART interface implementation
 *
 * @param iface Pointer to UART interface implementation
 * @return 0 on success, negative error code on failure
 */
int hal_uart_register(const hal_uart_interface_t *iface);

/**
 * @brief Get registered UART interface
 *
 * @return Pointer to UART interface, or NULL if not registered
 */
const hal_uart_interface_t *hal_uart_get_interface(void);

/** @} */

/**
 * @defgroup HAL_SPI_INTERFACE SPI Interface
 * @{
 */

/**
 * @brief SPI configuration structure
 */
typedef struct {
    uint32_t clock_hz;              /* Clock frequency in Hz */
    hal_spi_mode_t mode;            /* SPI mode (CPOL/CPHA) */
    hal_spi_bit_order_t bit_order;  /* MSB or LSB first */
    uint32_t cs_pin;                /* Chip select pin (platform-specific) */
} hal_spi_cfg_t;

/**
 * @brief SPI interface structure
 */
typedef struct {
    /**
     * @brief Initialize SPI peripheral
     *
     * @param bus SPI bus number (platform-specific)
     * @param cfg SPI configuration
     * @param[out] handle Handle to initialized SPI instance
     * @return 0 on success, negative error code on failure
     */
    int (*init)(uint8_t bus, const hal_spi_cfg_t *cfg, hal_handle_t *handle);

    /**
     * @brief Transfer data over SPI (full-duplex)
     *
     * Data is simultaneously transmitted from tx_buf and received into rx_buf.
     * If tx_buf is NULL, zeros are transmitted. If rx_buf is NULL, received
     * data is discarded.
     *
     * @param handle SPI handle
     * @param tx_buf Transmit buffer (can be NULL)
     * @param rx_buf Receive buffer (can be NULL)
     * @param len Number of bytes to transfer
     * @param timeout_ms Timeout in milliseconds
     * @return 0 on success, negative error code on failure
     */
    int (*transfer)(hal_handle_t handle, const uint8_t *tx_buf, uint8_t *rx_buf,
                    size_t len, uint32_t timeout_ms);

    /**
     * @brief Assert/deassert chip select
     *
     * @param handle SPI handle
     * @param assert true to assert CS (low), false to deassert (high)
     * @return 0 on success, negative error code on failure
     */
    int (*cs_control)(hal_handle_t handle, bool assert);

    /**
     * @brief Deinitialize SPI peripheral
     *
     * @param handle SPI handle
     * @return 0 on success, negative error code on failure
     */
    int (*deinit)(hal_handle_t handle);
} hal_spi_interface_t;

/**
 * @brief Register SPI interface implementation
 *
 * @param iface Pointer to SPI interface implementation
 * @return 0 on success, negative error code on failure
 */
int hal_spi_register(const hal_spi_interface_t *iface);

/**
 * @brief Get registered SPI interface
 *
 * @return Pointer to SPI interface, or NULL if not registered
 */
const hal_spi_interface_t *hal_spi_get_interface(void);

/** @} */

/**
 * @defgroup HAL_I2C_INTERFACE I2C Interface
 * @{
 */

/**
 * @brief I2C configuration structure
 */
typedef struct {
    hal_i2c_speed_t speed;  /* Bus speed */
} hal_i2c_cfg_t;

/**
 * @brief I2C interface structure
 */
typedef struct {
    /**
     * @brief Initialize I2C peripheral
     *
     * @param bus I2C bus number (platform-specific)
     * @param cfg I2C configuration
     * @param[out] handle Handle to initialized I2C instance
     * @return 0 on success, negative error code on failure
     */
    int (*init)(uint8_t bus, const hal_i2c_cfg_t *cfg, hal_handle_t *handle);

    /**
     * @brief Write data to I2C device
     *
     * @param handle I2C handle
     * @param addr 7-bit device address
     * @param data Data buffer to write
     * @param len Number of bytes to write
     * @param timeout_ms Timeout in milliseconds
     * @return 0 on success, negative error code on failure
     */
    int (*write)(hal_handle_t handle, uint8_t addr, const uint8_t *data,
                 size_t len, uint32_t timeout_ms);

    /**
     * @brief Read data from I2C device
     *
     * @param handle I2C handle
     * @param addr 7-bit device address
     * @param[out] data Buffer to receive data
     * @param len Number of bytes to read
     * @param timeout_ms Timeout in milliseconds
     * @return 0 on success, negative error code on failure
     */
    int (*read)(hal_handle_t handle, uint8_t addr, uint8_t *data,
                size_t len, uint32_t timeout_ms);

    /**
     * @brief Write then read (combined transaction)
     *
     * Performs a write followed by a repeated start and read.
     * Common for reading registers from I2C devices.
     *
     * @param handle I2C handle
     * @param addr 7-bit device address
     * @param tx_buf Write buffer
     * @param tx_len Number of bytes to write
     * @param rx_buf Read buffer
     * @param rx_len Number of bytes to read
     * @param timeout_ms Timeout in milliseconds
     * @return 0 on success, negative error code on failure
     */
    int (*write_read)(hal_handle_t handle, uint8_t addr,
                      const uint8_t *tx_buf, size_t tx_len,
                      uint8_t *rx_buf, size_t rx_len,
                      uint32_t timeout_ms);

    /**
     * @brief Deinitialize I2C peripheral
     *
     * @param handle I2C handle
     * @return 0 on success, negative error code on failure
     */
    int (*deinit)(hal_handle_t handle);
} hal_i2c_interface_t;

/**
 * @brief Register I2C interface implementation
 *
 * @param iface Pointer to I2C interface implementation
 * @return 0 on success, negative error code on failure
 */
int hal_i2c_register(const hal_i2c_interface_t *iface);

/**
 * @brief Get registered I2C interface
 *
 * @return Pointer to I2C interface, or NULL if not registered
 */
const hal_i2c_interface_t *hal_i2c_get_interface(void);

/** @} */

/**
 * @defgroup HAL_PWM_INTERFACE PWM Interface
 * @{
 */

/**
 * @brief PWM configuration structure
 */
typedef struct {
    uint32_t freq_hz;           /* PWM frequency in Hz */
    hal_pwm_polarity_t pol;     /* PWM polarity */
    float init_duty;            /* Initial duty cycle [0.0, 1.0] */
} hal_pwm_cfg_t;

/**
 * @brief PWM interface structure
 */
typedef struct {
    /**
     * @brief Initialize PWM channel
     *
     * @param channel PWM channel number (platform-specific)
     * @param cfg PWM configuration
     * @param[out] handle Handle to initialized PWM instance
     * @return 0 on success, negative error code on failure
     */
    int (*init)(uint8_t channel, const hal_pwm_cfg_t *cfg, hal_handle_t *handle);

    /**
     * @brief Set PWM duty cycle
     *
     * @param handle PWM handle
     * @param duty Duty cycle [0.0, 1.0]
     * @return 0 on success, negative error code on failure
     */
    int (*set_duty)(hal_handle_t handle, float duty);

    /**
     * @brief Set PWM frequency
     *
     * @param handle PWM handle
     * @param freq_hz Frequency in Hz
     * @return 0 on success, negative error code on failure
     */
    int (*set_freq)(hal_handle_t handle, uint32_t freq_hz);

    /**
     * @brief Enable/disable PWM output
     *
     * @param handle PWM handle
     * @param enable true to enable, false to disable
     * @return 0 on success, negative error code on failure
     */
    int (*enable)(hal_handle_t handle, bool enable);

    /**
     * @brief Deinitialize PWM channel
     *
     * @param handle PWM handle
     * @return 0 on success, negative error code on failure
     */
    int (*deinit)(hal_handle_t handle);
} hal_pwm_interface_t;

/**
 * @brief Register PWM interface implementation
 *
 * @param iface Pointer to PWM interface implementation
 * @return 0 on success, negative error code on failure
 */
int hal_pwm_register(const hal_pwm_interface_t *iface);

/**
 * @brief Get registered PWM interface
 *
 * @return Pointer to PWM interface, or NULL if not registered
 */
const hal_pwm_interface_t *hal_pwm_get_interface(void);

/** @} */

/**
 * @defgroup HAL_SYSTEM_INTERFACE System Interface
 * @{
 */

/**
 * @brief System interface structure
 */
typedef struct {
    /**
     * @brief Get current tick count in milliseconds
     *
     * This value wraps around after approximately 49.7 days.
     *
     * @return Milliseconds since system start
     */
    uint32_t (*get_tick_ms)(void);

    /**
     * @brief Delay for specified milliseconds
     *
     * This is a blocking delay. For non-blocking delays, use get_tick_ms().
     *
     * @param ms Milliseconds to delay
     */
    void (*delay_ms)(uint32_t ms);

    /**
     * @brief Delay for specified microseconds
     *
     * This is a blocking delay. Should only be used for short delays
     * (< 1000us) to avoid impacting system responsiveness.
     *
     * @param us Microseconds to delay
     */
    void (*delay_us)(uint32_t us);

    /**
     * @brief Disable interrupts
     *
     * @return Previous interrupt state (for restore)
     */
    uint32_t (*disable_irq)(void);

    /**
     * @brief Restore interrupt state
     *
     * @param state State returned by disable_irq()
     */
    void (*restore_irq)(uint32_t state);

    /**
     * @brief Enter critical section
     *
     * Disables interrupts and returns previous state.
     * Must be paired with exit_critical().
     *
     * @return Previous interrupt state
     */
    uint32_t (*enter_critical)(void);

    /**
     * @brief Exit critical section
     *
     * Restores interrupt state from enter_critical().
     *
     * @param state State returned by enter_critical()
     */
    void (*exit_critical)(uint32_t state);

    /**
     * @brief Reset the system
     *
     * Performs a software reset of the microcontroller.
     */
    void (*reset)(void);

    /**
     * @brief Get unique device ID
     *
     * @param[out] id Buffer to store device ID
     * @param len Buffer length (platform-specific ID length)
     * @return Number of bytes written, or negative error code
     */
    int (*get_unique_id)(uint8_t *id, size_t len);
} hal_system_interface_t;

/**
 * @brief Register system interface implementation
 *
 * @param iface Pointer to system interface implementation
 * @return 0 on success, negative error code on failure
 */
int hal_system_register(const hal_system_interface_t *iface);

/**
 * @brief Get registered system interface
 *
 * @return Pointer to system interface, or NULL if not registered
 */
const hal_system_interface_t *hal_system_get_interface(void);

/** @} */

/**
 * @defgroup HAL_INIT HAL Initialization
 * @{
 */

/**
 * @brief Initialize HAL layer
 *
 * Must be called before using any HAL interfaces. This function
 * initializes the HAL framework but does not initialize individual
 * peripherals.
 *
 * @return 0 on success, negative error code on failure
 */
int hal_init(void);

/**
 * @brief Deinitialize HAL layer
 *
 * @return 0 on success, negative error code on failure
 */
int hal_deinit(void);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* HAL_INTERFACE_H */
