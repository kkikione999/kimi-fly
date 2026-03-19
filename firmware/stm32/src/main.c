/**
 * @file main.c
 * @brief UART Communication Test - STM32 to ESP32
 * @note Tests bidirectional UART communication with ESP32-C3
 *       Hardware: USART2 on PA2(TX)/PA3(RX)
 *       Debug: USART1 on PA9(TX)/PA10(RX) @ 460800
 */

#include "stm32f4xx_hal.h"
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

/* ============================================================================
 * Configuration
 * ============================================================================ */

#define DEBUG_UART_BAUDRATE     460800
#define COMM_UART_BAUDRATE      115200

/* ============================================================================
 * Protocol Definitions (与ESP32兼容)
 * ============================================================================ */

#define PROTOCOL_HEADER         0xAA55
#define PROTOCOL_MAX_PAYLOAD    64

typedef enum {
    MSG_TYPE_HEARTBEAT = 0x01,
    MSG_TYPE_STATUS = 0x02,
    MSG_TYPE_CONTROL = 0x03,
    MSG_TYPE_SENSOR = 0x04,
    MSG_TYPE_DEBUG = 0x05,
    MSG_TYPE_ACK = 0x06,
    MSG_TYPE_ERROR = 0x07
} msg_type_t;

typedef struct __attribute__((packed)) {
    uint16_t header;        /* 0xAA55 */
    uint8_t type;           /* Message type */
    uint8_t length;         /* Payload length */
} protocol_header_t;

typedef struct __attribute__((packed)) {
    protocol_header_t header;
    uint8_t payload[PROTOCOL_MAX_PAYLOAD];
    uint8_t checksum;
} protocol_message_t;

typedef struct {
    uint32_t tx_count;
    uint32_t rx_count;
    uint32_t error_count;
    uint32_t last_rx_time;
    uint8_t connected;
} comm_stats_t;

/* Use STM32 HAL register definitions directly - no redefinitions needed */

/* ============================================================================
 * Global Variables
 * ============================================================================ */

static UART_HandleTypeDef huart1;
static comm_stats_t comm_stats = {0};

static uint8_t rx_buffer[256];
static uint8_t rx_index = 0;
static uint32_t last_heartbeat = 0;
static uint32_t loop_counter = 0;

/* ============================================================================
 * Protocol Functions
 * ============================================================================ */

static uint8_t comm_calc_checksum(const protocol_message_t *msg)
{
    uint8_t checksum = 0;
    const uint8_t *data = (const uint8_t *)msg;
    uint8_t length = sizeof(protocol_header_t) + msg->header.length;

    for (uint8_t i = 0; i < length; i++) {
        checksum ^= data[i];
    }
    return checksum;
}

static int comm_build_message(protocol_message_t *msg, msg_type_t type,
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

    return sizeof(protocol_header_t) + payload_len + 1;
}

static int comm_parse_message(const uint8_t *buffer, uint16_t len,
                               protocol_message_t *msg)
{
    if (len < sizeof(protocol_header_t)) {
        return 0;
    }

    for (uint16_t i = 0; i <= len - sizeof(protocol_header_t); i++) {
        uint16_t header = buffer[i] | (buffer[i+1] << 8);

        if (header == PROTOCOL_HEADER) {
            uint8_t payload_len = buffer[i + 3];
            uint8_t total_len = sizeof(protocol_header_t) + payload_len + 1;

            if (i + total_len > len) {
                return 0;
            }

            memcpy(msg, &buffer[i], total_len);

            if (comm_calc_checksum(msg) == msg->checksum) {
                return total_len;
            }
        }
    }

    return 0;
}

/* ============================================================================
 * Debug UART Functions (USART1 - PA9/PA10)
 * ============================================================================ */

static void debug_uart_init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_USART1_CLK_ENABLE();

    GPIO_InitTypeDef gpio_init;
    gpio_init.Pin = GPIO_PIN_9 | GPIO_PIN_10;
    gpio_init.Mode = GPIO_MODE_AF_PP;
    gpio_init.Pull = GPIO_PULLUP;
    gpio_init.Speed = GPIO_SPEED_FREQ_HIGH;
    gpio_init.Alternate = GPIO_AF7_USART1;
    HAL_GPIO_Init(GPIOA, &gpio_init);

    huart1.Instance = USART1;
    huart1.Init.BaudRate = DEBUG_UART_BAUDRATE;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;

    HAL_UART_Init(&huart1);
}

