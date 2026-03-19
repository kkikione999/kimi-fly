/**
 * @file wifi_sta.c
 * @brief ESP32-C3 WiFi STA Mode Implementation
 * @note Connects to SSID: whc, Password: 12345678
 */

#include <string.h>
#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_log.h"
#include "esp_system.h"

#include "wifi_sta.h"

/* ============================================================================
 * Constants
 * ============================================================================ */

#define TAG "WIFI_STA"

/* Event bits for WiFi connection status */
#define WIFI_CONNECTED_BIT      BIT0
#define WIFI_FAIL_BIT           BIT1

/* ============================================================================
 * Static Variables
 * ============================================================================ */

static EventGroupHandle_t s_wifi_event_group;
static wifi_connection_status_t s_connection_status = WIFI_DISCONNECTED;
static char s_ip_str[16] = {0};
static int s_retry_count = 0;
static bool s_initialized = false;

/* ============================================================================
 * Event Handler
 * ============================================================================ */

/**
 * @brief WiFi event handler callback
 */
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, "WiFi STA started, connecting to %s...", WIFI_SSID);
        s_connection_status = WIFI_CONNECTING;
        esp_wifi_connect();

    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED) {
        wifi_event_sta_connected_t* event = (wifi_event_sta_connected_t*) event_data;
        ESP_LOGI(TAG, "Connected to AP: %s", event->ssid);
        s_connection_status = WIFI_CONNECTING;
        s_retry_count = 0;

    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        snprintf(s_ip_str, sizeof(s_ip_str), IPSTR, IP2STR(&event->ip_info.ip));
        ESP_LOGI(TAG, "Got IP address: %s", s_ip_str);
        s_connection_status = WIFI_CONNECTED;
        s_retry_count = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);

    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        wifi_event_sta_disconnected_t* event = (wifi_event_sta_disconnected_t*) event_data;
        ESP_LOGW(TAG, "Disconnected from AP, reason: %d", event->reason);
        s_connection_status = WIFI_DISCONNECTED;
        memset(s_ip_str, 0, sizeof(s_ip_str));
        xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);

        /* Auto-reconnect logic */
        if (s_retry_count < WIFI_MAX_RETRY) {
            s_retry_count++;
            ESP_LOGI(TAG, "Attempting to reconnect... (%d/%d)", s_retry_count, WIFI_MAX_RETRY);
            vTaskDelay(pdMS_TO_TICKS(1000));
            esp_wifi_connect();
        } else {
            ESP_LOGE(TAG, "Failed to connect after %d attempts", WIFI_MAX_RETRY);
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
            /* Reset retry count and try again after delay */
            vTaskDelay(pdMS_TO_TICKS(WIFI_RETRY_DELAY_MS));
            s_retry_count = 0;
            esp_wifi_connect();
        }
    }
}

/* ============================================================================
 * Public Functions
 * ============================================================================ */

esp_err_t wifi_sta_init(void)
{
    esp_err_t ret;

    if (s_initialized) {
        ESP_LOGW(TAG, "WiFi STA already initialized");
        return ESP_OK;
    }

    ESP_LOGI(TAG, "================================");
    ESP_LOGI(TAG, "Initializing WiFi STA Mode");
    ESP_LOGI(TAG, "Target SSID: %s", WIFI_SSID);
    ESP_LOGI(TAG, "================================");

    /* Create event group for WiFi events */
    s_wifi_event_group = xEventGroupCreate();
    if (s_wifi_event_group == NULL) {
        ESP_LOGE(TAG, "Failed to create WiFi event group");
        return ESP_ERR_NO_MEM;
    }

    /* Initialize TCP/IP stack */
    ret = esp_netif_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "esp_netif_init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    /* Create default event loop */
    ret = esp_event_loop_create_default();
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "esp_event_loop_create_default failed: %s", esp_err_to_name(ret));
        return ret;
    }

    /* Create default WiFi STA interface */
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    if (sta_netif == NULL) {
        ESP_LOGE(TAG, "Failed to create default WiFi STA interface");
        return ESP_FAIL;
    }

    /* Initialize WiFi with default configuration */
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ret = esp_wifi_init(&cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "esp_wifi_init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    /* Register event handlers */
    ret = esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register WiFi event handler: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register IP event handler: %s", esp_err_to_name(ret));
        return ret;
    }

    /* Set WiFi mode to STA */
    ret = esp_wifi_set_mode(WIFI_MODE_STA);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "esp_wifi_set_mode failed: %s", esp_err_to_name(ret));
        return ret;
    }

    /* Configure WiFi connection parameters */
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWORD,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                .capable = true,
                .required = false
            },
        },
    };

    ret = esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "esp_wifi_set_config failed: %s", esp_err_to_name(ret));
        return ret;
    }

    /* Start WiFi driver */
    ret = esp_wifi_start();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "esp_wifi_start failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "WiFi STA initialization complete");
    s_initialized = true;

    return ESP_OK;
}

wifi_connection_status_t wifi_get_status(void)
{
    return s_connection_status;
}

const char* wifi_get_ip_str(void)
{
    if (s_connection_status == WIFI_CONNECTED) {
        return s_ip_str;
    }
    return NULL;
}

esp_err_t wifi_reconnect(void)
{
    if (!s_initialized) {
        ESP_LOGE(TAG, "WiFi not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "Manually triggering reconnect...");
    s_retry_count = 0;
    s_connection_status = WIFI_CONNECTING;
    return esp_wifi_connect();
}
