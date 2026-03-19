/**
 * @file wifi_config.h
 * @brief WiFi and UART configuration for ESP32-C3 drone communication
 * @note Hardware: ESP32-C3 connected to STM32F411 via UART
 */

#ifndef WIFI_CONFIG_H
#define WIFI_CONFIG_H

#include <Arduino.h>

/* ============================================================================
 * WiFi Configuration
 * ============================================================================ */

#ifndef WIFI_STA_SSID
#define WIFI_STA_SSID           "whc"
#endif

#ifndef WIFI_STA_PASS
#define WIFI_STA_PASS           "12345678"
#endif

#define WIFI_MAX_RETRIES        10
#define WIFI_CONNECT_TIMEOUT_MS 30000

/* ============================================================================
 * UART Configuration for STM32 Communication
 * ============================================================================ */

#define UART_ST_PORT            Serial1
#define UART_ST_TX_PIN          5       // GPIO5 - TX to STM32
#define UART_ST_RX_PIN          4       // GPIO4 - RX from STM32
#define UART_ST_BAUDRATE        115200
#define UART_ST_BUF_SIZE        256

/* ============================================================================
 * TCP Server Configuration
 * ============================================================================ */

#define TCP_SERVER_PORT         8080
#define TCP_MAX_CLIENTS         2
#define TCP_BUF_SIZE            256

/* ============================================================================
 * Protocol Defines
 * ============================================================================ */

#define PROTOCOL_HEADER         0xAA55
#define PROTOCOL_MAX_PAYLOAD    64

/* Message types */
enum MsgType {
    MSG_TYPE_HEARTBEAT = 0x01,
    MSG_TYPE_STATUS = 0x02,
    MSG_TYPE_CONTROL = 0x03,
    MSG_TYPE_SENSOR = 0x04,
    MSG_TYPE_DEBUG = 0x05,
    MSG_TYPE_ACK = 0x06,
    MSG_TYPE_ERROR = 0x07,
    MSG_TYPE_WIFI_STATUS = 0x10
};

/* Protocol header structure */
struct __attribute__((packed)) ProtocolHeader {
    uint16_t header;        // 0xAA55
    uint8_t type;           // Message type
    uint8_t length;         // Payload length
};

/* Complete message structure */
struct __attribute__((packed)) ProtocolMessage {
    ProtocolHeader header;
    uint8_t payload[PROTOCOL_MAX_PAYLOAD];
    uint8_t checksum;
};

/* ============================================================================
 * LED Configuration (if available)
 * ============================================================================ */

#define LED_BUILTIN_PIN         8       // ESP32-C3 built-in LED (usually GPIO8)

#endif // WIFI_CONFIG_H
