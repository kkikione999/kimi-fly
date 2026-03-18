/**
 * @file uart.h
 * @brief STM32 UART HAL接口定义
 * @note 用于USART2与ESP32-C3 WiFi模块通信
 */

#ifndef UART_H
#define UART_H

#include "hal_common.h"
#include "gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * UART实例定义
 * ============================================================================ */

typedef enum {
    UART_INSTANCE_1 = 0,
    UART_INSTANCE_2 = 1,
    UART_INSTANCE_6 = 2,
    UART_INSTANCE_MAX
} uart_instance_t;

/* ============================================================================
 * UART配置定义
 * ============================================================================ */

/* 波特率定义 */
#define UART_BAUDRATE_9600      9600U
#define UART_BAUDRATE_19200     19200U
#define UART_BAUDRATE_38400     38400U
#define UART_BAUDRATE_57600     57600U
#define UART_BAUDRATE_115200    115200U
#define UART_BAUDRATE_230400    230400U
#define UART_BAUDRATE_460800    460800U
#define UART_BAUDRATE_921600    921600U

#define UART_DEFAULT_BAUDRATE   UART_BAUDRATE_115200

/* 数据位定义 - 条件定义避免冲突 */
#ifndef UART_WORDLENGTH_8B
#define UART_WORDLENGTH_8B      0x00000000U
#endif
#ifndef UART_WORDLENGTH_9B
#define UART_WORDLENGTH_9B      ((uint32_t)USART_CR1_M)
#endif

typedef enum {
    UART_DATABITS_8 = 0,
    UART_DATABITS_9 = 1
} uart_databits_t;

/* 停止位定义 - 条件定义避免冲突 */
#ifndef UART_STOPBITS_1
#define UART_STOPBITS_1         0x00000000U
#endif
#ifndef UART_STOPBITS_0_5
#define UART_STOPBITS_0_5       ((uint32_t)USART_CR2_STOP_0)
#endif
#ifndef UART_STOPBITS_2
#define UART_STOPBITS_2         ((uint32_t)USART_CR2_STOP_1)
#endif
#ifndef UART_STOPBITS_1_5
#define UART_STOPBITS_1_5       ((uint32_t)(USART_CR2_STOP_0 | USART_CR2_STOP_1))
#endif

typedef enum {
    UART_STOPBITS_1_VAL   = 0,
    UART_STOPBITS_0_5_VAL = 1,
    UART_STOPBITS_2_VAL   = 2,
    UART_STOPBITS_1_5_VAL = 3
} uart_stopbits_t;

/* 校验位定义 - 条件定义避免冲突 */
#ifndef UART_PARITY_NONE
#define UART_PARITY_NONE        0x00000000U
#endif
#ifndef UART_PARITY_EVEN
#define UART_PARITY_EVEN        ((uint32_t)USART_CR1_PCE)
#endif
#ifndef UART_PARITY_ODD
#define UART_PARITY_ODD         ((uint32_t)(USART_CR1_PCE | USART_CR1_PS))
#endif

typedef enum {
    UART_PARITY_NONE_VAL = 0,
    UART_PARITY_EVEN_VAL = 1,
    UART_PARITY_ODD_VAL  = 2
} uart_parity_t;

/* 硬件流控定义 - 条件定义避免冲突 */
#ifndef UART_HWCONTROL_NONE
#define UART_HWCONTROL_NONE     0x00000000U
#endif
#ifndef UART_HWCONTROL_RTS
#define UART_HWCONTROL_RTS      ((uint32_t)USART_CR3_RTSE)
#endif
#ifndef UART_HWCONTROL_CTS
#define UART_HWCONTROL_CTS      ((uint32_t)USART_CR3_CTSE)
#endif
#ifndef UART_HWCONTROL_RTS_CTS
#define UART_HWCONTROL_RTS_CTS  ((uint32_t)(USART_CR3_RTSE | USART_CR3_CTSE))
#endif

typedef enum {
    UART_HWCONTROL_NONE_VAL    = 0,
    UART_HWCONTROL_RTS_VAL     = 1,
    UART_HWCONTROL_CTS_VAL     = 2,
    UART_HWCONTROL_RTS_CTS_VAL = 3
} uart_hwcontrol_t;

/* 模式定义 - 条件定义避免冲突 */
#ifndef UART_MODE_RX
#define UART_MODE_RX            ((uint32_t)USART_CR1_RE)
#endif
#ifndef UART_MODE_TX
#define UART_MODE_TX            ((uint32_t)USART_CR1_TE)
#endif
#ifndef UART_MODE_TX_RX
#define UART_MODE_TX_RX         ((uint32_t)(USART_CR1_TE | USART_CR1_RE))
#endif

typedef enum {
    UART_MODE_RX_VAL    = 0x01,
    UART_MODE_TX_VAL    = 0x02,
    UART_MODE_TX_RX_VAL = 0x03
} uart_mode_t;

/* ============================================================================
 * UART配置结构体
 * ============================================================================ */

typedef struct {
    uint32_t baudrate;
    uart_databits_t databits;
    uart_stopbits_t stopbits;
    uart_parity_t parity;
    uart_hwcontrol_t hwcontrol;
    uart_mode_t mode;
} uart_config_t;

/* ============================================================================
 * UART状态定义
 * ============================================================================ */

typedef enum {
    UART_STATE_RESET = 0x00U,
    UART_STATE_READY = 0x01U,
    UART_STATE_BUSY = 0x02U,
    UART_STATE_BUSY_TX = 0x03U,
    UART_STATE_BUSY_RX = 0x04U,
    UART_STATE_BUSY_TX_RX = 0x05U,
    UART_STATE_ERROR = 0x06U
} uart_state_t;

/* ============================================================================
 * UART句柄结构体
 * ============================================================================ */

typedef struct {
    uart_instance_t instance;
    uart_config_t config;
    uart_state_t state;
    uint8_t *tx_buffer;
    uint16_t tx_size;
    uint16_t tx_count;
    uint8_t *rx_buffer;
    uint16_t rx_size;
    uint16_t rx_count;
    uint32_t error_code;
} uart_handle_t;

/* ============================================================================
 * 错误码定义
 * ============================================================================ */

#define UART_ERROR_NONE         0x00000000U
#define UART_ERROR_PE           0x00000001U
#define UART_ERROR_NE           0x00000002U
#define UART_ERROR_FE           0x00000004U
#define UART_ERROR_ORE          0x00000008U
#define UART_ERROR_DMA          0x00000010U
#define UART_ERROR_BUSY         0x00000020U
#define UART_ERROR_TIMEOUT      0x00000040U

/* ============================================================================
 * API函数声明
 * ============================================================================ */

hal_status_t uart_init(uart_handle_t *huart, uart_instance_t instance, const uart_config_t *config);
hal_status_t uart_deinit(uart_handle_t *huart);
hal_status_t uart_send(uart_handle_t *huart, const uint8_t *data, uint16_t size, uint32_t timeout);
hal_status_t uart_receive(uart_handle_t *huart, uint8_t *data, uint16_t size, uint32_t timeout);
hal_status_t uart_set_baudrate(uart_handle_t *huart, uint32_t baudrate);
uint32_t uart_get_baudrate(const uart_handle_t *huart);
bool uart_is_busy(const uart_handle_t *huart);
void uart_clear_error(uart_handle_t *huart);
uint32_t uart_get_error(const uart_handle_t *huart);

#ifdef __cplusplus
}
#endif

#endif /* UART_H */
