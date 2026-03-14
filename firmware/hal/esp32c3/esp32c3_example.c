/*
 * Copyright (c) 2026 kimi-fly Project
 * SPDX-License-Identifier: MIT
 *
 * ESP32-C3 HAL Usage Example
 *
 * This file demonstrates how to use the ESP32-C3 HAL layer
 * for WiFi communication and drone control.
 */

#include "esp32c3_hal.h"
#include "hal_interface.h"
#include "board_config.h"
#include "freertos/task.h"

#define TAG "esp32c3_example"

/* ============================================================================
 * Example 1: Basic HAL Initialization
 * ============================================================================ */
void example_hal_init(void)
{
    /* Initialize ESP32-C3 HAL (NVS, event loop, system interface) */
    int rc = esp32c3_hal_init();
    if (rc != 0) {
        ESP_LOGE(TAG, "HAL init failed: %d", rc);
        return;
    }

    /* Register all HAL interfaces (GPIO, UART) */
    rc = esp32c3_hal_register_all();
    if (rc != 0) {
        ESP_LOGE(TAG, "HAL register failed: %d", rc);
        return;
    }

    ESP_LOGI(TAG, "HAL initialized successfully");
}

/* ============================================================================
 * Example 2: WiFi AP Mode
 * ============================================================================ */
void example_wifi_ap(void)
{
    /* Initialize WiFi subsystem */
    int rc = esp32c3_wifi_init();
    if (rc != 0) {
        ESP_LOGE(TAG, "WiFi init failed: %d", rc);
        return;
    }

    /* Start AP with default configuration */
    /* SSID: kimi-fly-XXXX (last 4 digits of MAC) */
    /* Password: kimifly123 */
    uint8_t mac[6];
    esp32c3_get_unique_id(mac, sizeof(mac));
    char ssid[32];
    snprintf(ssid, sizeof(ssid), "kimi-fly-%02X%02X", mac[4], mac[5]);

    rc = esp32c3_wifi_ap_start(ssid, ESP32C3_WIFI_AP_PASS,
                                ESP32C3_WIFI_AP_CHANNEL);
    if (rc != 0) {
        ESP_LOGE(TAG, "AP start failed: %d", rc);
        return;
    }

    /* Get and display AP IP address */
    char ip_str[16];
    rc = esp32c3_wifi_get_ap_ip(ip_str, sizeof(ip_str));
    if (rc == 0) {
        ESP_LOGI(TAG, "AP started: SSID=%s, IP=%s", ssid, ip_str);
    }
}

/* ============================================================================
 * Example 3: UDP Communication
 * ============================================================================ */
