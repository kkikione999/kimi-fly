/**
 * @file main.c
 * @brief ESP32-C3 UART Communication with STM32F411
 * @note Hardware: ESP32-C3 connected to STM32 via UART
 *       UART: TX=GPIO5, RX=GPIO4 (default UART1 pins)
 *       Baudrate: 115200
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"

#include "wifi_sta.h"
#include "tcp_server.h"

/* ============================================================================
 * Configuration
 * ============================================================================ */

#define TAG "ESP32_UART"

/* UART Configuration for STM32 communication */
#define UART_ST_PORT            UART_NUM_1
#define UART_ST_TX_PIN          GPIO_NUM_5
#define UART_ST_RX_PIN          GPIO_NUM_4
#define UART_ST_BAUDRATE        115200
#define UART_ST_BUF_SIZE        256
#define UART_ST_TASK_STACK_SIZE 4096
#define UART_ST_TASK_PRIORITY   10

/* Protocol defines */
#define PROTOCOL_HEADER         0xAA55
#define PROTOCOL_MAX_PAYLOAD    64

/* ============================================================================
 * Protocol Structures
 * ============================================================================ */

/* Message types */
typedef enum {
    MSG_TYPE_HEARTBEAT = 0x01,
    MSG_TYPE_STATUS = 0x02,
    MSG_TYPE_CONTROL = 0x03,
    MSG_TYPE_SENSOR = 0x04,
    MSG_TYPE_DEBUG = 0x05,
    MSG_TYPE_ACK = 0x06,
    MSG_TYPE_ERROR = 0x07
} msg_type_t;

/* Protocol header structure */
typedef struct __attribute__((packed)) {
    uint16_t header;        /* 0xAA55 */
    uint8_t type;           /* Message type */
    uint8_t length;         /* Payload length */
} protocol_header_t;

/* Complete message structure */
typedef struct __attribute__((packed)) {
    protocol_header_t header;
    uint8_t payload[PROTOCOL_MAX_PAYLOAD];
    uint8_t checksum;
} protocol_message_t;

/* ============================================================================
 * Global Variables
 * ============================================================================ */

static QueueHandle_t uart_queue;
static volatile uint32_t tx_counter = 0;
static volatile uint32_t rx_counter = 0;
static volatile bool communication_active = false;

/* ============================================================================
 * Protocol Functions
 * ============================================================================ */

/**
 * @brief Calculate checksum for a message
 */
static uint8_t calculate_checksum(const protocol_message_t *msg)
{
    uint8_t checksum = 0;
    const uint8_t *data = (const uint8_t *)msg;
    uint8_t length = sizeof(protocol_header_t) + msg->header.length;

    for (uint8_t i = 0; i < length; i++) {
        checksum ^= data[i];
    }
    return checksum;
}

/**
 * @brief Verify message checksum
 */
static bool verify_checksum(const protocol_message_t *msg)
{
    return calculate_checksum(msg) == msg->checksum;
}

/**
 * @brief Build a protocol message
 */
static int build_message(protocol_message_t *msg, msg_type_t type,
                         const uint8_t *payload, uint8_t payload_len)
{
    if (payload_len > PROTOCOL_MAX_PAYLOAD) {
        return -1;
    }

    msg->header.header = PROTOCOL_HEADER;
    msg->header.type = type;
    msg->header.length = payload_len;

    if (payload_len > 0 && payload != NULL) {
        memcpy(msg->payload, payload, payload_len);
    }

    msg->checksum = calculate_checksum(msg);

    return sizeof(protocol_header_t) + payload_len + 1; /* +1 for checksum */
}

/* ============================================================================
 * UART Functions
 * ============================================================================ */

/**
 * @brief Log detailed UART configuration for debugging
 */
