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
 * @file stm32_uart.c
 * @brief STM32 UART interface implementation
 *
 * This file implements the HAL UART interface for STM32F4 supporting:
 * - UART1: Debug/CLI (PA9 TX, PA10 RX)
 * - UART2: GPS (PA2 TX, PA3 RX)
 * - UART3: ESP32 communication (PB10 TX, PB11 RX)
 *
 * Uses HAL library for initialization and LL driver for time-critical operations.
 */

#include "stm32_hal.h"

#include <errno.h>
#include <string.h>

/**
 * @brief UART port definitions
 */
#define STM32_UART_PORT_1       0
#define STM32_UART_PORT_2       1
#define STM32_UART_PORT_3       2

/**
 * @brief UART instance structure
 */
typedef struct {
    USART_TypeDef *uart;
    uint8_t port;
    bool initialized;
    uint32_t timeout_start;
} stm32_uart_instance_t;

/**
 * @brief UART instance pool
 */
static stm32_uart_instance_t _uart_instances[STM32_UART_MAX_INSTANCES];

/**
 * @brief UART interface registered flag
 */
static bool _uart_registered = false;

/**
 * @brief HAL UART handles (for HAL compatibility)
 */
static UART_HandleTypeDef _uart_handles[STM32_UART_MAX_INSTANCES];

/**
 * @brief Get USART instance from port number
 *
 * @param port Port number (0=UART1, 1=UART2, 2=UART3)
 * @return USART_TypeDef pointer, or NULL if invalid
 */
static USART_TypeDef *port_to_usart(uint8_t port)
{
    switch (port) {
    case STM32_UART_PORT_1:
        return USART1;
    case STM32_UART_PORT_2:
        return USART2;
    case STM32_UART_PORT_3:
        return USART3;
    default:
        return NULL;
    }
}

/**
 * @brief Enable UART peripheral clock
 *
 * @param port Port number
 */
static void uart_clock_enable(uint8_t port)
{
    switch (port) {
    case STM32_UART_PORT_1:
        LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_USART1);
        break;
    case STM32_UART_PORT_2:
        LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_USART2);
        break;
    case STM32_UART_PORT_3:
        LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_USART3);
        break;
    default:
        break;
    }
}

/**
 * @brief Enable GPIO clocks for UART pins
 *
 * @param port Port number
 */
static void uart_gpio_clock_enable(uint8_t port)
{
    switch (port) {
    case STM32_UART_PORT_1:
        /* PA9 (TX), PA10 (RX) */
        LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOA);
        break;
    case STM32_UART_PORT_2:
        /* PA2 (TX), PA3 (RX) */
        LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOA);
        break;
    case STM32_UART_PORT_3:
        /* PB10 (TX), PB11 (RX) */
        LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOB);
        break;
    default:
        break;
    }
}

/**
 * @brief Configure GPIO pins for UART
 *
 * @param port Port number
 */
static void uart_gpio_init(uint8_t port)
{
    LL_GPIO_InitTypeDef gpio_init = {0};
    gpio_init.Mode = LL_GPIO_MODE_ALTERNATE;
    gpio_init.Speed = LL_GPIO_SPEED_FREQ_HIGH;
    gpio_init.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    gpio_init.Pull = LL_GPIO_PULL_NO;

    switch (port) {
    case STM32_UART_PORT_1:
        /* PA9 (TX) - AF7 */
        gpio_init.Pin = LL_GPIO_PIN_9;
        gpio_init.Alternate = LL_GPIO_AF_7;
        LL_GPIO_Init(GPIOA, &gpio_init);
        /* PA10 (RX) - AF7 */
        gpio_init.Pin = LL_GPIO_PIN_10;
        LL_GPIO_Init(GPIOA, &gpio_init);
        break;
    case STM32_UART_PORT_2:
        /* PA2 (TX) - AF7 */
        gpio_init.Pin = LL_GPIO_PIN_2;
        gpio_init.Alternate = LL_GPIO_AF_7;
        LL_GPIO_Init(GPIOA, &gpio_init);
        /* PA3 (RX) - AF7 */
        gpio_init.Pin = LL_GPIO_PIN_3;
        LL_GPIO_Init(GPIOA, &gpio_init);
        break;
    case STM32_UART_PORT_3:
        /* PB10 (TX) - AF7 */
        gpio_init.Pin = LL_GPIO_PIN_10;
        gpio_init.Alternate = LL_GPIO_AF_7;
        LL_GPIO_Init(GPIOB, &gpio_init);
        /* PB11 (RX) - AF7 */
        gpio_init.Pin = LL_GPIO_PIN_11;
        LL_GPIO_Init(GPIOB, &gpio_init);
        break;
    default:
        break;
    }
}

