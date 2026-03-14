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
 * @file hal_common.h
 * @brief Common types, error codes, and utility macros for HAL layer
 *
 * This file provides platform-independent definitions used throughout
 * the hardware abstraction layer.
 */

#ifndef HAL_COMMON_H
#define HAL_COMMON_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup HAL_VERSION Version Information
 * @{
 */
#define HAL_VERSION_MAJOR   0
#define HAL_VERSION_MINOR   1
#define HAL_VERSION_PATCH   0
/** @} */

/**
 * @defgroup HAL_ERROR_CODES Error Codes
 * @{
 */
#define HAL_OK              0
#define HAL_ERR_PARAM       -1
#define HAL_ERR_TIMEOUT     -2
#define HAL_ERR_IO          -3
#define HAL_ERR_BUSY        -4
#define HAL_ERR_NOMEM       -5
#define HAL_ERR_NOTSUPP     -6
/** @} */

/**
 * @brief HAL error code type
 */
typedef int hal_err_t;

/**
 * @defgroup HAL_UTILITY_MACROS Utility Macros
 * @{
 */

/**
 * @brief Compile-time assertion macro
 *
 * Generates a compile error if the condition is false.
 *
 * @param cond Condition to check at compile time
 * @param msg  Message to display on failure (used in typedef name)
 */
#define STATIC_ASSERT(cond, msg) _Static_assert(cond, #msg)

/**
 * @brief Get the number of elements in an array
 *
 * @param arr Array variable
 * @return Number of elements in the array
 */
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

/**
 * @brief Mark a parameter as intentionally unused
 *
 * Use this to suppress compiler warnings about unused parameters.
 *
 * @param x Parameter to mark as unused
 */
#define UNUSED(x) ((void)(x))

/**
 * @brief Get the minimum of two values
 *
 * @param a First value
 * @param b Second value
 * @return The smaller of a and b
 */
#define MIN(a, b) ((a) < (b) ? (a) : (b))

/**
 * @brief Get the maximum of two values
 *
 * @param a First value
 * @param b Second value
 * @return The larger of a and b
 */
#define MAX(a, b) ((a) > (b) ? (a) : (b))

/**
 * @brief Clamp a value to a range
 *
 * @param val Value to clamp
 * @param min Minimum allowed value
 * @param max Maximum allowed value
 * @return val clamped to [min, max]
 */
#define CLAMP(val, min, max) ((val) < (min) ? (min) : ((val) > (max) ? (max) : (val)))

/**
 * @brief Align a value up to a power-of-2 boundary
 *
 * @param val Value to align
 * @param align Alignment boundary (must be power of 2)
 * @return Value aligned up to boundary
 */
#define ALIGN_UP(val, align) (((val) + ((align) - 1)) & ~((align) - 1))

/**
 * @brief Align a value down to a power-of-2 boundary
 *
 * @param val Value to align
 * @param align Alignment boundary (must be power of 2)
 * @return Value aligned down to boundary
 */
#define ALIGN_DOWN(val, align) ((val) & ~((align) - 1))

/** @} */

/**
 * @defgroup HAL_TYPES Common Types
 * @{
 */

/**
 * @brief GPIO pin state
 */
typedef enum {
    HAL_GPIO_LOW = 0,
    HAL_GPIO_HIGH = 1
} hal_gpio_state_t;

/**
 * @brief GPIO pin direction
 */
typedef enum {
    HAL_GPIO_DIR_INPUT = 0,
    HAL_GPIO_DIR_OUTPUT = 1
} hal_gpio_dir_t;

/**
 * @brief GPIO pull configuration
 */
typedef enum {
    HAL_GPIO_PULL_NONE = 0,
    HAL_GPIO_PULL_UP,
    HAL_GPIO_PULL_DOWN
} hal_gpio_pull_t;

/**
 * @brief SPI mode (CPOL/CPHA combination)
 */
typedef enum {
    HAL_SPI_MODE_0 = 0,  /* CPOL=0, CPHA=0 */
    HAL_SPI_MODE_1,      /* CPOL=0, CPHA=1 */
    HAL_SPI_MODE_2,      /* CPOL=1, CPHA=0 */
    HAL_SPI_MODE_3       /* CPOL=1, CPHA=1 */
} hal_spi_mode_t;

/**
 * @brief SPI bit order
 */
typedef enum {
    HAL_SPI_MSB_FIRST = 0,
    HAL_SPI_LSB_FIRST = 1
} hal_spi_bit_order_t;

/**
 * @brief I2C clock speed
 */
typedef enum {
    HAL_I2C_SPEED_STANDARD = 100000,   /* 100 kHz */
    HAL_I2C_SPEED_FAST = 400000,       /* 400 kHz */
    HAL_I2C_SPEED_FAST_PLUS = 1000000  /* 1 MHz */
} hal_i2c_speed_t;

/**
 * @brief UART data bits
 */
typedef enum {
    HAL_UART_DATA_BITS_8 = 0,
    HAL_UART_DATA_BITS_9
} hal_uart_data_bits_t;

/**
 * @brief UART stop bits
 */
typedef enum {
    HAL_UART_STOP_BITS_1 = 0,
    HAL_UART_STOP_BITS_2
} hal_uart_stop_bits_t;

/**
 * @brief UART parity
 */
typedef enum {
    HAL_UART_PARITY_NONE = 0,
    HAL_UART_PARITY_EVEN,
    HAL_UART_PARITY_ODD
} hal_uart_parity_t;

/**
 * @brief PWM polarity
 */
typedef enum {
    HAL_PWM_POLARITY_NORMAL = 0,   /* High during duty cycle */
    HAL_PWM_POLARITY_INVERTED      /* Low during duty cycle */
} hal_pwm_polarity_t;

/**
 * @brief Handle type for HAL instances
 *
 * This opaque handle is used to reference HAL driver instances.
 * Platform-specific implementations define the actual structure.
 */
typedef struct hal_handle *hal_handle_t;

/**
 * @brief Invalid handle value
 */
#define HAL_INVALID_HANDLE NULL

/** @} */

/**
 * @defgroup HAL_ASSERT Assertions
 * @{
 */

#ifdef HAL_ENABLE_ASSERT
    /**
     * @brief Assert macro for debugging
     *
     * Halts execution if condition is false.
     *
     * @param cond Condition to assert
     */
    #define HAL_ASSERT(cond) do { \
        if (!(cond)) { \
            hal_assert_failed(#cond, __FILE__, __LINE__); \
        } \
    } while (0)

    /**
     * @brief Assertion failure handler
     *
     * Called when HAL_ASSERT fails. Platform-specific implementation
     * should halt or reset the system.
     *
     * @param expr Expression that failed
     * @param file Source file where assertion failed
     * @param line Line number where assertion failed
     */
    void hal_assert_failed(const char *expr, const char *file, int line);
#else
    #define HAL_ASSERT(cond) ((void)0)
#endif /* HAL_ENABLE_ASSERT */

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* HAL_COMMON_H */
