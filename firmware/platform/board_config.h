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
 * @file board_config.h
 * @brief Board-level configuration for kimi-fly flight controller
 *
 * This file contains hardware-specific definitions for the flight controller
 * board. Modify these definitions when porting to different hardware.
 */

#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup BOARD_PROCESSOR Processor Selection
 * @{
 */

/**
 * @brief Target processor selection
 *
 * Define exactly one of these to select the target processor.
 */
/* #define CONFIG_USE_STM32F4 */
/* #define CONFIG_USE_STM32H7 */
/* #define CONFIG_USE_ESP32C3 */

/**
 * @brief Default to STM32F4 if not specified
 */
#if !defined(CONFIG_USE_STM32F4) && !defined(CONFIG_USE_STM32H7) && \
    !defined(CONFIG_USE_ESP32C3)
    #define CONFIG_USE_STM32F4
#endif

/** @} */

/**
 * @defgroup BOARD_MOTOR Motor PWM Configuration
 * @{
 */

/**
 * @brief PWM frequency for motor control (Hz)
 *
 * Standard ESCs typically use 400-500 Hz for PWM control.
 * DShot is preferred when available.
 */
#define PWM_FREQ_HZ         400U

/**
 * @brief Motor PWM channel definitions
 *
 * These are platform-specific identifiers for motor PWM outputs.
 * For STM32, these typically map to timer channels.
 * For ESP32, these map to LEDC channels.
 */
#if defined(CONFIG_USE_STM32F4) || defined(CONFIG_USE_STM32H7)
    /* STM32: Use timer channel identifiers */
    #define MOTOR1_PIN      0   /* TIM2_CH1 or platform-specific */
    #define MOTOR2_PIN      1   /* TIM2_CH2 */
    #define MOTOR3_PIN      2   /* TIM2_CH3 */
    #define MOTOR4_PIN      3   /* TIM2_CH4 */
#elif defined(CONFIG_USE_ESP32C3)
    /* ESP32-C3: Use LEDC channel numbers */
    #define MOTOR1_PIN      0   /* LEDC channel 0 */
    #define MOTOR2_PIN      1   /* LEDC channel 1 */
    #define MOTOR3_PIN      2   /* LEDC channel 2 */
    #define MOTOR4_PIN      3   /* LEDC channel 3 */
#endif

/**
 * @brief Maximum number of motors
 */
#define MOTOR_MAX_COUNT     4

/**
 * @brief PWM duty cycle limits
 */
#define PWM_DUTY_MIN        0.0f
#define PWM_DUTY_MAX        1.0f

/** @} */

/**
 * @defgroup BOARD_CONTROL_LOOP Control Loop Configuration
 * @{
 */

/**
 * @brief Main control loop frequency (Hz)
 *
 * The flight control loop runs at this frequency.
 * 1000 Hz provides 1ms control loop timing.
 */
#define CONTROL_LOOP_HZ     1000U

/**
 * @brief Control loop period in microseconds
 */
#define CONTROL_LOOP_PERIOD_US  (1000000U / CONTROL_LOOP_HZ)

/**
 * @brief Control loop period in milliseconds
 */
#define CONTROL_LOOP_PERIOD_MS  (1000U / CONTROL_LOOP_HZ)

/** @} */

/**
 * @defgroup BOARD_SPI SPI Bus Configuration
 * @{
 */

/**
 * @brief SPI bus identifiers
 */
#define SPI_BUS_1           0
#define SPI_BUS_2           1
#define SPI_BUS_3           2

/**
 * @brief SPI1 configuration (IMU - high speed)
 *
 * SPI1 is dedicated to the IMU for high-speed data transfer.
 * Platform-specific pin definitions.
 */
#if defined(CONFIG_USE_STM32F4) || defined(CONFIG_USE_STM32H7)
    #define SPI1_SCK_PIN    0   /* PA5 - SPI1_SCK */
    #define SPI1_MISO_PIN   1   /* PA6 - SPI1_MISO */
    #define SPI1_MOSI_PIN   2   /* PA7 - SPI1_MOSI */
    #define SPI1_CS_PIN     3   /* PA4 - SPI1_NSS (software controlled) */
#elif defined(CONFIG_USE_ESP32C3)
    #define SPI1_SCK_PIN    4   /* GPIO4 - SCK */
    #define SPI1_MISO_PIN   5   /* GPIO5 - MISO */
    #define SPI1_MOSI_PIN   6   /* GPIO6 - MOSI */
    #define SPI1_CS_PIN     7   /* GPIO7 - CS (software controlled) */
#endif

/**
 * @brief SPI1 default configuration
 */
#define SPI1_DEFAULT_SPEED_HZ   8000000U    /* 8 MHz for IMU */
#define SPI1_DEFAULT_MODE       HAL_SPI_MODE_0

/** @} */

/**
 * @defgroup BOARD_I2C I2C Bus Configuration
 * @{
 */

/**
 * @brief I2C bus identifiers
 */
#define I2C_BUS_1           0
#define I2C_BUS_2           1

/**
 * @brief I2C1 configuration (Sensors - barometer, magnetometer)
 *
 * I2C1 is used for auxiliary sensors.
 * Platform-specific pin definitions.
 */
#if defined(CONFIG_USE_STM32F4) || defined(CONFIG_USE_STM32H7)
    #define I2C1_SCL_PIN    0   /* PB6 - I2C1_SCL */
    #define I2C1_SDA_PIN    1   /* PB7 - I2C1_SDA */
#elif defined(CONFIG_USE_ESP32C3)
    #define I2C1_SCL_PIN    8   /* GPIO8 - SCL */
    #define I2C1_SDA_PIN    9   /* GPIO9 - SDA */
#endif

/**
 * @brief I2C1 default configuration
 */
#define I2C1_DEFAULT_SPEED      HAL_I2C_SPEED_FAST    /* 400 kHz */

/**
 * @brief Sensor I2C addresses
 */
#define BMP280_ADDR         0x76    /* Barometer (alternate: 0x77) */
#define QMC5883_ADDR        0x0D    /* Magnetometer */
#define HMC5883_ADDR        0x1E    /* Alternative magnetometer */

/** @} */

/**
 * @defgroup BOARD_UART UART Configuration
 * @{
 */

/**
 * @brief UART port identifiers
 */
#define UART_PORT_1         0
#define UART_PORT_2         1
#define UART_PORT_3         2

/**
 * @brief UART for debug/CLI
 *
 * Used for command-line interface and debug output.
 */
#if defined(CONFIG_USE_STM32F4) || defined(CONFIG_USE_STM32H7)
    #define UART_DEBUG          UART_PORT_1
    #define UART_DEBUG_TX_PIN   0   /* PA9 - USART1_TX */
    #define UART_DEBUG_RX_PIN   1   /* PA10 - USART1_RX */
#elif defined(CONFIG_USE_ESP32C3)
    #define UART_DEBUG          UART_PORT_0
    #define UART_DEBUG_TX_PIN   21  /* GPIO21 - TX */
    #define UART_DEBUG_RX_PIN   20  /* GPIO20 - RX */
#endif

#define UART_DEBUG_BAUDRATE     115200U

/**
 * @brief UART for GPS module
 */
#if defined(CONFIG_USE_STM32F4) || defined(CONFIG_USE_STM32H7)
    #define UART_GPS            UART_PORT_2
    #define UART_GPS_TX_PIN     2   /* PA2 - USART2_TX */
    #define UART_GPS_RX_PIN     3   /* PA3 - USART2_RX */
#elif defined(CONFIG_USE_ESP32C3)
    #define UART_GPS            UART_PORT_1
    #define UART_GPS_TX_PIN     10  /* GPIO10 - TX */
    #define UART_GPS_RX_PIN     11  /* GPIO11 - RX */
#endif

#define UART_GPS_BAUDRATE       9600U

/**
 * @brief UART for ESP32-C3 communication (dual-processor setup)
 *
 * This UART is used for inter-processor communication between
 * STM32 (main) and ESP32-C3 (wireless coprocessor).
 */
#if defined(CONFIG_USE_STM32F4) || defined(CONFIG_USE_STM32H7)
    #define UART_ESP32          UART_PORT_3
    #define UART_ESP32_TX_PIN   4   /* PB10 - USART3_TX */
    #define UART_ESP32_RX_PIN   5   /* PB11 - USART3_RX */
#elif defined(CONFIG_USE_ESP32C3)
    #define UART_ESP32          UART_PORT_1
    #define UART_ESP32_TX_PIN   10
    #define UART_ESP32_RX_PIN   11
#endif

#define UART_ESP32_BAUDRATE     921600U

/** @} */

/**
 * @defgroup BOARD_GPIO General GPIO Definitions
 * @{
 */

/**
 * @brief Status LED pins
 */
#if defined(CONFIG_USE_STM32F4) || defined(CONFIG_USE_STM32H7)
    #define LED_STATUS_PIN      0   /* PC13 - Status LED */
    #define LED_ERROR_PIN       1   /* PC14 - Error LED */
#elif defined(CONFIG_USE_ESP32C3)
    #define LED_STATUS_PIN      2   /* GPIO2 - Status LED */
    #define LED_ERROR_PIN       3   /* GPIO3 - Error LED */
#endif

/**
 * @brief Button/input pins
 */
#if defined(CONFIG_USE_STM32F4) || defined(CONFIG_USE_STM32H7)
    #define BUTTON_USER_PIN     2   /* PA0 - User button */
#elif defined(CONFIG_USE_ESP32C3)
    #define BUTTON_USER_PIN     1   /* GPIO1 - User button */
#endif

/** @} */

/**
 * @defgroup BOARD_MISC Miscellaneous Configuration
 * @{
 */

/**
 * @brief System tick frequency (Hz)
 *
 * Defines the resolution of hal_get_tick_ms().
 * Must match the SysTick configuration.
 */
#define SYS_TICK_HZ         1000U

/**
 * @brief Maximum number of HAL handles
 *
 * Limits the total number of simultaneously open HAL instances.
 */
#define HAL_MAX_HANDLES     16

/**
 * @brief DMA buffer alignment
 *
 * DMA buffers must be aligned to this boundary for cache coherency.
 */
#define DMA_ALIGN           32

/**
 * @brief Board name string
 */
#if defined(CONFIG_USE_STM32F4)
    #define BOARD_NAME          "kimi-fly-stm32f4"
#elif defined(CONFIG_USE_STM32H7)
    #define BOARD_NAME          "kimi-fly-stm32h7"
#elif defined(CONFIG_USE_ESP32C3)
    #define BOARD_NAME          "kimi-fly-esp32c3"
#endif

/** @} */

/**
 * @defgroup BOARD_VALIDATION Compile-time Validation
 * @{
 */

#ifndef CONFIG_USE_STM32F4
#ifndef CONFIG_USE_STM32H7
#ifndef CONFIG_USE_ESP32C3
    #error "No target processor defined. Define CONFIG_USE_STM32F4, CONFIG_USE_STM32H7, or CONFIG_USE_ESP32C3"
#endif
#endif
#endif

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* BOARD_CONFIG_H */
