#include "pwm.h"
#include "stm32f4xx_hal.h"

/* Clock frequencies - must match board_config.h */
#define APB1_CLOCK_HZ   42000000U   /* 42 MHz - APB1 clock */
#define APB2_CLOCK_HZ   84000000U   /* 84 MHz - APB2 clock */

/* TIM handles for each timer instance */
static TIM_HandleTypeDef tim_handles[4] = {{0}};

/* PWM state for each TIM/channel */
static pwm_state_t pwm_states[4][4] = {{{0}}};

/* Channel mapping: tim_channel_t to HAL TIM_CHANNEL_x */
static const uint32_t hal_channels[] = {
    TIM_CHANNEL_1,
    TIM_CHANNEL_2,
    TIM_CHANNEL_3,
    TIM_CHANNEL_4
};

/* TIM instance mapping: tim_t to TIM_TypeDef* */
static TIM_TypeDef *const tim_instances[] = {
    (TIM_TypeDef *)TIM1_BASE,
    (TIM_TypeDef *)TIM2_BASE,
    (TIM_TypeDef *)TIM3_BASE,
    (TIM_TypeDef *)TIM4_BASE
};

/* Enable TIM clock using HAL RCC macros */
static void tim_clock_enable(tim_t tim)
{
    switch (tim) {
        case KF_TIM1:
            __HAL_RCC_TIM1_CLK_ENABLE();
            break;
        case KF_TIM2:
            __HAL_RCC_TIM2_CLK_ENABLE();
            break;
        case KF_TIM3:
            __HAL_RCC_TIM3_CLK_ENABLE();
            break;
        case KF_TIM4:
            __HAL_RCC_TIM4_CLK_ENABLE();
            break;
    }
}

/* Get TIM clock frequency */
static uint32_t tim_get_clock_freq(tim_t tim)
{
    if (tim == KF_TIM1) {
        return APB2_CLOCK_HZ;
    }
    return APB1_CLOCK_HZ;
}

/* Calculate prescaler and auto-reload for target frequency */
static void tim_calc_period(uint32_t tim_clk, uint32_t freq_hz, uint16_t *psc, uint16_t *arr)
{
    uint32_t period;
    uint32_t prescaler;

    /* period = tim_clk / freq_hz, distribute between PSC and ARR */
    period = tim_clk / freq_hz;

    /* Target ARR around 1000 for 0.1% duty resolution */
    if (period <= 1000) {
        *psc = 0;
        *arr = (uint16_t)(period - 1);
    } else {
        /* PSC = (period / 1000) - 1, ARR = 999 */
        prescaler = period / 1000;
        if (prescaler > 65536) {
            prescaler = 65536;
        }
        *psc = (uint16_t)(prescaler - 1);
        *arr = (uint16_t)((period / prescaler) - 1);
    }

    /* Ensure minimum values */
    if (*arr == 0) {
        *arr = 1;
    }
}

hal_status_t pwm_init(const pwm_init_t *init)
{
    TIM_HandleTypeDef *htim;
    TIM_OC_InitTypeDef sConfig = {0};
    uint32_t tim_clk;
    uint16_t psc, arr;
    uint32_t ccr_value;

    if (init == NULL) {
        return HAL_ERROR;
    }

    if (init->tim > KF_TIM4 || init->channel > TIM_CH4) {
        return HAL_ERROR;
    }

    if (init->frequency_hz == 0 || init->frequency_hz > 20000) {
        return HAL_ERROR;
    }

    if (init->duty_1000 > 1000) {
        return HAL_ERROR;
    }

    htim = &tim_handles[init->tim];
    tim_clk = tim_get_clock_freq(init->tim);

    /* Enable TIM clock */
    tim_clock_enable(init->tim);

    /* Calculate prescaler and auto-reload */
    tim_calc_period(tim_clk, init->frequency_hz, &psc, &arr);

    /* Configure TIM base */
    htim->Instance = tim_instances[init->tim];
    htim->Init.Prescaler = psc;
    htim->Init.CounterMode = TIM_COUNTERMODE_UP;
    htim->Init.Period = arr;
    htim->Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim->Init.RepetitionCounter = 0;
    htim->Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;

    /* Initialize TIM PWM */
    if (HAL_TIM_PWM_Init(htim) != HAL_OK) {
        return HAL_ERROR;
    }

    /* Configure PWM channel */
    ccr_value = ((arr + 1) * init->duty_1000) / 1000;

    sConfig.OCMode = TIM_OCMODE_PWM1;
    sConfig.Pulse = ccr_value;
    sConfig.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfig.OCFastMode = TIM_OCFAST_DISABLE;

    if (HAL_TIM_PWM_ConfigChannel(htim, &sConfig, hal_channels[init->channel]) != HAL_OK) {
        return HAL_ERROR;
    }

    /* Store state */
    pwm_states[init->tim][init->channel].current_arr = arr;
    pwm_states[init->tim][init->channel].current_duty = init->duty_1000;

    return HAL_OK;
}

