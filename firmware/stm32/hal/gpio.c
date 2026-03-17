#include "gpio.h"

/* STM32F411CEU6 GPIO Base Addresses */
#define GPIOA_BASE_ADDR     0x40020000UL
#define GPIOB_BASE_ADDR     0x40020400UL
#define GPIOC_BASE_ADDR     0x40020800UL

/* RCC Base Address and Register offsets */
#define RCC_BASE_ADDR       0x40023800UL
#define RCC_AHB1ENR_OFFSET  0x30UL

#define RCC_AHB1ENR         (*(volatile uint32_t *)(RCC_BASE_ADDR + RCC_AHB1ENR_OFFSET))

/* RCC AHB1ENR GPIO Clock Enable Bits */
#define RCC_AHB1ENR_GPIOAEN (1UL << 0)
#define RCC_AHB1ENR_GPIOBEN (1UL << 1)
#define RCC_AHB1ENR_GPIOCEN (1UL << 2)

/* GPIO Port base address table */
static gpio_reg_t *const gpio_ports[GPIO_PORT_MAX] = {
    (gpio_reg_t *)GPIOA_BASE_ADDR,
    (gpio_reg_t *)GPIOB_BASE_ADDR,
    (gpio_reg_t *)GPIOC_BASE_ADDR
};

/* Static helper function to enable GPIO clock */
static void gpio_clock_enable(gpio_port_t port)
{
    switch (port) {
        case GPIO_PORT_A:
            RCC_AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
            break;
        case GPIO_PORT_B:
            RCC_AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
            break;
        case GPIO_PORT_C:
            RCC_AHB1ENR |= RCC_AHB1ENR_GPIOCEN;
            break;
        default:
            break;
    }
}

/* Initialize GPIO pin according to init structure */
hal_status_t gpio_init(const gpio_init_t *init)
{
    gpio_reg_t *gpio;
    uint32_t pin = init->pin;
    uint32_t mode;
    uint32_t afr_index;
    uint32_t afr_shift;

    if (init == NULL) {
        return HAL_ERROR;
    }

    if (init->port >= GPIO_PORT_MAX || init->pin > GPIO_PIN_15) {
        return HAL_ERROR;
    }

    /* Enable clock for the port */
    gpio_clock_enable(init->port);

    gpio = gpio_ports[init->port];

    /* Configure mode (2 bits per pin) */
    mode = init->mode;
    gpio->MODER &= ~(3UL << (pin * 2));
    gpio->MODER |= (mode << (pin * 2));

    /* Configure output type (1 bit per pin) */
    gpio->OTYPER &= ~(1UL << pin);
    gpio->OTYPER |= (init->otype << pin);

    /* Configure speed (2 bits per pin) */
    gpio->OSPEEDR &= ~(3UL << (pin * 2));
    gpio->OSPEEDR |= (init->speed << (pin * 2));

    /* Configure pull-up/pull-down (2 bits per pin) */
    gpio->PUPDR &= ~(3UL << (pin * 2));
    gpio->PUPDR |= (init->pupd << (pin * 2));

    /* Configure alternate function if needed */
    if (init->mode == GPIO_MODE_AF) {
        afr_index = pin / 8;        /* AFR[0] for pins 0-7, AFR[1] for pins 8-15 */
        afr_shift = (pin % 8) * 4;  /* 4 bits per pin in AFR */
        gpio->AFR[afr_index] &= ~(0xFUL << afr_shift);
        gpio->AFR[afr_index] |= ((init->af & 0x0FUL) << afr_shift);
    }

    return HAL_OK;
}

/* Write value to GPIO output pin */
void gpio_write(gpio_port_t port, gpio_pin_t pin, uint8_t value)
{
    gpio_reg_t *gpio;

    if (port >= GPIO_PORT_MAX || pin > GPIO_PIN_15) {
        return;
    }

    gpio = gpio_ports[port];

    if (value) {
        /* Set bit in BSRR (lower 16 bits) */
        gpio->BSRR = (1UL << pin);
    } else {
        /* Reset bit in BSRR (upper 16 bits) */
        gpio->BSRR = (1UL << (pin + 16));
    }
}

/* Read value from GPIO input pin */
uint8_t gpio_read(gpio_port_t port, gpio_pin_t pin)
{
    gpio_reg_t *gpio;

    if (port >= GPIO_PORT_MAX || pin > GPIO_PIN_15) {
        return 0;
    }

    gpio = gpio_ports[port];

    /* Read from IDR register */
    return (uint8_t)((gpio->IDR >> pin) & 1UL);
}

/* Toggle GPIO output pin */
void gpio_toggle(gpio_port_t port, gpio_pin_t pin)
{
    gpio_reg_t *gpio;

    if (port >= GPIO_PORT_MAX || pin > GPIO_PIN_15) {
        return;
    }

    gpio = gpio_ports[port];

    /* Read current state from ODR and toggle */
    gpio->ODR ^= (1UL << pin);
}

/* Set GPIO mode for a pin */
hal_status_t gpio_set_mode(gpio_port_t port, gpio_pin_t pin, gpio_mode_t mode)
{
    gpio_reg_t *gpio;

    if (port >= GPIO_PORT_MAX || pin > GPIO_PIN_15) {
        return HAL_ERROR;
    }

    /* Enable clock for the port */
    gpio_clock_enable(port);

    gpio = gpio_ports[port];

    /* Configure mode (2 bits per pin) */
    gpio->MODER &= ~(3UL << (pin * 2));
    gpio->MODER |= (mode << (pin * 2));

    return HAL_OK;
}
