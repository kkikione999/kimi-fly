/**
 * @file uart.h
 * @brief STM32 UART HAL接口定义
 * @note 用于USART2与ESP32-C3 WiFi模块通信
 *       默认波特率: 115200
 *       高速波特率: 921600
 *       GPIO: PA2 (TX), PA3 (RX)
 *       时钟源: APB1 = 42MHz
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
    UART_INSTANCE_1 = 0,    /*!< USART1 - 未使用 */
    UART_INSTANCE_2 = 1,    /*!< USART2 - ESP32-C3通信 (PA2/PA3) */
    UART_INSTANCE_6 = 2,    /*!< USART6 - 未使用 */
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

/* 默认波特率 */
#define UART_DEFAULT_BAUDRATE   UART_BAUDRATE_115200

/* 数据位定义 */
typedef enum {
    UART_DATABITS_8 = 0,    /*!< 8位数据 */
    UART_DATABITS_9 = 1     /*!< 9位数据 */
} uart_databits_t;

/* 停止位定义 */
typedef enum {
    HAL_UART_STOPBITS_1   = 0,  /*!< 1个停止位 */
    HAL_UART_STOPBITS_0_5 = 1,  /*!< 0.5个停止位 */
    HAL_UART_STOPBITS_2   = 2,  /*!< 2个停止位 */
    HAL_UART_STOPBITS_1_5 = 3   /*!< 1.5个停止位 */
} uart_stopbits_t;

/* 校验位定义 */
typedef enum {
    HAL_UART_PARITY_NONE = 0,   /*!< 无校验 */
    HAL_UART_PARITY_EVEN = 1,   /*!< 偶校验 */
    HAL_UART_PARITY_ODD  = 2    /*!< 奇校验 */
} uart_parity_t;

/* 硬件流控定义 */
typedef enum {
    HAL_UART_HWCONTROL_NONE    = 0, /*!< 无硬件流控 */
    HAL_UART_HWCONTROL_RTS     = 1, /*!< RTS使能 */
    HAL_UART_HWCONTROL_CTS     = 2, /*!< CTS使能 */
    HAL_UART_HWCONTROL_RTS_CTS = 3  /*!< RTS和CTS都使能 */
} uart_hwcontrol_t;

/* 模式定义 */
typedef enum {
    HAL_UART_MODE_RX    = 0x01,     /*!< 接收模式 */
    HAL_UART_MODE_TX    = 0x02,     /*!< 发送模式 */
    HAL_UART_MODE_TX_RX = 0x03      /*!< 收发模式 */
} uart_mode_t;

/* ============================================================================
 * UART配置结构体
 * ============================================================================ */

typedef struct {
    uint32_t baudrate;          /*!< 波特率 */
    uart_databits_t databits;   /*!< 数据位 */
    uart_stopbits_t stopbits;   /*!< 停止位 */
    uart_parity_t parity;       /*!< 校验位 */
    uart_hwcontrol_t hwcontrol; /*!< 硬件流控 */
    uart_mode_t mode;           /*!< 工作模式 */
} uart_config_t;

/* ============================================================================
 * UART句柄结构体
 * ============================================================================ */

typedef struct {
    uart_instance_t instance;   /*!< UART实例 */
    uart_config_t config;       /*!< 当前配置 */
    hal_device_state_t state;    /*!< UART状态 */
    uint8_t *tx_buffer;         /*!< 发送缓冲区 */
    uint16_t tx_size;           /*!< 发送数据大小 */
    uint16_t tx_count;          /*!< 已发送字节数 */
    uint8_t *rx_buffer;         /*!< 接收缓冲区 */
    uint16_t rx_size;           /*!< 接收数据大小 */
    uint16_t rx_count;          /*!< 已接收字节数 */
    uint32_t error_code;        /*!< 错误码 */
} uart_handle_t;

/* ============================================================================
 * 错误码定义
 * ============================================================================ */

#define UART_ERROR_NONE         0x00000000U /*!< 无错误 */
#define UART_ERROR_PE           0x00000001U /*!< 校验错误 */
#define UART_ERROR_NE           0x00000002U /*!< 噪声错误 */
#define UART_ERROR_FE           0x00000004U /*!< 帧错误 */
#define UART_ERROR_ORE          0x00000008U /*!< 溢出错误 */
#define UART_ERROR_DMA          0x00000010U /*!< DMA传输错误 */
#define UART_ERROR_BUSY         0x00000020U /*!< 忙错误 */
#define UART_ERROR_TIMEOUT      0x00000040U /*!< 超时错误 */

/* ============================================================================
 * API函数声明
 * ============================================================================ */

/**
 * @brief 初始化UART外设
 * @param huart UART句柄指针
 * @param instance UART实例
 * @param config UART配置指针
 * @return HAL状态
 */
hal_status_t uart_init(uart_handle_t *huart, uart_instance_t instance, const uart_config_t *config);

/**
 * @brief 反初始化UART外设
 * @param huart UART句柄指针
 * @return HAL状态
 */
hal_status_t uart_deinit(uart_handle_t *huart);

/**
 * @brief 发送数据 (阻塞模式)
 * @param huart UART句柄指针
 * @param data 数据缓冲区指针
 * @param size 数据大小
 * @param timeout 超时时间(毫秒)
 * @return HAL状态
 */
hal_status_t uart_send(uart_handle_t *huart, const uint8_t *data, uint16_t size, uint32_t timeout);

/**
 * @brief 接收数据 (阻塞模式)
 * @param huart UART句柄指针
 * @param data 数据缓冲区指针
 * @param size 数据大小
 * @param timeout 超时时间(毫秒)
 * @return HAL状态
 */
hal_status_t uart_receive(uart_handle_t *huart, uint8_t *data, uint16_t size, uint32_t timeout);

/**
 * @brief 动态修改波特率
 * @param huart UART句柄指针
 * @param baudrate 新波特率
 * @return HAL状态
 */
hal_status_t uart_set_baudrate(uart_handle_t *huart, uint32_t baudrate);

/**
 * @brief 获取当前波特率
 * @param huart UART句柄指针
 * @return 当前波特率
 */
uint32_t uart_get_baudrate(const uart_handle_t *huart);

/**
 * @brief 检查UART是否忙
 * @param huart UART句柄指针
 * @return true=忙, false=空闲
 */
bool uart_is_busy(const uart_handle_t *huart);

/**
 * @brief 清除错误标志
 * @param huart UART句柄指针
 */
void uart_clear_error(uart_handle_t *huart);

/**
 * @brief 获取错误码
 * @param huart UART句柄指针
 * @return 错误码
 */
uint32_t uart_get_error(const uart_handle_t *huart);

/**
 * @brief 打印UART配置信息 (通过debug printf)
 * @param huart UART句柄指针
 * @note 输出波特率、数据位、校验、停止位等配置
 */
void uart_print_config(uart_handle_t *huart);

#ifdef __cplusplus
}
#endif

#endif /* UART_H */
