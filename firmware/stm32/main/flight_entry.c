/**
 * @file flight_entry.c
 * @brief STM32 飞控固件主入口 (flight PlatformIO 环境)
 *
 * @note 为 [env:flight] 提供 main() 入口点。
 *       定义全局 HAL 句柄 (hi2c1, hspi3, huart2)。
 *       实现 platform_* 弱引用接口。
 *
 * @author Drone Control System
 * @version 1.0
 */

#include "flight_main.h"
#include "../hal/hal_common.h"
#include "../hal/gpio.h"
#include "../hal/uart.h"
#include "../hal/i2c.h"
#include "../hal/spi.h"
#include "../comm/wifi_command.h"
#include <stdarg.h>
#include <stdio.h>

/* ============================================================================
 * 全局 HAL 句柄 (flight_main.c 通过 extern 引用)
 * ============================================================================ */

uart_handle_t huart1;   /**< USART1 - 调试串口 (PA9/PA10, 460800) */
uart_handle_t huart2;   /**< USART2 - ESP32-C3 WiFi 通信 (PA2/PA3, 115200) */
i2c_handle_t  hi2c1;    /**< I2C1 - ICM42688 IMU (PB6/PB7) */
spi_handle_t  hspi3;    /**< SPI3 - LPS22HBTR 气压计 */

/* ============================================================================
 * SysTick - 1ms 计时
 * STM32F411 SYSCLK = 84MHz → reload = 84000 - 1
 * ============================================================================ */

/* ============================================================================
 * RCC 寄存器 (裸机, STM32F411)
 * 目标: HSI(16MHz) + PLL -> 84MHz SYSCLK, APB1=42MHz, APB2=84MHz
 * ============================================================================ */
#define RCC_BASE_ADDR_FE    0x40023800UL

#define RCC_CR_FE       (*(volatile uint32_t *)(RCC_BASE_ADDR_FE + 0x00UL))
#define RCC_PLLCFGR_FE  (*(volatile uint32_t *)(RCC_BASE_ADDR_FE + 0x04UL))
#define RCC_CFGR_FE     (*(volatile uint32_t *)(RCC_BASE_ADDR_FE + 0x08UL))
#define RCC_AHB1ENR_FE  (*(volatile uint32_t *)(RCC_BASE_ADDR_FE + 0x30UL))
#define RCC_APB1ENR_FE  (*(volatile uint32_t *)(RCC_BASE_ADDR_FE + 0x40UL))
#define RCC_APB2ENR_FE  (*(volatile uint32_t *)(RCC_BASE_ADDR_FE + 0x44UL))

/* RCC_CR bits */
#define RCC_CR_HSION        (1UL << 0)
#define RCC_CR_HSIRDY       (1UL << 1)
#define RCC_CR_PLLON        (1UL << 24)
#define RCC_CR_PLLRDY       (1UL << 25)
#define RCC_AHB1ENR_DMA1EN  (1UL << 21)

/* RCC_CFGR bits */
#define RCC_CFGR_SW_PLL     0x00000002UL  /* SW[1:0] = 10 -> PLL */
#define RCC_CFGR_SWS_PLL    0x00000008UL  /* SWS[1:0] = 10 -> PLL used */
#define RCC_CFGR_HPRE_DIV1  0x00000000UL  /* AHB prescaler = 1 */
#define RCC_CFGR_PPRE1_DIV2 0x00001000UL  /* APB1 prescaler = 2 */
#define RCC_CFGR_PPRE2_DIV1 0x00000000UL  /* APB2 prescaler = 1 */

/* PWR 寄存器 */
#define PWR_BASE_ADDR       0x40007000UL
#define PWR_CR              (*(volatile uint32_t *)(PWR_BASE_ADDR + 0x00UL))
#define PWR_CR_VOS_SCALE1   (0x3UL << 14)  /* VOS = 11: Scale 1 (最高性能) */
#define RCC_APB1ENR_PWREN   (1UL << 28)

/* FLASH 寄存器 */
#define FLASH_BASE_ADDR     0x40023C00UL
#define FLASH_ACR           (*(volatile uint32_t *)(FLASH_BASE_ADDR + 0x00UL))
#define FLASH_ACR_LATENCY_2WS 0x00000002UL  /* 2 wait states for 84MHz */
#define FLASH_ACR_ICEN      (1UL << 9)
#define FLASH_ACR_DCEN      (1UL << 10)
#define FLASH_ACR_PRFTEN    (1UL << 8)

