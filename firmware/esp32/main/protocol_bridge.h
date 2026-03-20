/**
 * @file protocol_bridge.h
 * @brief STM32-compatible UART protocol helpers for ESP32-C3
 */

#ifndef PROTOCOL_BRIDGE_H
#define PROTOCOL_BRIDGE_H

#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define UART_PROTOCOL_FRAME_HEADER      0xAA55U
#define UART_PROTOCOL_MAX_PAYLOAD      64U

typedef enum {
    UART_PROTOCOL_CMD_HEARTBEAT    = 0x00,
    UART_PROTOCOL_CMD_ACK          = 0x01,
    UART_PROTOCOL_CMD_NACK         = 0x02,
    UART_PROTOCOL_CMD_VERSION      = 0x03,
    UART_PROTOCOL_CMD_STATUS       = 0x04,
    UART_PROTOCOL_CMD_ARM          = 0x10,
    UART_PROTOCOL_CMD_DISARM       = 0x11,
    UART_PROTOCOL_CMD_MODE         = 0x12,
    UART_PROTOCOL_CMD_RC_INPUT     = 0x13,
    UART_PROTOCOL_CMD_TELEMETRY_ATT = 0x20,
    UART_PROTOCOL_CMD_TELEMETRY_IMU = 0x21,
    UART_PROTOCOL_CMD_TELEMETRY_MOTOR = 0x22,
    UART_PROTOCOL_CMD_TELEMETRY_BATTERY = 0x23,
    UART_PROTOCOL_CMD_PID_GET      = 0x30,
    UART_PROTOCOL_CMD_PID_SET      = 0x31,
    UART_PROTOCOL_CMD_CALIBRATE    = 0x32,
} uart_protocol_cmd_t;

typedef struct {
    uint8_t cmd;
    uint8_t len;
    uint8_t data[UART_PROTOCOL_MAX_PAYLOAD];
} uart_protocol_frame_t;

uint16_t uart_protocol_calc_crc16(const uint8_t *data, size_t len);

esp_err_t uart_protocol_encode(const uart_protocol_frame_t *frame,
                               uint8_t *buffer,
                               size_t buffer_size,
                               size_t *out_len);

esp_err_t uart_protocol_decode(const uint8_t *buffer,
                               size_t len,
                               uart_protocol_frame_t *frame,
                               size_t *consumed);

const char *uart_protocol_cmd_name(uint8_t cmd);

#ifdef __cplusplus
}
#endif

#endif /* PROTOCOL_BRIDGE_H */
