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
 * @file esp32c3_uart.c
 * @brief ESP32-C3 HAL UART implementation
 *
 * Implements UART interface for ESP32-C3 using ESP-IDF UART driver.
 *
 * UART Configuration:
 * - UART0: Debug/CLI (GPIO20=RX, GPIO21=TX, 115200bps)
 * - UART1: STM32 communication (GPIO11=RX, GPIO10=TX, 921600bps)
 */

#include "esp32c3_hal.h"
#include "hal_interface.h"
#include "board_config.h"
#include <errno.h>
#include <string.h>

#define TAG "esp32c3_uart"

/* UART handle structure */
typedef struct {
    uart_port_t port;
    bool initialized;
    uint8_t rx_buf[ESP32C3_UART_BUF_SIZE];
} uart_handle_t;

/* UART handle pool - static allocation */
static uart_handle_t s_uart_handles[ESP32C3_UART_PORT_NUM];
static bool s_uart_initialized = false;

/* Forward declarations for UART interface */
static int esp32c3_uart_init(uint8_t port, const hal_uart_cfg_t *cfg,
                              hal_handle_t *handle);
static int esp32c3_uart_send(hal_handle_t handle, const uint8_t *data,
                              size_t len, uint32_t timeout_ms);
static int esp32c3_uart_recv(hal_handle_t handle, uint8_t *data,
                              size_t len, uint32_t timeout_ms);
static int esp32c3_uart_flush(hal_handle_t handle);
static int esp32c3_uart_deinit(hal_handle_t handle);

/* UART interface instance */
static const hal_uart_interface_t s_esp32c3_uart_iface = {
    .init = esp32c3_uart_init,
    .send = esp32c3_uart_send,
    .recv = esp32c3_uart_recv,
    .flush = esp32c3_uart_flush,
    .deinit = esp32c3_uart_deinit
};

/**
 * @brief Convert HAL data bits to ESP-IDF data bits
 */
static uart_word_length_t hal_data_bits_to_esp(hal_uart_data_bits_t bits)
{
    switch (bits) {
    case HAL_UART_DATA_BITS_9:
        return UART_DATA_8_BITS;  /* ESP32-C3 doesn't support 9 bits */
    case HAL_UART_DATA_BITS_8:
    default:
        return UART_DATA_8_BITS;
    }
}

/**
 * @brief Convert HAL stop bits to ESP-IDF stop bits
 */
static uart_stop_bits_t hal_stop_bits_to_esp(hal_uart_stop_bits_t bits)
{
    switch (bits) {
    case HAL_UART_STOP_BITS_2:
        return UART_STOP_BITS_2;
    case HAL_UART_STOP_BITS_1:
    default:
        return UART_STOP_BITS_1;
    }
}

/**
 * @brief Convert HAL parity to ESP-IDF parity
 */
static uart_parity_t hal_parity_to_esp(hal_uart_parity_t parity)
{
    switch (parity) {
    case HAL_UART_PARITY_EVEN:
        return UART_PARITY_EVEN;
    case HAL_UART_PARITY_ODD:
        return UART_PARITY_ODD;
    case HAL_UART_PARITY_NONE:
    default:
        return UART_PARITY_DISABLE;
    }
}

/**
 * @brief Get UART handle for a port
 */
static uart_handle_t *uart_handle_get(uart_port_t port)
{
    if (port < ESP32C3_UART_PORT_NUM) {
        return &s_uart_handles[port];
    }
    return NULL;
}

/**
 * @brief Get default pin configuration for a UART port
 */
static void uart_get_default_pins(uart_port_t port, int *tx_pin, int *rx_pin)
{
    if (port == UART_NUM_0) {
        /* UART0: Debug/CLI - uses USB pins on ESP32-C3 */
        *tx_pin = UART_DEBUG_TX_PIN;  /* GPIO21 */
        *rx_pin = UART_DEBUG_RX_PIN;  /* GPIO20 */
    } else if (port == UART_NUM_1) {
        /* UART1: STM32 communication */
        *tx_pin = UART_ESP32_TX_PIN;  /* GPIO10 */
        *rx_pin = UART_ESP32_RX_PIN;  /* GPIO11 */
    } else {
        *tx_pin = -1;
        *rx_pin = -1;
    }
}

/* ============================================================================
 * UART Interface Implementations
 * ============================================================================ */