hal_status_t pwm_set_duty(tim_t tim, tim_channel_t channel, uint16_t duty_1000)
{
    TIM_HandleTypeDef *htim;
    uint32_t ccr_value;
    uint32_t arr;

    if (tim > KF_TIM4 || channel > TIM_CH4) {
        return HAL_ERROR;
    }

    if (duty_1000 > 1000) {
        return HAL_ERROR;
    }

    htim = &tim_handles[tim];
    arr = pwm_states[tim][channel].current_arr;

    /* Calculate CCR value: CCR = (ARR + 1) * duty / 1000 */
    ccr_value = ((arr + 1) * duty_1000) / 1000;

    /* Update CCR using HAL macro */
    __HAL_TIM_SET_COMPARE(htim, hal_channels[channel], ccr_value);

    /* Store current duty */
    pwm_states[tim][channel].current_duty = duty_1000;

    return HAL_OK;
}

hal_status_t pwm_set_frequency(tim_t tim, uint32_t frequency_hz)
{
    TIM_HandleTypeDef *htim;
    uint32_t tim_clk;
    uint16_t psc, arr;
    uint32_t old_arr;
    uint32_t new_ccr;
    int i;

    if (tim > KF_TIM4) {
        return HAL_ERROR;
    }

    if (frequency_hz == 0 || frequency_hz > 20000) {
        return HAL_ERROR;
    }

    htim = &tim_handles[tim];
    tim_clk = tim_get_clock_freq(tim);
    old_arr = __HAL_TIM_GET_AUTORELOAD(htim);

    /* Calculate new prescaler and auto-reload */
    tim_calc_period(tim_clk, frequency_hz, &psc, &arr);

    /* Stop counter temporarily */
    HAL_TIM_Base_Stop(htim);

    /* Update prescaler and auto-reload using HAL macros */
    __HAL_TIM_SET_PRESCALER(htim, psc);
    __HAL_TIM_SET_AUTORELOAD(htim, arr);

    /* Update CCR values proportionally for all channels */
    for (i = 0; i < 4; i++) {
        if (pwm_states[tim][i].current_duty > 0) {
            /* Scale CCR proportionally: new_CCR = old_CCR * new_ARR / old_ARR */
            new_ccr = (__HAL_TIM_GET_COMPARE(htim, hal_channels[i]) * (arr + 1)) / (old_arr + 1);
            if (new_ccr > arr) {
                new_ccr = arr;
            }
            __HAL_TIM_SET_COMPARE(htim, hal_channels[i], new_ccr);
        }
        pwm_states[tim][i].current_arr = arr;
    }

    /* Generate update event */
    __HAL_TIM_SET_COUNTER(htim, 0);

    /* Re-enable counter if any channel is running */
    for (i = 0; i < 4; i++) {
        if (pwm_states[tim][i].is_running) {
            HAL_TIM_Base_Start(htim);
            break;
        }
    }

    return HAL_OK;
}

hal_status_t pwm_start(tim_t tim, tim_channel_t channel)
{
    TIM_HandleTypeDef *htim;

    if (tim > KF_TIM4 || channel > TIM_CH4) {
        return HAL_ERROR;
    }

    htim = &tim_handles[tim];

    /* Start PWM output for this channel */
    if (HAL_TIM_PWM_Start(htim, hal_channels[channel]) != HAL_OK) {
        return HAL_ERROR;
    }

    /* Mark as running */
    pwm_states[tim][channel].is_running = 1;

    return HAL_OK;
}

hal_status_t pwm_stop(tim_t tim, tim_channel_t channel)
{
    TIM_HandleTypeDef *htim;
    int i;
    int any_running = 0;

    if (tim > KF_TIM4 || channel > TIM_CH4) {
        return HAL_ERROR;
    }

    htim = &tim_handles[tim];

    /* Stop PWM output for this channel */
    if (HAL_TIM_PWM_Stop(htim, hal_channels[channel]) != HAL_OK) {
        return HAL_ERROR;
    }

    /* Mark as stopped */
    pwm_states[tim][channel].is_running = 0;

    /* Check if any channel is still running */
    for (i = 0; i < 4; i++) {
        if (pwm_states[tim][i].is_running) {
            any_running = 1;
            break;
        }
    }

    /* Disable counter if no channels are running */
    if (!any_running) {
        HAL_TIM_Base_Stop(htim);
    }

    return HAL_OK;
}
