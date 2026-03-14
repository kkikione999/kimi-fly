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
 * @file esp32c3_wifi.c
 * @brief ESP32-C3 WiFi and UDP communication implementation
 *
 * Implements WiFi AP mode and UDP communication for drone control.
 *
 * Features:
 * - WiFi AP mode with configurable SSID/password
 * - UDP server for command reception (port 8888)
 * - Protocol parsing for flight control commands
 * - Telemetry packet construction
 */

#include "esp32c3_hal.h"
#include <string.h>
#include <errno.h>

#define TAG "esp32c3_wifi"

/* CRC8 polynomial: x^8 + x^2 + x + 1 */
#define CRC8_POLYNOMIAL 0x07

/* Internal state */
static bool s_wifi_initialized = false;
static bool s_wifi_ap_started = false;
static esp_netif_t *s_ap_netif = NULL;
static esp32c3_wifi_event_cb_t s_wifi_event_cb = NULL;

/* UDP state */
static int s_udp_sock = -1;
static uint16_t s_udp_port = 0;
static struct sockaddr_in s_udp_client_addr;
static bool s_udp_client_valid = false;

/* Protocol state */
static uint8_t s_rx_buf[ESP32C3_UDP_BUF_SIZE];
static uint8_t s_tx_buf[ESP32C3_UDP_BUF_SIZE];

/* Forward declarations */
static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data);
static uint8_t crc8_update(uint8_t crc, uint8_t data);

/* ============================================================================
 * CRC8 Calculation
 * ============================================================================ */

static uint8_t crc8_update(uint8_t crc, uint8_t data)
{
    crc ^= data;
    for (int i = 0; i < 8; i++) {
        if (crc & 0x80) {
            crc = (crc << 1) ^ CRC8_POLYNOMIAL;
        } else {
            crc <<= 1;
        }
    }
    return crc;
}

uint8_t esp32c3_proto_crc8(const uint8_t *data, size_t len)
{
    uint8_t crc = 0x00;
    for (size_t i = 0; i < len; i++) {
        crc = crc8_update(crc, data[i]);
    }
    return crc;
}

/* ============================================================================
 * WiFi Event Handler
 * ============================================================================ */

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT) {
        switch (event_id) {
        case WIFI_EVENT_AP_START:
            ESP_LOGI(TAG, "WiFi AP started");
            s_wifi_ap_started = true;
            break;

        case WIFI_EVENT_AP_STOP:
            ESP_LOGI(TAG, "WiFi AP stopped");
            s_wifi_ap_started = false;
            break;

        case WIFI_EVENT_AP_STACONNECTED: {
            wifi_event_ap_staconnected_t *event =
                (wifi_event_ap_staconnected_t *)event_data;
            ESP_LOGI(TAG, "Station connected: MAC=" MACSTR ", AID=%d",
                     MAC2STR(event->mac), event->aid);
            break;
        }

        case WIFI_EVENT_AP_STADISCONNECTED: {
            wifi_event_ap_stadisconnected_t *event =
                (wifi_event_ap_stadisconnected_t *)event_data;
            ESP_LOGI(TAG, "Station disconnected: MAC=" MACSTR ", AID=%d",
                     MAC2STR(event->mac), event->aid);
            break;
        }

        default:
            ESP_LOGD(TAG, "WiFi event: %ld", event_id);
            break;
        }
    }

    /* Call user callback if registered */
    if (s_wifi_event_cb != NULL) {
        s_wifi_event_cb(event_base, event_id, event_data);
    }
}

/* ============================================================================
 * WiFi Functions
 * ============================================================================ */

int esp32c3_wifi_init(void)
{
    esp_err_t err;

    if (s_wifi_initialized) {
        ESP_LOGW(TAG, "WiFi already initialized");
        return 0;
    }

    ESP_LOGI(TAG, "Initializing WiFi subsystem");

    /* Initialize TCP/IP stack */
    err = esp_netif_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Netif init failed: %d", err);
        return -EIO;
    }

    /* Create default event loop (if not already created) */
    err = esp_event_loop_create_default();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "Event loop create failed: %d", err);
        return -EIO;
    }

    /* Initialize WiFi */
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    err = esp_wifi_init(&cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "WiFi init failed: %d", err);
        return -EIO;
    }

    /* Register event handler */
    err = esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                     &wifi_event_handler, NULL);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Event handler register failed: %d", err);
        esp_wifi_deinit();
        return -EIO;
    }

    s_wifi_initialized = true;
    ESP_LOGI(TAG, "WiFi subsystem initialized");

    return 0;
}