static int esp32c3_uart_init(uint8_t port, const hal_uart_cfg_t *cfg,
                              hal_handle_t *handle)
{
    if (cfg == NULL || handle == NULL) {
        ESP_LOGE(TAG, "Invalid parameters");
        return -EINVAL;
    }

    if (port >= ESP32C3_UART_PORT_NUM) {
        ESP_LOGE(TAG, "Invalid UART port: %d", port);
        return -EINVAL;
    }

    uart_port_t uart_port = (uart_port_t)port;
    uart_handle_t *uart_handle = uart_handle_get(uart_port);

    if (uart_handle->initialized) {
        ESP_LOGW(TAG, "UART%d already initialized", port);
        return -EBUSY;
    }

    /* Get pin configuration */
    int tx_pin, rx_pin;
    uart_get_default_pins(uart_port, &tx_pin, &rx_pin);

    if (tx_pin < 0 || rx_pin < 0) {
        ESP_LOGE(TAG, "Invalid pin configuration for UART%d", port);
        return -EINVAL;
    }

    /* Configure UART parameters */
    uart_config_t uart_config = {
        .baud_rate = (int)cfg->baudrate,
        .data_bits = hal_data_bits_to_esp(cfg->data_bits),
        .parity = hal_parity_to_esp(cfg->parity),
        .stop_bits = hal_stop_bits_to_esp(cfg->stop_bits),
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT
    };

    esp_err_t err = uart_param_config(uart_port, &uart_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "UART%d param config failed: %d", port, err);
        return -EIO;
    }

    /* Set pins */
    err = uart_set_pin(uart_port, tx_pin, rx_pin, UART_PIN_NO_CHANGE,
                       UART_PIN_NO_CHANGE);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "UART%d set pin failed: %d", port, err);
        return -EIO;
    }

    /* Install driver */
    err = uart_driver_install(uart_port, ESP32C3_UART_BUF_SIZE * 2,
                               ESP32C3_UART_BUF_SIZE * 2,
                               ESP32C3_UART_QUEUE_SIZE, NULL, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "UART%d driver install failed: %d", port, err);
        return -EIO;
    }

    uart_handle->port = uart_port;
    uart_handle->initialized = true;
    *handle = (hal_handle_t)uart_handle;

    ESP_LOGI(TAG, "UART%d initialized at %lu baud (TX=%d, RX=%d)",
             port, cfg->baudrate, tx_pin, rx_pin);

    return 0;
}

static int esp32c3_uart_send(hal_handle_t handle, const uint8_t *data,
                              size_t len, uint32_t timeout_ms)
{
    uart_handle_t *uart_handle = (uart_handle_t *)handle;

    if (uart_handle == NULL || !uart_handle->initialized) {
        ESP_LOGE(TAG, "Invalid handle");
        return -EINVAL;
    }

    if (data == NULL || len == 0) {
        return 0;
    }

    TickType_t ticks = (timeout_ms == 0) ? 0 : pdMS_TO_TICKS(timeout_ms);

    int written = uart_write_bytes(uart_handle->port, data, len);
    if (written < 0) {
        ESP_LOGE(TAG, "UART%d write failed: %d", uart_handle->port, written);
        return -EIO;
    }

    /* Wait for TX complete if timeout specified */
    if (timeout_ms > 0) {
        esp_err_t err = uart_wait_tx_done(uart_handle->port, ticks);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "UART%d wait TX done timeout", uart_handle->port);
        }
    }

    return written;
}

static int esp32c3_uart_recv(hal_handle_t handle, uint8_t *data,
                              size_t len, uint32_t timeout_ms)
{
    uart_handle_t *uart_handle = (uart_handle_t *)handle;

    if (uart_handle == NULL || !uart_handle->initialized) {
        ESP_LOGE(TAG, "Invalid handle");
        return -EINVAL;
    }

    if (data == NULL || len == 0) {
        return 0;
    }

    TickType_t ticks = (timeout_ms == 0) ? 0 : pdMS_TO_TICKS(timeout_ms);

    int read = uart_read_bytes(uart_handle->port, data, len, ticks);
    if (read < 0) {
        ESP_LOGE(TAG, "UART%d read failed: %d", uart_handle->port, read);
        return -EIO;
    }

    return read;
}

static int esp32c3_uart_flush(hal_handle_t handle)
{
    uart_handle_t *uart_handle = (uart_handle_t *)handle;

    if (uart_handle == NULL || !uart_handle->initialized) {
        ESP_LOGE(TAG, "Invalid handle");
        return -EINVAL;
    }

    esp_err_t err = uart_flush(uart_handle->port);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "UART%d flush failed: %d", uart_handle->port, err);
        return -EIO;
    }

    return 0;
}

static int esp32c3_uart_deinit(hal_handle_t handle)
{
    uart_handle_t *uart_handle = (uart_handle_t *)handle;

    if (uart_handle == NULL || !uart_handle->initialized) {
        ESP_LOGE(TAG, "Invalid handle");
        return -EINVAL;
    }

    esp_err_t err = uart_driver_delete(uart_handle->port);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "UART%d driver delete failed: %d", uart_handle->port, err);
        return -EIO;
    }

    ESP_LOGI(TAG, "UART%d deinitialized", uart_handle->port);

    uart_handle->initialized = false;

    return 0;
}

/* ============================================================================
 * Public Functions
 * ============================================================================ */

int hal_uart_register_esp32c3(void)
{
    if (s_uart_initialized) {
        ESP_LOGW(TAG, "UART already registered");
        return 0;
    }

    /* Initialize handle pool */
    memset(s_uart_handles, 0, sizeof(s_uart_handles));

    int rc = hal_uart_register(&s_esp32c3_uart_iface);
    if (rc != 0) {
        ESP_LOGE(TAG, "Failed to register UART interface: %d", rc);
        return rc;
    }

    s_uart_initialized = true;
    ESP_LOGI(TAG, "ESP32-C3 UART interface registered");

    return 0;
}

hal_handle_t esp32c3_uart_get_handle(uint8_t port)
{
    if (port >= ESP32C3_UART_PORT_NUM) {
        return NULL;
    }

    uart_handle_t *handle = &s_uart_handles[port];
    if (!handle->initialized) {
        return NULL;
    }

    return (hal_handle_t)handle;
}
