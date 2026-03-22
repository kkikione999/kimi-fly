/**
 * @file comm_protocol.h
 * @brief STM32与ESP32-C3通信协议定义
 * @note 协议格式: [0x55][CMD][LEN][DATA...][SUM8][0xAA]
 *       与参考代码 /Users/ll/fly/zmgjb/code 一致
 *
 * @author Drone Control System
 * @version 3.0
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
 * 协议常量定义 (与参考代码一致)
 * ============================================================================ */

#define PROTOCOL_HEADER         0x55U   /**< 帧头 (参考代码) */
#define PROTOCOL_TAIL           0xAAU   /**< 帧尾 (参考代码) */
#define PROTOCOL_MAX_PAYLOAD    64      /**< 最大负载字节数 */

/* ============================================================================
 * 消息类型定义 (与参考代码C3/src/myserial.h一致)
 * ============================================================================ */

typedef enum {
    MSG_TYPE_PAUSE = 0x00,             /**< 暂停命令 */
    MSG_TYPE_QUATERNION = 0x01,         /**< 四元数 */
    MSG_TYPE_JOYSTICK = 0x02,           /**< 摇杆数据 */
    MSG_TYPE_SET_PITCH_PID = 0x03,      /**< 设置Pitch PID */
    MSG_TYPE_SET_ROLL_PID = 0x04,       /**< 设置Roll PID */
    MSG_TYPE_SET_YAW_PID = 0x05,        /**< 设置Yaw PID */
    MSG_TYPE_SOFTWARE_RESTART = 0x06,   /**< 软件重启 */
    MSG_TYPE_PID_CHECK = 0x07           /**< PID检查 */
} msg_type_t;

/* ============================================================================
 * 协议帧结构 (与参考代码格式一致)
 * ============================================================================ */

/* 协议帧: [0x55][CMD][LEN][DATA:N][SUM8][0xAA] */
typedef struct __attribute__((packed)) {
    uint8_t header;        /* 0x55 */
    uint8_t cmd;           /* 命令码 */
    uint8_t length;       /* 负载长度 */
    uint8_t payload[PROTOCOL_MAX_PAYLOAD];
    uint8_t sum8;         /* SUM8校验和 */
    uint8_t tail;          /* 0xAA */
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
 * @brief 计算SUM8校验码 (与参考代码一致)
 * @param cmd 命令码
 * @param len 负载长度
 * @param data 负载数据
 * @return SUM8校验码
 */
static inline uint8_t comm_calc_sum8(uint8_t cmd, uint8_t len, const uint8_t *data)
{
    uint8_t sum = cmd + len;
    for (uint8_t i = 0; i < len; i++) {
        sum += data[i];
    }
    return sum & 0xFF;
}

/**
 * @brief 构建消息 (与参考代码格式一致)
 * @param msg 消息结构指针
 * @param cmd 命令码
 * @param payload 负载数据
 * @param payload_len 负载长度
 * @return 消息总长度
 */
static inline int comm_build_message(protocol_message_t *msg, msg_type_t cmd,
                                      const uint8_t *payload, uint8_t payload_len)
{
    if (payload_len > PROTOCOL_MAX_PAYLOAD) {
        return -1;
    }

    msg->header = PROTOCOL_HEADER;
    msg->cmd = cmd;
    msg->length = payload_len;

    if (payload_len > 0 && payload != NULL) {
        memcpy(msg->payload, payload, payload_len);
    }

    /* 计算SUM8: CMD + LEN + DATA */
    msg->sum8 = comm_calc_sum8(cmd, payload_len, payload);
    msg->tail = PROTOCOL_TAIL;

    return 1 + 1 + 1 + payload_len + 1 + 1; /* header + cmd + len + payload + sum + tail */
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
    if (len < 6) {
        return 0;
    }

    /* 查找帧头 */
    for (uint16_t i = 0; i <= len - 6; i++) {
        if (buffer[i] == PROTOCOL_HEADER) {
            /* 可能找到帧头 */
            uint8_t cmd = buffer[i + 1];
            uint8_t payload_len = buffer[i + 2];
            uint8_t total_len = 1 + 1 + 1 + payload_len + 1 + 1; /* 完整帧长度 */

            if (i + total_len > len) {
                /* 数据不足，等待更多数据 */
                continue;
            }

            /* 检查帧尾 */
            if (buffer[i + total_len - 1] != PROTOCOL_TAIL) {
                continue;
            }

            /* 验证SUM8 */
            uint8_t calc_sum = comm_calc_sum8(cmd, payload_len,
                                              payload_len > 0 ? &buffer[i + 3] : NULL);
            uint8_t rx_sum = buffer[i + 3 + payload_len];

            if (calc_sum != rx_sum) {
                /* 校验失败，跳过这个帧头 */
                continue;
            }

            /* 复制消息 */
            memcpy(msg, &buffer[i], total_len);
            return total_len;
        }
    }

    return 0;
}

#ifdef __cplusplus
}
#endif

#endif /* COMM_PROTOCOL_H */