int esp32c3_wifi_deinit(void)
{
    esp_err_t err;

    if (!s_wifi_initialized) {
        return 0;
    }

    /* Stop AP if running */
    if (s_wifi_ap_started) {
        esp32c3_wifi_ap_stop();
    }

    /* Unregister event handler */
    err = esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                       &wifi_event_handler);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Event handler unregister failed: %d", err);
    }

    /* Deinit WiFi */
    err = esp_wifi_deinit();
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "WiFi deinit failed: %d", err);
    }

    s_wifi_initialized = false;
    ESP_LOGI(TAG, "WiFi subsystem deinitialized");

    return 0;
}

int esp32c3_wifi_ap_start(const char *ssid, const char *pass, uint8_t channel)
{
    esp_err_t err;

    if (!s_wifi_initialized) {
        ESP_LOGE(TAG, "WiFi not initialized");
        return -EINVAL;
    }

    if (s_wifi_ap_started) {
        ESP_LOGW(TAG, "AP already started, stopping first");
        esp32c3_wifi_ap_stop();
    }

    if (ssid == NULL || strlen(ssid) == 0 || strlen(ssid) > 32) {
        ESP_LOGE(TAG, "Invalid SSID");
        return -EINVAL;
    }

    if (channel < 1 || channel > 13) {
        ESP_LOGE(TAG, "Invalid channel: %d", channel);
        return -EINVAL;
    }

    /* Create AP interface */
    s_ap_netif = esp_netif_create_default_wifi_ap();
    if (s_ap_netif == NULL) {
        ESP_LOGE(TAG, "Failed to create AP netif");
        return -ENOMEM;
    }

    /* Configure AP */
    wifi_config_t wifi_config = {
        .ap = {
            .channel = channel,
            .max_connection = ESP32C3_WIFI_AP_MAX_CONN,
            .authmode = WIFI_AUTH_WPA2_PSK,
            .ssid_hidden = 0,
            .beacon_interval = 100
        }
    };

    strncpy((char *)wifi_config.ap.ssid, ssid, sizeof(wifi_config.ap.ssid) - 1);
    wifi_config.ap.ssid_len = strlen(ssid);

    if (pass != NULL && strlen(pass) >= 8) {
        strncpy((char *)wifi_config.ap.password, pass,
                sizeof(wifi_config.ap.password) - 1);
    } else {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
        wifi_config.ap.password[0] = '\0';
    }

    /* Set mode and config */
    err = esp_wifi_set_mode(WIFI_MODE_AP);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Set WiFi mode failed: %d", err);
        return -EIO;
    }

    err = esp_wifi_set_config(WIFI_IF_AP, &wifi_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Set WiFi config failed: %d", err);
        return -EIO;
    }

    /* Start WiFi */
    err = esp_wifi_start();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "WiFi start failed: %d", err);
        return -EIO;
    }

    /* Set transmit power (lower for drone use to reduce interference) */
    esp_wifi_set_max_tx_power(52);  /* ~10 dBm */

    ESP_LOGI(TAG, "WiFi AP started: SSID=%s, Channel=%d, Auth=%s",
             ssid, channel,
             (wifi_config.ap.authmode == WIFI_AUTH_OPEN) ? "Open" : "WPA2");

    return 0;
}

int esp32c3_wifi_ap_stop(void)
{
    esp_err_t err;

    if (!s_wifi_ap_started) {
        return 0;
    }

    err = esp_wifi_stop();
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "WiFi stop failed: %d", err);
    }

    if (s_ap_netif != NULL) {
        esp_netif_destroy_default_wifi(s_ap_netif);
        s_ap_netif = NULL;
    }

    s_wifi_ap_started = false;
    ESP_LOGI(TAG, "WiFi AP stopped");

    return 0;
}

