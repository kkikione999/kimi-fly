/**
 * @file gpio.c
 * @brief STM32 GPIO HAL实现 (STM32Cube HAL API)
 *
 * @note 本文件为Ralph-loop v2.0 HAL层基础文件
 */

#include "gpio.h"
#include "stm32f4xx_hal.h"

/* GPIO端口映射表 */
static GPIO_TypeDef *const GPIO_PORTS[] = {
    GPIOA,
    GPIOB,
    GPIOC,
    GPIOD,
    GPIOE,
    GPIOH
};

hal_status_t gpio_init(gpio_handle_t *gpio, const gpio_config_t *config)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    uint32_t i;

    if (gpio == NULL || config == NULL || gpio->port >= 6) {
        return HAL_ERROR;
    }

    /* 使能时钟 */
    gpio_clk_enable(gpio->port);

    /* 配置GPIO参数 */
    GPIO_InitStruct.Pin = gpio->pin_mask;

    /* 转换模式 */
    switch (config->mode) {
        case GPIO_MODE_INPUT:
            GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
            break;
        case KF_GPIO_MODE_OUTPUT:
            GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
            break;
        case KF_GPIO_MODE_AF:
            GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
            break;
        case GPIO_MODE_ANALOG:
            GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
            break;
        default:
            return HAL_ERROR;
    }

    /* 转换输出类型 (仅在输出模式下有效) */
    if (config->mode == KF_GPIO_MODE_OUTPUT) {
        if (config->otype == GPIO_OTYPE_OD) {
            GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
        }
    } else if (config->mode == KF_GPIO_MODE_AF) {
        if (config->otype == GPIO_OTYPE_OD) {
            GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
        }
    }

    /* 转换速度 */
    switch (config->speed) {
        case GPIO_SPEED_LOW:
            GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
            break;
        case GPIO_SPEED_MEDIUM:
            GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
            break;
        case GPIO_SPEED_FAST:
            GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
            break;
        case GPIO_SPEED_HIGH:
            GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
            break;
        default:
            GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
            break;
    }

    /* 转换上下拉 */
    switch (config->pupd) {
        case GPIO_PUPD_NONE:
            GPIO_InitStruct.Pull = GPIO_NOPULL;
            break;
        case GPIO_PUPD_UP:
            GPIO_InitStruct.Pull = GPIO_PULLUP;
            break;
        case GPIO_PUPD_DOWN:
            GPIO_InitStruct.Pull = GPIO_PULLDOWN;
            break;
        default:
            GPIO_InitStruct.Pull = GPIO_NOPULL;
            break;
    }

    /* 配置复用功能 */
    if (config->mode == KF_GPIO_MODE_AF) {
        /* 找到第一个使能的引脚，设置其复用功能 */
        for (i = 0; i < 16; i++) {
            if (gpio->pin_mask & (1U << i)) {
                GPIO_InitStruct.Alternate = (uint8_t)config->af;
                break;
            }
        }
    }

    /* 调用HAL_GPIO_Init */
    HAL_GPIO_Init(GPIO_PORTS[gpio->port], &GPIO_InitStruct);

    return HAL_OK;
}

hal_status_t gpio_deinit(gpio_handle_t *gpio)
{
    if (gpio == NULL || gpio->port >= 6) {
        return HAL_ERROR;
    }

    /* 调用HAL_GPIO_DeInit */
    HAL_GPIO_DeInit(GPIO_PORTS[gpio->port], gpio->pin_mask);

    return HAL_OK;
}

hal_status_t gpio_write(gpio_handle_t *gpio, uint8_t value)
{
    if (gpio == NULL || gpio->port >= 6) {
        return HAL_ERROR;
    }

    /* 找到第一个使能的引脚 */
    uint16_t pin = 0;
    for (uint32_t i = 0; i < 16; i++) {
        if (gpio->pin_mask & (1U << i)) {
            pin = (1U << i);
            break;
        }
    }

    if (pin == 0) {
        return HAL_ERROR;
    }

    HAL_GPIO_WritePin(GPIO_PORTS[gpio->port], pin,
                      value ? GPIO_PIN_SET : GPIO_PIN_RESET);

    return HAL_OK;
}

hal_status_t gpio_read(gpio_handle_t *gpio, uint8_t *value)
{
    if (gpio == NULL || value == NULL || gpio->port >= 6) {
        return HAL_ERROR;
    }

    /* 找到第一个使能的引脚 */
    uint16_t pin = 0;
    for (uint32_t i = 0; i < 16; i++) {
        if (gpio->pin_mask & (1U << i)) {
            pin = (1U << i);
            break;
        }
    }

    if (pin == 0) {
        return HAL_ERROR;
    }

    *value = (HAL_GPIO_ReadPin(GPIO_PORTS[gpio->port], pin) == GPIO_PIN_SET) ? 1 : 0;

    return HAL_OK;
}

hal_status_t gpio_toggle(gpio_handle_t *gpio)
{
    if (gpio == NULL || gpio->port >= 6) {
        return HAL_ERROR;
    }

    /* 找到第一个使能的引脚 */
    uint16_t pin = 0;
    for (uint32_t i = 0; i < 16; i++) {
        if (gpio->pin_mask & (1U << i)) {
            pin = (1U << i);
            break;
        }
    }

    if (pin == 0) {
        return HAL_ERROR;
    }

    HAL_GPIO_TogglePin(GPIO_PORTS[gpio->port], pin);

    return HAL_OK;
}

hal_status_t gpio_clk_enable(gpio_port_t port)
{
    if (port >= 6) {
        return HAL_ERROR;
    }

    switch (port) {
        case GPIO_PORT_A:
            __HAL_RCC_GPIOA_CLK_ENABLE();
            break;
        case GPIO_PORT_B:
            __HAL_RCC_GPIOB_CLK_ENABLE();
            break;
        case GPIO_PORT_C:
            __HAL_RCC_GPIOC_CLK_ENABLE();
            break;
        case GPIO_PORT_D:
            __HAL_RCC_GPIOD_CLK_ENABLE();
            break;
        case GPIO_PORT_E:
            __HAL_RCC_GPIOE_CLK_ENABLE();
            break;
        case GPIO_PORT_H:
            __HAL_RCC_GPIOH_CLK_ENABLE();
            break;
        default:
            return HAL_ERROR;
    }

    return HAL_OK;
}

hal_status_t gpio_clk_disable(gpio_port_t port)
{
    if (port >= 6) {
        return HAL_ERROR;
    }

    switch (port) {
        case GPIO_PORT_A:
            __HAL_RCC_GPIOA_CLK_DISABLE();
            break;
        case GPIO_PORT_B:
            __HAL_RCC_GPIOB_CLK_DISABLE();
            break;
        case GPIO_PORT_C:
            __HAL_RCC_GPIOC_CLK_DISABLE();
            break;
        case GPIO_PORT_D:
            __HAL_RCC_GPIOD_CLK_DISABLE();
            break;
        case GPIO_PORT_E:
            __HAL_RCC_GPIOE_CLK_DISABLE();
            break;
        case GPIO_PORT_H:
            __HAL_RCC_GPIOH_CLK_DISABLE();
            break;
        default:
            return HAL_ERROR;
    }

    return HAL_OK;
}