static void log_uart_config(void)
{
    ESP_LOGI(TAG, "UART1 Configuration:");
    ESP_LOGI(TAG, "  Baud rate: %d", UART_ST_BAUDRATE);
    ESP_LOGI(TAG, "  Data bits: 8");
    ESP_LOGI(TAG, "  Parity: None");
    ESP_LOGI(TAG, "  Stop bits: 1");
    ESP_LOGI(TAG, "  Flow control: None");
    ESP_LOGI(TAG, "  TX Pin: GPIO%d", UART_ST_TX_PIN);
    ESP_LOGI(TAG, "  RX Pin: GPIO%d", UART_ST_RX_PIN);
    ESP_LOGI(TAG, "  RX buffer: %d bytes", UART_ST_BUF_SIZE * 2);
    ESP_LOGI(TAG, "  TX buffer: %d bytes", UART_ST_BUF_SIZE * 2);
}

/**
 * @brief Initialize UART for STM32 communication
 */
static esp_err_t uart_st_init(void)
{
    const uart_config_t uart_config = {
        .baud_rate = UART_ST_BAUDRATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    /* Install UART driver */
    esp_err_t ret = uart_driver_install(UART_ST_PORT, UART_ST_BUF_SIZE * 2,
                                        UART_ST_BUF_SIZE * 2, 20, &uart_queue,
                                        0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "UART driver install failed: %s", esp_err_to_name(ret));
        return ret;
    }

    /* Configure UART parameters */
    ret = uart_param_config(UART_ST_PORT, &uart_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "UART param config failed: %s", esp_err_to_name(ret));
        return ret;
    }

    /* Set pins */
    ret = uart_set_pin(UART_ST_PORT, UART_ST_TX_PIN, UART_ST_RX_PIN,
                       UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "UART set pin failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "UART initialized: TX=GPIO%d, RX=GPIO%d, Baud=%d",
             UART_ST_TX_PIN, UART_ST_RX_PIN, UART_ST_BAUDRATE);

    /* Log detailed configuration */
    log_uart_config();

    return ESP_OK;
}

/**
 * @brief Send message to STM32
 */
static esp_err_t uart_st_send_message(const protocol_message_t *msg, size_t len)
{
    int tx_bytes = uart_write_bytes(UART_ST_PORT, msg, len);

    if (tx_bytes < 0) {
        ESP_LOGE(TAG, "UART send failed");
        return ESP_FAIL;
    }

    tx_counter++;
    ESP_LOGD(TAG, "Sent %d bytes to STM32", tx_bytes);
    return ESP_OK;
}

/**
 * @brief Send heartbeat message
 */
static void send_heartbeat(void)
{
    protocol_message_t msg;
    uint8_t payload[4] = {0x01, 0x02, 0x03, 0x04}; /* Sequence number */

    int len = build_message(&msg, MSG_TYPE_HEARTBEAT, payload, sizeof(payload));
    if (len > 0) {
        uart_st_send_message(&msg, len);
    }
}

/**
 * @brief Send status message
 */
static void send_status(const char *status_text)
{
    protocol_message_t msg;
    uint8_t payload[PROTOCOL_MAX_PAYLOAD];

    strncpy((char *)payload, status_text, PROTOCOL_MAX_PAYLOAD - 1);
    payload[PROTOCOL_MAX_PAYLOAD - 1] = '\0';

    int len = build_message(&msg, MSG_TYPE_STATUS, payload, strlen((char *)payload));
    if (len > 0) {
        uart_st_send_message(&msg, len);
    }
}

/**
 * @brief Send ACK message
 */
static void send_ack(uint8_t msg_type)
{
    protocol_message_t msg;
    uint8_t payload = msg_type;

    int len = build_message(&msg, MSG_TYPE_ACK, &payload, 1);
    if (len > 0) {
        uart_st_send_message(&msg, len);
    }
}

/**
 * @brief Process received message
 */
