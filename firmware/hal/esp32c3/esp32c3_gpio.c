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
 * @file esp32c3_gpio.c
 * @brief ESP32-C3 HAL GPIO implementation
 *
 * Implements GPIO interface for ESP32-C3 using ESP-IDF GPIO driver.
 */

#include "esp32c3_hal.h"
#include "hal_interface.h"
#include <errno.h>

#define TAG "esp32c3_gpio"

/* Maximum number of GPIO handles */
#define GPIO_MAX_HANDLES    16

/* GPIO handle structure */
typedef struct {
    gpio_num_t pin;
    bool initialized;
} gpio_handle_t;

/* GPIO handle pool - static allocation, no malloc */
static gpio_handle_t s_gpio_handles[GPIO_MAX_HANDLES];
static bool s_gpio_initialized = false;

/* Forward declarations for GPIO interface */
static int esp32c3_gpio_init(const hal_gpio_cfg_t *cfg, hal_handle_t *handle);
static int esp32c3_gpio_set(hal_handle_t handle, hal_gpio_state_t state);
static int esp32c3_gpio_get(hal_handle_t handle, hal_gpio_state_t *state);
static int esp32c3_gpio_toggle(hal_handle_t handle);
static int esp32c3_gpio_deinit(hal_handle_t handle);

/* GPIO interface instance */
static const hal_gpio_interface_t s_esp32c3_gpio_iface = {
    .init = esp32c3_gpio_init,
    .set = esp32c3_gpio_set,
    .get = esp32c3_gpio_get,
    .toggle = esp32c3_gpio_toggle,
    .deinit = esp32c3_gpio_deinit
};

/**
 * @brief Convert HAL pull configuration to ESP-IDF pull mode
 */
static gpio_pull_mode_t hal_pull_to_esp(hal_gpio_pull_t pull)
{
    switch (pull) {
    case HAL_GPIO_PULL_UP:
        return GPIO_PULLUP_ONLY;
    case HAL_GPIO_PULL_DOWN:
        return GPIO_PULLDOWN_ONLY;
    case HAL_GPIO_PULL_NONE:
    default:
        return GPIO_FLOATING;
    }
}

/**
 * @brief Allocate a GPIO handle from the pool
 */
static gpio_handle_t *gpio_handle_alloc(void)
{
    for (int i = 0; i < GPIO_MAX_HANDLES; i++) {
        if (!s_gpio_handles[i].initialized) {
            s_gpio_handles[i].initialized = true;
            return &s_gpio_handles[i];
        }
    }
    return NULL;
}

/**
 * @brief Free a GPIO handle back to the pool
 */
static void gpio_handle_free(gpio_handle_t *handle)
{
    if (handle != NULL) {
        handle->initialized = false;
        handle->pin = GPIO_NUM_NC;
    }
}

/**
 * @brief Validate GPIO pin number
 */
static bool gpio_pin_valid(gpio_num_t pin)
{
    /* ESP32-C3 has GPIO 0-20 available */
    return (pin >= 0 && pin <= GPIO_NUM_MAX);
}

/* ============================================================================
 * GPIO Interface Implementations
 * ============================================================================ */

static int esp32c3_gpio_init(const hal_gpio_cfg_t *cfg, hal_handle_t *handle)
{
    if (cfg == NULL || handle == NULL) {
        ESP_LOGE(TAG, "Invalid parameters");
        return -EINVAL;
    }

    gpio_num_t pin = (gpio_num_t)cfg->pin;

    if (!gpio_pin_valid(pin)) {
        ESP_LOGE(TAG, "Invalid GPIO pin: %d", pin);
        return -EINVAL;
    }

    /* Allocate handle */
    gpio_handle_t *gpio_handle = gpio_handle_alloc();
    if (gpio_handle == NULL) {
        ESP_LOGE(TAG, "No free GPIO handles");
        return -ENOMEM;
    }

    /* Configure GPIO */
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << pin),
        .mode = (cfg->dir == HAL_GPIO_DIR_OUTPUT) ? GPIO_MODE_OUTPUT : GPIO_MODE_INPUT,
        .pull_up_en = (cfg->pull == HAL_GPIO_PULL_UP) ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE,
        .pull_down_en = (cfg->pull == HAL_GPIO_PULL_DOWN) ? GPIO_PULLDOWN_ENABLE : GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    esp_err_t err = gpio_config(&io_conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "GPIO config failed for pin %d: %d", pin, err);
        gpio_handle_free(gpio_handle);
        return -EIO;
    }

    /* Set initial value for output pins */
    if (cfg->dir == HAL_GPIO_DIR_OUTPUT) {
        uint32_t level = (cfg->init_val == HAL_GPIO_HIGH) ? 1 : 0;
        err = gpio_set_level(pin, level);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "GPIO set level failed for pin %d: %d", pin, err);
            gpio_handle_free(gpio_handle);
            return -EIO;
        }
    }

    gpio_handle->pin = pin;
    *handle = (hal_handle_t)gpio_handle;

    ESP_LOGI(TAG, "GPIO %d initialized as %s", pin,
             (cfg->dir == HAL_GPIO_DIR_OUTPUT) ? "output" : "input");

    return 0;
}

