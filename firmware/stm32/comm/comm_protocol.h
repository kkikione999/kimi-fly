/**
 * @file comm_protocol.h
 * @brief 简化的通信协议定义 (用于uart_comm_test.c)
 * @note 与ESP32-C3协议兼容
 *
 * @author Drone Control System
 * @version 1.0
 */

#ifndef COMM_PROTOCOL_H
#define COMM_PROTOCOL_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * 协议常量定义
 * ============================================================================ */

#define PROTOCOL_HEADER         0xAA55  /**< 帧头 (2字节，小端序在ESP32端) */
#define PROTOCOL_MAX_PAYLOAD    64      /**< 最大负载字节数 */

/* ============================================================================
 * 消息类型定义 (与ESP32 main.c 兼容)
 * ============================================================================ */

typedef enum {
    MSG_TYPE_HEARTBEAT = 0x01,
    MSG_TYPE_STATUS = 0x02,
    MSG_TYPE_CONTROL = 0x03,
    MSG_TYPE_SENSOR = 0x04,
    MSG_TYPE_DEBUG = 0x05,
    MSG_TYPE_ACK = 0x06,
    MSG_TYPE_ERROR = 0x07
} msg_type_t;

/* ============================================================================
 * 协议帧结构 (与ESP32兼容)
 * ============================================================================ */

/* 协议头部结构 */
typedef struct __attribute__((packed)) {
    uint16_t header;        /* 0xAA55 */
    uint8_t type;           /* 消息类型 */
    uint8_t length;         /* 负载长度 */
} protocol_header_t;

/* 完整消息结构 */
typedef struct __attribute__((packed)) {
    protocol_header_t header;
    uint8_t payload[PROTOCOL_MAX_PAYLOAD];
    uint8_t checksum;
} protocol_message_t;

/* 通信统计 */
typedef struct {
    uint32_t tx_count;
    uint32_t rx_count;
    uint32_t error_count;
    uint32_t last_rx_time;
    bool connected;
} comm_stats_t;

/* ============================================================================
 * API函数声明
 * ============================================================================ */

/**
 * @brief 计算校验和
 * @param msg 消息指针
 * @return 校验和
 */
static inline uint8_t comm_calc_checksum(const protocol_message_t *msg)
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
 * @brief 验证校验和
 * @param msg 消息指针
 * @return true=校验通过
 */
static inline bool comm_verify_checksum(const protocol_message_t *msg)
{
    return comm_calc_checksum(msg) == msg->checksum;
}

/**
 * @brief 构建消息
 * @param msg 消息结构指针
 * @param type 消息类型
 * @param payload 负载数据
 * @param payload_len 负载长度
 * @return 消息总长度
 */
static inline int comm_build_message(protocol_message_t *msg, msg_type_t type,
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

    msg->checksum = comm_calc_checksum(msg);

    return sizeof(protocol_header_t) + payload_len + 1; /* +1 for checksum */
}

/**
 * @brief 解析消息
 * @param buffer 输入缓冲区
 * @param len 缓冲区长度
 * @param msg 输出消息结构
 * @return 消耗的字节数 (0=未找到完整消息)
 */
static inline int comm_parse_message(const uint8_t *buffer, uint16_t len,
                                      protocol_message_t *msg)
{
    if (len < sizeof(protocol_header_t)) {
        return 0;
    }

    /* 查找帧头 */
    for (uint16_t i = 0; i <= len - sizeof(protocol_header_t); i++) {
        uint16_t header = buffer[i] | (buffer[i+1] << 8);

        if (header == PROTOCOL_HEADER) {
            /* 找到帧头 */
            uint8_t payload_len = buffer[i + 3];
            uint8_t total_len = sizeof(protocol_header_t) + payload_len + 1;

            if (i + total_len > len) {
                /* 数据不足，等待更多数据 */
                return 0;
            }

            /* 复制消息 */
            memcpy(msg, &buffer[i], total_len);

            /* 验证校验和 */
            if (comm_verify_checksum(msg)) {
                return total_len;
            } else {
                /* 校验失败，跳过这个帧头继续查找 */
                continue;
            }
        }
    }

    return 0;
}

#ifdef __cplusplus
}
#endif

#endif /* COMM_PROTOCOL_H */
