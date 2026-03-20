/**
 * @file main.c
 * @brief ESP32-C3 UART bridge to STM32 flight controller
 * @note Default bring-up path:
 *       - Initialize NVS
 *       - Initialize WiFi STA
 *       - Initialize UART1 for STM32 link
 *       - Run a single UART bridge task
 *
 *       Physical map:
 *       - GPIO0 (TX) -> STM32 PA3 (USART2_RX)
 *       - GPIO1 (RX) <- STM32 PA2 (USART2_TX)
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_err.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "protocol_bridge.h"
#include "wifi_sta.h"

/* ============================================================================
 * Configuration
 * ============================================================================ */

#define TAG "ESP32_UART"

#define UART_ST_PORT             UART_NUM_1
#define UART_ST_TX_PIN           GPIO_NUM_0
#define UART_ST_RX_PIN           GPIO_NUM_1
#define UART_ST_BAUDRATE         115200
#define UART_ST_RX_BUFFER_SIZE    512
#define UART_ST_TX_BUFFER_SIZE    256
#define UART_ST_TASK_STACK_SIZE   4096
#define UART_ST_TASK_PRIORITY     10

#define UART_RX_CHUNK_SIZE        128
#define UART_HEARTBEAT_PERIOD_MS  2000U
#define UART_STATUS_PERIOD_MS     5000U

/* ============================================================================
 * Protocol payloads
 * ============================================================================ */

typedef struct __attribute__((packed)) {
    uint8_t version;
} protocol_version_resp_t;

typedef struct __attribute__((packed)) {
    uint8_t cmd;
} protocol_ack_t;

typedef struct __attribute__((packed)) {
    uint8_t cmd;
    uint8_t error;
} protocol_nack_t;

typedef struct __attribute__((packed)) {
    uint8_t version;
    uint8_t armed;
    uint8_t mode;
    uint8_t status;
    uint16_t error_flags;
} protocol_status_t;

typedef struct __attribute__((packed)) {
    int16_t roll;
    int16_t pitch;
    int16_t yaw;
    int16_t roll_rate;
    int16_t pitch_rate;
    int16_t yaw_rate;
} protocol_attitude_t;

/* ============================================================================
 * State
 * ============================================================================ */

static uint8_t s_rx_buffer[UART_ST_RX_BUFFER_SIZE];
static size_t s_rx_len;
static uint32_t s_tx_frames;
static uint32_t s_rx_frames;
static uint32_t s_rx_errors;
static uint32_t s_last_rx_ms;
static bool s_link_active;

/* ============================================================================
 * Helpers
 * ============================================================================ */

static void log_uart_config(void)
{
    ESP_LOGI(TAG, "UART1 configuration:");
    ESP_LOGI(TAG, "  TX pin: GPIO%d", UART_ST_TX_PIN);
    ESP_LOGI(TAG, "  RX pin: GPIO%d", UART_ST_RX_PIN);
    ESP_LOGI(TAG, "  Baud: %d", UART_ST_BAUDRATE);
    ESP_LOGI(TAG, "  Frame: 8N1, no flow control");
}

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

    esp_err_t ret = uart_driver_install(UART_ST_PORT,
                                        UART_ST_RX_BUFFER_SIZE,
                                        UART_ST_TX_BUFFER_SIZE,
                                        0,
                                        NULL,
                                        0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "uart_driver_install failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = uart_param_config(UART_ST_PORT, &uart_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "uart_param_config failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = uart_set_pin(UART_ST_PORT,
                       UART_ST_TX_PIN,
                       UART_ST_RX_PIN,
                       UART_PIN_NO_CHANGE,
                       UART_PIN_NO_CHANGE);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "uart_set_pin failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = uart_flush_input(UART_ST_PORT);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "uart_flush_input failed: %s", esp_err_to_name(ret));
    }

    log_uart_config();
    return ESP_OK;
}

