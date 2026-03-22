/**
 * @file protocol_bridge.c
 * @brief STM32-compatible UART protocol helpers for ESP32-C3
 * @note 支持两种协议格式:
 *       1. 参考代码格式: [0x55][CMD][LEN][DATA...][SUM8][0xAA]
 *       2. 兼容格式: [0x55][0xAA][CMD][LEN][DATA...][XOR]
 */

#include "protocol_bridge.h"

#include <stdbool.h>
#include <string.h>

uint8_t uart_protocol_calc_sum8(uint8_t cmd, uint8_t len, const uint8_t *data)
{
    uint8_t sum = cmd + len;
    for (uint8_t i = 0; i < len; i++) {
        sum += data[i];
    }
    return sum & 0xFF;
}

// XOR checksum (兼容我们的STM32旧协议)
static uint8_t calc_xor(const uint8_t *data, uint8_t len)
{
    uint8_t cs = 0;
    for (uint8_t i = 0; i < len; i++) {
        cs ^= data[i];
    }
    return cs;
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

    // 使用参考代码格式: [0x55][CMD][LEN][DATA...][SUM8][0xAA]
    size_t need = 1 + 1 + 1 + frame->len + 1 + 1;
    if (buffer_size < need) {
        return ESP_ERR_NO_MEM;
    }

    size_t w = 0;
    buffer[w++] = 0x55U;           // HEADER
    buffer[w++] = frame->cmd;       // CMD
    buffer[w++] = frame->len;       // LEN

    if (frame->len > 0U) {
        memcpy(&buffer[w], frame->data, frame->len);
        w += frame->len;
    }

    buffer[w++] = uart_protocol_calc_sum8(frame->cmd, frame->len, frame->data); // SUM8
    buffer[w++] = 0xAAU;           // TAIL

    *out_len = w;
    return ESP_OK;
}

// 解析器状态
typedef enum {
    PARSER_STATE_WAIT_HEADER,
    PARSER_STATE_CMD_OR_HEADER2,
    PARSER_STATE_CMD,
    PARSER_STATE_LEN,
    PARSER_STATE_DATA,
    PARSER_STATE_SUM,
    PARSER_STATE_TAIL
} parser_state_t;

// 解析器上下文
typedef struct {
    parser_state_t state;
    uint8_t cmd;
    uint8_t len;
    uint8_t data[UART_PROTOCOL_MAX_PAYLOAD];
    uint8_t data_idx;  // 已接收的数据字节数
    uint8_t sum;
    uint8_t sum_calc;
    bool expect_xor;  // true = XOR校验 (旧格式), false = SUM8校验 (参考格式)
} uart_parser_ctx_t;

static uart_parser_ctx_t s_parser_ctx;

