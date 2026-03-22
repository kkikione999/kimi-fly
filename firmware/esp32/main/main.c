/**
 * @file main.c
 * @brief ESP32-C3 UART bridge to STM32 flight controller
 * @note 协议格式: [0x55][CMD][LEN][DATA...][SUM8][0xAA]
 *       与参考代码 /Users/ll/fly/zmgjb/code/C3/src/myserial.cpp 一致
 *
 * Physical map:
 * - GPIO0 (TX) -> STM32 PA3 (USART2_RX)
 * - GPIO1 (RX) <- STM32 PA2 (USART2_TX)
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_err.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "protocol_bridge.h"
#include "wifi_sta.h"
#include "tcp_server.h"

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

#define UART_HEARTBEAT_PERIOD_MS  2000U
#define UART_STATUS_PERIOD_MS     5000U

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

static uint8_t s_tx_buffer[UART_ST_TX_BUFFER_SIZE];

/* Attitude state for TCP forwarding */
static float s_last_roll = 0.0f;
static float s_last_pitch = 0.0f;
static float s_last_yaw = 0.0f;
static uint32_t s_last_attitude_ms = 0;
static uint32_t s_last_test_attitude_ms = 0;
#define TEST_ATTITUDE_PERIOD_MS  500U

/* ============================================================================
 * Helpers
 * ============================================================================ */

/**
 * @brief Convert quaternion to Euler angles (roll, pitch, yaw in degrees)
 */
static void quaternion_to_euler(float q0, float q1, float q2, float q3,
                                float *roll, float *pitch, float *yaw)
{
    // Roll (x-axis rotation)
    float sinr_cosp = 2.0f * (q0 * q1 + q2 * q3);
    float cosr_cosp = 1.0f - 2.0f * (q1 * q1 + q2 * q2);
    *roll = atan2f(sinr_cosp, cosr_cosp) * 180.0f / M_PI;

    // Pitch (y-axis rotation)
    float sinp = 2.0f * (q0 * q2 - q3 * q1);
    if (fabsf(sinp) >= 1.0f) {
        *pitch = copysignf(90.0f, sinp);  // Use 90 degrees if out of range
    } else {
        *pitch = asinf(sinp) * 180.0f / M_PI;
    }

    // Yaw (z-axis rotation)
    float siny_cosp = 2.0f * (q0 * q3 + q1 * q2);
    float cosy_cosp = 1.0f - 2.0f * (q2 * q2 + q3 * q3);
    *yaw = atan2f(siny_cosp, cosy_cosp) * 180.0f / M_PI;
}

/**
 * @brief Format and send attitude data to TCP viewer
 * @note Protocol: [AA][CMD=01][LEN=13][roll(2)][pitch(2)][yaw(2)][roll_rate(2)][pitch_rate(2)][yaw_rate(2)][SUM]
 */
static void send_attitude_to_tcp(float roll, float pitch, float yaw,
                                  float roll_rate, float pitch_rate, float yaw_rate)
{
    if (!tcp_server_is_running() || tcp_server_get_client_count() == 0) {
        return;
    }

    // Convert to int16_t * 100 for roll/pitch/yaw (0.01 degree resolution)
    // Convert to int16_t * 10 for rates (0.1 deg/s resolution)
    int16_t roll_i16 = (int16_t)(roundf(roll * 100.0f));
    int16_t pitch_i16 = (int16_t)(roundf(pitch * 100.0f));
    int16_t yaw_i16 = (int16_t)(roundf(yaw * 100.0f));
    int16_t roll_rate_i16 = (int16_t)(roundf(roll_rate * 10.0f));
    int16_t pitch_rate_i16 = (int16_t)(roundf(pitch_rate * 10.0f));
    int16_t yaw_rate_i16 = (int16_t)(roundf(yaw_rate * 10.0f));

    uint8_t frame[4 + 12 + 1];  // [AA][CMD][LEN][DATA...][SUM]
    uint8_t pos = 0;
    frame[pos++] = 0xAA;        // HEADER
    frame[pos++] = 0x01;        // CMD_ATTITUDE
    frame[pos++] = 12;          // LEN = 12 (roll+pitch+yaw+rates = 6*2=12 bytes)

    // DATA (13 bytes)
    memcpy(&frame[pos], &roll_i16, 2);      pos += 2;
    memcpy(&frame[pos], &pitch_i16, 2);     pos += 2;
    memcpy(&frame[pos], &yaw_i16, 2);       pos += 2;
    memcpy(&frame[pos], &roll_rate_i16, 2);  pos += 2;
    memcpy(&frame[pos], &pitch_rate_i16, 2); pos += 2;
    memcpy(&frame[pos], &yaw_rate_i16, 2);   pos += 2;

    // CHECKSUM = sum of CMD + LEN + DATA
    uint8_t sum = frame[1] + frame[2];
    for (int i = 3; i < pos; i++) {
        sum += frame[i];
    }
    frame[pos++] = sum;

    // Broadcast to all TCP clients
    tcp_server_broadcast(frame, pos);
}

