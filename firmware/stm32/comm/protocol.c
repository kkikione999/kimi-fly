/**
 * @file protocol.c
 * @brief WiFi通信协议实现
 *
 * @note 实现STM32与ESP32-C3之间的通信协议编解码
 *       使用CRC16进行数据完整性校验
 *
 * @author Drone Control System
 * @version 1.0
 */

#include "protocol.h"
#include <string.h>

/* ============================================================================
 * 静态常量
 * ============================================================================ */

/* CRC16查找表 (CRC-16/CCITT-FALSE) */
static const uint16_t crc16_table[256] = {
    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7,
    0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF,
    0x1231, 0x0210, 0x3273, 0x2252, 0x52B5, 0x4294, 0x72F7, 0x62D6,
    0x9339, 0x8318, 0xB37B, 0xA35A, 0xD3BD, 0xC39C, 0xF3FF, 0xE3DE,
    0x2462, 0x3443, 0x0420, 0x1401, 0x64E6, 0x74C7, 0x44A4, 0x5485,
    0xA56A, 0xB54B, 0x8528, 0x9509, 0xE5EE, 0xF5CF, 0xC5AC, 0xD58D,
    0x3653, 0x2672, 0x1611, 0x0630, 0x76D7, 0x66F6, 0x5695, 0x46B4,
    0xB75B, 0xA77A, 0x9719, 0x8738, 0xF7DF, 0xE7FE, 0xD79D, 0xC7BC,
    0x48C4, 0x58E5, 0x6886, 0x78A7, 0x0840, 0x1861, 0x2802, 0x3823,
    0xC9CC, 0xD9ED, 0xE98E, 0xF9AF, 0x8948, 0x9969, 0xA90A, 0xB92B,
    0x5AF5, 0x4AD4, 0x7AB7, 0x6A96, 0x1A71, 0x0A50, 0x3A33, 0x2A12,
    0xDBFD, 0xCBDC, 0xFBBF, 0xEB9E, 0x9B79, 0x8B58, 0xBB3B, 0xAB1A,
    0x6CA6, 0x7C87, 0x4CE4, 0x5CC5, 0x2C22, 0x3C03, 0x0C60, 0x1C41,
    0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD, 0xAD2A, 0xBD0B, 0x8D68, 0x9D49,
    0x7E97, 0x6EB6, 0x5ED5, 0x4EF4, 0x3E13, 0x2E32, 0x1E51, 0x0E70,
    0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B, 0xAF3A, 0x9F59, 0x8F78,
    0x9188, 0x81A9, 0xB1CA, 0xA1EB, 0xD10C, 0xC12D, 0xF14E, 0xE16F,
    0x1080, 0x00A1, 0x30C2, 0x20E3, 0x5004, 0x4025, 0x7046, 0x6067,
    0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC33D, 0xD31C, 0xE37F, 0xF35E,
    0x02B1, 0x1290, 0x22F3, 0x32D2, 0x4235, 0x5214, 0x6277, 0x7256,
    0xB5EA, 0xA5CB, 0x95A8, 0x8589, 0xF56E, 0xE54F, 0xD52C, 0xC50D,
    0x34E2, 0x24C3, 0x14A0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
    0xA7DB, 0xB7FA, 0x8799, 0x97B8, 0xE75F, 0xF77E, 0xC71D, 0xD73C,
    0x26D3, 0x36F2, 0x0691, 0x16B0, 0x6657, 0x7676, 0x4615, 0x5634,
    0xD94C, 0xC96D, 0xF90E, 0xE92F, 0x99C8, 0x89E9, 0xB98A, 0xA9AB,
    0x5844, 0x4865, 0x7806, 0x6827, 0x18C0, 0x08E1, 0x3882, 0x28A3,
    0xCB7D, 0xDB5C, 0xEB3F, 0xFB1E, 0x8BF9, 0x9BD8, 0xABBB, 0xBB9A,
    0x4A75, 0x5A54, 0x6A37, 0x7A16, 0x0AF1, 0x1AD0, 0x2AB3, 0x3A92,
    0xFD2E, 0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B, 0x9DE8, 0x8DC9,
    0x7C26, 0x6C07, 0x5C64, 0x4C45, 0x3CA2, 0x2C83, 0x1CE0, 0x0CC1,
    0xEF1F, 0xFF3E, 0xCF5D, 0xDF7C, 0xAF9B, 0xBFBA, 0x8FD9, 0x9FF8,
    0x6E17, 0x7E36, 0x4E55, 0x5E74, 0x2E93, 0x3EB2, 0x0ED1, 0x1EF0
};

