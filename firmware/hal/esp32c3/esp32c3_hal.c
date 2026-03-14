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
 * @file esp32c3_hal.c
 * @brief ESP32-C3 HAL system implementation
 *
 * Implements system-level functions for ESP32-C3 including initialization,
 * timing, delays, and interrupt control.
 */

#include "esp32c3_hal.h"
#include "hal_interface.h"
#include <string.h>

#define TAG "esp32c3_hal"

/* Internal state */
static bool s_hal_initialized = false;
static portMUX_TYPE s_critical_mutex = portMUX_INITIALIZER_UNLOCKED;

/* System interface forward declarations */
static uint32_t esp32c3_system_get_tick_ms(void);
static void esp32c3_system_delay_ms(uint32_t ms);
static void esp32c3_system_delay_us(uint32_t us);
static uint32_t esp32c3_system_disable_irq(void);
static void esp32c3_system_restore_irq(uint32_t state);
static uint32_t esp32c3_system_enter_critical(void);
static void esp32c3_system_exit_critical(uint32_t state);
static void esp32c3_system_reset(void);
static int esp32c3_system_get_unique_id(uint8_t *id, size_t len);

/* System interface instance */
static const hal_system_interface_t s_esp32c3_system_iface = {
    .get_tick_ms = esp32c3_system_get_tick_ms,
    .delay_ms = esp32c3_system_delay_ms,
    .delay_us = esp32c3_system_delay_us,
    .disable_irq = esp32c3_system_disable_irq,
    .restore_irq = esp32c3_system_restore_irq,
    .enter_critical = esp32c3_system_enter_critical,
    .exit_critical = esp32c3_system_exit_critical,
    .reset = esp32c3_system_reset,
    .get_unique_id = esp32c3_system_get_unique_id
};

/* ============================================================================
 * System Functions
 * ============================================================================ */

uint32_t esp32c3_get_tick_ms(void)
{
    return (uint32_t)(esp_timer_get_time() / 1000ULL);
}

void esp32c3_delay_ms(uint32_t ms)
{
    vTaskDelay(pdMS_TO_TICKS(ms));
}

void esp32c3_delay_us(uint32_t us)
{
    esp_rom_delay_us(us);
}

uint32_t esp32c3_enter_critical(void)
{
    uint32_t state = portSET_INTERRUPT_MASK_FROM_ISR();
    portENTER_CRITICAL(&s_critical_mutex);
    return state;
}

void esp32c3_exit_critical(uint32_t state)
{
    portEXIT_CRITICAL(&s_critical_mutex);
    portCLEAR_INTERRUPT_MASK_FROM_ISR(state);
}

int esp32c3_get_unique_id(uint8_t *id, size_t len)
{
    uint8_t mac[6];

    if (id == NULL || len < 6) {
        return -EINVAL;
    }

    esp_err_t err = esp_read_mac(mac, ESP_MAC_WIFI_STA);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read MAC: %d", err);
        return -EIO;
    }

    memcpy(id, mac, 6);
    return 6;
}

void esp32c3_system_reset(void)
{
    ESP_LOGI(TAG, "System reset initiated");
    esp_restart();
}

/* ============================================================================
 * System Interface Implementations
 * ============================================================================ */

static uint32_t esp32c3_system_get_tick_ms(void)
{
    return esp32c3_get_tick_ms();
}

static void esp32c3_system_delay_ms(uint32_t ms)
{
    esp32c3_delay_ms(ms);
}

static void esp32c3_system_delay_us(uint32_t us)
{
    esp32c3_delay_us(us);
}

static uint32_t esp32c3_system_disable_irq(void)
{
    return portSET_INTERRUPT_MASK_FROM_ISR();
}

static void esp32c3_system_restore_irq(uint32_t state)
{
    portCLEAR_INTERRUPT_MASK_FROM_ISR(state);
}

static uint32_t esp32c3_system_enter_critical(void)
{
    return esp32c3_enter_critical();
}

static void esp32c3_system_exit_critical(uint32_t state)
{
    esp32c3_exit_critical(state);
}

static void esp32c3_system_reset(void)
{
    esp32c3_system_reset();
}

static int esp32c3_system_get_unique_id(uint8_t *id, size_t len)
{
    return esp32c3_get_unique_id(id, len);
}

/* ============================================================================
 * HAL Initialization
 * ============================================================================ */

int esp32c3_hal_init(void)
{
    esp_err_t err;

    if (s_hal_initialized) {
        ESP_LOGW(TAG, "HAL already initialized");
        return 0;
    }

    ESP_LOGI(TAG, "Initializing ESP32-C3 HAL v%d.%d.%d",
             ESP32C3_HAL_VERSION_MAJOR,
             ESP32C3_HAL_VERSION_MINOR,
             ESP32C3_HAL_VERSION_PATCH);

    /* Initialize NVS */
    err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES ||
        err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS needs erase, erasing...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "NVS init failed: %d", err);
        return -EIO;
    }
    ESP_LOGI(TAG, "NVS initialized");

    /* Initialize event loop */
    err = esp_event_loop_create_default();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "Event loop init failed: %d", err);
        return -EIO;
    }
    ESP_LOGI(TAG, "Event loop initialized");

    /* Register system interface */
    int rc = hal_system_register(&s_esp32c3_system_iface);
    if (rc != 0) {
        ESP_LOGE(TAG, "Failed to register system interface: %d", rc);
        return rc;
    }

    s_hal_initialized = true;
    ESP_LOGI(TAG, "ESP32-C3 HAL initialized successfully");

    return 0;
}

int esp32c3_hal_deinit(void)
{
    if (!s_hal_initialized) {
        return 0;
    }

    ESP_LOGI(TAG, "Deinitializing ESP32-C3 HAL");

    /* Note: NVS and event loop are typically not deinitialized
     * in production code as they may be used by other components */

    s_hal_initialized = false;
    ESP_LOGI(TAG, "ESP32-C3 HAL deinitialized");

    return 0;
}

int esp32c3_hal_register_all(void)
{
    int rc;

    rc = esp32c3_hal_init();
    if (rc != 0) {
        return rc;
    }

    rc = hal_gpio_register_esp32c3();
    if (rc != 0) {
        ESP_LOGE(TAG, "Failed to register GPIO interface: %d", rc);
        return rc;
    }
    ESP_LOGI(TAG, "GPIO interface registered");

    rc = hal_uart_register_esp32c3();
    if (rc != 0) {
        ESP_LOGE(TAG, "Failed to register UART interface: %d", rc);
        return rc;
    }
    ESP_LOGI(TAG, "UART interface registered");

    ESP_LOGI(TAG, "All ESP32-C3 HAL interfaces registered");

    return 0;
}
