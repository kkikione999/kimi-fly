#ifndef GPIO_H
#define GPIO_H

#include "hal_common.h"

/* GPIO Port definitions */
typedef enum {
    GPIO_PORT_A = 0,
    GPIO_PORT_B = 1,
    GPIO_PORT_C = 2,
    GPIO_PORT_MAX
} gpio_port_t;

/* GPIO Pin definitions */
typedef enum {
    GPIO_PIN_0 = 0,
    GPIO_PIN_1 = 1,
    GPIO_PIN_2 = 2,
    GPIO_PIN_3 = 3,
    GPIO_PIN_4 = 4,
    GPIO_PIN_5 = 5,
    GPIO_PIN_6 = 6,
    GPIO_PIN_7 = 7,
    GPIO_PIN_8 = 8,
    GPIO_PIN_9 = 9,
    GPIO_PIN_10 = 10,
    GPIO_PIN_11 = 11,
    GPIO_PIN_12 = 12,
    GPIO_PIN_13 = 13,
    GPIO_PIN_14 = 14,
    GPIO_PIN_15 = 15
} gpio_pin_t;

/* GPIO Mode definitions */
typedef enum {
    GPIO_MODE_INPUT = 0,
    GPIO_MODE_OUTPUT = 1,
    GPIO_MODE_AF = 2,
    GPIO_MODE_ANALOG = 3
} gpio_mode_t;

/* GPIO Output Type definitions */
typedef enum {
    GPIO_OTYPE_PP = 0,  /* Push-pull */
    GPIO_OTYPE_OD = 1   /* Open-drain */
} gpio_otype_t;

/* GPIO Speed definitions */
typedef enum {
    GPIO_SPEED_LOW = 0,      /* 2 MHz */
    GPIO_SPEED_MEDIUM = 1,   /* 25 MHz */
    GPIO_SPEED_HIGH = 2,     /* 50 MHz */
    GPIO_SPEED_VERYHIGH = 3  /* 100 MHz */
} gpio_speed_t;

/* GPIO Pull-up/Pull-down definitions */
typedef enum {
    GPIO_PUPD_NONE = 0,
    GPIO_PUPD_UP = 1,
    GPIO_PUPD_DOWN = 2
} gpio_pupd_t;

/* GPIO Initialization structure */
typedef struct {
    gpio_port_t port;
    gpio_pin_t pin;
    gpio_mode_t mode;
    gpio_otype_t otype;
    gpio_speed_t speed;
    gpio_pupd_t pupd;
    uint8_t af;             /* Alternate function (0-15), valid when mode is AF */
} gpio_init_t;

/* GPIO Register structure */
typedef struct {
    volatile uint32_t MODER;    /* 0x00 - Mode register */
    volatile uint32_t OTYPER;   /* 0x04 - Output type register */
    volatile uint32_t OSPEEDR;  /* 0x08 - Output speed register */
    volatile uint32_t PUPDR;    /* 0x0C - Pull-up/pull-down register */
    volatile uint32_t IDR;      /* 0x10 - Input data register */
    volatile uint32_t ODR;      /* 0x14 - Output data register */
    volatile uint32_t BSRR;     /* 0x18 - Bit set/reset register */
    volatile uint32_t LCKR;     /* 0x1C - Configuration lock register */
    volatile uint32_t AFR[2];   /* 0x20 - Alternate function registers */
} gpio_reg_t;

/* Function declarations */
hal_status_t gpio_init(const gpio_init_t *init);
void gpio_write(gpio_port_t port, gpio_pin_t pin, uint8_t value);
uint8_t gpio_read(gpio_port_t port, gpio_pin_t pin);
void gpio_toggle(gpio_port_t port, gpio_pin_t pin);
hal_status_t gpio_set_mode(gpio_port_t port, gpio_pin_t pin, gpio_mode_t mode);

#endif /* GPIO_H */