/* ============================================================================
 * API实现 - CRC计算
 * ============================================================================ */

uint16_t protocol_calc_crc16(const uint8_t *data, uint16_t len)
{
    uint16_t crc = 0xFFFF;  /* CRC16初始值 */

    for (uint16_t i = 0; i < len; i++) {
        crc = (crc << 8) ^ crc16_table[((crc >> 8) ^ data[i]) & 0xFF];
    }

    return crc;
}

bool protocol_verify_crc(const uint8_t *frame, uint16_t len)
{
    if (len < 4) {
        return false;
    }

    /* CRC位于帧的最后2字节 */
    uint16_t rx_crc = ((uint16_t)frame[len - 2] << 8) | frame[len - 1];
    uint16_t calc_crc = protocol_calc_crc16(frame, len - 2);

    return (rx_crc == calc_crc);
}

/* ============================================================================
 * API实现 - 帧编码/解码
 * ============================================================================ */

hal_status_t protocol_encode(const protocol_frame_t *frame,
                              uint8_t *buffer,
                              uint16_t buf_size,
                              uint16_t *out_len)
{
    if (frame == NULL || buffer == NULL || out_len == NULL) {
        return HAL_ERROR;
    }

    /* 计算需要的长度: Header(2) + Len(1) + Cmd(1) + Data(N) + CRC(2) = N + 6 */
    uint16_t frame_len = frame->len + 6;

    if (buf_size < frame_len) {
        return HAL_ERROR;
    }

    if (frame->len > PROTOCOL_MAX_PAYLOAD_SIZE) {
        return HAL_ERROR;
    }

    /* 构建帧 */
    uint16_t idx = 0;

    /* 帧头 (大端序) */
    buffer[idx++] = (PROTOCOL_FRAME_HEADER >> 8) & 0xFF;
    buffer[idx++] = PROTOCOL_FRAME_HEADER & 0xFF;

    /* 长度 (数据部分) */
    buffer[idx++] = frame->len;

    /* 命令 */
    buffer[idx++] = frame->cmd;

    /* 数据负载 */
    if (frame->len > 0) {
        memcpy(&buffer[idx], frame->data, frame->len);
        idx += frame->len;
    }

    /* CRC16 (覆盖帧头到数据) */
    uint16_t crc = protocol_calc_crc16(buffer, idx);
    buffer[idx++] = (crc >> 8) & 0xFF;
    buffer[idx++] = crc & 0xFF;

    *out_len = idx;
    return HAL_OK;
}