/* ============================================================================
 * PLL 配置: 16MHz HSI / 16 * 336 / 4 = 84MHz
 * PLLCFGR: PLLM=16(bit5:0), PLLN=336(bit14:6), PLLP=4->01(bit17:16),
 *           PLLSRC=HSI->0(bit22), PLLQ=7(bit27:24)
 * ============================================================================ */
#define PLL_M   16U
#define PLL_N   336U
#define PLL_P   1U   /* PLLP bits: 00=2, 01=4, 10=6, 11=8; we want /4 -> 01 */
#define PLL_Q   7U
#define PLL_SRC 0U   /* 0 = HSI */

static void clock_init(void)
{
    /* 1. 使能PWR时钟, 设置电压调节到Scale1 */
    RCC_APB1ENR_FE |= RCC_APB1ENR_PWREN;
    PWR_CR = (PWR_CR & ~(0x3UL << 14)) | PWR_CR_VOS_SCALE1;

    /* 2. 设置FLASH等待周期 (84MHz需要2WS) + 指令/数据缓存 + 预取 */
    FLASH_ACR = FLASH_ACR_LATENCY_2WS | FLASH_ACR_ICEN | FLASH_ACR_DCEN | FLASH_ACR_PRFTEN;

    /* 3. 确认HSI已就绪 */
    RCC_CR_FE |= RCC_CR_HSION;
    while (!(RCC_CR_FE & RCC_CR_HSIRDY)) { /* 等待HSI稳定 */ }

    /* 4. 配置AHB/APB分频: AHB/1, APB1/2, APB2/1 */
    RCC_CFGR_FE = (RCC_CFGR_FE & ~(0x3FFUL << 4)) |
                   RCC_CFGR_HPRE_DIV1 |
                   RCC_CFGR_PPRE1_DIV2 |
                   RCC_CFGR_PPRE2_DIV1;

    /* 5. 配置PLL: PLLM=16, PLLN=336, PLLP=4, PLLQ=7, SRC=HSI */
    RCC_PLLCFGR_FE = (PLL_M        <<  0) |  /* PLLM [5:0]  */
                      (PLL_N        <<  6) |  /* PLLN [14:6] */
                      (PLL_P        << 16) |  /* PLLP [17:16]: 01 = /4 */
                      (PLL_SRC      << 22) |  /* PLLSRC: 0 = HSI */
                      (PLL_Q        << 24);   /* PLLQ [27:24] */

    /* 6. 使能PLL, 等待锁定 */
    RCC_CR_FE |= RCC_CR_PLLON;
    while (!(RCC_CR_FE & RCC_CR_PLLRDY)) { /* 等待PLL锁定 */ }

    /* 7. 切换系统时钟到PLL */
    RCC_CFGR_FE = (RCC_CFGR_FE & ~0x3UL) | RCC_CFGR_SW_PLL;
    while ((RCC_CFGR_FE & 0xCUL) != RCC_CFGR_SWS_PLL) { /* 等待切换完成 */ }
}

/* SysTick 寄存器 (ARM Cortex-M4 标准地址) */
#define SYST_CSR   (*(volatile uint32_t *)0xE000E010UL)
#define SYST_RVR   (*(volatile uint32_t *)0xE000E014UL)
#define SYST_CVR   (*(volatile uint32_t *)0xE000E018UL)

static volatile uint32_t g_tick_ms = 0U;

void SysTick_Handler(void)
{
    g_tick_ms++;
}

uint32_t platform_get_time_ms(void)
{
    return g_tick_ms;
}

uint32_t platform_get_time_us(void)
{
    /* ms * 1000 + elapsed ticks / (84 ticks per us) */
    uint32_t ms = g_tick_ms;
    uint32_t reload = SYST_RVR + 1U;
    uint32_t cur = SYST_CVR;
    uint32_t elapsed = reload - cur;
    return ms * 1000U + elapsed / 84U;
}

void platform_delay_us(uint32_t us)
{
    uint32_t start = platform_get_time_us();
    while ((platform_get_time_us() - start) < us) {
        /* busy-wait */
    }
}

/* ============================================================================
 * Debug output via UART2
 * ============================================================================ */

void platform_debug_print(const char *fmt, ...)
{
    char buf[128];
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    if (len > 0) {
        uart_send(&huart1, (const uint8_t *)buf, (uint16_t)len, 10U);
    }
}

/* ============================================================================
 * Hardware initialisation
 * ============================================================================ */

static void systick_init(void)
{
    SYST_RVR = 84000U - 1U;  /* 1ms at 84MHz */
    SYST_CVR = 0U;
    SYST_CSR = 0x00000007U;  /* CLKSOURCE=1, TICKINT=1, ENABLE=1 */
}