static int esp32c3_gpio_set(hal_handle_t handle, hal_gpio_state_t state)
{
    gpio_handle_t *gpio_handle = (gpio_handle_t *)handle;

    if (gpio_handle == NULL || !gpio_handle->initialized) {
        ESP_LOGE(TAG, "Invalid handle");
        return -EINVAL;
    }

    uint32_t level = (state == HAL_GPIO_HIGH) ? 1 : 0;
    esp_err_t err = gpio_set_level(gpio_handle->pin, level);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "GPIO set failed for pin %d: %d", gpio_handle->pin, err);
        return -EIO;
    }

    return 0;
}

static int esp32c3_gpio_get(hal_handle_t handle, hal_gpio_state_t *state)
{
    gpio_handle_t *gpio_handle = (gpio_handle_t *)handle;

    if (gpio_handle == NULL || !gpio_handle->initialized || state == NULL) {
        ESP_LOGE(TAG, "Invalid parameters");
        return -EINVAL;
    }

    int level = gpio_get_level(gpio_handle->pin);
    if (level < 0) {
        ESP_LOGE(TAG, "GPIO get failed for pin %d", gpio_handle->pin);
        return -EIO;
    }

    *state = (level != 0) ? HAL_GPIO_HIGH : HAL_GPIO_LOW;

    return 0;
}

static int esp32c3_gpio_toggle(hal_handle_t handle)
{
    gpio_handle_t *gpio_handle = (gpio_handle_t *)handle;

    if (gpio_handle == NULL || !gpio_handle->initialized) {
        ESP_LOGE(TAG, "Invalid handle");
        return -EINVAL;
    }

    /* Read current level */
    int current_level = gpio_get_level(gpio_handle->pin);
    if (current_level < 0) {
        ESP_LOGE(TAG, "GPIO get failed for pin %d", gpio_handle->pin);
        return -EIO;
    }

    /* Toggle and set */
    uint32_t new_level = (current_level == 0) ? 1 : 0;
    esp_err_t err = gpio_set_level(gpio_handle->pin, new_level);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "GPIO toggle failed for pin %d: %d", gpio_handle->pin, err);
        return -EIO;
    }

    return 0;
}

static int esp32c3_gpio_deinit(hal_handle_t handle)
{
    gpio_handle_t *gpio_handle = (gpio_handle_t *)handle;

    if (gpio_handle == NULL || !gpio_handle->initialized) {
        ESP_LOGE(TAG, "Invalid handle");
        return -EINVAL;
    }

    /* Reset pin to input floating state */
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << gpio_handle->pin),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    esp_err_t err = gpio_config(&io_conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "GPIO deinit config failed for pin %d: %d", gpio_handle->pin, err);
        return -EIO;
    }

    ESP_LOGI(TAG, "GPIO %d deinitialized", gpio_handle->pin);

    gpio_handle_free(gpio_handle);

    return 0;
}

/* ============================================================================
 * Public Functions
 * ============================================================================ */

int hal_gpio_register_esp32c3(void)
{
    if (s_gpio_initialized) {
        ESP_LOGW(TAG, "GPIO already registered");
        return 0;
    }

    /* Initialize handle pool */
    for (int i = 0; i < GPIO_MAX_HANDLES; i++) {
        s_gpio_handles[i].initialized = false;
        s_gpio_handles[i].pin = GPIO_NUM_NC;
    }

    int rc = hal_gpio_register(&s_esp32c3_gpio_iface);
    if (rc != 0) {
        ESP_LOGE(TAG, "Failed to register GPIO interface: %d", rc);
        return rc;
    }

    s_gpio_initialized = true;
    ESP_LOGI(TAG, "ESP32-C3 GPIO interface registered");

    return 0;
}