static void debug_puts(const char *str)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)str, strlen(str), 100);
}

static void debug_printf(const char *fmt, ...)
{
    char buffer[128];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    debug_puts(buffer);
}

/* ============================================================================
 * ESP32 UART Functions (USART2 - PA2/PA3, via STM32 HAL)
 * ============================================================================ */

static UART_HandleTypeDef huart2;

static void uart2_init(void)
{
    /* Enable clocks */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_USART2_CLK_ENABLE();

    /* Configure PA2 (TX) and PA3 (RX) as alternate function */
    GPIO_InitTypeDef gpio_init;
    gpio_init.Pin = GPIO_PIN_2 | GPIO_PIN_3;
    gpio_init.Mode = GPIO_MODE_AF_PP;
    gpio_init.Pull = GPIO_PULLUP;
    gpio_init.Speed = GPIO_SPEED_FREQ_HIGH;
    gpio_init.Alternate = GPIO_AF7_USART2;
    HAL_GPIO_Init(GPIOA, &gpio_init);

    /* Configure USART2 */
    huart2.Instance = USART2;
    huart2.Init.BaudRate = COMM_UART_BAUDRATE;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;

    HAL_UART_Init(&huart2);
}

static int uart2_send(const uint8_t *data, uint16_t size, uint32_t timeout)
{
    HAL_StatusTypeDef status = HAL_UART_Transmit(&huart2, (uint8_t *)data, size, timeout);
    return (status == HAL_OK) ? 0 : -1;
}

static int uart2_receive_byte(uint8_t *byte, uint32_t timeout)
{
    HAL_StatusTypeDef status = HAL_UART_Receive(&huart2, byte, 1, timeout);
    return (status == HAL_OK) ? 0 : -1;
}

/* ============================================================================
 * ESP32 Communication Functions
 * ============================================================================ */

static void esp32_send_message(const protocol_message_t *msg, uint8_t len)
{
    if (uart2_send((uint8_t *)msg, len, 100) == 0) {
        comm_stats.tx_count++;
    } else {
        comm_stats.error_count++;
    }
}

static void esp32_send_heartbeat(void)
{
    protocol_message_t msg;
    uint8_t payload[4] = {0x01, 0x02, 0x03, 0x04};

    int len = comm_build_message(&msg, MSG_TYPE_HEARTBEAT, payload, sizeof(payload));
    if (len > 0) {
        esp32_send_message(&msg, len);
    }
}

static void esp32_send_status(const char *status)
{
    protocol_message_t msg;
    uint8_t payload[PROTOCOL_MAX_PAYLOAD];

    strncpy((char *)payload, status, PROTOCOL_MAX_PAYLOAD - 1);
    payload[PROTOCOL_MAX_PAYLOAD - 1] = '\0';

    int len = comm_build_message(&msg, MSG_TYPE_STATUS, payload, strlen((char *)payload));
    if (len > 0) {
        esp32_send_message(&msg, len);
    }
}

static void esp32_send_ack(uint8_t msg_type)
{
    protocol_message_t msg;
    uint8_t payload = msg_type;

    int len = comm_build_message(&msg, MSG_TYPE_ACK, &payload, 1);
    if (len > 0) {
        esp32_send_message(&msg, len);
    }
}

static void process_message(const protocol_message_t *msg)
{
    comm_stats.rx_count++;
    comm_stats.last_rx_time = HAL_GetTick();
    comm_stats.connected = 1;

    switch (msg->header.type) {
        case MSG_TYPE_HEARTBEAT:
            debug_puts("[RX] Heartbeat from ESP32\r\n");
            esp32_send_ack(MSG_TYPE_HEARTBEAT);
            break;

        case MSG_TYPE_STATUS:
            debug_printf("[RX] ESP32 Status: %s\r\n", msg->payload);
            break;

        case MSG_TYPE_CONTROL:
            debug_puts("[RX] Control command received\r\n");
            break;

        case MSG_TYPE_DEBUG:
            debug_printf("[RX] ESP32 Debug: %s\r\n", msg->payload);
            break;

        case MSG_TYPE_ACK:
            debug_printf("[RX] ACK for msg type 0x%02X\r\n", msg->payload[0]);
            break;

        default:
            debug_printf("[RX] Unknown msg type: 0x%02X\r\n", msg->header.type);
            break;
    }
}

