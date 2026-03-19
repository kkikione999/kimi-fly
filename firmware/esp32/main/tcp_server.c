/**
 * @file tcp_server.c
 * @brief ESP32-C3 TCP Server Implementation
 * @note Receives commands from ground station and forwards to STM32 via UART
 *       Protocol: [HEADER:2][LEN:1][CMD:1][DATA:N][CRC:2]
 *       Header: 0xAA55
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "driver/uart.h"
#include "esp_log.h"
#include "esp_err.h"

#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "arpa/inet.h"

#include "tcp_server.h"
#include "wifi_sta.h"

/* ============================================================================
 * Configuration
 * ============================================================================ */

#define TAG "TCP_SERVER"

/* UART Configuration for STM32 communication - matches main.c */
#define UART_ST_PORT            UART_NUM_1
#define UART_ST_BAUDRATE        115200
#define UART_ST_BUF_SIZE        256

/* Protocol defines - matches main.c */
#define PROTOCOL_HEADER         0xAA55
#define PROTOCOL_MAX_PAYLOAD    64

/* ============================================================================
 * Global Variables
 * ============================================================================ */

static volatile bool server_running = false;
static volatile int client_count = 0;
static int listen_sock = -1;
static SemaphoreHandle_t client_mutex = NULL;

/* ============================================================================
 * Protocol Functions
 * ============================================================================ */

/**
 * @brief Calculate CCITT CRC16
 */
static uint16_t calculate_crc16(const uint8_t *data, size_t length)
{
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < length; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for (int j = 0; j < 8; j++) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ 0x1021;
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}

/**
 * @brief Parse and validate protocol frame
 * @return Frame length if valid, 0 if incomplete, -1 if invalid
 */
static int parse_frame(const uint8_t *data, size_t len, uint8_t *cmd, uint8_t *payload, size_t *payload_len)
{
    if (len < 4) {
        return 0; /* Too short, need more data */
    }

    /* Check header */
    uint16_t header = data[0] | (data[1] << 8);
    if (header != PROTOCOL_HEADER) {
        return -1; /* Invalid header */
    }

    uint8_t data_len = data[2];
    uint8_t command = data[3];
    size_t total_len = 4 + data_len + 2; /* header(2) + len(1) + cmd(1) + data + crc(2) */

    if (len < total_len) {
        return 0; /* Incomplete frame */
    }

    /* Verify CRC */
    uint16_t rx_crc = data[total_len - 2] | (data[total_len - 1] << 8);
    uint16_t calc_crc = calculate_crc16(data, total_len - 2);

    if (rx_crc != calc_crc) {
        ESP_LOGW(TAG, "CRC mismatch: rx=%04X calc=%04X", rx_crc, calc_crc);
        return -1;
    }

    /* Extract command and payload */
    *cmd = command;
    *payload_len = data_len;
    if (data_len > 0 && payload != NULL) {
        memcpy(payload, &data[4], data_len);
    }

    return (int)total_len;
}

/* ============================================================================
 * UART Forwarding
 * ============================================================================ */

/**
 * @brief Forward data to STM32 via UART
 */