static void process_received_message(const protocol_message_t *msg)
{
    ESP_LOGI(TAG, "[RX] Parsing message: type=0x%02X, len=%d",
             msg->header.type, msg->header.length);

    if (!verify_checksum(msg)) {
        ESP_LOGW(TAG, "[RX] Checksum error, ignoring message");
        return;
    }

    rx_counter++;
    communication_active = true;
    ESP_LOGI(TAG, "[RX] Message valid, RX count: %lu", rx_counter);

    switch (msg->header.type) {
        case MSG_TYPE_HEARTBEAT:
            ESP_LOGI(TAG, "Received heartbeat from STM32");
            send_ack(MSG_TYPE_HEARTBEAT);
            break;

        case MSG_TYPE_STATUS:
            ESP_LOGI(TAG, "STM32 status: %s", msg->payload);
            break;

        case MSG_TYPE_CONTROL:
            ESP_LOGI(TAG, "Received control command");
            /* Process control command */
            break;

        case MSG_TYPE_SENSOR:
            ESP_LOGD(TAG, "Received sensor data");
            break;

        case MSG_TYPE_DEBUG:
            ESP_LOGI(TAG, "STM32 debug: %s", msg->payload);
            break;

        case MSG_TYPE_ACK:
            ESP_LOGD(TAG, "Received ACK for msg type 0x%02X", msg->payload[0]);
            break;

        default:
            ESP_LOGW(TAG, "Unknown message type: 0x%02X", msg->header.type);
            break;
    }
}

/* ============================================================================
 * UART Receive Task
 * ============================================================================ */

