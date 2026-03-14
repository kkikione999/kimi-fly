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
 * @file esp32c3_hal.h
 * @brief ESP32-C3 HAL platform-specific definitions
 *
 * This file contains ESP32-C3 specific definitions and ESP-IDF includes
 * for the hardware abstraction layer.
 */

#ifndef ESP32C3_HAL_H
#define ESP32C3_HAL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* ESP-IDF headers */
#include "esp_system.h"
#include "esp_log.h"
#include "esp_err.h"
#include "nvs_flash.h"
#include "esp_timer.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* GPIO */
#include "driver/gpio.h"

/* UART */
#include "driver/uart.h"

/* WiFi */
#include "esp_wifi.h"
#include "esp_event.h"

/* Network */
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "lwip/inet.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup ESP32C3_HAL_DEFS ESP32-C3 HAL Definitions
 * @{
 */

/**
 * @brief ESP32-C3 HAL version
 */
#define ESP32C3_HAL_VERSION_MAJOR   0
#define ESP32C3_HAL_VERSION_MINOR   1
#define ESP32C3_HAL_VERSION_PATCH   0

/**
 * @brief ESP32-C3 specific constants
 */
#define ESP32C3_GPIO_MAX_PIN        21  /* GPIO 0-20 available on ESP32-C3 */
#define ESP32C3_UART_PORT_NUM       2   /* UART0 and UART1 */
#define ESP32C3_WIFI_SSID_MAX_LEN   32
#define ESP32C3_WIFI_PASS_MAX_LEN   64

/**
 * @brief Default WiFi AP configuration
 */
#define ESP32C3_WIFI_AP_SSID        "kimi-fly"
#define ESP32C3_WIFI_AP_PASS        "kimifly123"
#define ESP32C3_WIFI_AP_CHANNEL     6
#define ESP32C3_WIFI_AP_MAX_CONN    4

/**
 * @brief Default UDP configuration
 */
#define ESP32C3_UDP_PORT_DEFAULT    8888
#define ESP32C3_UDP_BUF_SIZE        256
#define ESP32C3_UDP_TIMEOUT_MS      100

/**
 * @brief UART configuration for ESP32-C3
 */
#define ESP32C3_UART_BUF_SIZE       256
#define ESP32C3_UART_QUEUE_SIZE     10

/** @} */

/**
 * @defgroup ESP32C3_HAL_INIT Initialization Functions
 * @{
 */

/**
 * @brief Initialize ESP32-C3 HAL layer
 *
 * Initializes NVS, event loop, and other ESP-IDF subsystems.
 * Must be called before using any other HAL functions.
 *
 * @return 0 on success, negative error code on failure
 */
int esp32c3_hal_init(void);

/**
 * @brief Deinitialize ESP32-C3 HAL layer
 *
 * @return 0 on success, negative error code on failure
 */
int esp32c3_hal_deinit(void);

/**
 * @brief Register all ESP32-C3 HAL interfaces
 *
 * Convenience function to register GPIO, UART, and system interfaces.
 *
 * @return 0 on success, negative error code on failure
 */
int esp32c3_hal_register_all(void);

/** @} */

/**
 * @defgroup ESP32C3_HAL_SYSTEM System Functions
 * @{
 */

/**
 * @brief Get current tick count in milliseconds
 *
 * Uses esp_timer_get_time() for high-resolution timing.
 *
 * @return Milliseconds since system start
 */
uint32_t esp32c3_get_tick_ms(void);

/**
 * @brief Delay for specified milliseconds
 *
 * Uses vTaskDelay for FreeRTOS-aware blocking.
 *
 * @param ms Milliseconds to delay
 */
void esp32c3_delay_ms(uint32_t ms);

/**
 * @brief Delay for specified microseconds
 *
 * Uses esp_rom_delay_us for busy-wait delay.
 * Should only be used for short delays (< 1000us).
 *
 * @param us Microseconds to delay
 */
void esp32c3_delay_us(uint32_t us);

/**
 * @brief Enter critical section
 *
 * Disables interrupts and returns previous state.
 *
 * @return Previous interrupt state
 */
uint32_t esp32c3_enter_critical(void);

/**
 * @brief Exit critical section
 *
 * Restores interrupt state.
 *
 * @param state State returned by esp32c3_enter_critical()
 */
void esp32c3_exit_critical(uint32_t state);

/**
 * @brief Get unique device ID
 *
 * Reads the MAC address as the unique device identifier.
 *
 * @param[out] id Buffer to store device ID (at least 6 bytes)
 * @param len Buffer length
 * @return Number of bytes written, or negative error code
 */
int esp32c3_get_unique_id(uint8_t *id, size_t len);

/**
 * @brief Reset the system
 *
 * Performs a software reset of the ESP32-C3.
 */
void esp32c3_system_reset(void);

/** @} */

/**
 * @defgroup ESP32C3_HAL_GPIO GPIO Functions
 * @{
 */

/**
 * @brief Register ESP32-C3 GPIO interface
 *
 * @return 0 on success, negative error code on failure
 */
int hal_gpio_register_esp32c3(void);

/** @} */

/**
 * @defgroup ESP32C3_HAL_UART UART Functions
 * @{
 */

/**
 * @brief Register ESP32-C3 UART interface
 *
 * @return 0 on success, negative error code on failure
 */
int hal_uart_register_esp32c3(void);

/**
 * @brief Get UART handle for a specific port
 *
 * @param port UART port number (0 or 1)
 * @return Handle to UART instance, or NULL if not initialized
 */
hal_handle_t esp32c3_uart_get_handle(uint8_t port);

/** @} */

/**
 * @defgroup ESP32C3_HAL_WIFI WiFi Functions
 * @{
 */

/**
 * @brief WiFi event callback type
 */
typedef void (*esp32c3_wifi_event_cb_t)(esp_event_base_t event_base,
                                         int32_t event_id,
                                         void *event_data);

/**
 * @brief Initialize WiFi subsystem
 *
 * Initializes WiFi driver and event loop. Must be called before
 * starting AP or STA mode.
 *
 * @return 0 on success, negative error code on failure
 */
int esp32c3_wifi_init(void);

/**
 * @brief Deinitialize WiFi subsystem
 *
 * @return 0 on success, negative error code on failure
 */
int esp32c3_wifi_deinit(void);

/**
 * @brief Start WiFi in AP mode
 *
 * Creates a WiFi access point with the specified SSID and password.
 *
 * @param ssid Network name (1-32 characters)
 * @param pass Password (8-63 characters, or NULL for open network)
 * @param channel WiFi channel (1-13)
 * @return 0 on success, negative error code on failure
 */
int esp32c3_wifi_ap_start(const char *ssid, const char *pass, uint8_t channel);

/**
 * @brief Stop WiFi AP mode
 *
 * @return 0 on success, negative error code on failure
 */
int esp32c3_wifi_ap_stop(void);

/**
 * @brief Get AP IP address
 *
 * @param[out] ip_str Buffer to store IP string (at least 16 bytes)
 * @param len Buffer length
 * @return 0 on success, negative error code on failure
 */
int esp32c3_wifi_get_ap_ip(char *ip_str, size_t len);

/**
 * @brief Set WiFi event callback
 *
 * @param cb Callback function, or NULL to disable
 */
void esp32c3_wifi_set_event_cb(esp32c3_wifi_event_cb_t cb);

/** @} */

/**
 * @defgroup ESP32C3_HAL_UDP UDP Communication Functions
 * @{
 */

/**
 * @brief Initialize UDP server
 *
 * Creates a UDP socket bound to the specified port.
 *
 * @param port UDP port number
 * @return 0 on success, negative error code on failure
 */
int esp32c3_udp_init(uint16_t port);

/**
 * @brief Deinitialize UDP server
 *
 * @return 0 on success, negative error code on failure
 */
int esp32c3_udp_deinit(void);

/**
 * @brief Receive UDP data
 *
 * Blocks until data is received or timeout occurs.
 *
 * @param[out] buf Buffer to receive data
 * @param len Maximum bytes to receive
 * @param timeout_ms Timeout in milliseconds (0 for non-blocking)
 * @return Number of bytes received, 0 on timeout, negative on error
 */
int esp32c3_udp_recv(uint8_t *buf, size_t len, uint32_t timeout_ms);

/**
 * @brief Send UDP data
 *
 * @param buf Data buffer to send
 * @param len Number of bytes to send
 * @param ip Destination IP address string
 * @param port Destination port
 * @return Number of bytes sent, or negative error code
 */
int esp32c3_udp_send(const uint8_t *buf, size_t len, const char *ip,
                      uint16_t port);

/**
 * @brief Get last sender's IP address
 *
 * After receiving data, this returns the sender's address.
 *
 * @param[out] ip_str Buffer to store IP string (at least 16 bytes)
 * @param len Buffer length
 * @return 0 on success, negative error code on failure
 */
int esp32c3_udp_get_sender_ip(char *ip_str, size_t len);

/**
 * @brief Get last sender's port
 *
 * @return Port number, or 0 if no data received
 */
uint16_t esp32c3_udp_get_sender_port(void);

/** @} */

/**
 * @defgroup ESP32C3_HAL_PROTOCOL Communication Protocol
 * @{
 */

/**
 * @brief Protocol packet header bytes
 */
#define ESP32C3_PROTO_HDR_BYTE1     0xAA
#define ESP32C3_PROTO_HDR_BYTE2     0x55

/**
 * @brief Command types
 */
typedef enum {
    ESP32C3_CMD_CONTROL = 0x01,     /* Flight control command */
    ESP32C3_CMD_CONFIG = 0x02,      /* Configuration command */
    ESP32C3_CMD_CALIBRATE = 0x03,   /* Calibration command */
    ESP32C3_CMD_ARM = 0x04,         /* Arm/disarm motors */
    ESP32C3_CMD_MODE = 0x05,        /* Set flight mode */
} esp32c3_cmd_type_t;

/**
 * @brief Telemetry types
 */
typedef enum {
    ESP32C3_TEL_STATUS = 0x81,      /* Flight status */
    ESP32C3_TEL_IMU = 0x82,         /* IMU data */
    ESP32C3_TEL_BATTERY = 0x83,     /* Battery status */
    ESP32C3_TEL_GPS = 0x84,         /* GPS data */
} esp32c3_tel_type_t;

/**
 * @brief Control command payload
 */
typedef struct __attribute__((packed)) {
    int16_t throttle;   /* 0-1000, throttle value */
    int16_t roll;       /* -500 to 500, roll angle */
    int16_t pitch;      /* -500 to 500, pitch angle */
    int16_t yaw;        /* -500 to 500, yaw rate */
} esp32c3_control_cmd_t;

/**
 * @brief Status telemetry payload
 */
typedef struct __attribute__((packed)) {
    uint8_t armed;      /* 0 = disarmed, 1 = armed */
    uint8_t mode;       /* Flight mode */
    uint16_t voltage;   /* Battery voltage * 100 (e.g., 1260 = 12.6V) */
    int16_t rssi;       /* Signal strength in dBm */
} esp32c3_status_tel_t;

/**
 * @brief Parse a control command packet
 *
 * Validates and extracts control data from a protocol packet.
 *
 * @param[in] packet Raw packet data
 * @param len Packet length
 * @param[out] cmd Parsed control command
 * @return 0 on success, negative error code on failure
 */
int esp32c3_proto_parse_control(const uint8_t *packet, size_t len,
                                 esp32c3_control_cmd_t *cmd);

/**
 * @brief Build a telemetry packet
 *
 * Constructs a protocol packet with telemetry data.
 *
 * @param[out] buf Output buffer
 * @param buf_len Buffer size
 * @param type Telemetry type
 * @param[in] data Telemetry data
 * @param data_len Data length
 * @return Packet length, or negative error code
 */
int esp32c3_proto_build_telemetry(uint8_t *buf, size_t buf_len,
                                   esp32c3_tel_type_t type,
                                   const void *data, size_t data_len);

/**
 * @brief Calculate CRC8 checksum
 *
 * @param[in] data Data buffer
 * @param len Data length
 * @return CRC8 value
 */
uint8_t esp32c3_proto_crc8(const uint8_t *data, size_t len);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ESP32C3_HAL_H */