void example_udp_server(void)
{
    /* Initialize UDP server on port 8888 */
    int rc = esp32c3_udp_init(ESP32C3_UDP_PORT_DEFAULT);
    if (rc != 0) {
        ESP_LOGE(TAG, "UDP init failed: %d", rc);
        return;
    }

    ESP_LOGI(TAG, "UDP server listening on port %d", ESP32C3_UDP_PORT_DEFAULT);

    /* Receive buffer */
    uint8_t rx_buf[256];

    /* Main receive loop */
    while (1) {
        /* Receive data with 100ms timeout */
        int len = esp32c3_udp_recv(rx_buf, sizeof(rx_buf), 100);

        if (len > 0) {
            ESP_LOGI(TAG, "Received %d bytes", len);

            /* Parse control command */
            esp32c3_control_cmd_t cmd;
            rc = esp32c3_proto_parse_control(rx_buf, len, &cmd);

            if (rc == 0) {
                ESP_LOGI(TAG, "Control: throttle=%d, roll=%d, pitch=%d, yaw=%d",
                         cmd.throttle, cmd.roll, cmd.pitch, cmd.yaw);

                /* Send acknowledgment */
                esp32c3_status_tel_t status = {
                    .armed = 1,
                    .mode = 0,
                    .voltage = 1260,  /* 12.6V */
                    .rssi = -45
                };

                uint8_t tx_buf[32];
                int tx_len = esp32c3_proto_build_telemetry(
                    tx_buf, sizeof(tx_buf),
                    ESP32C3_TEL_STATUS, &status, sizeof(status));

                if (tx_len > 0) {
                    char client_ip[16];
                    if (esp32c3_udp_get_sender_ip(client_ip, sizeof(client_ip)) == 0) {
                        uint16_t client_port = esp32c3_udp_get_sender_port();
                        esp32c3_udp_send(tx_buf, tx_len, client_ip, client_port);
                    }
                }
            }
        }

        /* Small delay to prevent busy-looping */
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

/* ============================================================================
 * Example 4: GPIO Control
 * ============================================================================ */
void example_gpio(void)
{
    const hal_gpio_interface_t *gpio = hal_gpio_get_interface();
    if (gpio == NULL) {
        ESP_LOGE(TAG, "GPIO interface not available");
        return;
    }

    /* Configure status LED as output */
    hal_gpio_cfg_t led_cfg = {
        .pin = LED_STATUS_PIN,
        .dir = HAL_GPIO_DIR_OUTPUT,
        .pull = HAL_GPIO_PULL_NONE,
        .init_val = HAL_GPIO_LOW
    };

    hal_handle_t led_handle;
    int rc = gpio->init(&led_cfg, &led_handle);
    if (rc != 0) {
        ESP_LOGE(TAG, "GPIO init failed: %d", rc);
        return;
    }

    /* Blink LED */
    for (int i = 0; i < 10; i++) {
        gpio->set(led_handle, HAL_GPIO_HIGH);
        esp32c3_delay_ms(500);
        gpio->set(led_handle, HAL_GPIO_LOW);
        esp32c3_delay_ms(500);
    }

    gpio->deinit(led_handle);
}

/* ============================================================================
 * Example 5: UART Communication with STM32
 * ============================================================================ */
void example_uart_stm32(void)
{
    const hal_uart_interface_t *uart = hal_uart_get_interface();
    if (uart == NULL) {
        ESP_LOGE(TAG, "UART interface not available");
        return;
    }

    /* Configure UART1 for STM32 communication */
    hal_uart_cfg_t uart_cfg = {
        .baudrate = UART_ESP32_BAUDRATE,  /* 921600 */
        .data_bits = HAL_UART_DATA_BITS_8,
        .stop_bits = HAL_UART_STOP_BITS_1,
        .parity = HAL_UART_PARITY_NONE
    };

    hal_handle_t uart_handle;
    int rc = uart->init(UART_ESP32, &uart_cfg, &uart_handle);
    if (rc != 0) {
        ESP_LOGE(TAG, "UART init failed: %d", rc);
        return;
    }

    /* Send data to STM32 */
    const uint8_t tx_data[] = {0xAA, 0x55, 0x01, 0x00, 0x00};
    int sent = uart->send(uart_handle, tx_data, sizeof(tx_data), 100);
    ESP_LOGI(TAG, "Sent %d bytes to STM32", sent);

    /* Receive response */
    uint8_t rx_buf[64];
    int received = uart->recv(uart_handle, rx_buf, sizeof(rx_buf), 100);
    if (received > 0) {
        ESP_LOGI(TAG, "Received %d bytes from STM32", received);
    }

    uart->deinit(uart_handle);
}

/* ============================================================================
 * Example 6: Complete Drone Control Task
 * ============================================================================ */
void drone_control_task(void *pvParameters)
{
    (void)pvParameters;

    ESP_LOGI(TAG, "Starting drone control task");

    /* Initialize HAL */
    example_hal_init();

    /* Initialize WiFi AP */
    example_wifi_ap();

    /* Initialize UDP server */
    int rc = esp32c3_udp_init(ESP32C3_UDP_PORT_DEFAULT);
    if (rc != 0) {
        ESP_LOGE(TAG, "UDP init failed");
        vTaskDelete(NULL);
        return;
    }

    /* Control loop */
    uint8_t rx_buf[256];
    esp32c3_control_cmd_t last_cmd = {0};

    while (1) {
        /* Receive command */
        int len = esp32c3_udp_recv(rx_buf, sizeof(rx_buf), 10);

        if (len > 0) {
            /* Parse command */
            esp32c3_control_cmd_t cmd;
            rc = esp32c3_proto_parse_control(rx_buf, len, &cmd);

            if (rc == 0) {
                /* Valid command received - update control values */
                last_cmd = cmd;

                /* Forward to STM32 via UART */
                const hal_uart_interface_t *uart = hal_uart_get_interface();
                if (uart != NULL) {
                    hal_handle_t uart_handle = esp32c3_uart_get_handle(UART_ESP32);
                    if (uart_handle != NULL) {
                        uart->send(uart_handle, rx_buf, len, 10);
                    }
                }

                ESP_LOGD(TAG, "CMD: T=%d R=%d P=%d Y=%d",
                         cmd.throttle, cmd.roll, cmd.pitch, cmd.yaw);
            }
        }

        /* Send telemetry periodically (every 100ms) */
        static uint32_t last_tel_time = 0;
        uint32_t now = esp32c3_get_tick_ms();

        if (now - last_tel_time >= 100) {
            last_tel_time = now;

            /* Build status telemetry */
            esp32c3_status_tel_t status = {
                .armed = 1,
                .mode = 0,
                .voltage = 1260,  /* 12.6V * 100 */
                .rssi = -50
            };

            uint8_t tx_buf[32];
            int tx_len = esp32c3_proto_build_telemetry(
                tx_buf, sizeof(tx_buf),
                ESP32C3_TEL_STATUS, &status, sizeof(status));

            if (tx_len > 0) {
                char client_ip[16];
                if (esp32c3_udp_get_sender_ip(client_ip, sizeof(client_ip)) == 0) {
                    uint16_t client_port = esp32c3_udp_get_sender_port();
                    esp32c3_udp_send(tx_buf, tx_len, client_ip, client_port);
                }
            }
        }

        /* Small delay */
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

/* ============================================================================
 * Main Entry Point
 * ============================================================================ */
void app_main(void)
{
    ESP_LOGI(TAG, "kimi-fly ESP32-C3 starting...");

    /* Create drone control task */
    xTaskCreate(drone_control_task, "drone_ctrl", 4096, NULL, 5, NULL);
}