int esp32c3_wifi_get_ap_ip(char *ip_str, size_t len)
{
    if (ip_str == NULL || len < 16) {
        return -EINVAL;
    }

    if (!s_wifi_ap_started || s_ap_netif == NULL) {
        return -ENOTCONN;
    }

    esp_netif_ip_info_t ip_info;
    esp_err_t err = esp_netif_get_ip_info(s_ap_netif, &ip_info);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Get IP info failed: %d", err);
        return -EIO;
    }

    snprintf(ip_str, len, IPSTR, IP2STR(&ip_info.ip));

    return 0;
}

void esp32c3_wifi_set_event_cb(esp32c3_wifi_event_cb_t cb)
{
    s_wifi_event_cb = cb;
}

/* ============================================================================
 * UDP Functions
 * ============================================================================ */

int esp32c3_udp_init(uint16_t port)
{
    int sock;
    struct sockaddr_in server_addr;
    int opt = 1;

    if (s_udp_sock >= 0) {
        ESP_LOGW(TAG, "UDP already initialized, closing first");
        esp32c3_udp_deinit();
    }

    /* Create socket */
    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0) {
        ESP_LOGE(TAG, "Socket creation failed: errno=%d", errno);
        return -EIO;
    }

    /* Set socket options */
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        ESP_LOGW(TAG, "Set SO_REUSEADDR failed: errno=%d", errno);
    }

    /* Set receive timeout */
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = ESP32C3_UDP_TIMEOUT_MS * 1000;
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        ESP_LOGW(TAG, "Set receive timeout failed: errno=%d", errno);
    }

    /* Bind to port */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        ESP_LOGE(TAG, "Socket bind failed: errno=%d", errno);
        close(sock);
        return -EIO;
    }

    s_udp_sock = sock;
    s_udp_port = port;
    s_udp_client_valid = false;

    ESP_LOGI(TAG, "UDP server initialized on port %d", port);

    return 0;
}

int esp32c3_udp_deinit(void)
{
    if (s_udp_sock < 0) {
        return 0;
    }

    close(s_udp_sock);
    s_udp_sock = -1;
    s_udp_port = 0;
    s_udp_client_valid = false;

    ESP_LOGI(TAG, "UDP server deinitialized");

    return 0;
}

