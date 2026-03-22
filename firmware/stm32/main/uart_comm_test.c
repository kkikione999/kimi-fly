/**
 * @file uart_comm_test.c
 * @brief UART Communication Test - STM32 to ESP32
 * @note Tests bidirectional UART communication with ESP32-C3
 *       Hardware: USART2 on PA2(TX)/PA3(RX)
 */

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#include "stm32f4xx_hal.h"
#include "uart.h"
#include "gpio.h"
#include "board_config.h"
#include "comm_protocol.h"

/* ============================================================================
 * Configuration
 * ============================================================================ */

#define COMM_UART_BAUDRATE      115200

/* ============================================================================
 * Global Variables
 * ============================================================================ */

static UART_HandleTypeDef huart1;  /* Debug UART */
static uart_handle_t wifi_uart;
static comm_stats_t comm_stats = {0};

static uint8_t rx_buffer[256];
static uint8_t rx_index = 0;
static uint32_t last_heartbeat = 0;
static uint32_t loop_counter = 0;

/* ============================================================================
 * Debug UART Functions (USART1 - PA9/PA10)
 * ============================================================================ */

/**
 * @brief Initialize debug UART (USART1)
 */
static void debug_uart_init(void)
{
    /* Enable clocks */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_USART1_CLK_ENABLE();

    /* Configure PA9 (TX) and PA10 (RX) as alternate function */
    GPIO_InitTypeDef gpio_init = {
        .Pin = GPIO_PIN_9 | GPIO_PIN_10,
        .Mode = GPIO_MODE_AF_PP,
        .Pull = GPIO_PULLUP,
        .Speed = GPIO_SPEED_FREQ_HIGH,
        .Alternate = GPIO_AF7_USART1
    };
    HAL_GPIO_Init(GPIOA, &gpio_init);

    /* Configure USART1 */
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

/**
 * @brief Send single character to debug UART
 */
static void debug_putc(char c)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)&c, 1, 10);
}

/**
 * @brief Send string to debug UART
 */
static void debug_puts(const char *str)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)str, strlen(str), 100);
}

/**
 * @brief printf-style output to debug UART
 */
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
 * ESP32 Communication Functions
 * ============================================================================ */

/**
 * @brief Initialize WiFi UART (USART2 for ESP32 communication)
 */
static hal_status_t wifi_uart_init(void)
{
    uart_config_t config = {
        .baudrate = COMM_UART_BAUDRATE,
        .databits = UART_DATABITS_8,
        .stopbits = HAL_UART_STOPBITS_1,
        .parity = HAL_UART_PARITY_NONE,
        .hwcontrol = HAL_UART_HWCONTROL_NONE,
        .mode = HAL_UART_MODE_TX_RX
    };

    hal_status_t status = uart_init(&wifi_uart, UART_INSTANCE_2, &config);

    if (status == HAL_OK) {
        debug_puts("[UART] WiFi UART initialized (USART2, 115200)\r\n");
    } else {
        debug_puts("[ERROR] WiFi UART init failed!\r\n");
    }

    return status;
}

/**
 * @brief Send message to ESP32
 */
static bool esp32_send_message(const protocol_message_t *msg, uint8_t len)
{
    hal_status_t status = uart_send(&wifi_uart, (uint8_t *)msg, len, 100);

    if (status == HAL_OK) {
        comm_stats.tx_count++;
        return true;
    } else {
        comm_stats.error_count++;
        return false;
    }
}

/**
 * @brief Send heartbeat message
 */
static void esp32_send_heartbeat(void)
{
    protocol_message_t msg;
    uint8_t payload[4] = {0x01, 0x02, 0x03, 0x04};
    static uint32_t send_errors = 0;

    int len = comm_build_message(&msg, MSG_TYPE_HEARTBEAT, payload, sizeof(payload));
    if (len > 0) {
        hal_status_t status = uart_send(&wifi_uart, (uint8_t *)&msg, len, 100);
        if (status == HAL_OK) {
            comm_stats.tx_count++;
            if (comm_stats.tx_count % 10 == 0) {
                debug_printf("[TX] Heartbeat OK (total:%lu err:%lu)\r\n",
                           comm_stats.tx_count, send_errors);
            }
        } else {
            send_errors++;
            comm_stats.error_count++;
            debug_printf("[TX] Heartbeat FAILED (err:%lu code:%d)\r\n",
                       send_errors, status);
        }
    } else {
        debug_puts("[TX] Message build failed!\r\n");
    }
}

