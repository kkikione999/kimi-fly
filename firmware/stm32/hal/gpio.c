/**
 * @file gpio.c
 * @brief STM32 GPIO HAL实现
 *
 * @note 本文件为Ralph-loop v2.0 HAL层基础文件
 */

#include "gpio.h"

/* STM32F4xx 寄存器定义 (用于裸机开发) */
#define RCC_BASE            0x40023800U
#define RCC_AHB1ENR         (*(volatile uint32_t *)(RCC_BASE + 0x30U))

#define GPIOA_BASE          0x40020000U
#define GPIOB_BASE          0x40020400U
#define GPIOC_BASE          0x40020800U
#define GPIOD_BASE          0x40020C00U
#define GPIOE_BASE          0x40021000U
#define GPIOH_BASE          0x40021C00U

#define RCC_AHB1ENR_GPIOAEN (1U << 0)
#define RCC_AHB1ENR_GPIOBEN (1U << 1)
#define RCC_AHB1ENR_GPIOCEN (1U << 2)
#define RCC_AHB1ENR_GPIODEN (1U << 3)
#define RCC_AHB1ENR_GPIOEEN (1U << 4)
#define RCC_AHB1ENR_GPIOHEN (1U << 7)

typedef struct {
    volatile uint32_t MODER;
    volatile uint32_t OTYPER;
    volatile uint32_t OSPEEDR;
    volatile uint32_t PUPDR;
    volatile uint32_t IDR;
    volatile uint32_t ODR;
    volatile uint32_t BSRR;
    volatile uint32_t LCKR;
    volatile uint32_t AFRL;
    volatile uint32_t AFRH;
} GPIO_TypeDef;

static GPIO_TypeDef *const GPIO_PORTS[] = {
    (GPIO_TypeDef *)GPIOA_BASE,
    (GPIO_TypeDef *)GPIOB_BASE,
    (GPIO_TypeDef *)GPIOC_BASE,
    (GPIO_TypeDef *)GPIOD_BASE,
    (GPIO_TypeDef *)GPIOE_BASE,
    (GPIO_TypeDef *)GPIOH_BASE
};

static const uint32_t GPIO_CLK_EN_BITS[] = {
    RCC_AHB1ENR_GPIOAEN,
    RCC_AHB1ENR_GPIOBEN,
    RCC_AHB1ENR_GPIOCEN,
    RCC_AHB1ENR_GPIODEN,
    RCC_AHB1ENR_GPIOEEN,
    RCC_AHB1ENR_GPIOHEN
};

hal_status_t gpio_init(gpio_handle_t *gpio, const gpio_config_t *config)
{
    GPIO_TypeDef *port;
    uint32_t pin_num;
    uint32_t i;

    if (gpio == NULL || config == NULL || gpio->port >= 6) {
        return HAL_ERROR;
    }

    /* 使能时钟 */
    gpio_clk_enable(gpio->port);

    port = GPIO_PORTS[gpio->port];

    for (i = 0; i < 16; i++) {
        if (gpio->pin_mask & (1U << i)) {
            pin_num = i;

            /* 配置模式 */
            port->MODER &= ~(3U << (pin_num * 2));
            port->MODER |= ((uint32_t)config->mode << (pin_num * 2));

            /* 配置输出类型 */
            port->OTYPER &= ~(1U << pin_num);
            port->OTYPER |= ((uint32_t)config->otype << pin_num);

            /* 配置速度 */
            port->OSPEEDR &= ~(3U << (pin_num * 2));
            port->OSPEEDR |= ((uint32_t)config->speed << (pin_num * 2));

            /* 配置上下拉 */
            port->PUPDR &= ~(3U << (pin_num * 2));
            port->PUPDR |= ((uint32_t)config->pupd << (pin_num * 2));

            /* 配置复用功能 */
            if (pin_num < 8) {
                port->AFRL &= ~(0xFU << (pin_num * 4));
                port->AFRL |= ((uint32_t)config->af << (pin_num * 4));
            } else {
                port->AFRH &= ~(0xFU << ((pin_num - 8) * 4));
                port->AFRH |= ((uint32_t)config->af << ((pin_num - 8) * 4));
            }
        }
    }

    return HAL_OK;
}

hal_status_t gpio_deinit(gpio_handle_t *gpio)
{
    GPIO_TypeDef *port;
    uint32_t i;

    if (gpio == NULL || gpio->port >= 6) {
        return HAL_ERROR;
    }

    port = GPIO_PORTS[gpio->port];

    for (i = 0; i < 16; i++) {
        if (gpio->pin_mask & (1U << i)) {
            /* 恢复为输入模式 */
            port->MODER &= ~(3U << (i * 2));
        }
    }

    return HAL_OK;
}

hal_status_t gpio_write(gpio_handle_t *gpio, uint8_t value)
{
    GPIO_TypeDef *port;

    if (gpio == NULL || gpio->port >= 6) {
        return HAL_ERROR;
    }

    port = GPIO_PORTS[gpio->port];

    if (value) {
        port->BSRR = gpio->pin_mask;
    } else {
        port->BSRR = (uint32_t)gpio->pin_mask << 16;
    }

    return HAL_OK;
}

hal_status_t gpio_read(gpio_handle_t *gpio, uint8_t *value)
{
    GPIO_TypeDef *port;

    if (gpio == NULL || value == NULL || gpio->port >= 6) {
        return HAL_ERROR;
    }

    port = GPIO_PORTS[gpio->port];

    *value = (port->IDR & gpio->pin_mask) ? 1 : 0;

    return HAL_OK;
}

hal_status_t gpio_toggle(gpio_handle_t *gpio)
{
    GPIO_TypeDef *port;

    if (gpio == NULL || gpio->port >= 6) {
        return HAL_ERROR;
    }

    port = GPIO_PORTS[gpio->port];

    port->ODR ^= gpio->pin_mask;

    return HAL_OK;
}

hal_status_t gpio_clk_enable(gpio_port_t port)
{
    if (port >= 6) {
        return HAL_ERROR;
    }

    RCC_AHB1ENR |= GPIO_CLK_EN_BITS[port];

    return HAL_OK;
}

hal_status_t gpio_clk_disable(gpio_port_t port)
{
    if (port >= 6) {
        return HAL_ERROR;
    }

    RCC_AHB1ENR &= ~GPIO_CLK_EN_BITS[port];

    return HAL_OK;
}
