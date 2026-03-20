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

/* Directed GPIO test mode:
 *   phase 1: PA2 samples ESP32 GPIO1
 *   phase 2: PA3 samples ESP32 GPIO0
 */
#define HARDWARE_LOOPBACK_TEST  1

/* Cross-wire test: validate both physical wires in the PDF-verified map
 * PA2 <-> GPIO1 and PA3 <-> GPIO0.
 */
#define CROSS_WIRE_TEST         0

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
static UART_HandleTypeDef huart2;
static comm_stats_t comm_stats = {0};

static uint8_t rx_buffer[256];
static uint16_t rx_index = 0;
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

static int comm_build_wire(uint8_t *wire_buf, msg_type_t type,
                            const uint8_t *payload, uint8_t payload_len)
{
    if (payload_len > PROTOCOL_MAX_PAYLOAD || wire_buf == NULL) {
        return -1;
    }

    /* Header: 0xAA55 little-endian */
    wire_buf[0] = 0x55U;
    wire_buf[1] = 0xAAU;
    wire_buf[2] = (uint8_t)type;
    wire_buf[3] = payload_len;

    if (payload_len > 0 && payload != NULL) {
        memcpy(&wire_buf[4], payload, payload_len);
    }

    /* Checksum: XOR of all bytes so far */
    uint8_t cs = 0;
    for (uint8_t i = 0; i < 4 + payload_len; i++) {
        cs ^= wire_buf[i];
    }
    wire_buf[4 + payload_len] = cs;

    return 4 + payload_len + 1;
}