int esp32c3_udp_recv(uint8_t *buf, size_t len, uint32_t timeout_ms)
{
    if (s_udp_sock < 0) {
        return -ENOTCONN;
    }

    if (buf == NULL || len == 0) {
        return -EINVAL;
    }

    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    /* Set timeout if specified */
    if (timeout_ms != ESP32C3_UDP_TIMEOUT_MS) {
        struct timeval tv;
        tv.tv_sec = timeout_ms / 1000;
        tv.tv_usec = (timeout_ms % 1000) * 1000;
        setsockopt(s_udp_sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    }

    ssize_t received = recvfrom(s_udp_sock, buf, len, 0,
                                (struct sockaddr *)&client_addr, &client_len);

    /* Restore default timeout */
    if (timeout_ms != ESP32C3_UDP_TIMEOUT_MS) {
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = ESP32C3_UDP_TIMEOUT_MS * 1000;
        setsockopt(s_udp_sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    }

    if (received < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return 0;  /* Timeout */
        }
        ESP_LOGE(TAG, "UDP recv failed: errno=%d", errno);
        return -EIO;
    }

    /* Save client address for reply */
    memcpy(&s_udp_client_addr, &client_addr, sizeof(client_addr));
    s_udp_client_valid = true;

    return (int)received;
}

int esp32c3_udp_send(const uint8_t *buf, size_t len, const char *ip,
                     uint16_t port)
{
    if (s_udp_sock < 0) {
        return -ENOTCONN;
    }

    if (buf == NULL || len == 0) {
        return -EINVAL;
    }

    struct sockaddr_in dest_addr;
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(port);

    if (inet_aton(ip, &dest_addr.sin_addr) == 0) {
        ESP_LOGE(TAG, "Invalid IP address: %s", ip);
        return -EINVAL;
    }

    ssize_t sent = sendto(s_udp_sock, buf, len, 0,
                          (struct sockaddr *)&dest_addr, sizeof(dest_addr));

    if (sent < 0) {
        ESP_LOGE(TAG, "UDP send failed: errno=%d", errno);
        return -EIO;
    }

    return (int)sent;
}

int esp32c3_udp_get_sender_ip(char *ip_str, size_t len)
{
    if (ip_str == NULL || len < 16) {
        return -EINVAL;
    }

    if (!s_udp_client_valid) {
        return -ENOTCONN;
    }

    strncpy(ip_str, inet_ntoa(s_udp_client_addr.sin_addr), len - 1);
    ip_str[len - 1] = '\0';

    return 0;
}

uint16_t esp32c3_udp_get_sender_port(void)
{
    if (!s_udp_client_valid) {
        return 0;
    }

    return ntohs(s_udp_client_addr.sin_port);
}

/* ============================================================================
 * Protocol Functions
 * ============================================================================ */

int esp32c3_proto_parse_control(const uint8_t *packet, size_t len,
                                esp32c3_control_cmd_t *cmd)
{
    if (packet == NULL || cmd == NULL || len < 12) {
        return -EINVAL;
    }

    /* Check header */
    if (packet[0] != ESP32C3_PROTO_HDR_BYTE1 ||
        packet[1] != ESP32C3_PROTO_HDR_BYTE2) {
        ESP_LOGW(TAG, "Invalid packet header");
        return -EINVAL;
    }

    /* Check command type */
    if (packet[2] != ESP32C3_CMD_CONTROL) {
        ESP_LOGW(TAG, "Invalid command type: 0x%02X", packet[2]);
        return -EINVAL;
    }

    /* Check length */
    uint8_t payload_len = packet[3];
    if (payload_len != sizeof(esp32c3_control_cmd_t)) {
        ESP_LOGW(TAG, "Invalid payload length: %d", payload_len);
        return -EINVAL;
    }

    /* Check total length */
    if (len < (size_t)(4 + payload_len + 1)) {
        ESP_LOGW(TAG, "Packet too short");
        return -EINVAL;
    }

    /* Verify CRC */
    uint8_t crc = esp32c3_proto_crc8(packet, 4 + payload_len);
    if (crc != packet[4 + payload_len]) {
        ESP_LOGW(TAG, "CRC mismatch: calc=0x%02X, recv=0x%02X",
                 crc, packet[4 + payload_len]);
        return -EINVAL;
    }

    /* Extract control values (little-endian) */
    cmd->throttle = (int16_t)(packet[4] | (packet[5] << 8));
    cmd->roll = (int16_t)(packet[6] | (packet[7] << 8));
    cmd->pitch = (int16_t)(packet[8] | (packet[9] << 8));
    cmd->yaw = (int16_t)(packet[10] | (packet[11] << 8));

    /* Validate ranges */
    if (cmd->throttle < 0 || cmd->throttle > 1000) {
        ESP_LOGW(TAG, "Invalid throttle: %d", cmd->throttle);
        return -EINVAL;
    }

    if (cmd->roll < -500 || cmd->roll > 500) {
        ESP_LOGW(TAG, "Invalid roll: %d", cmd->roll);
        return -EINVAL;
    }

    if (cmd->pitch < -500 || cmd->pitch > 500) {
        ESP_LOGW(TAG, "Invalid pitch: %d", cmd->pitch);
        return -EINVAL;
    }

    if (cmd->yaw < -500 || cmd->yaw > 500) {
        ESP_LOGW(TAG, "Invalid yaw: %d", cmd->yaw);
        return -EINVAL;
    }

    return 0;
}

int esp32c3_proto_build_telemetry(uint8_t *buf, size_t buf_len,
                                  esp32c3_tel_type_t type,
                                  const void *data, size_t data_len)
{
    if (buf == NULL || data == NULL) {
        return -EINVAL;
    }

    size_t packet_len = 4 + data_len + 1;  /* header + type + len + data + crc */
    if (buf_len < packet_len) {
        return -ENOMEM;
    }

    /* Build packet */
    buf[0] = ESP32C3_PROTO_HDR_BYTE1;
    buf[1] = ESP32C3_PROTO_HDR_BYTE2;
    buf[2] = (uint8_t)type;
    buf[3] = (uint8_t)data_len;

    memcpy(&buf[4], data, data_len);

    /* Calculate CRC */
    buf[4 + data_len] = esp32c3_proto_crc8(buf, 4 + data_len);

    return (int)packet_len;
}
