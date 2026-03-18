/*
 * UART test for STM32F411CEU6
 * HSE 8MHz with PLL -> 100MHz, USART1 PA9/PA10, 115200 baud
 */

#include <stdint.h>
#include <string.h>

/* Register definitions */
#define RCC_BASE        0x40023800
#define GPIOA_BASE      0x40020000
#define USART1_BASE     0x40011000

#define RCC             ((RCC_TypeDef *)RCC_BASE)
#define GPIOA           ((GPIO_TypeDef *)GPIOA_BASE)
#define USART1          ((USART_TypeDef *)USART1_BASE)

typedef struct {
    volatile uint32_t CR;
    volatile uint32_t PLLCFGR;
    volatile uint32_t CFGR;
    volatile uint32_t CIR;
    volatile uint32_t AHB1RSTR;
    volatile uint32_t AHB2RSTR;
    volatile uint32_t AHB3RSTR;
    uint32_t RESERVED0;
    volatile uint32_t APB1RSTR;
    volatile uint32_t APB2RSTR;
    uint32_t RESERVED1[2];
    volatile uint32_t AHB1ENR;
    volatile uint32_t AHB2ENR;
    volatile uint32_t AHB3ENR;
    uint32_t RESERVED2;
    volatile uint32_t APB1ENR;
    volatile uint32_t APB2ENR;
} RCC_TypeDef;

typedef struct {
    volatile uint32_t MODER;
    volatile uint32_t OTYPER;
    volatile uint32_t OSPEEDR;
    volatile uint32_t PUPDR;
    volatile uint32_t IDR;
    volatile uint32_t ODR;
    volatile uint32_t BSRR;
    volatile uint32_t LCKR;
    volatile uint32_t AFR[2];
} GPIO_TypeDef;

typedef struct {
    volatile uint32_t SR;
    volatile uint32_t DR;
    volatile uint32_t BRR;
    volatile uint32_t CR1;
    volatile uint32_t CR2;
    volatile uint32_t CR3;
    volatile uint32_t GTPR;
} USART_TypeDef;

/* Bit definitions */
#define RCC_CR_HSION        (1U << 0)
#define RCC_CR_HSIRDY       (1U << 1)
#define RCC_CR_HSEON        (1U << 16)
#define RCC_CR_HSERDY       (1U << 17)
#define RCC_CR_PLLON        (1U << 24)
#define RCC_CR_PLLRDY       (1U << 25)
#define RCC_CFGR_SW_HSI     0x00
#define RCC_CFGR_SWS_HSI    0x00
#define RCC_CFGR_SW_PLL     0x02
#define RCC_CFGR_SWS_PLL    0x08
#define RCC_AHB1ENR_GPIOAEN (1U << 0)
#define RCC_APB2ENR_USART1EN (1U << 4)

/* Flash registers */
#define FLASH_BASE      0x40023C00
#define FLASH_ACR       (*(volatile uint32_t *)(FLASH_BASE + 0x00))
#define FLASH_ACR_LATENCY_3WS 3

#define GPIO_MODER_AF       2
#define GPIO_OSPEEDR_HIGH   3
#define GPIO_AFRH_AF7       7

#define USART_CR1_UE        (1U << 13)
#define USART_CR1_TE        (1U << 3)
#define USART_CR1_M         (1U << 12)
#define USART_CR1_PCE       (1U << 10)
#define USART_SR_TXE        (1U << 7)

static void delay(volatile uint32_t count)
{
    while (count--) __asm__("nop");
}

static void uart_init_simple(void)
{
    /* Enable GPIOA clock */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

    /* Enable USART1 clock */
    RCC->APB2ENR |= RCC_APB2ENR_USART1EN;

    /* Configure PA9 (TX) as AF7 */
    GPIOA->MODER &= ~(3U << (9 * 2));
    GPIOA->MODER |= (GPIO_MODER_AF << (9 * 2));
    GPIOA->OSPEEDR |= (GPIO_OSPEEDR_HIGH << (9 * 2));
    GPIOA->AFR[1] &= ~(0xF << ((9 - 8) * 4));
    GPIOA->AFR[1] |= (GPIO_AFRH_AF7 << ((9 - 8) * 4));

    /* Configure PA10 (RX) as AF7 */
    GPIOA->MODER &= ~(3U << (10 * 2));
    GPIOA->MODER |= (GPIO_MODER_AF << (10 * 2));
    GPIOA->OSPEEDR |= (GPIO_OSPEEDR_HIGH << (10 * 2));
    GPIOA->AFR[1] &= ~(0xF << ((10 - 8) * 4));
    GPIOA->AFR[1] |= (GPIO_AFRH_AF7 << ((10 - 8) * 4));

    /* USART1 115200 @ 100MHz: BRR = 100MHz / 115200 = 868.055 = 0x364 */
    USART1->BRR = 0x364;

    /* Enable TX, Enable USART */
    USART1->CR1 = USART_CR1_TE | USART_CR1_UE;
}

static void uart_putc(char c)
{
    while (!(USART1->SR & USART_SR_TXE));
    USART1->DR = c;
}

static void uart_puts(const char *str)
{
    while (*str) {
        uart_putc(*str++);
    }
}

static void clock_init_hse(void)
{
    /* Enable HSE and wait */
    RCC->CR |= RCC_CR_HSEON;
    while (!(RCC->CR & RCC_CR_HSERDY));

    /* Configure PLL: 8MHz / 4 * 100 / 2 = 100MHz
     * PLLM = 4, PLLN = 100, PLLP = 2
     * PLLCFGR = (PLLN << 6) | (PLLP << 16) | (PLL_SOURCE_HSE << 22) | PLLM
     */
    RCC->PLLCFGR = (100U << 6) | (0U << 16) | (1U << 22) | 4U;

    /* Enable PLL and wait */
    RCC->CR |= RCC_CR_PLLON;
    while (!(RCC->CR & RCC_CR_PLLRDY));

    /* Set FLASH latency for 100MHz (3 wait states) */
    FLASH_ACR = (FLASH_ACR & ~7) | FLASH_ACR_LATENCY_3WS;

    /* Switch to PLL */
    RCC->CFGR &= ~3;
    RCC->CFGR |= RCC_CFGR_SW_PLL;
    while ((RCC->CFGR & 0x0C) != RCC_CFGR_SWS_PLL);
}

int main(void)
{
    /* Enable HSE with PLL -> 100MHz */
    clock_init_hse();

    /* Simple delay for stabilization */
    delay(100000);

    /* Init UART @ 100MHz */
    uart_init_simple();

    /* Output test message in loop */
    int count = 0;
    while (1) {
        uart_puts("Hello from STM32 HSE 8MHz! Loop=");

        /* Print number */
        char buf[8];
        int n = count++;
        int i = 0;
        do {
            buf[i++] = '0' + (n % 10);
            n /= 10;
        } while (n > 0);
        while (i > 0) {
            uart_putc(buf[--i]);
        }
        uart_puts("\r\n");

        delay(1000000); /* ~500ms at 16MHz */
    }

    return 0;
}