static int comm_parse_message(const uint8_t *buffer, uint16_t len,
                               protocol_message_t *msg)
{
    if (len < 5U) {  /* minimum: 4 header + 1 checksum */
        return 0;
    }

    for (uint16_t i = 0; i <= len - 4U; i++) {
        uint16_t header = (uint16_t)(buffer[i]) | ((uint16_t)(buffer[i+1]) << 8);

        if (header == PROTOCOL_HEADER) {
            uint8_t payload_len = buffer[i + 3];
            uint8_t total_len = (uint8_t)(4U + payload_len + 1U);

            if ((uint16_t)(i + total_len) > len) {
                return 0;
            }

            /* Verify checksum from raw buffer */
            uint8_t cs = 0;
            for (uint8_t k = 0; k < 4U + payload_len; k++) {
                cs ^= buffer[i + k];
            }
            uint8_t wire_cs = buffer[i + 4U + payload_len];

            if (cs == wire_cs) {
                /* Populate struct from wire buffer */
                msg->header.header = header;
                msg->header.type   = buffer[i + 2];
                msg->header.length = payload_len;
                if (payload_len > 0 && payload_len <= PROTOCOL_MAX_PAYLOAD) {
                    memcpy(msg->payload, &buffer[i + 4], payload_len);
                }
                msg->checksum = wire_cs;
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
 * GPIO/UART Diagnostic Functions
 * ============================================================================ */

/**
 * @brief 读取GPIO MODER寄存器中指定引脚的模式 (2位)
 */
static uint8_t gpio_get_mode(GPIO_TypeDef *port, uint8_t pin)
{
    return (uint8_t)((port->MODER >> (pin * 2)) & 0x3U);
}

/**
 * @brief 读取GPIO AFR寄存器中指定引脚的AF编号
 */
static uint8_t gpio_get_af(GPIO_TypeDef *port, uint8_t pin)
{
    if (pin < 8) {
        return (uint8_t)((port->AFR[0] >> (pin * 4)) & 0xFU);
    } else {
        return (uint8_t)((port->AFR[1] >> ((pin - 8) * 4)) & 0xFU);
    }
}

/**
 * @brief 诊断USART2 GPIO和寄存器配置
 * @note  验证 PA2(TX)/PA3(RX) 是否正确配置为AF7
 */
static void uart_gpio_diag(void)
{
    uint8_t pa2_mode = gpio_get_mode(GPIOA, 2);
    uint8_t pa3_mode = gpio_get_mode(GPIOA, 3);
    uint8_t pa2_af   = gpio_get_af(GPIOA, 2);
    uint8_t pa3_af   = gpio_get_af(GPIOA, 3);

    /* MODER: 0=Input 1=Output 2=AF 3=Analog */
    debug_printf("[DIAG] PA2: mode=%u(%s) AF=%u %s\r\n",
                 pa2_mode,
                 pa2_mode == 2 ? "AF" : (pa2_mode == 1 ? "OUT" : pa2_mode == 0 ? "IN" : "ANA"),
                 pa2_af,
                 (pa2_mode == 2 && pa2_af == 7) ? "[OK]" : "[FAIL]");

    debug_printf("[DIAG] PA3: mode=%u(%s) AF=%u %s\r\n",
                 pa3_mode,
                 pa3_mode == 2 ? "AF" : (pa3_mode == 1 ? "OUT" : pa3_mode == 0 ? "IN" : "ANA"),
                 pa3_af,
                 (pa3_mode == 2 && pa3_af == 7) ? "[OK]" : "[FAIL]");

    /* USART2 寄存器状态 */
    debug_printf("[DIAG] USART2 CR1=0x%08lX CR2=0x%08lX BRR=0x%08lX\r\n",
                 USART2->CR1, USART2->CR2, USART2->BRR);

    /* 计算实际波特率: APB1=42MHz, BRR=42000000/115200=364 */
    uint32_t brr = USART2->BRR;
    uint32_t actual_baud = (brr > 0) ? (42000000UL / brr) : 0;
    debug_printf("[DIAG] USART2 BRR=%lu actual_baud~%lu (expect 115200) %s\r\n",
                 brr, actual_baud,
                 (actual_baud > 110000 && actual_baud < 120000) ? "[OK]" : "[FAIL]");

    /* RCC clock enable check */
    uint32_t apb1enr = RCC->APB1ENR;
    debug_printf("[DIAG] RCC APB1ENR USART2EN=%lu %s\r\n",
                 (apb1enr >> 17) & 1UL,
                 ((apb1enr >> 17) & 1UL) ? "[OK]" : "[FAIL]");
}

/**
 * @brief USART2回环测试 (需要PA2-PA3短接 或 ESP32回传)
 */
static void uart_loopback_test(void)
{
    uint8_t tx_data[4] = {0xAA, 0x55, 0xA5, 0x5A};
    uint8_t rx_data[4] = {0};
    uint8_t pass = 0;

    debug_puts("[LOOP] Starting USART2 loopback test...\r\n");
    debug_puts("[LOOP] (Requires PA2-PA3 shorted or ESP32 echo mode)\r\n");

    /* 发送测试字节 */
    HAL_StatusTypeDef tx_status = HAL_UART_Transmit(&huart2, tx_data, 4, 100);
    debug_printf("[LOOP] TX: %s (0xAA 0x55 0xA5 0x5A)\r\n",
                 tx_status == HAL_OK ? "OK" : "FAIL");

    if (tx_status == HAL_OK) {
        /* 尝试接收回响 (500ms timeout) */
        HAL_StatusTypeDef rx_status = HAL_UART_Receive(&huart2, rx_data, 4, 500);
        if (rx_status == HAL_OK) {
            pass = (rx_data[0] == 0xAA && rx_data[1] == 0x55 &&
                    rx_data[2] == 0xA5 && rx_data[3] == 0x5A);
            debug_printf("[LOOP] RX: %02X %02X %02X %02X -> %s\r\n",
                         rx_data[0], rx_data[1], rx_data[2], rx_data[3],
                         pass ? "[PASS]" : "[MISMATCH]");
        } else {
            debug_printf("[LOOP] RX timeout (status=%d) - no loopback or ESP32 not echoing\r\n",
                         rx_status);
        }
    }

    if (!pass) {
        debug_puts("[LOOP] NOTE: If RX timeout, physical wiring issue or ESP32 not ready\r\n");
    }
}

/* ============================================================================
 * ESP32 UART Functions (USART2 - PA2/PA3, via STM32 HAL)
 * ============================================================================ */

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
    /* Clear overrun error before receive - ORE locks RDR and causes data loss */
    if (__HAL_UART_GET_FLAG(&huart2, UART_FLAG_ORE)) {
        __HAL_UART_CLEAR_OREFLAG(&huart2);
        comm_stats.error_count++;
    }
    HAL_StatusTypeDef status = HAL_UART_Receive(&huart2, byte, 1, timeout);
    return (status == HAL_OK) ? 0 : -1;
}

/* ============================================================================
 * Hardware Connectivity Test (GPIO Toggle Detection)
 * ============================================================================
 * This test verifies physical connection between ESP32 GPIO0 and STM32 PA3.
 * ESP32 toggles GPIO0 rapidly; STM32 samples PA3 as GPIO input.
 * If transitions are detected, the wire is connected.
 *
 * Fix history:
 *   v1: HAL_UART_DeInit only - PA2/PA3 still owned by USART2 AF mux
 *   v2: Added RCC force-reset - but uart2_init() never called so USART2 clock
 *       was never enabled, RCC reset was harmless but MODER was already INPUT
 *   v3 (current): Explicitly disable USART2 clock, write MODER directly via
 *       register to guarantee INPUT mode, verify with readback before sampling.
 *       Extended window to 20s, progress every 200ms, debounced edge detection.
 */

/**
 * @brief Release USART2 and configure PA2/PA3 as plain GPIO inputs.
 *
 * Sequence:
 *   1. Disable USART2 (UE bit in CR1) to release the AF mux
 *   2. Disable USART2 APB1 clock via RCC
 *   3. Write MODER bits directly (bypass HAL to guarantee order)
 *   4. Enable pull-ups via PUPDR
 *   5. Readback MODER and print for diagnosis
 */
static void release_usart2_force_gpio_input(void)
{
    /* Step 1: Disable USART2 UE bit (releases the AF mux hold on PA2/PA3) */
    USART2->CR1 &= ~USART_CR1_UE;
    __DSB();

    /* Step 2: Disable USART2 APB1 clock */
    __HAL_RCC_USART2_CLK_DISABLE();
    __DSB();
    __ISB();
    HAL_Delay(2);

    /* Step 3: Write MODER bits for PA2 and PA3 directly to INPUT (00b each)
     * MODER bits: PA2 = bits [5:4], PA3 = bits [7:6]
     * Clear both pin pairs to 00 = Input mode */
    GPIOA->MODER &= ~(GPIO_MODER_MODER2 | GPIO_MODER_MODER3);
    __DSB();

    /* Step 4: Enable pull-ups for PA2 and PA3
     * PUPDR: PA2 = bits [5:4] = 01 (pull-up), PA3 = bits [7:6] = 01 (pull-up) */
    GPIOA->PUPDR &= ~(GPIO_PUPDR_PUPDR2 | GPIO_PUPDR_PUPDR3);
    GPIOA->PUPDR |=  (GPIO_PUPDR_PUPDR2_0 | GPIO_PUPDR_PUPDR3_0);
    __DSB();

    HAL_Delay(5); /* allow pull-ups to settle */

    /* Step 5: Readback and verify */
    uint32_t moder = GPIOA->MODER;
    uint32_t pa2_mode = (moder >> 4) & 0x3U;  /* bits [5:4] */
    uint32_t pa3_mode = (moder >> 6) & 0x3U;  /* bits [7:6] */
    uint32_t pa2_pupd = (GPIOA->PUPDR >> 4) & 0x3U;
    uint32_t pa3_pupd = (GPIOA->PUPDR >> 6) & 0x3U;

    debug_printf("[GPIO] MODER readback: PA2=0x%lx(%s) PA3=0x%lx(%s)\r\n",
                 pa2_mode, pa2_mode == 0 ? "INPUT-OK" : "FAIL",
                 pa3_mode, pa3_mode == 0 ? "INPUT-OK" : "FAIL");
    debug_printf("[GPIO] PUPDR readback: PA2=%lu(%s) PA3=%lu(%s)\r\n",
                 pa2_pupd, pa2_pupd == 1 ? "PULLUP-OK" : "FAIL",
                 pa3_pupd, pa3_pupd == 1 ? "PULLUP-OK" : "FAIL");
}

#if HARDWARE_LOOPBACK_TEST
static void single_pin_connectivity_test(uint32_t pin_shift,
                                         const char *pin_label,
                                         const char *esp_label)
{
    uint32_t toggle_count = 0;
    uint32_t last_idr, cur_idr;

    const uint32_t TEST_DURATION_MS     = 10000;
    const uint32_t PROGRESS_INTERVAL_MS = 200;
    const uint32_t DEBOUNCE_COUNT       = 3;

    debug_puts("\r\n=== Directed GPIO Connectivity Test ===\r\n");
    debug_printf("Testing ESP32 %s -> STM32 %s connection...\r\n", esp_label, pin_label);
    debug_printf("Monitoring window: %lus, progress every %lums\r\n\r\n",
                 TEST_DURATION_MS / 1000, PROGRESS_INTERVAL_MS);

    release_usart2_force_gpio_input();

    last_idr = (GPIOA->IDR >> pin_shift) & 1U;
    debug_printf("Initial %s IDR=%lu (%s)\r\n", pin_label, last_idr, last_idr ? "HIGH" : "LOW");

    uint32_t start_ms      = HAL_GetTick();
    uint32_t last_print_ms = start_ms;
    uint32_t sample_count  = 0;
    uint32_t stable_count  = 0;
    uint32_t candidate_idr = last_idr;

    while ((HAL_GetTick() - start_ms) < TEST_DURATION_MS) {
        cur_idr = (GPIOA->IDR >> pin_shift) & 1U;

        if (cur_idr != last_idr) {
            if (cur_idr == candidate_idr) {
                stable_count++;
                if (stable_count >= DEBOUNCE_COUNT) {
                    toggle_count++;
                    last_idr = cur_idr;
                    stable_count = 0;
                }
            } else {
                candidate_idr = cur_idr;
                stable_count = 1;
            }
        } else {
            stable_count = 0;
            candidate_idr = last_idr;
        }
        sample_count++;

        for (volatile uint32_t d = 0; d < 84U; d++);

        uint32_t now = HAL_GetTick();
        if ((now - last_print_ms) >= PROGRESS_INTERVAL_MS) {
            uint32_t elapsed = now - start_ms;
            debug_printf("[%s] t=%4lums  IDR=%lu  toggles=%lu  samples=%lu\r\n",
                         pin_label, elapsed, (GPIOA->IDR >> pin_shift) & 1U,
                         toggle_count, sample_count);
            last_print_ms = now;
        }
    }

    debug_printf("\r\n%s final: samples=%lu toggles=%lu\r\n",
                 pin_label, sample_count, toggle_count);

    if (toggle_count > 10) {
        debug_printf("[PASS] %s is receiving toggles from %s.\r\n", pin_label, esp_label);
    } else {
        debug_printf("[FAIL] No signal detected on %s from %s.\r\n", pin_label, esp_label);
    }

    debug_puts("\r\n=== Test Complete ===\r\n");
}

#if CROSS_WIRE_TEST
/**
 * @brief Cross-wire test: PA2 and PA3 as GPIO inputs, detect toggles from ESP32.
 *
 * Fixes vs previous version:
 *   - USART2 UE bit explicitly cleared before clock disable (releases AF mux)
 *   - MODER written directly via registers, not HAL (avoids any HAL ordering issues)
 *   - MODER readback printed for diagnosis
 *   - Debounced edge detection (3-sample confirmation)
 *   - 20s window (up from 15s)
 *   - 200ms progress interval (down from 500ms)
 *   - Direct IDR register read (not HAL_GPIO_ReadPin) for speed
 */
static void cross_wire_test(void)
{
    uint32_t toggle_pa2 = 0;
    uint32_t toggle_pa3 = 0;

    const uint32_t TEST_DURATION_MS     = 20000;
    const uint32_t PROGRESS_INTERVAL_MS = 200;
    const uint32_t DEBOUNCE_COUNT       = 3;

    debug_puts("\r\n=== Cross-Wire Test (PA2 + PA3 as GPIO inputs) ===\r\n");
    debug_puts("Monitoring ESP32 GPIO1->PA2 and ESP32 GPIO0->PA3 simultaneously\r\n");
    debug_printf("Window: %lus, progress every %lums\r\n\r\n",
                 TEST_DURATION_MS / 1000, PROGRESS_INTERVAL_MS);

    /* Release USART2 and configure PA2+PA3 as plain GPIO inputs with pull-ups.
     * This function also prints MODER/PUPDR readback for diagnosis. */
    release_usart2_force_gpio_input();

    /* Read initial IDR states */
    uint32_t last_pa2 = (GPIOA->IDR >> 2) & 1U;
    uint32_t last_pa3 = (GPIOA->IDR >> 3) & 1U;
    debug_printf("Initial  PA2_IDR=%lu(%s)  PA3_IDR=%lu(%s)\r\n",
                 last_pa2, last_pa2 ? "HIGH" : "LOW",
                 last_pa3, last_pa3 ? "HIGH" : "LOW");

    /* Debounce state per pin */
    uint32_t cand_pa2 = last_pa2, cand_pa3 = last_pa3;
    uint32_t stab_pa2 = 0,       stab_pa3 = 0;

    uint32_t start_ms      = HAL_GetTick();
    uint32_t last_print_ms = start_ms;
    uint32_t sample_count  = 0;

    while ((HAL_GetTick() - start_ms) < TEST_DURATION_MS) {
        uint32_t idr = GPIOA->IDR;
        uint32_t cur_pa2 = (idr >> 2) & 1U;
        uint32_t cur_pa3 = (idr >> 3) & 1U;

        /* Debounced edge detection for PA2 */
        if (cur_pa2 != last_pa2) {
            if (cur_pa2 == cand_pa2) {
                stab_pa2++;
                if (stab_pa2 >= DEBOUNCE_COUNT) {
                    toggle_pa2++;
                    last_pa2 = cur_pa2;
                    stab_pa2 = 0;
                }
            } else {
                cand_pa2 = cur_pa2;
                stab_pa2 = 1;
            }
        } else {
            stab_pa2 = 0;
            cand_pa2 = last_pa2;
        }

        /* Debounced edge detection for PA3 */
        if (cur_pa3 != last_pa3) {
            if (cur_pa3 == cand_pa3) {
                stab_pa3++;
                if (stab_pa3 >= DEBOUNCE_COUNT) {
                    toggle_pa3++;
                    last_pa3 = cur_pa3;
                    stab_pa3 = 0;
                }
            } else {
                cand_pa3 = cur_pa3;
                stab_pa3 = 1;
            }
        } else {
            stab_pa3 = 0;
            cand_pa3 = last_pa3;
        }

        sample_count++;

        /* ~10us delay per sample iteration */
        for (volatile uint32_t d = 0; d < 84U; d++);

        uint32_t now = HAL_GetTick();
        if ((now - last_print_ms) >= PROGRESS_INTERVAL_MS) {
            uint32_t elapsed = now - start_ms;
            uint32_t idr_now = GPIOA->IDR;
            debug_printf("[XWT] t=%4lums  PA2=%lu  PA3=%lu  PA2_tog=%lu  PA3_tog=%lu\r\n",
                         elapsed,
                         (idr_now >> 2) & 1U, (idr_now >> 3) & 1U,
                         toggle_pa2, toggle_pa3);
            last_print_ms = now;
        }
    }

    /* Final verdict */
    debug_puts("\r\n--- Cross-Wire Test Results ---\r\n");
    debug_printf("Total samples : %lu\r\n", sample_count);
    debug_printf("PA2 toggles   : %lu -> %s\r\n",
                 toggle_pa2, (toggle_pa2 > 10) ? "[CONNECTED]" : "[NO SIGNAL]");
    debug_printf("PA3 toggles   : %lu -> %s\r\n",
                 toggle_pa3, (toggle_pa3 > 10) ? "[CONNECTED]" : "[NO SIGNAL]");

    if (toggle_pa2 > 10 && toggle_pa3 > 10) {
        debug_puts("[PASS] Both PA2 and PA3 are receiving signals.\r\n");
    } else if (toggle_pa2 > 10) {
        debug_puts("[PARTIAL] PA2 OK, PA3 no signal - check GPIO0/PA3 wiring.\r\n");
    } else if (toggle_pa3 > 10) {
        debug_puts("[PARTIAL] PA3 OK, PA2 no signal - check GPIO1/PA2 wiring.\r\n");
    } else {
        debug_puts("[FAIL] Neither PA2 nor PA3 received signal.\r\n");
        uint32_t idr_final = GPIOA->IDR;
        debug_printf("       Final IDR: PA2=%lu PA3=%lu (expected toggling 0/1)\r\n",
                     (idr_final >> 2) & 1U, (idr_final >> 3) & 1U);
        debug_puts("       Check: ESP32 toggle code running? Wires connected?\r\n");
        debug_puts("       Check: MODER readback above shows INPUT-OK for both pins?\r\n");
    }

    debug_puts("\r\n=== Cross-Wire Test Complete ===\r\n");
}
#endif /* CROSS_WIRE_TEST */
#endif /* HARDWARE_LOOPBACK_TEST */

/* ============================================================================
 * ESP32 Communication Functions
 * ============================================================================ */

static void esp32_send_wire(uint8_t *wire_buf, uint8_t len)
{
    if (uart2_send(wire_buf, len, 100) == 0) {
        comm_stats.tx_count++;
    } else {
        comm_stats.error_count++;
    }
}

static void esp32_send_heartbeat(void)
{
    uint8_t wire[16];
    uint8_t payload[4] = {0x01, 0x02, 0x03, 0x04};
    int len = comm_build_wire(wire, MSG_TYPE_HEARTBEAT, payload, sizeof(payload));
    if (len > 0) {
        esp32_send_wire(wire, (uint8_t)len);
    }
}

static void esp32_send_status(const char *status)
{
    uint8_t wire[72];  /* 4 header + 64 payload max + 1 checksum + margin */
    uint8_t slen = (uint8_t)strlen(status);
    if (slen > PROTOCOL_MAX_PAYLOAD) slen = PROTOCOL_MAX_PAYLOAD;
    int len = comm_build_wire(wire, MSG_TYPE_STATUS, (const uint8_t *)status, slen);
    if (len > 0) {
        esp32_send_wire(wire, (uint8_t)len);
    }
}

static void esp32_send_ack(uint8_t msg_type)
{
    uint8_t wire[8];
    int len = comm_build_wire(wire, MSG_TYPE_ACK, &msg_type, 1);
    if (len > 0) {
        esp32_send_wire(wire, (uint8_t)len);
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
    uint8_t byte;

    /* Direct register-level drain: avoids HAL timeout overhead entirely.
     * Clears ORE (overrun) before each read so the flag never locks the UART. */
    while (1) {
        if (__HAL_UART_GET_FLAG(&huart2, UART_FLAG_ORE)) {
            __HAL_UART_CLEAR_OREFLAG(&huart2);
            comm_stats.error_count++;
        }
        if (!__HAL_UART_GET_FLAG(&huart2, UART_FLAG_RXNE)) {
            break;  /* no data available */
        }
        byte = (uint8_t)(USART2->DR & 0xFFU);

        if (rx_index < sizeof(rx_buffer) - 1U) {
            rx_buffer[rx_index++] = byte;
        }

        protocol_message_t msg;
        int consumed = comm_parse_message(rx_buffer, rx_index, &msg);
        if (consumed > 0) {
            process_message(&msg);
            rx_index -= (uint16_t)consumed;
            if (rx_index > 0U) {
                memmove(rx_buffer, &rx_buffer[consumed], rx_index);
            }
        } else if (rx_index > 200U) {
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
    /* NOTE: uart2_init() is called AFTER all GPIO tests so that the tests
     *       can freely take ownership of PA2/PA3 without USART2 interference. */

    debug_puts("\r\n");
    debug_puts("================================\r\n");
    debug_puts("STM32F411 UART Test\r\n");
    debug_puts("Target: ESP32-C3 Communication\r\n");
    debug_puts("PDF-verified map: PA2<->GPIO1, PA3<->GPIO0\r\n");
    debug_puts("UART: USART2 (PA2/PA3) @ 115200\r\n");
    debug_puts("Debug: USART1 (PA9/PA10) @ 460800\r\n");
    debug_puts("================================\r\n\r\n");

#if HARDWARE_LOOPBACK_TEST
    /* Run connectivity tests BEFORE uart2_init() so USART2 never holds the
     * pins and the RCC force-reset in each test is the sole gatekeeper. */
    debug_puts("[HWTEST] Waiting 8s for ESP32 directed GPIO phases...\r\n");
    HAL_Delay(8000);

    single_pin_connectivity_test(2U, "PA2", "GPIO1");
    single_pin_connectivity_test(3U, "PA3", "GPIO0");
    debug_puts("\r\n");

#if CROSS_WIRE_TEST
    /* Run cross-wire test (PA2 + PA3 as GPIO inputs simultaneously) */
    cross_wire_test();
    debug_puts("\r\n");
#endif
    /* Pause to let user read results */
    HAL_Delay(2000);
#endif /* HARDWARE_LOOPBACK_TEST */

    /* Initialise USART2 now that all GPIO tests have completed */
    uart2_init();

    /* Run GPIO diagnostic and loopback test with UART fully configured */
    debug_puts("--- GPIO Diagnostic ---\r\n");
    uart_gpio_diag();
    debug_puts("\r\n");

    debug_puts("--- Loopback Test ---\r\n");
    uart_loopback_test();
    debug_puts("\r\n");

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
            /* USART2 register diagnostic: SR shows ORE/RXNE status */
            debug_printf("[REG] USART2 SR=0x%04lX BRR=%lu CR1=0x%04lX rxbuf=%u\r\n",
                        USART2->SR, USART2->BRR, USART2->CR1, rx_index);
            /* PA2 and PA3 GPIO pin states for cross-wire test */
            debug_printf("[PIN] PA2=%lu PA3=%lu\r\n",
                        (GPIOA->IDR >> 2) & 1, (GPIOA->IDR >> 3) & 1);
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

        HAL_Delay(1);
    }

    return 0;
}