static hal_status_t uart1_init(void)
{
    uart_config_t cfg = {
        .baudrate  = UART_BAUDRATE_460800,  /* 调试串口 PA9/PA10 */
        .databits  = UART_DATABITS_8,
        .stopbits  = HAL_UART_STOPBITS_1,
        .parity    = HAL_UART_PARITY_NONE,
        .hwcontrol = HAL_UART_HWCONTROL_NONE,
        .mode      = HAL_UART_MODE_TX_RX,
    };
    return uart_init(&huart1, UART_INSTANCE_1, &cfg);
}

static hal_status_t uart2_init(void)
{
    uart_config_t cfg = {
        .baudrate  = UART_BAUDRATE_115200,  /* ESP32-C3 通信 PA2/PA3 */
        .databits  = UART_DATABITS_8,
        .stopbits  = HAL_UART_STOPBITS_1,
        .parity    = HAL_UART_PARITY_NONE,
        .hwcontrol = HAL_UART_HWCONTROL_NONE,
        .mode      = HAL_UART_MODE_TX_RX,
    };
    return uart_init(&huart2, UART_INSTANCE_2, &cfg);
}

static hal_status_t i2c1_init(void)
{
    i2c_config_t cfg = {
        .clock_speed = I2C_SPEED_FAST,      /* 400kHz fast mode */
        .addr_mode   = I2C_ADDR_MODE_7BIT,
        .own_address = 0U,
    };
    return i2c_init(&hi2c1, &cfg);
}

static hal_status_t spi3_init(void)
{
    return spi3_init_for_imu(&hspi3);
}

/* ============================================================================
 * UART2 <-> WiFi command bridge
 * ============================================================================ */

static flight_main_handle_t g_flight;  /* file-scope so wifi_platform_send can access */

/**
 * @brief 覆盖弱引用 wifi_platform_send — 通过 USART2 发送给 ESP32
 */
uint16_t wifi_platform_send(const uint8_t *data, uint16_t len)
{
    if (data == NULL || len == 0) {
        return 0;
    }
    hal_status_t st = uart_send(&huart2, data, len, 50U);
    platform_debug_print("[UART2_TX] len=%u st=%d\r\n", (unsigned)len, (int)st);
    return (st == HAL_OK) ? len : 0U;
}

/* USART2 SR 寄存器直接访问 (用于非阻塞 RXNE 检测) */
#define USART2_BASE_ADDR_FE  0x40004400UL
#define USART2_SR_REG        (*(volatile uint32_t *)(USART2_BASE_ADDR_FE + 0x00UL))
#define USART2_DR_REG        (*(volatile uint32_t *)(USART2_BASE_ADDR_FE + 0x04UL))
#define USART2_CR3_REG       (*(volatile uint32_t *)(USART2_BASE_ADDR_FE + 0x14UL))
#define USART_SR_PE_BIT      (1UL << 0)
#define USART_SR_FE_BIT      (1UL << 1)
#define USART_SR_NE_BIT      (1UL << 2)
#define USART_SR_ORE_BIT     (1UL << 3)
#define USART_SR_RXNE_BIT    (1UL << 5)
#define USART_CR3_DMAR_BIT   (1UL << 6)

/* DMA1 Stream5 (USART2_RX) - 参考 /Users/ll/fly/zmgjb/code/411/Core/Src/usart.c */
#define DMA1_BASE_ADDR_FE        0x40026000UL
#define DMA1_HISR_REG            (*(volatile uint32_t *)(DMA1_BASE_ADDR_FE + 0x04UL))
#define DMA1_HIFCR_REG           (*(volatile uint32_t *)(DMA1_BASE_ADDR_FE + 0x0CUL))
#define DMA1_STREAM5_BASE_ADDR   (DMA1_BASE_ADDR_FE + 0x88UL)
#define DMA1_S5_CR_REG           (*(volatile uint32_t *)(DMA1_STREAM5_BASE_ADDR + 0x00UL))
#define DMA1_S5_NDTR_REG         (*(volatile uint32_t *)(DMA1_STREAM5_BASE_ADDR + 0x04UL))
#define DMA1_S5_PAR_REG          (*(volatile uint32_t *)(DMA1_STREAM5_BASE_ADDR + 0x08UL))
#define DMA1_S5_M0AR_REG         (*(volatile uint32_t *)(DMA1_STREAM5_BASE_ADDR + 0x0CUL))
#define DMA1_S5_FCR_REG          (*(volatile uint32_t *)(DMA1_STREAM5_BASE_ADDR + 0x14UL))

