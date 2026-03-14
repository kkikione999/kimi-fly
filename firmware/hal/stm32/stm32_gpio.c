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
 * @file stm32_gpio.c
 * @brief STM32 GPIO interface implementation
 *
 * This file implements the HAL GPIO interface for STM32F4 using the LL driver
 * for efficient register-level access.
 */

#include "stm32_hal.h"

#include <errno.h>
#include <string.h>

/**
 * @brief GPIO instance structure
 */
typedef struct {
    GPIO_TypeDef *port;
    uint32_t pin;
    hal_gpio_dir_t dir;
    bool initialized;
} stm32_gpio_instance_t;

/**
 * @brief GPIO instance pool
 */
static stm32_gpio_instance_t _gpio_instances[STM32_GPIO_MAX_INSTANCES];

/**
 * @brief GPIO interface registered flag
 */
static bool _gpio_registered = false;

/**
 * @brief Convert HAL pin number to STM32 GPIO port
 *
 * Pin encoding: [7:4] = port index (0=A, 1=B, ...), [3:0] = pin number
 *
 * @param hal_pin HAL pin number
 * @return GPIO_TypeDef pointer, or NULL if invalid
 */
static GPIO_TypeDef *pin_to_port(uint32_t hal_pin)
{
    uint32_t port_idx = STM32_PIN_GET_PORT(hal_pin);

    switch (port_idx) {
    case 0: return GPIOA;
    case 1: return GPIOB;
    case 2: return GPIOC;
    case 3: return GPIOD;
    case 4: return GPIOE;
    case 5: return GPIOF;
    case 6: return GPIOG;
    case 7: return GPIOH;
    case 8: return GPIOI;
    default: return NULL;
    }
}

/**
 * @brief Convert HAL pin number to STM32 LL pin mask
 *
 * @param hal_pin HAL pin number
 * @return LL GPIO pin mask
 */
static uint32_t pin_to_ll_pin(uint32_t hal_pin)
{
    uint32_t pin_num = STM32_PIN_GET_NUM(hal_pin);
    return (1U << pin_num);
}

/**
 * @brief Enable GPIO port clock
 *
 * @param port GPIO port
 */
static void gpio_port_clock_enable(GPIO_TypeDef *port)
{
    if (port == GPIOA) {
        LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOA);
    } else if (port == GPIOB) {
        LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOB);
    } else if (port == GPIOC) {
        LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOC);
    } else if (port == GPIOD) {
        LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOD);
    } else if (port == GPIOE) {
        LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOE);
    } else if (port == GPIOF) {
        LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOF);
    } else if (port == GPIOG) {
        LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOG);
    } else if (port == GPIOH) {
        LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOH);
    } else if (port == GPIOI) {
        LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOI);
    }
}

/**
 * @brief Convert HAL pull configuration to LL pull mode
 *
 * @param pull HAL pull configuration
 * @return LL pull mode
 */
static uint32_t pull_to_ll_mode(hal_gpio_pull_t pull)
{
    switch (pull) {
    case HAL_GPIO_PULL_UP:
        return LL_GPIO_PULL_UP;
    case HAL_GPIO_PULL_DOWN:
        return LL_GPIO_PULL_DOWN;
    case HAL_GPIO_PULL_NONE:
    default:
        return LL_GPIO_PULL_NO;
    }
}

/**
 * @brief Find or allocate a GPIO instance
 *
 * @return Pointer to free instance, or NULL if full
 */
static stm32_gpio_instance_t *alloc_instance(void)
{
    for (int i = 0; i < STM32_GPIO_MAX_INSTANCES; i++) {
        if (!_gpio_instances[i].initialized) {
            return &_gpio_instances[i];
        }
    }
    return NULL;
}

/**
 * @brief Get instance from handle
 *
 * @param handle GPIO handle
 * @return Pointer to instance, or NULL if invalid
 */
static stm32_gpio_instance_t *handle_to_instance(hal_handle_t handle)
{
    if (!handle) {
        return NULL;
    }

    stm32_gpio_instance_t *inst = (stm32_gpio_instance_t *)handle;

    /* Validate pointer is within our instance pool */
    if (inst < &_gpio_instances[0] ||
        inst > &_gpio_instances[STM32_GPIO_MAX_INSTANCES - 1]) {
        return NULL;
    }

    /* Check if initialized */
    if (!inst->initialized) {
        return NULL;
    }

    return inst;
}

/**
 * @brief Initialize a GPIO pin
 *
 * @param cfg GPIO configuration
 * @param[out] handle Handle to initialized GPIO instance
 * @return 0 on success, negative error code on failure
 */