static void poll_receive(void)
{
    uint8_t temp_buf[32];

    /* Try to receive a byte with short timeout */
    if (uart2_receive_byte(&temp_buf[0], 10) == 0) {
        /* Add to buffer */
        if (rx_index < sizeof(rx_buffer)) {
            rx_buffer[rx_index++] = temp_buf[0];
        }

        /* Try to parse message */
        protocol_message_t msg;
        int consumed = comm_parse_message(rx_buffer, rx_index, &msg);

        if (consumed > 0) {
            /* Valid message found */
            process_message(&msg);

            /* Shift remaining data */
            rx_index -= consumed;
            if (rx_index > 0) {
                memmove(rx_buffer, &rx_buffer[consumed], rx_index);
            }
        } else if (rx_index > 200) {
            /* Buffer getting full, discard oldest data */
            rx_index = 0;
        }
    }
}

/* ============================================================================
 * System Functions
 * ============================================================================ */

static void system_clock_config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    /* Step 1: Enable PWR clock and set voltage scaling to Scale1 (required for PLL) */
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    /* Step 2: Configure HSI (16MHz internal) + PLL for 84MHz SYSCLK
     * HSI is always available, no external crystal dependency
     * 16MHz HSI / PLLM(16) * PLLN(336) / PLLP(4) = 84MHz SYSCLK
     * STM32F411 max SYSCLK = 100MHz, APB1 max = 50MHz */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
    RCC_OscInitStruct.PLL.PLLM = 16;   /* 16MHz / 16 = 1MHz VCO input */
    RCC_OscInitStruct.PLL.PLLN = 336;  /* 1MHz * 336 = 336MHz VCO output */
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;  /* 336MHz / 4 = 84MHz SYSCLK */
    RCC_OscInitStruct.PLL.PLLQ = 7;

    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        /* PLL failed - continue with HSI at 16MHz */
        return;
    }

    /* Step 3: Configure bus clocks
     * SYSCLK = 84MHz
     * HCLK   = 84MHz (no divider)
     * APB1   = 84MHz / 2 = 42MHz (max for STM32F411)
     * APB2   = 84MHz / 1 = 84MHz
     * FLASH_LATENCY_2 for HCLK 60-84MHz @ 2.7-3.6V */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                  RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);
}

/* ============================================================================
 * Interrupt Handlers (required by HAL)
 * ============================================================================ */

/**
 * @brief SysTick interrupt handler
 * @note  MUST override weak Default_Handler to prevent MCU lockup.
 *        HAL_Init() configures SysTick to 1ms; without this handler
 *        the MCU hangs in Default_Handler on first SysTick tick.
 */
void SysTick_Handler(void)
{
    HAL_IncTick();
}

/* ============================================================================
 * Main
 * ============================================================================ */

int main(void)
{
    HAL_Init();
    system_clock_config();
    debug_uart_init();
    uart2_init();

    debug_puts("\r\n");
    debug_puts("================================\r\n");
    debug_puts("STM32F411 UART Test\r\n");
    debug_puts("Target: ESP32-C3 Communication\r\n");
    debug_puts("UART: USART2 (PA2/PA3) @ 115200\r\n");
    debug_puts("Debug: USART1 (PA9/PA10) @ 460800\r\n");
    debug_puts("================================\r\n\r\n");

    esp32_send_status("STM32 initialized and ready");
    debug_puts("[INIT] Ready for communication\r\n\r\n");

    while (1) {
        loop_counter++;
        uint32_t now = HAL_GetTick();

        /* Poll for incoming messages */
        poll_receive();

        /* Send heartbeat every 2 seconds */
        if (now - last_heartbeat >= 2000) {
            esp32_send_heartbeat();
            last_heartbeat = now;

            /* Print statistics */
            debug_printf("[STAT] TX:%lu RX:%lu ERR:%lu CONN:%s\r\n",
                        comm_stats.tx_count,
                        comm_stats.rx_count,
                        comm_stats.error_count,
                        comm_stats.connected ? "YES" : "NO");
        }

        /* Send periodic status */
        if (loop_counter % 500 == 0) {
            char status[32];
            snprintf(status, sizeof(status), "Loop:%lu", loop_counter);
            esp32_send_status(status);
        }

        /* Check connection timeout (5 seconds) */
        if (comm_stats.connected && (now - comm_stats.last_rx_time > 5000)) {
            comm_stats.connected = 0;
            debug_puts("[WARN] Connection lost!\r\n");
        }

        HAL_Delay(10);
    }

    return 0;
}