static esp_err_t forward_to_stm32(const uint8_t *data, size_t len)
{
    if (len == 0 || data == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    int tx_bytes = uart_write_bytes(UART_ST_PORT, data, len);
    if (tx_bytes < 0) {
        ESP_LOGE(TAG, "UART write failed");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Forwarded %d bytes to STM32", tx_bytes);
    return ESP_OK;
}

/**
 * @brief Forward data from STM32 to TCP client
 */
static int forward_to_client(int client_sock, const uint8_t *data, size_t len)
{
    if (client_sock < 0 || data == NULL || len == 0) {
        return -1;
    }

    int sent = send(client_sock, data, len, 0);
    if (sent < 0) {
        ESP_LOGE(TAG, "Send to client failed: errno=%d", errno);
        return -1;
    }

    ESP_LOGI(TAG, "Forwarded %d bytes to client", sent);
    return sent;
}

/* ============================================================================
 * Client Handler
 * ============================================================================ */

/**
 * @brief Handle a single client connection
 */
static void handle_client(int client_sock, struct sockaddr_in *client_addr)
{
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr->sin_addr, client_ip, sizeof(client_ip));

    ESP_LOGI(TAG, "Client connected from %s:%d", client_ip, ntohs(client_addr->sin_port));

    /* Update client count */
    if (xSemaphoreTake(client_mutex, portMAX_DELAY) == pdTRUE) {
        client_count++;
        xSemaphoreGive(client_mutex);
    }

    uint8_t rx_buffer[TCP_BUF_SIZE];
    uint8_t uart_buffer[UART_ST_BUF_SIZE];
    int rx_len;

    /* Set socket timeout */
    struct timeval tv;
    tv.tv_sec = TCP_CLIENT_TIMEOUT_SEC;
    tv.tv_usec = 0;
    setsockopt(client_sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    while (server_running) {
        /* Receive data from TCP client */
        rx_len = recv(client_sock, rx_buffer, sizeof(rx_buffer), 0);

        if (rx_len > 0) {
            ESP_LOGI(TAG, "Received %d bytes from client", rx_len);

            /* Log first few bytes for debugging */
            if (rx_len >= 4) {
                ESP_LOGI(TAG, "Data: %02X %02X %02X %02X ...",
                         rx_buffer[0], rx_buffer[1], rx_buffer[2], rx_buffer[3]);
            }

            /* Forward to STM32 via UART */
            esp_err_t ret = forward_to_stm32(rx_buffer, rx_len);
            if (ret != ESP_OK) {
                ESP_LOGW(TAG, "Failed to forward to STM32");
            }

            /* Check for response from STM32 (non-blocking) */
            int uart_len = uart_read_bytes(UART_ST_PORT, uart_buffer,
                                           sizeof(uart_buffer),
                                           pdMS_TO_TICKS(100));
            if (uart_len > 0) {
                ESP_LOGI(TAG, "Received %d bytes from STM32", uart_len);
                forward_to_client(client_sock, uart_buffer, uart_len);
            }

        } else if (rx_len == 0) {
            /* Client disconnected */
            ESP_LOGI(TAG, "Client disconnected");
            break;
        } else {
            /* Error or timeout */
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                /* Timeout - check for STM32 data anyway */
                int uart_len = uart_read_bytes(UART_ST_PORT, uart_buffer,
                                               sizeof(uart_buffer),
                                               pdMS_TO_TICKS(50));
                if (uart_len > 0) {
                    ESP_LOGI(TAG, "Received %d bytes from STM32 (timeout check)", uart_len);
                    forward_to_client(client_sock, uart_buffer, uart_len);
                }
                continue;
            }
            ESP_LOGE(TAG, "recv failed: errno=%d", errno);
            break;
        }
    }

    /* Close client socket */
    close(client_sock);

    /* Update client count */
    if (xSemaphoreTake(client_mutex, portMAX_DELAY) == pdTRUE) {
        client_count--;
        if (client_count < 0) client_count = 0;
        xSemaphoreGive(client_mutex);
    }

    ESP_LOGI(TAG, "Client handler exited");
}

/* ============================================================================
 * TCP Server Task
 * ============================================================================ */

static void tcp_server_task(void *pvParameters)
{
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);

    ESP_LOGI(TAG, "Starting TCP server task");

    /* Wait for WiFi connection */
    ESP_LOGI(TAG, "Waiting for WiFi connection...");
    while (wifi_get_status() != WIFI_CONNECTED) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    const char *ip_str = wifi_get_ip_str();
    ESP_LOGI(TAG, "WiFi connected, IP: %s", ip_str ? ip_str : "unknown");

    /* Create listening socket */
    listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sock < 0) {
        ESP_LOGE(TAG, "Failed to create socket: errno=%d", errno);
        vTaskDelete(NULL);
        return;
    }

    /* Allow socket reuse */
    int opt = 1;
    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    /* Bind to port */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(TCP_SERVER_PORT);

    if (bind(listen_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        ESP_LOGE(TAG, "Failed to bind socket: errno=%d", errno);
        close(listen_sock);
        listen_sock = -1;
        vTaskDelete(NULL);
        return;
    }

    /* Start listening */
    if (listen(listen_sock, TCP_SERVER_MAX_CONN) < 0) {
        ESP_LOGE(TAG, "Failed to listen on socket: errno=%d", errno);
        close(listen_sock);
        listen_sock = -1;
        vTaskDelete(NULL);
        return;
    }

    server_running = true;
    ESP_LOGI(TAG, "================================");
    ESP_LOGI(TAG, "TCP server started on port %d", TCP_SERVER_PORT);
    ESP_LOGI(TAG, "Listening on %s:%d", ip_str ? ip_str : "0.0.0.0", TCP_SERVER_PORT);
    ESP_LOGI(TAG, "================================");

    /* Main accept loop */
    while (server_running) {
        int client_sock = accept(listen_sock, (struct sockaddr *)&client_addr, &addr_len);
        if (client_sock < 0) {
            if (errno == EINTR) {
                continue; /* Interrupted, try again */
            }
            ESP_LOGE(TAG, "Accept failed: errno=%d", errno);
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        /* Handle client in the same task (single client mode) */
        /* For multiple clients, create a new task here */
        handle_client(client_sock, &client_addr);
    }

    /* Cleanup */
    close(listen_sock);
    listen_sock = -1;
    server_running = false;

    ESP_LOGI(TAG, "TCP server stopped");
    vTaskDelete(NULL);
}

/* ============================================================================
 * Public Functions
 * ============================================================================ */

esp_err_t tcp_server_init(void)
{
    if (server_running) {
        ESP_LOGW(TAG, "TCP server already running");
        return ESP_OK;
    }

    /* Create mutex for client count */
    if (client_mutex == NULL) {
        client_mutex = xSemaphoreCreateMutex();
        if (client_mutex == NULL) {
            ESP_LOGE(TAG, "Failed to create client mutex");
            return ESP_ERR_NO_MEM;
        }
    }

    /* Create TCP server task */
    BaseType_t ret = xTaskCreate(tcp_server_task, "tcp_server",
                                 TCP_SERVER_TASK_STACK, NULL,
                                 TCP_SERVER_TASK_PRIO, NULL);
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create TCP server task");
        return ESP_ERR_NO_MEM;
    }

    return ESP_OK;
}

bool tcp_server_is_running(void)
{
    return server_running;
}

int tcp_server_get_client_count(void)
{
    int count = 0;
    if (xSemaphoreTake(client_mutex, portMAX_DELAY) == pdTRUE) {
        count = client_count;
        xSemaphoreGive(client_mutex);
    }
    return count;
}

int tcp_server_broadcast(const uint8_t *data, size_t len)
{
    /* Note: This is a placeholder for future multi-client support */
    /* Currently only single client is supported */
    ESP_LOGW(TAG, "Broadcast not implemented for single-client mode");
    return 0;
}