/**
 * @brief Convert HAL data bits to LL data bits
 *
 * @param data_bits HAL data bits
 * @return LL data bits
 */
static uint32_t data_bits_to_ll(hal_uart_data_bits_t data_bits)
{
    switch (data_bits) {
    case HAL_UART_DATA_BITS_9:
        return LL_USART_DATAWIDTH_9B;
    case HAL_UART_DATA_BITS_8:
    default:
        return LL_USART_DATAWIDTH_8B;
    }
}

/**
 * @brief Convert HAL stop bits to LL stop bits
 *
 * @param stop_bits HAL stop bits
 * @return LL stop bits
 */
static uint32_t stop_bits_to_ll(hal_uart_stop_bits_t stop_bits)
{
    switch (stop_bits) {
    case HAL_UART_STOP_BITS_2:
        return LL_USART_STOPBITS_2;
    case HAL_UART_STOP_BITS_1:
    default:
        return LL_USART_STOPBITS_1;
    }
}

/**
 * @brief Convert HAL parity to LL parity
 *
 * @param parity HAL parity
 * @return LL parity
 */
static uint32_t parity_to_ll(hal_uart_parity_t parity)
{
    switch (parity) {
    case HAL_UART_PARITY_EVEN:
        return LL_USART_PARITY_EVEN;
    case HAL_UART_PARITY_ODD:
        return LL_USART_PARITY_ODD;
    case HAL_UART_PARITY_NONE:
    default:
        return LL_USART_PARITY_NONE;
    }
}

/**
 * @brief Find or allocate a UART instance
 *
 * @param port Port number
 * @return Pointer to instance, or NULL if full/invalid
 */
static stm32_uart_instance_t *alloc_instance(uint8_t port)
{
    if (port >= STM32_UART_MAX_INSTANCES) {
        return NULL;
    }

    /* Check if already initialized */
    if (_uart_instances[port].initialized) {
        return NULL;
    }

    return &_uart_instances[port];
}

/**
 * @brief Get instance from handle
 *
 * @param handle UART handle
 * @return Pointer to instance, or NULL if invalid
 */
static stm32_uart_instance_t *handle_to_instance(hal_handle_t handle)
{
    if (!handle) {
        return NULL;
    }

    stm32_uart_instance_t *inst = (stm32_uart_instance_t *)handle;

    /* Validate pointer is within our instance pool */
    if (inst < &_uart_instances[0] ||
        inst > &_uart_instances[STM32_UART_MAX_INSTANCES - 1]) {
        return NULL;
    }

    /* Check if initialized */
    if (!inst->initialized) {
        return NULL;
    }

    return inst;
}

/**
 * @brief Initialize UART peripheral
 *
 * @param port UART port number
 * @param cfg UART configuration
 * @param[out] handle Handle to initialized UART instance
 * @return 0 on success, negative error code on failure
 */
static int stm32_uart_init(uint8_t port, const hal_uart_cfg_t *cfg,
                           hal_handle_t *handle)
{
    if (!cfg || !handle) {
        return -EINVAL;
    }

    if (port >= STM32_UART_MAX_INSTANCES) {
        return -EINVAL;
    }

    stm32_uart_instance_t *inst = alloc_instance(port);
    if (!inst) {
        return -EBUSY;
    }

    USART_TypeDef *usart = port_to_usart(port);
    if (!usart) {
        return -EINVAL;
    }

    /* Enable clocks */
    uart_gpio_clock_enable(port);
    uart_clock_enable(port);

    /* Configure GPIO */
    uart_gpio_init(port);

    /* Configure UART using LL for efficiency */
    LL_USART_InitTypeDef uart_init = {0};
    uart_init.BaudRate = cfg->baudrate;
    uart_init.DataWidth = data_bits_to_ll(cfg->data_bits);
    uart_init.StopBits = stop_bits_to_ll(cfg->stop_bits);
    uart_init.Parity = parity_to_ll(cfg->parity);
    uart_init.TransferDirection = LL_USART_DIRECTION_TX_RX;
    uart_init.HardwareFlowControl = LL_USART_HWCONTROL_NONE;
    uart_init.OverSampling = LL_USART_OVERSAMPLING_16;

    if (LL_USART_Init(usart, &uart_init) != SUCCESS) {
        return -EIO;
    }

    LL_USART_Enable(usart);

    /* Store instance data */
    inst->uart = usart;
    inst->port = port;
    inst->initialized = true;

    *handle = (hal_handle_t)inst;
    return 0;
}

/**
 * @brief Send data over UART
 *
 * @param handle UART handle
 * @param data Data buffer to send
 * @param len Number of bytes to send
 * @param timeout_ms Timeout in milliseconds (0 for non-blocking)
 * @return Number of bytes sent, or negative error code
 */
