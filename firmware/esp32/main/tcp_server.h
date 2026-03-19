/**
 * @file tcp_server.h
 * @brief ESP32-C3 TCP Server for Ground Station Communication
 * @note Receives commands from ground station and forwards to STM32 via UART
 */

#ifndef TCP_SERVER_H
#define TCP_SERVER_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Configuration
 * ============================================================================ */

#define TCP_SERVER_PORT         8888
#define TCP_SERVER_MAX_CONN     3
#define TCP_BUF_SIZE            256
#define TCP_SERVER_TASK_STACK   4096
#define TCP_SERVER_TASK_PRIO    8
#define TCP_CLIENT_TIMEOUT_SEC  30

/* ============================================================================
 * Public Functions
 * ============================================================================ */

/**
 * @brief Initialize and start TCP server task
 * @return ESP_OK on success, error code otherwise
 * @note This function should be called after WiFi is connected
 */
esp_err_t tcp_server_init(void);

/**
 * @brief Check if TCP server is running
 * @return true if running, false otherwise
 */
bool tcp_server_is_running(void);

/**
 * @brief Get number of connected clients
 * @return Number of active client connections
 */
int tcp_server_get_client_count(void);

/**
 * @brief Send data to all connected clients
 * @param data Data buffer to send
 * @param len Data length
 * @return Number of bytes sent, or -1 on error
 */
int tcp_server_broadcast(const uint8_t *data, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* TCP_SERVER_H */
