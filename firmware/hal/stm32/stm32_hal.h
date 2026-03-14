/*
 * Copyright (c) 2026 kimi-fly Project
 * SPDX-License-Identifier: MIT
 */

/**
 * @file stm32_hal.h
 * @brief STM32 HAL layer header for F4 series
 */

#ifndef STM32_HAL_H
#define STM32_HAL_H

#include "../hal_interface.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* STM32 HAL initialization */
int stm32_hal_init(void);

/* Component registration functions */
int stm32_gpio_register_hal(void);
int stm32_uart_register_hal(void);
int stm32_pwm_register_hal(void);
int stm32_system_register_hal(void);

/* Error handler */
void stm32_error_handler(void);

#ifdef __cplusplus
}
#endif

#endif /* STM32_HAL_H */