static int stm32_uart_send(hal_handle_t handle, const uint8_t *data, size_t len,
                           uint32_t timeout_ms)
{
    if (!data || len == 0) {
        return -EINVAL;
    }

    stm32_uart_instance_t *inst = handle_to_instance(handle);
    if (!inst) {
        return -EINVAL;
    }

    size_t sent = 0;
    uint32_t start_tick = stm32_get_tick_ms();

    while (sent < len) {
        /* Wait for TXE (transmit data register empty) */
        if (!LL_USART_IsActiveFlag_TXE(inst->uart)) {
            if (timeout_ms > 0) {
                if ((stm32_get_tick_ms() - start_tick) >= timeout_ms) {
                    return (int)sent;  /* Return bytes sent so far */
                }
            }
            continue;
        }

        /* Send byte */
        LL_USART_TransmitData8(inst->uart, data[sent]);
        sent++;
    }

    /* Wait for transmission complete if timeout specified */
    if (timeout_ms > 0) {
        while (!LL_USART_IsActiveFlag_TC(inst->uart)) {
            if ((stm32_get_tick_ms() - start_tick) >= timeout_ms) {
                break;
            }
        }
    }

    return (int)sent;
}

/**
 * @brief Receive data from UART
 *
 * @param handle UART handle
 * @param[out] data Buffer to receive data
 * @param len Maximum number of bytes to receive
 * @param timeout_ms Timeout in milliseconds (0 for non-blocking)
 * @return Number of bytes received, or negative error code
 */
static int stm32_uart_recv(hal_handle_t handle, uint8_t *data, size_t len,
                           uint32_t timeout_ms)
{
    if (!data || len == 0) {
        return -EINVAL;
    }

    stm32_uart_instance_t *inst = handle_to_instance(handle);
    if (!inst) {
        return -EINVAL;
    }

    size_t received = 0;
    uint32_t start_tick = stm32_get_tick_ms();

    while (received < len) {
        /* Check for RXNE (read data register not empty) */
        if (LL_USART_IsActiveFlag_RXNE(inst->uart)) {
            data[received] = LL_USART_ReceiveData8(inst->uart);
            received++;

            /* Clear overrun error if present */
            if (LL_USART_IsActiveFlag_ORE(inst->uart)) {
                LL_USART_ClearFlag_ORE(inst->uart);
            }
        } else {
            /* Check timeout */
            if (timeout_ms > 0) {
                if ((stm32_get_tick_ms() - start_tick) >= timeout_ms) {
                    break;
                }
            } else {
                /* Non-blocking: return immediately if no data */
                break;
            }
        }
    }

    return (int)received;
}

/**
 * @brief Flush UART buffers
 *
 * @param handle UART handle
 * @return 0 on success, negative error code on failure
 */
static int stm32_uart_flush(hal_handle_t handle)
{
    stm32_uart_instance_t *inst = handle_to_instance(handle);
    if (!inst) {
        return -EINVAL;
    }

    /* Wait for transmission complete */
    while (!LL_USART_IsActiveFlag_TC(inst->uart)) {
        /* Busy wait */
    }

    /* Clear receive data register */
    while (LL_USART_IsActiveFlag_RXNE(inst->uart)) {
        (void)LL_USART_ReceiveData8(inst->uart);
    }

    /* Clear error flags */
    LL_USART_ClearFlag_ORE(inst->uart);
    LL_USART_ClearFlag_FE(inst->uart);
    LL_USART_ClearFlag_NE(inst->uart);

    return 0;
}

/**
 * @brief Deinitialize UART peripheral
 *
 * @param handle UART handle
 * @return 0 on success, negative error code on failure
 */
static int stm32_uart_deinit(hal_handle_t handle)
{
    stm32_uart_instance_t *inst = handle_to_instance(handle);
    if (!inst) {
        return -EINVAL;
    }

    /* Disable UART */
    LL_USART_Disable(inst->uart);

    /* Mark instance as free */
    inst->initialized = false;
    inst->uart = NULL;

    return 0;
}

/**
 * @brief UART interface implementation
 */
static const hal_uart_interface_t _stm32_uart_iface = {
    .init = stm32_uart_init,
    .send = stm32_uart_send,
    .recv = stm32_uart_recv,
    .flush = stm32_uart_flush,
    .deinit = stm32_uart_deinit,
};

int hal_uart_register_stm32(void)
{
    if (_uart_registered) {
        return 0;
    }

    /* Clear instance pool */
    memset(_uart_instances, 0, sizeof(_uart_instances));
    memset(_uart_handles, 0, sizeof(_uart_handles));

    int rc = hal_uart_register(&_stm32_uart_iface);
    if (rc == 0) {
        _uart_registered = true;
    }

    return rc;
}
