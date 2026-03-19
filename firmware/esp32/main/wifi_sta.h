/**
 * @file wifi_sta.h
 * @brief ESP32-C3 WiFi STA Mode Interface
 * @note Connects to SSID: whc, Password: 12345678
 */

#ifndef WIFI_STA_H
#define WIFI_STA_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Configuration
 * ============================================================================ */

#define WIFI_SSID           "whc"
#define WIFI_PASSWORD       "12345678"

#define WIFI_MAX_RETRY      5
#define WIFI_RETRY_DELAY_MS 5000

/* ============================================================================
 * Types
 * ============================================================================ */

typedef enum {
    WIFI_DISCONNECTED,
    WIFI_CONNECTING,
    WIFI_CONNECTED
} wifi_connection_status_t;

/* ============================================================================
 * Public Functions
 * ============================================================================ */

/**
 * @brief Initialize WiFi STA mode
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t wifi_sta_init(void);

/**
 * @brief Get current WiFi connection status
 * @return Current connection status
 */
wifi_connection_status_t wifi_get_status(void);

/**
 * @brief Get IP address as string (valid only when connected)
 * @return IP address string or NULL if not connected
 */
const char* wifi_get_ip_str(void);

/**
 * @brief Manually trigger reconnect
 * @return ESP_OK on success
 */
esp_err_t wifi_reconnect(void);

#ifdef __cplusplus
}
#endif

#endif /* WIFI_STA_H */
