/**
 * @file comm_protocol.h
 * @brief Shared communication protocol between STM32 and ESP32
 * @note This header is shared between STM32 and ESP32 firmware
 */

#ifndef COMM_PROTOCOL_H
#define COMM_PROTOCOL_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Protocol Constants
 * ============================================================================ */

#define PROTOCOL_HEADER         0xAA55
#define PROTOCOL_MAX_PAYLOAD    64
#define PROTOCOL_VERSION        1

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

/* Error codes */
typedef enum {
    COMM_ERR_NONE = 0,
    COMM_ERR_TIMEOUT = 1,
    COMM_ERR_CHECKSUM = 2,
    COMM_ERR_BUFFER_OVERFLOW = 3,
    COMM_ERR_INVALID_TYPE = 4,
    COMM_ERR_UART_ERROR = 5
} comm_error_t;

/* ============================================================================
 * Protocol Structures
 * ============================================================================ */

/* Protocol header structure - packed for consistent wire format */
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

/* Communication statistics */
typedef struct {
    uint32_t tx_count;
    uint32_t rx_count;
    uint32_t error_count;
    uint32_t timeout_count;
    uint32_t last_rx_time;
    bool connected;
} comm_stats_t;

/* ============================================================================
 * Protocol Functions
 * ============================================================================ */

/**
 * @brief Calculate checksum for a message
 * @param msg Message pointer
 * @return Calculated checksum (XOR of all bytes except checksum field)
 */
static inline uint8_t comm_calculate_checksum(const protocol_message_t *msg)
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
 * @param msg Message pointer
 * @return true if checksum is valid, false otherwise
 */
static inline bool comm_verify_checksum(const protocol_message_t *msg)
{
    return comm_calculate_checksum(msg) == msg->checksum;
}

/**
 * @brief Build a protocol message
 * @param msg Message buffer to fill
 * @param type Message type
 * @param payload Payload data (can be NULL if length is 0)
 * @param payload_len Payload length (max PROTOCOL_MAX_PAYLOAD)
 * @return Total message length in bytes, or -1 on error
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
        for (uint8_t i = 0; i < payload_len; i++) {
            msg->payload[i] = payload[i];
        }
    }

    msg->checksum = comm_calculate_checksum(msg);

    return sizeof(protocol_header_t) + payload_len + 1; /* +1 for checksum */
}

/**
 * @brief Parse a received buffer to extract a message
 * @param buffer Raw received data
 * @param buf_len Buffer length
 * @param msg Output message structure
 * @return Number of bytes consumed from buffer, or 0 if no valid message found
 */
static inline int comm_parse_message(const uint8_t *buffer, uint16_t buf_len,
                                      protocol_message_t *msg)
{
    if (buf_len < sizeof(protocol_header_t)) {
        return 0;
    }

    /* Look for header */
    for (uint16_t i = 0; i <= buf_len - sizeof(protocol_header_t); i++) {
        uint16_t header = buffer[i] | ((uint16_t)buffer[i + 1] << 8);

        if (header == PROTOCOL_HEADER) {
            /* Found header, check if we have complete message */
            uint8_t payload_len = buffer[i + 3];
            uint16_t total_len = sizeof(protocol_header_t) + payload_len + 1;

            if (i + total_len <= buf_len) {
                /* Copy message */
                const uint8_t *msg_start = &buffer[i];
                for (uint16_t j = 0; j < total_len; j++) {
                    ((uint8_t *)msg)[j] = msg_start[j];
                }

                /* Verify checksum */
                if (comm_verify_checksum(msg)) {
                    return total_len;
                }
            }
            return 0; /* Header found but incomplete message */
        }
    }

    return 0;
}

#ifdef __cplusplus
}
#endif

#endif /* COMM_PROTOCOL_H */