/**
 * @brief Send status message
 */
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

/**
 * @brief Send debug message
 */
static void esp32_send_debug(const char *debug_msg)
{
    protocol_message_t msg;
    uint8_t payload[PROTOCOL_MAX_PAYLOAD];

    strncpy((char *)payload, debug_msg, PROTOCOL_MAX_PAYLOAD - 1);
    payload[PROTOCOL_MAX_PAYLOAD - 1] = '\0';

    int len = comm_build_message(&msg, MSG_TYPE_DEBUG, payload, strlen((char *)payload));
    if (len > 0) {
        esp32_send_message(&msg, len);
    }
}

/**
 * @brief Send ACK message
 */
static void esp32_send_ack(uint8_t msg_type)
{
    protocol_message_t msg;
    uint8_t payload = msg_type;

    int len = comm_build_message(&msg, MSG_TYPE_ACK, &payload, 1);
    if (len > 0) {
        esp32_send_message(&msg, len);
    }
}

/**
 * @brief Process received message from ESP32
 */
static void process_message(const protocol_message_t *msg)
{
    comm_stats.rx_count++;
    comm_stats.last_rx_time = HAL_GetTick();
    comm_stats.connected = true;

    switch (msg->header.cmd) {
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

/**
 * @brief Poll for incoming messages
 */
static void poll_receive(void)
{
    uint8_t temp_buf[32];
    hal_status_t status;

    /* Try to receive a byte with short timeout */
    status = uart_receive(&wifi_uart, temp_buf, 1, 10);

    if (status == HAL_OK) {
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
 * Interrupt Handlers
 * ============================================================================ */

/**
 * @brief Window Watchdog interrupt handler
 */
void WWDG_IRQHandler(void)
{
    /* Clear WWDG interrupt flag */
    WWDG->SR = 0;
    /* System reset on watchdog timeout */
    NVIC_SystemReset();
}

/**
 * @brief PVD interrupt handler (placeholder)
 */
void PVD_IRQHandler(void)
{
    /* Clear pending bit */
    EXTI->PR = EXTI_PR_PR16;
}

/**
 * @brief Default fault handler
 */
void HardFault_Handler(void)
{
    debug_puts("[FATAL] HardFault!\r\n");
    while (1) {
        __NOP();
    }
}

/* ============================================================================
 * System Functions
 * ============================================================================ */

/**
 * @brief System clock configuration (100MHz from HSE)
 * @note  Uses internal HSI as fallback if HSE fails
 */
static void system_clock_config(void)
{
    uint32_t timeout;

    /* Try HSE first */
    __HAL_RCC_HSE_CONFIG(RCC_HSE_ON);

    /* Wait for HSE with timeout */
    timeout = 100000;
    while (__HAL_RCC_GET_FLAG(RCC_FLAG_HSERDY) == RESET) {
        if (--timeout == 0) {
            /* HSE failed, use HSI fallback */
            __HAL_RCC_HSE_CONFIG(RCC_HSE_OFF);
            /* HSI is 16MHz, configure for 64MHz sysclk */
            __HAL_RCC_PLL_CONFIG(RCC_PLLSOURCE_HSI, 16, 256, 2, 4);
            __HAL_RCC_PLL_ENABLE();
            timeout = 100000;
            while (__HAL_RCC_GET_FLAG(RCC_FLAG_PLLRDY) == RESET) {
                if (--timeout == 0) {
                    /* PLL failed too, stay on HSI */
                    return;
                }
            }
            /* Configure Flash latency for 64MHz */
            __HAL_FLASH_SET_LATENCY(FLASH_LATENCY_2);
            /* Switch to PLL */
            __HAL_RCC_SYSCLK_CONFIG(RCC_SYSCLKSOURCE_PLLCLK);
            timeout = 100000;
            while (__HAL_RCC_GET_SYSCLK_SOURCE() != RCC_SYSCLKSOURCE_STATUS_PLLCLK) {
                if (--timeout == 0) break;
            }
            /* Configure bus clocks */
            RCC->CFGR &= ~RCC_CFGR_HPRE_Msk;
            RCC->CFGR |= RCC_CFGR_HPRE_DIV1;      /* HCLK = 64MHz */
            RCC->CFGR &= ~RCC_CFGR_PPRE1_Msk;
            RCC->CFGR |= RCC_CFGR_PPRE1_DIV2;     /* APB1 = 32MHz */
            RCC->CFGR &= ~RCC_CFGR_PPRE2_Msk;
            RCC->CFGR |= RCC_CFGR_PPRE2_DIV1;     /* APB2 = 64MHz */
            return;
        }
    }

    /* HSE is ready, configure PLL for 84MHz (APB1 max = 42MHz) */
    /* 8MHz / 8 * 84 / 2 = 84MHz */
    __HAL_RCC_PLL_CONFIG(RCC_PLLSOURCE_HSE, 8, 84, 2, 4);
    __HAL_RCC_PLL_ENABLE();

    /* Wait for PLL with timeout */
    timeout = 100000;
    while (__HAL_RCC_GET_FLAG(RCC_FLAG_PLLRDY) == RESET) {
        if (--timeout == 0) {
            /* PLL failed */
            return;
        }
    }

    /* Configure Flash latency for 84MHz */
    __HAL_FLASH_SET_LATENCY(FLASH_LATENCY_2);

    /* Switch to PLL */
    __HAL_RCC_SYSCLK_CONFIG(RCC_SYSCLKSOURCE_PLLCLK);
    timeout = 100000;
    while (__HAL_RCC_GET_SYSCLK_SOURCE() != RCC_SYSCLKSOURCE_STATUS_PLLCLK) {
        if (--timeout == 0) break;
    }

    /* Configure bus clocks - APB1 max 42MHz for STM32F411 */
    RCC->CFGR &= ~RCC_CFGR_HPRE_Msk;
    RCC->CFGR |= RCC_CFGR_HPRE_DIV1;          /* HCLK = 84MHz */
    RCC->CFGR &= ~RCC_CFGR_PPRE1_Msk;
    RCC->CFGR |= RCC_CFGR_PPRE1_DIV2;         /* APB1 = 42MHz (max for F411) */
    RCC->CFGR &= ~RCC_CFGR_PPRE2_Msk;
    RCC->CFGR |= RCC_CFGR_PPRE2_DIV1;         /* APB2 = 84MHz */
}

/**
 * @brief Delay in milliseconds
 */
static void delay_ms(uint32_t ms)
{
    HAL_Delay(ms);
}

/* ============================================================================
 * Main
 * ============================================================================ */

int main(void)
{
    /* Initialize HAL */
    HAL_Init();

    /* Configure system clock */
    system_clock_config();

    /* Initialize debug UART */
    debug_uart_init();

    debug_puts("\r\n");
    debug_puts("================================\r\n");
    debug_puts("STM32F411 UART Test\r\n");
    debug_puts("Target: ESP32-C3 Communication\r\n");
    debug_puts("UART: USART2 (PA2/PA3) @ 115200\r\n");
    debug_puts("Debug: USART1 (PA9/PA10) @ 921600\r\n");
    debug_puts("================================\r\n\r\n");

    /* Initialize WiFi UART */
    if (wifi_uart_init() != HAL_OK) {
        debug_puts("[FATAL] Failed to initialize WiFi UART!\r\n");
        while (1) {
            delay_ms(500);
        }
    }

    /* Send initial status */
    esp32_send_status("STM32 initialized and ready");
    debug_puts("[INIT] Ready for communication\r\n\r\n");

    /* Main loop */
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
            comm_stats.connected = false;
            debug_puts("[WARN] Connection lost!\r\n");
        }

        delay_ms(10);
    }

    return 0;
}