void uart_protocol_parser_reset(void)
{
    memset(&s_parser_ctx, 0, sizeof(s_parser_ctx));
    s_parser_ctx.state = PARSER_STATE_WAIT_HEADER;
    s_parser_ctx.expect_xor = false;
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
        *consumed = 0;
    }

    for (size_t i = 0; i < len; i++) {
        uint8_t byte = buffer[i];

        switch (s_parser_ctx.state) {
            case PARSER_STATE_WAIT_HEADER:
                if (byte == 0x55U) {
                    s_parser_ctx.state = PARSER_STATE_CMD_OR_HEADER2;
                    s_parser_ctx.expect_xor = false;
                    s_parser_ctx.data_idx = 0;
                }
                if (consumed != NULL) {
                    *consumed = i + 1;
                }
                break;

            case PARSER_STATE_CMD_OR_HEADER2:
                if (byte == 0xAAU) {
                    // 双字节帧头 [55 AA] - 旧格式
                    s_parser_ctx.state = PARSER_STATE_CMD;
                    s_parser_ctx.expect_xor = true;
                    s_parser_ctx.data_idx = 0;
                } else {
                    // 单字节帧头后直接跟CMD - 参考格式
                    s_parser_ctx.cmd = byte;
                    s_parser_ctx.state = PARSER_STATE_LEN;
                    s_parser_ctx.expect_xor = false;
                    s_parser_ctx.data_idx = 0;
                }
                if (consumed != NULL) {
                    *consumed = i + 1;
                }
                break;

            case PARSER_STATE_CMD:
                s_parser_ctx.cmd = byte;
                s_parser_ctx.state = PARSER_STATE_LEN;
                s_parser_ctx.data_idx = 0;
                if (consumed != NULL) {
                    *consumed = i + 1;
                }
                break;

            case PARSER_STATE_LEN:
                s_parser_ctx.len = byte;
                s_parser_ctx.data_idx = 0;
                if (s_parser_ctx.len > UART_PROTOCOL_MAX_PAYLOAD) {
                    // 数据太长，重置
                    uart_protocol_parser_reset();
                    if (consumed != NULL) {
                        *consumed = i + 1;
                    }
                } else if (s_parser_ctx.len == 0) {
                    // 无数据，跳到校验
                    if (s_parser_ctx.expect_xor) {
                        // 旧格式: XOR覆盖 [55][AA][CMD][LEN]
                        uint8_t xor_data[4] = {0x55, 0xAA, s_parser_ctx.cmd, 0};
                        s_parser_ctx.sum_calc = calc_xor(xor_data, 4);
                        s_parser_ctx.state = PARSER_STATE_SUM;
                    } else {
                        // 参考格式: SUM8覆盖 [CMD][LEN]
                        s_parser_ctx.sum_calc = uart_protocol_calc_sum8(s_parser_ctx.cmd, 0, NULL);
                        s_parser_ctx.state = PARSER_STATE_SUM;
                    }
                    if (consumed != NULL) {
                        *consumed = i + 1;
                    }
                } else {
                    s_parser_ctx.state = PARSER_STATE_DATA;
                    if (consumed != NULL) {
                        *consumed = i + 1;
                    }
                }
                break;

            case PARSER_STATE_DATA:
                s_parser_ctx.data[s_parser_ctx.data_idx++] = byte;
                if (s_parser_ctx.data_idx >= s_parser_ctx.len) {
                    // 数据接收完成
                    if (s_parser_ctx.expect_xor) {
                        // 旧格式: XOR覆盖 [55][AA][CMD][LEN][DATA]
                        // 重新计算XOR，覆盖整个帧头+命令+长度+数据
                        uint8_t xor_data[6] = {0x55, 0xAA, s_parser_ctx.cmd, s_parser_ctx.len,
                                               s_parser_ctx.data[0], s_parser_ctx.data[1]};
                        if (s_parser_ctx.len > 0) {
                            // XOR覆盖 [55][AA][CMD][LEN][DATA]
                            s_parser_ctx.sum_calc = calc_xor(xor_data, 4);
                            for (uint8_t j = 0; j < s_parser_ctx.len; j++) {
                                s_parser_ctx.sum_calc ^= s_parser_ctx.data[j];
                            }
                        } else {
                            s_parser_ctx.sum_calc = calc_xor(xor_data, 4);
                        }
                        s_parser_ctx.state = PARSER_STATE_SUM;
                    } else {
                        // 参考格式: SUM8覆盖 [CMD][LEN][DATA]
                        s_parser_ctx.sum_calc = uart_protocol_calc_sum8(s_parser_ctx.cmd, s_parser_ctx.len, s_parser_ctx.data);
                        s_parser_ctx.state = PARSER_STATE_TAIL;
                    }
                }
                if (consumed != NULL) {
                    *consumed = i + 1;
                }
                break;

            case PARSER_STATE_SUM:
                s_parser_ctx.sum = byte;
                if (s_parser_ctx.expect_xor) {
                    // 旧格式: 收到XOR后直接验证
                    if (s_parser_ctx.sum == s_parser_ctx.sum_calc) {
                        frame->cmd = s_parser_ctx.cmd;
                        frame->len = s_parser_ctx.len;
                        if (s_parser_ctx.len > 0) {
                            memcpy(frame->data, s_parser_ctx.data, s_parser_ctx.len);
                        }
                        if (consumed != NULL) {
                            *consumed = i + 1;
                        }
                        uart_protocol_parser_reset();
                        return ESP_OK;
                    } else {
                        // 校验失败，重置
                        uart_protocol_parser_reset();
                    }
                } else {
                    // 参考格式: 跳到TAIL
                    s_parser_ctx.state = PARSER_STATE_TAIL;
                }
                if (consumed != NULL) {
                    *consumed = i + 1;
                }
                break;

            case PARSER_STATE_TAIL:
                if (byte == 0xAAU) {
                    // 完整帧接收成功 (参考格式)
                    if (s_parser_ctx.sum == s_parser_ctx.sum_calc) {
                        frame->cmd = s_parser_ctx.cmd;
                        frame->len = s_parser_ctx.len;
                        if (s_parser_ctx.len > 0) {
                            memcpy(frame->data, s_parser_ctx.data, s_parser_ctx.len);
                        }
                        if (consumed != NULL) {
                            *consumed = i + 1;
                        }
                        uart_protocol_parser_reset();
                        return ESP_OK;
                    }
                }
                // 帧尾错误或校验失败，重置
                uart_protocol_parser_reset();
                if (consumed != NULL) {
                    *consumed = i + 1;
                }
                break;
        }
    }

    return ESP_ERR_INVALID_SIZE;
}

const char *uart_protocol_cmd_name(uint8_t cmd)
{
    switch (cmd) {
        case 0x00: return "PAUSE";
        case 0x01: return "QUATERNION";
        case 0x02: return "JOYSTICK";
        case 0x03: return "SET_PITCH_PID";
        case 0x04: return "SET_ROLL_PID";
        case 0x05: return "SET_YAW_PID";
        case 0x06: return "SOFTWARE_RESTART";
        case 0x07: return "PID_CHECK";
        default:   return "UNKNOWN";
    }
}