static esp_err_t uart_send_frame(const uart_protocol_frame_t *frame)
{
    uint8_t wire[UART_ST_TX_BUFFER_SIZE];
    size_t wire_len = 0;

    esp_err_t ret = uart_protocol_encode(frame, wire, sizeof(wire), &wire_len);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Frame encode failed for cmd 0x%02X", frame ? frame->cmd : 0);
        s_rx_errors++;
        return ret;
    }

    int written = uart_write_bytes(UART_ST_PORT, (const char *)wire, wire_len);
    if (written != (int)wire_len) {
        ESP_LOGE(TAG, "uart_write_bytes wrote %d/%u bytes", written, (unsigned)wire_len);
        s_rx_errors++;
        return ESP_FAIL;
    }

    ret = uart_wait_tx_done(UART_ST_PORT, pdMS_TO_TICKS(100));
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "uart_wait_tx_done failed: %s", esp_err_to_name(ret));
    }

    s_tx_frames++;
    ESP_LOGI(TAG, "TX cmd=0x%02X (%s) len=%u",
             frame->cmd,
             uart_protocol_cmd_name(frame->cmd),
             (unsigned)frame->len);
    return ESP_OK;
}

static esp_err_t uart_send_simple_cmd(uint8_t cmd)
{
    uart_protocol_frame_t frame = {
        .cmd = cmd,
        .len = 0,
    };

    return uart_send_frame(&frame);
}

static void uart_handle_frame(const uart_protocol_frame_t *frame)
{
    s_rx_frames++;
    s_link_active = true;
    s_last_rx_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;

    ESP_LOGI(TAG, "RX cmd=0x%02X (%s) len=%u",
             frame->cmd,
             uart_protocol_cmd_name(frame->cmd),
             (unsigned)frame->len);

    switch (frame->cmd) {
        case UART_PROTOCOL_CMD_ACK:
            if (frame->len == sizeof(protocol_ack_t)) {
                protocol_ack_t ack;
                memcpy(&ack, frame->data, sizeof(ack));
                ESP_LOGI(TAG, "ACK for cmd 0x%02X (%s)",
                         ack.cmd,
                         uart_protocol_cmd_name(ack.cmd));
            }
            break;

        case UART_PROTOCOL_CMD_NACK:
            if (frame->len == sizeof(protocol_nack_t)) {
                protocol_nack_t nack;
                memcpy(&nack, frame->data, sizeof(nack));
                ESP_LOGW(TAG, "NACK for cmd 0x%02X (%s), error=0x%02X",
                         nack.cmd,
                         uart_protocol_cmd_name(nack.cmd),
                         nack.error);
            }
            break;

        case UART_PROTOCOL_CMD_VERSION:
            if (frame->len == sizeof(protocol_version_resp_t)) {
                protocol_version_resp_t version;
                memcpy(&version, frame->data, sizeof(version));
                ESP_LOGI(TAG, "STM32 protocol version: %u", version.version);
            }
            break;

        case UART_PROTOCOL_CMD_STATUS:
            if (frame->len == sizeof(protocol_status_t)) {
                protocol_status_t status;
                memcpy(&status, frame->data, sizeof(status));
                ESP_LOGI(TAG,
                         "Status version=%u armed=%u mode=%u state=%u err=0x%04X",
                         status.version,
                         status.armed,
                         status.mode,
                         status.status,
                         status.error_flags);
            }
            break;

        case UART_PROTOCOL_CMD_TELEMETRY_ATT:
            if (frame->len == sizeof(protocol_attitude_t)) {
                protocol_attitude_t att;
                memcpy(&att, frame->data, sizeof(att));
                ESP_LOGI(TAG,
                         "Attitude roll=%d pitch=%d yaw=%d rrate=%d prate=%d yrate=%d",
                         att.roll, att.pitch, att.yaw,
                         att.roll_rate, att.pitch_rate, att.yaw_rate);
            }
            break;

        default:
            break;
    }
}

static void uart_process_rx_buffer(void)
{
    while (s_rx_len > 0) {
        uart_protocol_frame_t frame;
        size_t consumed = 0;
        esp_err_t ret = uart_protocol_decode(s_rx_buffer, s_rx_len, &frame, &consumed);

        if (consumed > 0) {
            memmove(s_rx_buffer, s_rx_buffer + consumed, s_rx_len - consumed);
            s_rx_len -= consumed;
        }

        if (ret != ESP_OK) {
            if (consumed == 0) {
                break;
            }

            s_rx_errors++;
            continue;
        }

        uart_handle_frame(&frame);
    }
}