hal_status_t protocol_decode(const uint8_t *buffer,
                              uint16_t len,
                              protocol_frame_t *frame,
                              uint16_t *consumed)
{
    if (buffer == NULL || frame == NULL || len < 6) {
        return HAL_ERROR;
    }

    /* 查找帧头 */
    uint16_t start_idx = 0;
    bool found = false;

    for (uint16_t i = 0; i < len - 1; i++) {
        if (((buffer[i] << 8) | buffer[i + 1]) == PROTOCOL_FRAME_HEADER) {
            start_idx = i;
            found = true;
            break;
        }
    }

    if (!found) {
        /* 未找到帧头，消耗所有数据 */
        if (consumed != NULL) {
            *consumed = len;
        }
        return HAL_ERROR;
    }

    /* 检查是否有足够的数据 */
    if (len - start_idx < 6) {
        /* 数据不足，等待更多数据 */
        if (consumed != NULL) {
            *consumed = start_idx;  /* 只消耗帧头之前的数据 */
        }
        return HAL_ERROR;
    }

    /* 解析长度 */
    uint8_t payload_len = buffer[start_idx + 2];

    /* 检查数据长度有效性 */
    if (payload_len > PROTOCOL_MAX_PAYLOAD_SIZE) {
        /* 无效长度，跳过帧头继续查找 */
        if (consumed != NULL) {
            *consumed = start_idx + 1;
        }
        return HAL_ERROR;
    }

    /* 检查完整帧长度 */
    uint16_t total_len = 6 + payload_len;
    if (len - start_idx < total_len) {
        /* 帧不完整，等待更多数据 */
        if (consumed != NULL) {
            *consumed = start_idx;
        }
        return HAL_ERROR;
    }

    /* 验证CRC */
    if (!protocol_verify_crc(&buffer[start_idx], total_len)) {
        /* CRC错误，跳过帧头继续查找 */
        if (consumed != NULL) {
            *consumed = start_idx + 1;
        }
        return HAL_ERROR;
    }

    /* 解析帧 */
    frame->len = payload_len;
    frame->cmd = buffer[start_idx + 3];

    if (payload_len > 0) {
        memcpy(frame->data, &buffer[start_idx + 4], payload_len);
    }

    if (consumed != NULL) {
        *consumed = start_idx + total_len;
    }

    return HAL_OK;
}

/* ============================================================================
 * API实现 - 数据打包辅助函数
 * ============================================================================ */

void protocol_pack_rc(const protocol_rc_data_t *rc, protocol_frame_t *frame)
{
    if (rc == NULL || frame == NULL) {
        return;
    }

    frame->cmd = CMD_RC_INPUT;
    frame->len = sizeof(protocol_rc_data_t);
    memcpy(frame->data, rc, sizeof(protocol_rc_data_t));
}

bool protocol_unpack_rc(const protocol_frame_t *frame, protocol_rc_data_t *rc)
{
    if (frame == NULL || rc == NULL) {
        return false;
    }

    if (frame->cmd != CMD_RC_INPUT || frame->len != sizeof(protocol_rc_data_t)) {
        return false;
    }

    memcpy(rc, frame->data, sizeof(protocol_rc_data_t));
    return true;
}

void protocol_pack_attitude(const protocol_attitude_t *att, protocol_frame_t *frame)
{
    if (att == NULL || frame == NULL) {
        return;
    }

    frame->cmd = CMD_TELEMETRY_ATT;
    frame->len = sizeof(protocol_attitude_t);
    memcpy(frame->data, att, sizeof(protocol_attitude_t));
}

void protocol_pack_motor(const protocol_motor_t *motor, protocol_frame_t *frame)
{
    if (motor == NULL || frame == NULL) {
        return;
    }

    frame->cmd = CMD_TELEMETRY_MOTOR;
    frame->len = sizeof(protocol_motor_t);
    memcpy(frame->data, motor, sizeof(protocol_motor_t));
}

void protocol_pack_status(const protocol_status_t *status, protocol_frame_t *frame)
{
    if (status == NULL || frame == NULL) {
        return;
    }

    frame->cmd = CMD_STATUS;
    frame->len = sizeof(protocol_status_t);
    memcpy(frame->data, status, sizeof(protocol_status_t));
}

void protocol_make_ack(uint8_t cmd, protocol_frame_t *frame)
{
    if (frame == NULL) {
        return;
    }

    frame->cmd = CMD_ACK;
    frame->len = 1;
    frame->data[0] = cmd;
}

void protocol_make_nack(uint8_t cmd, uint8_t error, protocol_frame_t *frame)
{
    if (frame == NULL) {
        return;
    }

    frame->cmd = CMD_NACK;
    frame->len = 2;
    frame->data[0] = cmd;
    frame->data[1] = error;
}
