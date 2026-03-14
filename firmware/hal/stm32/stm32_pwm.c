/*
 * Copyright (c) 2026 kimi-fly Project
 * SPDX-License-Identifier: MIT
 */

/**
 * @file stm32_pwm.c
 * @brief STM32 PWM implementation for motor control
 */

#include "stm32_hal.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_tim.h"

#define PWM_TIM_INSTANCE    TIM3
#define PWM_TIM_CLK_ENABLE  __HAL_RCC_TIM3_CLK_ENABLE

static TIM_HandleTypeDef pwm_tim_handle = {0};

static uint32_t get_tim_channel(uint8_t channel)
{
    switch (channel) {
        case 0: return TIM_CHANNEL_1;
        case 1: return TIM_CHANNEL_2;
        case 2: return TIM_CHANNEL_3;
        case 3: return TIM_CHANNEL_4;
        default: return TIM_CHANNEL_1;
    }
}

static int stm32_pwm_init(uint8_t channel, const hal_pwm_cfg_t *cfg, hal_handle_t *handle)
{
    if (channel >= 4) {
        return HAL_ERR_PARAM;
    }
    
    PWM_TIM_CLK_ENABLE();
    
    pwm_tim_handle.Instance = PWM_TIM_INSTANCE;
    pwm_tim_handle.Init.Prescaler = (SystemCoreClock / 2) / (cfg->freq_hz * 1000) - 1;
    pwm_tim_handle.Init.CounterMode = TIM_COUNTERMODE_UP;
    pwm_tim_handle.Init.Period = 1000 - 1;  /* 0.1% resolution */
    pwm_tim_handle.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    HAL_TIM_PWM_Init(&pwm_tim_handle);
    
    TIM_OC_InitTypeDef sConfig = {0};
    sConfig.OCMode = TIM_OCMODE_PWM1;
    sConfig.Pulse = (uint16_t)(cfg->init_duty * 1000.0f);
    sConfig.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfig.OCFastMode = TIM_OCFAST_DISABLE;
    
    HAL_TIM_PWM_ConfigChannel(&pwm_tim_handle, &sConfig, get_tim_channel(channel));
    HAL_TIM_PWM_Start(&pwm_tim_handle, get_tim_channel(channel));
    
    *handle = (hal_handle_t)(uintptr_t)(channel + 1);
    return HAL_OK;
}

static int stm32_pwm_set_duty(hal_handle_t handle, float duty)
{
    uint8_t channel = (uint8_t)((uintptr_t)handle - 1);
    if (channel >= 4) {
        return HAL_ERR_PARAM;
    }
    
    uint32_t pulse = (uint32_t)(CLAMP(duty, 0.0f, 1.0f) * 1000.0f);
    __HAL_TIM_SET_COMPARE(&pwm_tim_handle, get_tim_channel(channel), pulse);
    
    return HAL_OK;
}

static int stm32_pwm_set_freq(hal_handle_t handle, uint32_t freq_hz)
{
    (void)handle;
    (void)freq_hz;
    /* Frequency change not supported in runtime for this implementation */
    return HAL_ERR_NOTSUPP;
}

static int stm32_pwm_enable(hal_handle_t handle, bool enable)
{
    uint8_t channel = (uint8_t)((uintptr_t)handle - 1);
    if (channel >= 4) {
        return HAL_ERR_PARAM;
    }
    
    if (enable) {
        HAL_TIM_PWM_Start(&pwm_tim_handle, get_tim_channel(channel));
    } else {
        HAL_TIM_PWM_Stop(&pwm_tim_handle, get_tim_channel(channel));
    }
    return HAL_OK;
}

static int stm32_pwm_deinit(hal_handle_t handle)
{
    (void)handle;
    HAL_TIM_PWM_DeInit(&pwm_tim_handle);
    return HAL_OK;
}

static const hal_pwm_interface_t stm32_pwm_iface = {
    .init = stm32_pwm_init,
    .set_duty = stm32_pwm_set_duty,
    .set_freq = stm32_pwm_set_freq,
    .enable = stm32_pwm_enable,
    .deinit = stm32_pwm_deinit
};

int stm32_pwm_register_hal(void)
{
    return hal_pwm_register(&stm32_pwm_iface);
}