static void uart_feed_rx_bytes(const uint8_t *data, size_t len)
{
    if (len == 0 || data == NULL) {
        return;
    }

    if (len > sizeof(s_rx_buffer)) {
        data += (len - sizeof(s_rx_buffer));
        len = sizeof(s_rx_buffer);
    }

    if ((s_rx_len + len) > sizeof(s_rx_buffer)) {
        ESP_LOGW(TAG, "RX buffer overflow, dropping %u bytes", (unsigned)s_rx_len);
        s_rx_len = 0;
        s_rx_errors++;
    }

    memcpy(&s_rx_buffer[s_rx_len], data, len);
    s_rx_len += len;
}

static void uart_bridge_task(void *pvParameters)
{
    (void)pvParameters;

    uint8_t rx_chunk[UART_RX_CHUNK_SIZE];
    TickType_t last_heartbeat = xTaskGetTickCount();
    TickType_t last_status = last_heartbeat;
    TickType_t last_stats = last_heartbeat;

    ESP_LOGI(TAG, "UART bridge task started");

    uart_send_simple_cmd(UART_PROTOCOL_CMD_VERSION);

    while (1) {
        int rx_len = uart_read_bytes(UART_ST_PORT,
                                     rx_chunk,
                                     sizeof(rx_chunk),
                                     pdMS_TO_TICKS(100));
        if (rx_len > 0) {
            uart_feed_rx_bytes(rx_chunk, (size_t)rx_len);
            uart_process_rx_buffer();
        }

        TickType_t now = xTaskGetTickCount();
        if ((now - last_heartbeat) >= pdMS_TO_TICKS(UART_HEARTBEAT_PERIOD_MS)) {
            uart_send_simple_cmd(UART_PROTOCOL_CMD_HEARTBEAT);
            last_heartbeat = now;
        }

        if ((now - last_status) >= pdMS_TO_TICKS(UART_STATUS_PERIOD_MS)) {
            uart_send_simple_cmd(UART_PROTOCOL_CMD_STATUS);
            last_status = now;
        }

        if ((now - last_stats) >= pdMS_TO_TICKS(5000U)) {
            uint32_t age_ms = (s_last_rx_ms == 0U)
                                  ? 0U
                                  : ((xTaskGetTickCount() * portTICK_PERIOD_MS) - s_last_rx_ms);
            ESP_LOGI(TAG,
                     "Stats tx=%lu rx=%lu rx_err=%lu link=%s rx_age=%lums",
                     (unsigned long)s_tx_frames,
                     (unsigned long)s_rx_frames,
                     (unsigned long)s_rx_errors,
                     s_link_active ? "up" : "down",
                     (unsigned long)age_ms);
            last_stats = now;
        }
    }
}

/* ============================================================================
 * Entry Point
 * ============================================================================ */

void app_main(void)
{
    esp_err_t ret;

    ESP_LOGI(TAG, "================================");
    ESP_LOGI(TAG, "ESP32-C3 UART bridge");
    ESP_LOGI(TAG, "Target: STM32F411CEU6");
    ESP_LOGI(TAG, "UART map: GPIO0<->PA3, GPIO1<->PA2");
    ESP_LOGI(TAG, "Protocol: AA 55 + LEN + CMD + PAYLOAD + CRC16");
    ESP_LOGI(TAG, "================================");

    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ret = wifi_sta_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "WiFi init failed: %s", esp_err_to_name(ret));
    }

    ret = uart_st_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "UART init failed: %s", esp_err_to_name(ret));
        return;
    }

    s_last_rx_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
    s_link_active = false;

    BaseType_t task_ret = xTaskCreate(uart_bridge_task,
                                      "uart_bridge",
                                      UART_ST_TASK_STACK_SIZE,
                                      NULL,
                                      UART_ST_TASK_PRIORITY,
                                      NULL);
    if (task_ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create UART bridge task");
        return;
    }

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