#define DMA_SXCR_EN_BIT          (1UL << 0)
#define DMA_SXCR_MINC_BIT        (1UL << 10)
#define DMA_SXCR_CIRC_BIT        (1UL << 8)
#define DMA_SXCR_PL_HIGH         (0x2UL << 16)
#define DMA_SXCR_CHSEL_4         (0x4UL << 25)
#define DMA_SXCR_DIR_P2M         (0x0UL << 6)
#define DMA_HISR_FEIF5_BIT       (1UL << 6)
#define DMA_HISR_DMEIF5_BIT      (1UL << 8)
#define DMA_HISR_TEIF5_BIT       (1UL << 9)
#define DMA_HIFCR_CFEIF5_BIT     (1UL << 6)
#define DMA_HIFCR_CDMEIF5_BIT    (1UL << 8)
#define DMA_HIFCR_CTEIF5_BIT     (1UL << 9)
#define DMA_HIFCR_CHTIF5_BIT     (1UL << 10)
#define DMA_HIFCR_CTCIF5_BIT     (1UL << 11)

#define UART2_DMA_RX_BUF_SIZE    256U

static uint32_t g_uart2_rx_bytes = 0U;
static uint32_t g_uart2_ore_count = 0U;
static uint32_t g_uart2_fe_count = 0U;
static uint32_t g_uart2_ne_count = 0U;
static uint32_t g_uart2_pe_count = 0U;
static uint32_t g_uart2_err_bytes = 0U;
static uint32_t g_uart2_dma_err_count = 0U;
static uint8_t g_uart2_dma_rx_buf[UART2_DMA_RX_BUF_SIZE];
static uint16_t g_uart2_dma_rd = 0U;

static void uart2_dma_rx_init(void)
{
    /* 1. 使能 DMA1 时钟 */
    RCC_AHB1ENR_FE |= RCC_AHB1ENR_DMA1EN;

    /* 2. 关闭 Stream5，等待失能 */
    DMA1_S5_CR_REG &= ~DMA_SXCR_EN_BIT;
    for (volatile uint32_t i = 0; i < 100000UL; i++) {
        if ((DMA1_S5_CR_REG & DMA_SXCR_EN_BIT) == 0U) {
            break;
        }
    }

    /* 3. 清除历史标志 */
    DMA1_HIFCR_REG = DMA_HIFCR_CFEIF5_BIT |
                     DMA_HIFCR_CDMEIF5_BIT |
                     DMA_HIFCR_CTEIF5_BIT |
                     DMA_HIFCR_CHTIF5_BIT |
                     DMA_HIFCR_CTCIF5_BIT;

    /* 4. 配置 Stream5: Channel4, 外设到内存, 内存递增, 循环模式, 高优先级 */
    DMA1_S5_CR_REG = 0U;
    DMA1_S5_FCR_REG = 0U;
    DMA1_S5_PAR_REG = (uint32_t)&USART2_DR_REG;
    DMA1_S5_M0AR_REG = (uint32_t)g_uart2_dma_rx_buf;
    DMA1_S5_NDTR_REG = UART2_DMA_RX_BUF_SIZE;
    DMA1_S5_CR_REG = DMA_SXCR_CHSEL_4 |
                     DMA_SXCR_DIR_P2M |
                     DMA_SXCR_MINC_BIT |
                     DMA_SXCR_CIRC_BIT |
                     DMA_SXCR_PL_HIGH;

    /* 5. 清空可能遗留的 USART2 错误状态并打开 DMA 接收 */
    if (USART2_SR_REG & (USART_SR_PE_BIT | USART_SR_FE_BIT | USART_SR_NE_BIT | USART_SR_ORE_BIT | USART_SR_RXNE_BIT)) {
        volatile uint32_t sr = USART2_SR_REG;
        volatile uint32_t dr = USART2_DR_REG;
        (void)sr;
        (void)dr;
    }
    USART2_CR3_REG |= USART_CR3_DMAR_BIT;
    DMA1_S5_CR_REG |= DMA_SXCR_EN_BIT;
    g_uart2_dma_rd = 0U;
}

static uint16_t uart2_dma_wr(void)
{
    uint32_t ndtr = DMA1_S5_NDTR_REG;
    if (ndtr > UART2_DMA_RX_BUF_SIZE) {
        ndtr = UART2_DMA_RX_BUF_SIZE;
    }
    return (uint16_t)(UART2_DMA_RX_BUF_SIZE - ndtr);
}