static void log_uart_config(void)
{
    ESP_LOGI(TAG, "UART1 configuration:");
    ESP_LOGI(TAG, "  TX pin: GPIO%d -> PA3", UART_ST_TX_PIN);
    ESP_LOGI(TAG, "  RX pin: GPIO%d <- PA2", UART_ST_RX_PIN);
    ESP_LOGI(TAG, "  Baud: %d", UART_ST_BAUDRATE);
    ESP_LOGI(TAG, "  Protocol: [55][CMD][LEN][DATA][SUM][AA]");
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

    // 初始化协议解析器
    uart_protocol_parser_reset();

    log_uart_config();
    return ESP_OK;
}

static esp_err_t uart_send_frame(uint8_t cmd, const uint8_t *data, uint8_t len)
{
    uart_protocol_frame_t frame = {
        .cmd = cmd,
        .len = len,
    };

    if (len > 0 && data != NULL) {
        memcpy(frame.data, data, len);
    }

    size_t out_len = 0;
    esp_err_t ret = uart_protocol_encode(&frame, s_tx_buffer, sizeof(s_tx_buffer), &out_len);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Frame encode failed for cmd 0x%02X", cmd);
        return ret;
    }

    int written = uart_write_bytes(UART_ST_PORT, (const char *)s_tx_buffer, out_len);
    if (written != (int)out_len) {
        ESP_LOGE(TAG, "uart_write_bytes wrote %d/%u bytes", written, (unsigned)out_len);
        return ESP_FAIL;
    }

    ret = uart_wait_tx_done(UART_ST_PORT, pdMS_TO_TICKS(100));
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "uart_wait_tx_done failed: %s", esp_err_to_name(ret));
    }

    s_tx_frames++;
    ESP_LOGI(TAG, "TX cmd=0x%02X (%s) len=%u",
             cmd,
             uart_protocol_cmd_name(cmd),
             (unsigned)len);
    return ESP_OK;
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

    // 处理不同命令
    switch (frame->cmd) {
        case UART_PROTOCOL_CMD_QUATERNION:
            // 四元数数据，16字节 (4 x float)
            if (frame->len == 16) {
                float *q = (float *)frame->data;
                ESP_LOGI(TAG, "Quaternion: %.3f, %.3f, %.3f, %.3f",
                         q[0], q[1], q[2], q[3]);

                // 转换为欧拉角
                float roll, pitch, yaw;
                quaternion_to_euler(q[0], q[1], q[2], q[3], &roll, &pitch, &yaw);

                // 计算角速率 (deg/s)
                uint32_t now_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
                float dt = (s_last_attitude_ms > 0) ?
                           (now_ms - s_last_attitude_ms) / 1000.0f : 0.0f;
                float roll_rate = (dt > 0.0f) ? (roll - s_last_roll) / dt : 0.0f;
                float pitch_rate = (dt > 0.0f) ? (pitch - s_last_pitch) / dt : 0.0f;
                float yaw_rate = (dt > 0.0f) ? (yaw - s_last_yaw) / dt : 0.0f;

                // 更新历史值
                s_last_roll = roll;
                s_last_pitch = pitch;
                s_last_yaw = yaw;
                s_last_attitude_ms = now_ms;

                ESP_LOGI(TAG, "Euler: roll=%.1f pitch=%.1f yaw=%.1f rates=%.1f/%.1f/%.1f",
                         roll, pitch, yaw, roll_rate, pitch_rate, yaw_rate);

                // 发送到TCP查看器
                send_attitude_to_tcp(roll, pitch, yaw, roll_rate, pitch_rate, yaw_rate);
            }
            break;

        case UART_PROTOCOL_CMD_JOYSTICK:
            // 摇杆数据，16字节 (4 x float)
            if (frame->len == 16) {
                float *rc = (float *)frame->data;
                ESP_LOGI(TAG, "RC Lx=%.2f Ly=%.2f Rx=%.2f Ry=%.2f",
                         rc[0], rc[1], rc[2], rc[3]);
            }
            // Fall-through: also send test attitude data for TCP forwarding test
            // (STM32 test code doesn't send QUATERNION, so we simulate it)
            {
                static float test_roll = 0.0f, test_pitch = 5.0f, test_yaw = 0.0f;
                static uint32_t test_count = 0;
                test_count++;
                // Simulate slow rotation
                test_yaw += 1.0f;
                if (test_yaw > 360.0f) test_yaw -= 360.0f;
                send_attitude_to_tcp(test_roll, test_pitch, test_yaw,
                                     0.0f, 0.0f, 30.0f);  // Simulated rates
            }
            break;

        case UART_PROTOCOL_CMD_PID_CHECK:
            // PID参数回传
            ESP_LOGI(TAG, "PID check response, len=%u", (unsigned)frame->len);
            break;

        default:
            ESP_LOGI(TAG, "Unhandled cmd 0x%02X", frame->cmd);
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

        if (ret == ESP_OK) {
            uart_handle_frame(&frame);
        } else {
            // 解析失败，可能需要更多数据或帧错误
            // 只在非等待HEADER状态时增加错误计数
            s_rx_errors++;
        }
    }
}