static void uart_receive_task(void *pvParameters)
{
    uint8_t rx_buffer[UART_ST_BUF_SIZE];
    protocol_message_t rx_msg;
    int rx_len;

    ESP_LOGI(TAG, "UART receive task started");

    while (1) {
        /* Read data from UART */
        rx_len = uart_read_bytes(UART_ST_PORT, rx_buffer,
                                  sizeof(rx_buffer),
                                  pdMS_TO_TICKS(100));

        if (rx_len > 0) {
            ESP_LOGI(TAG, "[RX] Received %d bytes", rx_len);

            /* Print first few bytes for debugging */
            if (rx_len >= 4) {
                ESP_LOGI(TAG, "[RX] Data: %02X %02X %02X %02X ...",
                         rx_buffer[0], rx_buffer[1], rx_buffer[2], rx_buffer[3]);
            }

            /* Simple protocol parsing - look for header */
            for (int i = 0; i < rx_len - sizeof(protocol_header_t); i++) {
                uint16_t header = rx_buffer[i] | (rx_buffer[i+1] << 8);

                if (header == PROTOCOL_HEADER) {
                    /* Found header, try to parse message */
                    uint8_t msg_len = rx_buffer[i+3]; /* payload length */
                    uint8_t total_len = sizeof(protocol_header_t) + msg_len + 1;

                    ESP_LOGI(TAG, "[RX] Found header at offset %d, msg_len=%d", i, msg_len);

                    if (i + total_len <= rx_len) {
                        memcpy(&rx_msg, &rx_buffer[i], total_len);
                        process_received_message(&rx_msg);
                    } else {
                        ESP_LOGW(TAG, "[RX] Incomplete message: need %d bytes, have %d",
                                 total_len, rx_len - i);
                    }
                    break;
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

/* ============================================================================
 * Diagnostic Task
 * ============================================================================ */

static void diagnostic_task(void *pvParameters)
{
    uint32_t last_tx = 0;
    uint32_t last_rx = 0;
    int no_rx_count = 0;

    /* Wait for initialization */
    vTaskDelay(pdMS_TO_TICKS(3000));

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(5000));

        /* Output statistics */
        ESP_LOGI(TAG, "=== Communication Statistics ===");
        ESP_LOGI(TAG, "TX total: %lu (+%lu)", tx_counter, tx_counter - last_tx);
        ESP_LOGI(TAG, "RX total: %lu (+%lu)", rx_counter, rx_counter - last_rx);
        ESP_LOGI(TAG, "UART Connection: %s", communication_active ? "YES" : "NO");

        /* WiFi status */
        wifi_connection_status_t wifi_status = wifi_get_status();
        const char* wifi_status_str = "UNKNOWN";
        switch (wifi_status) {
            case WIFI_DISCONNECTED: wifi_status_str = "DISCONNECTED"; break;
            case WIFI_CONNECTING:   wifi_status_str = "CONNECTING"; break;
            case WIFI_CONNECTED:    wifi_status_str = "CONNECTED"; break;
        }
        ESP_LOGI(TAG, "WiFi Status: %s", wifi_status_str);
        if (wifi_status == WIFI_CONNECTED) {
            ESP_LOGI(TAG, "WiFi IP: %s", wifi_get_ip_str());
        }

        /* Check receive status */
        if (rx_counter == last_rx) {
            no_rx_count++;
            if (no_rx_count >= 2) {
                ESP_LOGW(TAG, "WARNING: No data received for %d seconds", no_rx_count * 5);
                ESP_LOGW(TAG, "  Possible causes:");
                ESP_LOGW(TAG, "  1. STM32 not sending (check Task 002)");
                ESP_LOGW(TAG, "  2. Hardware connection issue (TX/RX cross, GND)");
                ESP_LOGW(TAG, "  3. Baud rate mismatch");
            }
        } else {
            no_rx_count = 0;
            ESP_LOGI(TAG, "STATUS: Receiving data normally");
        }

        last_tx = tx_counter;
        last_rx = rx_counter;
    }
}

/* ============================================================================
 * Main Task
 * ============================================================================ */

static void main_task(void *pvParameters)
{
    uint32_t last_heartbeat = 0;
    uint32_t loop_counter = 0;

    ESP_LOGI(TAG, "Main task started");
    send_status("ESP32 initialized and ready");

    while (1) {
        loop_counter++;

        /* Send heartbeat every 2 seconds */
        if (xTaskGetTickCount() - last_heartbeat > pdMS_TO_TICKS(2000)) {
            send_heartbeat();
            last_heartbeat = xTaskGetTickCount();

            /* Log statistics */
            ESP_LOGI(TAG, "Stats - TX: %lu, RX: %lu, Active: %s",
                     tx_counter, rx_counter,
                     communication_active ? "YES" : "NO");
        }

        /* Send periodic status */
        if (loop_counter % 50 == 0) {
            char status[32];
            snprintf(status, sizeof(status), "Loop: %lu", loop_counter);
            send_status(status);
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

/* ============================================================================
 * Entry Point
 * ============================================================================ */

void app_main(void)
{
    esp_err_t ret;

    ESP_LOGI(TAG, "================================");
    ESP_LOGI(TAG, "Kimi-Fly ESP32-C3 UART Bridge");
    ESP_LOGI(TAG, "Target: STM32F411CEU6");
    ESP_LOGI(TAG, "UART: TX=GPIO%d, RX=GPIO%d", UART_ST_TX_PIN, UART_ST_RX_PIN);
    ESP_LOGI(TAG, "Baudrate: %d", UART_ST_BAUDRATE);
    ESP_LOGI(TAG, "================================");

    /* Initialize NVS (required for WiFi) */
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    /* Initialize WiFi STA */
    ret = wifi_sta_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "WiFi initialization failed!");
    } else {
        ESP_LOGI(TAG, "WiFi STA initialized successfully");
    }

    /* Initialize UART */
    ret = uart_st_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "UART initialization failed!");
        return;
    }

    /* Create receive task */
    xTaskCreate(uart_receive_task, "uart_rx", UART_ST_TASK_STACK_SIZE,
                NULL, UART_ST_TASK_PRIORITY, NULL);

    /* Create main task */
    xTaskCreate(main_task, "main_task", 4096, NULL, 5, NULL);

    /* Create diagnostic task */
    xTaskCreate(diagnostic_task, "diag_task", 4096, NULL, 3, NULL);

    /* Start TCP server (will wait for WiFi connection internally) */
    ret = tcp_server_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "TCP server initialization failed!");
    } else {
        ESP_LOGI(TAG, "TCP server task created");
    }

    /* This task is done */
    ESP_LOGI(TAG, "Initialization complete");

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