/**
 * @brief 从 USART2 轮询接收, 将字节送入 wifi_command 接收缓冲区
 * @note  直接读寄存器，完全非阻塞
 */
static void uart2_poll_rx(void)
{
    /* 确保 huart2 处于就绪状态 (防止之前的超时错误) */
    if (huart2.state == HAL_STATE_ERROR || huart2.state == HAL_STATE_RESET) {
        huart2.state = HAL_STATE_READY;
        huart2.error_code = 0U;
    }

    /* DMA 错误标志检查 */
    if (DMA1_HISR_REG & (DMA_HISR_FEIF5_BIT | DMA_HISR_DMEIF5_BIT | DMA_HISR_TEIF5_BIT)) {
        g_uart2_dma_err_count++;
        DMA1_HIFCR_REG = DMA_HIFCR_CFEIF5_BIT |
                         DMA_HIFCR_CDMEIF5_BIT |
                         DMA_HIFCR_CTEIF5_BIT |
                         DMA_HIFCR_CHTIF5_BIT |
                         DMA_HIFCR_CTCIF5_BIT;
        platform_debug_print("[UART2_DMA] ERR count=%lu\r\n", (unsigned long)g_uart2_dma_err_count);
    }

    /* USART2 错误状态检查 */
    uint32_t sr = USART2_SR_REG;
    if (sr & (USART_SR_PE_BIT | USART_SR_FE_BIT | USART_SR_NE_BIT | USART_SR_ORE_BIT)) {
        uint8_t discard = (uint8_t)USART2_DR_REG;
        (void)discard;
        if (sr & USART_SR_PE_BIT) g_uart2_pe_count++;
        if (sr & USART_SR_FE_BIT) g_uart2_fe_count++;
        if (sr & USART_SR_NE_BIT) g_uart2_ne_count++;
        if (sr & USART_SR_ORE_BIT) g_uart2_ore_count++;
        g_uart2_err_bytes++;
        platform_debug_print("[UART2] ERR PE=%lu FE=%lu NE=%lu ORE=%lu dropped=%lu\r\n",
                             (unsigned long)g_uart2_pe_count,
                             (unsigned long)g_uart2_fe_count,
                             (unsigned long)g_uart2_ne_count,
                             (unsigned long)g_uart2_ore_count,
                             (unsigned long)g_uart2_err_bytes);
    }

    /* 使用 DMA 环形缓冲消费新字节 */
    uint32_t rx_count = 0U;
    uint16_t wr = uart2_dma_wr();
    while (g_uart2_dma_rd != wr) {
        uint8_t byte = g_uart2_dma_rx_buf[g_uart2_dma_rd++];
        if (g_uart2_dma_rd >= UART2_DMA_RX_BUF_SIZE) {
            g_uart2_dma_rd = 0U;
        }
        wifi_command_rx_byte(&g_flight.wifi_cmd, byte);
        rx_count++;
    }

    if (rx_count > 0U) {
        g_uart2_rx_bytes += rx_count;
        platform_debug_print("[UART2] RX %lu bytes (total=%lu)\r\n",
                             (unsigned long)rx_count,
                             (unsigned long)g_uart2_rx_bytes);
    }

}

/* ============================================================================
 * main()
 * ============================================================================ */

int main(void)
{
    hal_status_t status;

    clock_init();    /* 必须最先调用: HSI->PLL 84MHz */
    systick_init();  /* 依赖 84MHz SYSCLK */

    /* Initialise peripherals */
    uart1_init();   /* 调试串口先初始化，后续 debug_print 才能工作 */
    uart2_init();
    uart2_dma_rx_init();
    i2c1_init();
    spi3_init();

    platform_debug_print("\r\n[BOOT] Flight controller starting...\r\n");

    /* Initialise flight control system */
    status = flight_main_init(&g_flight);
    if (status != HAL_OK) {
        platform_debug_print("[BOOT] flight_main_init failed: %d\r\n", (int)status);
        while (1) { /* halt on fatal error */ }
    }

    platform_debug_print("[BOOT] Init OK, entering 1kHz control loop\r\n");

    /* Main loop: 1kHz control loop + 200Hz WiFi task */
    uint32_t last_wifi_ms = 0U;
    while (1) {
        uint32_t now = platform_get_time_ms();

        /* 轮询 USART2 接收 (每个循环都执行，非阻塞) */
        uart2_poll_rx();

        flight_main_control_loop(&g_flight);

        if ((now - last_wifi_ms) >= 5U) {
            last_wifi_ms = now;
            flight_main_wifi_task(&g_flight);
        }
    }
}