static void uart_feed_rx_bytes(const uint8_t *data, size_t len)
{
    if (len == 0 || data == NULL) {
        return;
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

    uint8_t rx_chunk[128];
    TickType_t last_heartbeat = xTaskGetTickCount();
    TickType_t last_stats = last_heartbeat;

    ESP_LOGI(TAG, "UART bridge task started");
    ESP_LOGI(TAG, "Protocol: [55][CMD][LEN][DATA][SUM][AA] or [55][AA][CMD][LEN][DATA][XOR]");
    ESP_LOGI(TAG, "Initializing parser...");

    // 确保解析器已初始化
    uart_protocol_parser_reset();

    while (1) {
        int rx_len = uart_read_bytes(UART_ST_PORT,
                                     rx_chunk,
                                     sizeof(rx_chunk),
                                     pdMS_TO_TICKS(100));
        if (rx_len > 0) {
            // Debug: 打印原始接收字节
            char hex_buf[256] = {0};
            int pos = 0;
            for (int i = 0; i < rx_len && pos < 200; i++) {
                pos += snprintf(hex_buf + pos, sizeof(hex_buf) - pos, "%02X ", rx_chunk[i]);
            }
            ESP_LOGI(TAG, "RX raw %d: %s", rx_len, hex_buf);
            uart_feed_rx_bytes(rx_chunk, (size_t)rx_len);
            uart_process_rx_buffer();
        }

        TickType_t now = xTaskGetTickCount();

        // 定期发送心跳 (CMD 0x00 = PAUSE，但作为心跳)
        if ((now - last_heartbeat) >= pdMS_TO_TICKS(UART_HEARTBEAT_PERIOD_MS)) {
            // 参考代码中0x00是PAUSE命令，这里用于心跳
            uint8_t heartbeat_data[4] = {0x01, 0x02, 0x03, 0x04};
            uart_send_frame(0x00, heartbeat_data, sizeof(heartbeat_data));
            last_heartbeat = now;
        }

        // 定期发送测试姿态数据 (用于TCP转发测试)
        if ((now - s_last_test_attitude_ms) >= pdMS_TO_TICKS(TEST_ATTITUDE_PERIOD_MS)) {
            static float test_roll = 0.0f, test_pitch = 5.0f, test_yaw = 0.0f;
            test_yaw += 1.0f;
            if (test_yaw > 360.0f) test_yaw -= 360.0f;
            send_attitude_to_tcp(test_roll, test_pitch, test_yaw, 0.0f, 0.0f, 30.0f);
            s_last_test_attitude_ms = now;
        }

        if ((now - last_stats) >= pdMS_TO_TICKS(5000U)) {
            uint32_t age_ms = (s_last_rx_ms == 0U)
                                  ? 0U
                                  : ((xTaskGetTickCount() * portTICK_PERIOD_MS) - s_last_rx_ms);
            const char *ip_str = wifi_get_ip_str();
            ESP_LOGI(TAG,
                     "Stats tx=%lu rx=%lu rx_err=%lu link=%s ip=%s rx_age=%lums",
                     (unsigned long)s_tx_frames,
                     (unsigned long)s_rx_frames,
                     (unsigned long)s_rx_errors,
                     s_link_active ? "up" : "down",
                     ip_str ? ip_str : "none",
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
    ESP_LOGI(TAG, "Protocol: [55][CMD][LEN][DATA][SUM][AA]");
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

    ret = tcp_server_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "TCP server init failed: %s", esp_err_to_name(ret));
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