static int stm32_gpio_init(const hal_gpio_cfg_t *cfg, hal_handle_t *handle)
{
    if (!cfg || !handle) {
        return -EINVAL;
    }

    /* Find free instance */
    stm32_gpio_instance_t *inst = alloc_instance();
    if (!inst) {
        return -ENOMEM;
    }

    /* Get GPIO port and pin */
    GPIO_TypeDef *port = pin_to_port(cfg->pin);
    if (!port) {
        return -EINVAL;
    }

    uint32_t ll_pin = pin_to_ll_pin(cfg->pin);

    /* Enable port clock */
    gpio_port_clock_enable(port);

    /* Configure pin */
    LL_GPIO_InitTypeDef gpio_init = {0};
    gpio_init.Pin = ll_pin;
    gpio_init.Mode = (cfg->dir == HAL_GPIO_DIR_OUTPUT) ?
                     LL_GPIO_MODE_OUTPUT : LL_GPIO_MODE_INPUT;
    gpio_init.Speed = LL_GPIO_SPEED_FREQ_HIGH;
    gpio_init.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    gpio_init.Pull = pull_to_ll_mode(cfg->pull);

    LL_GPIO_Init(port, &gpio_init);

    /* Set initial value if output */
    if (cfg->dir == HAL_GPIO_DIR_OUTPUT) {
        if (cfg->init_val == HAL_GPIO_HIGH) {
            LL_GPIO_SetOutputPin(port, ll_pin);
        } else {
            LL_GPIO_ResetOutputPin(port, ll_pin);
        }
    }

    /* Store instance data */
    inst->port = port;
    inst->pin = ll_pin;
    inst->dir = cfg->dir;
    inst->initialized = true;

    *handle = (hal_handle_t)inst;
    return 0;
}

/**
 * @brief Set GPIO output state
 *
 * @param handle GPIO handle
 * @param state State to set
 * @return 0 on success, negative error code on failure
 */
static int stm32_gpio_set(hal_handle_t handle, hal_gpio_state_t state)
{
    stm32_gpio_instance_t *inst = handle_to_instance(handle);
    if (!inst) {
        return -EINVAL;
    }

    if (inst->dir != HAL_GPIO_DIR_OUTPUT) {
        return -EINVAL;
    }

    if (state == HAL_GPIO_HIGH) {
        LL_GPIO_SetOutputPin(inst->port, inst->pin);
    } else {
        LL_GPIO_ResetOutputPin(inst->port, inst->pin);
    }

    return 0;
}

/**
 * @brief Get GPIO input state
 *
 * @param handle GPIO handle
 * @param[out] state Current pin state
 * @return 0 on success, negative error code on failure
 */
static int stm32_gpio_get(hal_handle_t handle, hal_gpio_state_t *state)
{
    if (!state) {
        return -EINVAL;
    }

    stm32_gpio_instance_t *inst = handle_to_instance(handle);
    if (!inst) {
        return -EINVAL;
    }

    *state = LL_GPIO_IsInputPinSet(inst->port, inst->pin) ?
             HAL_GPIO_HIGH : HAL_GPIO_LOW;

    return 0;
}

/**
 * @brief Toggle GPIO output state
 *
 * @param handle GPIO handle
 * @return 0 on success, negative error code on failure
 */
static int stm32_gpio_toggle(hal_handle_t handle)
{
    stm32_gpio_instance_t *inst = handle_to_instance(handle);
    if (!inst) {
        return -EINVAL;
    }

    if (inst->dir != HAL_GPIO_DIR_OUTPUT) {
        return -EINVAL;
    }

    LL_GPIO_TogglePin(inst->port, inst->pin);
    return 0;
}

/**
 * @brief Deinitialize GPIO pin
 *
 * @param handle GPIO handle
 * @return 0 on success, negative error code on failure
 */
static int stm32_gpio_deinit(hal_handle_t handle)
{
    stm32_gpio_instance_t *inst = handle_to_instance(handle);
    if (!inst) {
        return -EINVAL;
    }

    /* Reset pin to input floating */
    LL_GPIO_InitTypeDef gpio_init = {0};
    gpio_init.Pin = inst->pin;
    gpio_init.Mode = LL_GPIO_MODE_INPUT;
    gpio_init.Pull = LL_GPIO_PULL_NO;
    LL_GPIO_Init(inst->port, &gpio_init);

    /* Mark instance as free */
    inst->initialized = false;
    inst->port = NULL;
    inst->pin = 0;

    return 0;
}

/**
 * @brief GPIO interface implementation
 */
static const hal_gpio_interface_t _stm32_gpio_iface = {
    .init = stm32_gpio_init,
    .set = stm32_gpio_set,
    .get = stm32_gpio_get,
    .toggle = stm32_gpio_toggle,
    .deinit = stm32_gpio_deinit,
};

int hal_gpio_register_stm32(void)
{
    if (_gpio_registered) {
        return 0;
    }

    /* Clear instance pool */
    memset(_gpio_instances, 0, sizeof(_gpio_instances));

    int rc = hal_gpio_register(&_stm32_gpio_iface);
    if (rc == 0) {
        _gpio_registered = true;
    }

    return rc;
}
