/**
 * @file protocol_bridge.h
 * @brief STM32-compatible UART protocol helpers for ESP32-C3
 * @note 协议格式: [0x55][CMD][LEN][DATA...][SUM8][0xAA]
 */

#ifndef PROTOCOL_BRIDGE_H
#define PROTOCOL_BRIDGE_H

#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define UART_PROTOCOL_MAX_PAYLOAD  64U

// CMD IDs (与参考代码一致)
typedef enum {
    UART_PROTOCOL_CMD_PAUSE      = 0x00,
    UART_PROTOCOL_CMD_QUATERNION = 0x01,
    UART_PROTOCOL_CMD_JOYSTICK    = 0x02,
    UART_PROTOCOL_CMD_SET_PITCH_PID = 0x03,
    UART_PROTOCOL_CMD_SET_ROLL_PID  = 0x04,
    UART_PROTOCOL_CMD_SET_YAW_PID   = 0x05,
    UART_PROTOCOL_CMD_SOFTWARE_RESTART = 0x06,
    UART_PROTOCOL_CMD_PID_CHECK = 0x07
} uart_protocol_cmd_t;

typedef struct {
    uint8_t cmd;
    uint8_t len;
    uint8_t data[UART_PROTOCOL_MAX_PAYLOAD];
} uart_protocol_frame_t;

uint8_t uart_protocol_calc_sum8(uint8_t cmd, uint8_t len, const uint8_t *data);

esp_err_t uart_protocol_encode(const uart_protocol_frame_t *frame,
                               uint8_t *buffer,
                               size_t buffer_size,
                               size_t *out_len);

esp_err_t uart_protocol_decode(const uint8_t *buffer,
                               size_t len,
                               uart_protocol_frame_t *frame,
                               size_t *consumed);

void uart_protocol_parser_reset(void);

const char *uart_protocol_cmd_name(uint8_t cmd);

#ifdef __cplusplus
}
#endif

#endif /* PROTOCOL_BRIDGE_H */
