/**
 * @file protocol_bridge.c
 * @brief STM32-compatible UART protocol helpers for ESP32-C3
 */

#include "protocol_bridge.h"

#include <stdbool.h>
#include <string.h>

uint16_t uart_protocol_calc_crc16(const uint8_t *data, size_t len)
{
    uint16_t crc = 0xFFFFU;

    if (data == NULL) {
        return crc;
    }

    for (size_t i = 0; i < len; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for (int bit = 0; bit < 8; bit++) {
            if (crc & 0x8000U) {
                crc = (uint16_t)((crc << 1) ^ 0x1021U);
            } else {
                crc = (uint16_t)(crc << 1);
            }
        }
    }

    return crc;
}

esp_err_t uart_protocol_encode(const uart_protocol_frame_t *frame,
                               uint8_t *buffer,
                               size_t buffer_size,
                               size_t *out_len)
{
    if (frame == NULL || buffer == NULL || out_len == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (frame->len > UART_PROTOCOL_MAX_PAYLOAD) {
        return ESP_ERR_INVALID_SIZE;
    }

    const size_t total_len = (size_t)frame->len + 6U;
    if (buffer_size < total_len) {
        return ESP_ERR_NO_MEM;
    }

    buffer[0] = 0xAAU;
    buffer[1] = 0x55U;
    buffer[2] = frame->len;
    buffer[3] = frame->cmd;

    if (frame->len > 0U) {
        memcpy(&buffer[4], frame->data, frame->len);
    }

    const uint16_t crc = uart_protocol_calc_crc16(buffer, 4U + frame->len);
    buffer[4U + frame->len] = (uint8_t)((crc >> 8) & 0xFFU);
    buffer[5U + frame->len] = (uint8_t)(crc & 0xFFU);
    *out_len = total_len;

    return ESP_OK;
}

esp_err_t uart_protocol_decode(const uint8_t *buffer,
                               size_t len,
                               uart_protocol_frame_t *frame,
                               size_t *consumed)
{
    if (buffer == NULL || frame == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (consumed != NULL) {
        *consumed = 0U;
    }

    if (len < 2U) {
        return ESP_ERR_INVALID_SIZE;
    }

    size_t start = 0U;
    bool found = false;

    for (size_t i = 0; i + 1U < len; i++) {
        if (buffer[i] == 0xAAU && buffer[i + 1U] == 0x55U) {
            start = i;
            found = true;
            break;
        }
    }

    if (!found) {
        if (consumed != NULL) {
            *consumed = len;
        }
        return ESP_FAIL;
    }

    if ((len - start) < 6U) {
        if (consumed != NULL) {
            *consumed = start;
        }
        return ESP_ERR_INVALID_SIZE;
    }

    const uint8_t payload_len = buffer[start + 2U];
    if (payload_len > UART_PROTOCOL_MAX_PAYLOAD) {
        if (consumed != NULL) {
            *consumed = start + 1U;
        }
        return ESP_ERR_INVALID_SIZE;
    }

    const size_t total_len = (size_t)payload_len + 6U;
    if ((len - start) < total_len) {
        if (consumed != NULL) {
            *consumed = start;
        }
        return ESP_ERR_INVALID_SIZE;
    }

    const uint16_t rx_crc = ((uint16_t)buffer[start + total_len - 2U] << 8)
                          | (uint16_t)buffer[start + total_len - 1U];
    const uint16_t calc_crc = uart_protocol_calc_crc16(&buffer[start], total_len - 2U);
    if (rx_crc != calc_crc) {
        if (consumed != NULL) {
            *consumed = start + 1U;
        }
        return ESP_FAIL;
    }

    frame->cmd = buffer[start + 3U];
    frame->len = payload_len;
    if (payload_len > 0U) {
        memcpy(frame->data, &buffer[start + 4U], payload_len);
    }

    if (consumed != NULL) {
        *consumed = start + total_len;
    }

    return ESP_OK;
}

const char *uart_protocol_cmd_name(uint8_t cmd)
{
    switch (cmd) {
        case UART_PROTOCOL_CMD_HEARTBEAT:       return "HEARTBEAT";
        case UART_PROTOCOL_CMD_ACK:             return "ACK";
        case UART_PROTOCOL_CMD_NACK:            return "NACK";
        case UART_PROTOCOL_CMD_VERSION:         return "VERSION";
        case UART_PROTOCOL_CMD_STATUS:          return "STATUS";
        case UART_PROTOCOL_CMD_ARM:             return "ARM";
        case UART_PROTOCOL_CMD_DISARM:          return "DISARM";
        case UART_PROTOCOL_CMD_MODE:            return "MODE";
        case UART_PROTOCOL_CMD_RC_INPUT:        return "RC_INPUT";
        case UART_PROTOCOL_CMD_TELEMETRY_ATT:   return "TELEMETRY_ATT";
        case UART_PROTOCOL_CMD_TELEMETRY_IMU:   return "TELEMETRY_IMU";
        case UART_PROTOCOL_CMD_TELEMETRY_MOTOR: return "TELEMETRY_MOTOR";
        case UART_PROTOCOL_CMD_TELEMETRY_BATTERY:return "TELEMETRY_BATTERY";
        case UART_PROTOCOL_CMD_PID_GET:         return "PID_GET";
        case UART_PROTOCOL_CMD_PID_SET:         return "PID_SET";
        case UART_PROTOCOL_CMD_CALIBRATE:       return "CALIBRATE";
        default:                                return "UNKNOWN";
    }
}
