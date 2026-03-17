#include "pwm.h"

/* TIM register structure */
typedef struct {
    volatile uint32_t CR1;    /* 0x00 - Control register 1 */
    volatile uint32_t CR2;    /* 0x04 - Control register 2 */
    volatile uint32_t SMCR;   /* 0x08 - Slave mode control register */
    volatile uint32_t DIER;   /* 0x0C - DMA/Interrupt enable register */
    volatile uint32_t SR;     /* 0x10 - Status register */
    volatile uint32_t EGR;    /* 0x14 - Event generation register */
    volatile uint32_t CCMR1;  /* 0x18 - Capture/compare mode register 1 */
    volatile uint32_t CCMR2;  /* 0x1C - Capture/compare mode register 2 */
    volatile uint32_t CCER;   /* 0x20 - Capture/compare enable register */
    volatile uint32_t CNT;    /* 0x24 - Counter */
    volatile uint32_t PSC;    /* 0x28 - Prescaler */
    volatile uint32_t ARR;    /* 0x2C - Auto-reload register */
    volatile uint32_t RCR;    /* 0x30 - Repetition counter register (TIM1 only) */
    volatile uint32_t CCR[4]; /* 0x34, 0x38, 0x3C, 0x40 - Capture/compare registers */
    volatile uint32_t BDTR;   /* 0x44 - Break and dead-time register (TIM1 only) */
    volatile uint32_t DCR;    /* 0x48 - DMA control register */
    volatile uint32_t DMAR;   /* 0x4C - DMA address for full transfer */
} tim_reg_t;

/* TIM base addresses */
#define TIM1_BASE   0x40010000  /* APB2 */
#define TIM2_BASE   0x40000000  /* APB1 */
#define TIM3_BASE   0x40000400  /* APB1 */
#define TIM4_BASE   0x40000800  /* APB1 */

/* RCC registers */
#define RCC_BASE    0x40023800
#define RCC_APB1ENR (*(volatile uint32_t *)(RCC_BASE + 0x40))
#define RCC_APB2ENR (*(volatile uint32_t *)(RCC_BASE + 0x44))

/* RCC enable bits */
#define RCC_APB1ENR_TIM2EN  (1U << 0)
#define RCC_APB1ENR_TIM3EN  (1U << 1)
#define RCC_APB1ENR_TIM4EN  (1U << 2)
#define RCC_APB2ENR_TIM1EN  (1U << 0)

/* TIM register bit definitions */
#define TIM_CR1_CEN     (1U << 0)   /* Counter enable */
#define TIM_CR1_ARPE    (1U << 7)   /* Auto-reload preload enable */

#define TIM_CCMR1_OC1PE (1U << 3)   /* Output compare 1 preload enable */
#define TIM_CCMR1_OC2PE (1U << 11)  /* Output compare 2 preload enable */
#define TIM_CCMR2_OC3PE (1U << 3)   /* Output compare 3 preload enable */
#define TIM_CCMR2_OC4PE (1U << 11)  /* Output compare 4 preload enable */

#define TIM_CCER_CC1E   (1U << 0)   /* Capture/compare 1 output enable */
#define TIM_CCER_CC2E   (1U << 4)   /* Capture/compare 2 output enable */
#define TIM_CCER_CC3E   (1U << 8)   /* Capture/compare 3 output enable */
#define TIM_CCER_CC4E   (1U << 12)  /* Capture/compare 4 output enable */

#define TIM_BDTR_MOE    (1U << 15)  /* Main output enable (TIM1 only) */

#define TIM_EGR_UG      (1U << 0)   /* Update generation */

/* PWM Mode 1: OCxM = 110 (bits 6:4 for CH1, 14:12 for CH2) */
#define TIM_CCMR1_OC1M_PWM1 (6U << 4)
#define TIM_CCMR1_OC2M_PWM1 (6U << 12)
#define TIM_CCMR2_OC3M_PWM1 (6U << 4)
#define TIM_CCMR2_OC4M_PWM1 (6U << 12)

/* Clock frequencies */
#define APB1_CLOCK_HZ   50000000U   /* 50 MHz */
#define APB2_CLOCK_HZ   100000000U  /* 100 MHz */

/* TIM instance array */
static tim_reg_t *const tim_instances[] = {
    (tim_reg_t *)TIM1_BASE,
    (tim_reg_t *)TIM2_BASE,
    (tim_reg_t *)TIM3_BASE,
    (tim_reg_t *)TIM4_BASE
};

/* PWM state for each TIM/channel */
static pwm_state_t pwm_states[4][4] = {{{0}}};

/* Enable TIM clock */
static void tim_clock_enable(tim_t tim)
{
    switch (tim) {
        case TIM1:
            RCC_APB2ENR |= RCC_APB2ENR_TIM1EN;
            break;
        case TIM2:
            RCC_APB1ENR |= RCC_APB1ENR_TIM2EN;
            break;
        case TIM3:
            RCC_APB1ENR |= RCC_APB1ENR_TIM3EN;
            break;
        case TIM4:
            RCC_APB1ENR |= RCC_APB1ENR_TIM4EN;
            break;
    }
}

/* Get TIM clock frequency */
static uint32_t tim_get_clock_freq(tim_t tim)
{
    if (tim == TIM1) {
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

/* Get CCMR register for channel */
static volatile uint32_t *get_ccmr_reg(tim_reg_t *tim, tim_channel_t channel)
{
    if (channel <= TIM_CH2) {
        return &tim->CCMR1;
    }
    return &tim->CCMR2;
}

/* Get CCMR bit position for channel */
static uint8_t get_ccmr_bitpos(tim_channel_t channel)
{
    if (channel == TIM_CH1 || channel == TIM_CH3) {
        return 4;  /* OCxM starts at bit 4 */
    }
    return 12;     /* OCxM starts at bit 12 */
}

/* Get CCER bit for channel */
static uint16_t get_ccer_bit(tim_channel_t channel)
{
    switch (channel) {
        case TIM_CH1: return TIM_CCER_CC1E;
        case TIM_CH2: return TIM_CCER_CC2E;
        case TIM_CH3: return TIM_CCER_CC3E;
        case TIM_CH4: return TIM_CCER_CC4E;
    }
    return 0;
}

/* Get preload enable bit for channel */
static uint16_t get_preload_bit(tim_channel_t channel)
{
    switch (channel) {
        case TIM_CH1: return TIM_CCMR1_OC1PE;
        case TIM_CH2: return TIM_CCMR1_OC2PE;
        case TIM_CH3: return TIM_CCMR2_OC3PE;
        case TIM_CH4: return TIM_CCMR2_OC4PE;
    }
    return 0;
}

/* Get PWM mode value for channel */
static uint32_t get_pwm_mode(tim_channel_t channel)
{
    switch (channel) {
        case TIM_CH1: return TIM_CCMR1_OC1M_PWM1;
        case TIM_CH2: return TIM_CCMR1_OC2M_PWM1;
        case TIM_CH3: return TIM_CCMR2_OC3M_PWM1;
        case TIM_CH4: return TIM_CCMR2_OC4M_PWM1;
    }
    return 0;
}

hal_status_t pwm_init(const pwm_init_t *init)
{
    tim_reg_t *tim;
    uint32_t tim_clk;
    uint16_t psc, arr;
    volatile uint32_t *ccmr;
    uint8_t bitpos;

    if (init == NULL) {
        return HAL_ERROR;
    }

    if (init->tim > TIM4 || init->channel > TIM_CH4) {
        return HAL_ERROR;
    }

    if (init->frequency_hz == 0 || init->frequency_hz > 20000) {
        return HAL_ERROR;
    }

    if (init->duty_1000 > 1000) {
        return HAL_ERROR;
    }

    tim = tim_instances[init->tim];
    tim_clk = tim_get_clock_freq(init->tim);

    /* Enable TIM clock */
    tim_clock_enable(init->tim);

    /* Calculate prescaler and auto-reload */
    tim_calc_period(tim_clk, init->frequency_hz, &psc, &arr);

    /* Configure prescaler */
    tim->PSC = psc;

    /* Configure auto-reload */
    tim->ARR = arr;

    /* Store state */
    pwm_states[init->tim][init->channel].current_arr = arr;
    pwm_states[init->tim][init->channel].current_duty = init->duty_1000;

    /* Configure PWM mode 1 for the channel */
    ccmr = get_ccmr_reg(tim, init->channel);
    bitpos = get_ccmr_bitpos(init->channel);

    /* Clear OCxM bits and set PWM mode 1 (110) */
    *ccmr &= ~(7U << bitpos);
    *ccmr |= get_pwm_mode(init->channel);

    /* Enable output compare preload */
    *ccmr |= get_preload_bit(init->channel);

    /* Set initial duty cycle */
    pwm_set_duty(init->tim, init->channel, init->duty_1000);

    /* Enable auto-reload preload */
    tim->CR1 |= TIM_CR1_ARPE;

    /* Generate update event to load registers */
    tim->EGR = TIM_EGR_UG;

    return HAL_OK;
}

hal_status_t pwm_set_duty(tim_t tim, tim_channel_t channel, uint16_t duty_1000)
{
    tim_reg_t *tim_reg;
    uint32_t ccr_value;
    uint32_t arr;

    if (tim > TIM4 || channel > TIM_CH4) {
        return HAL_ERROR;
    }

    if (duty_1000 > 1000) {
        return HAL_ERROR;
    }

    tim_reg = tim_instances[tim];
    arr = pwm_states[tim][channel].current_arr;

    /* Calculate CCR value: CCR = (ARR + 1) * duty / 1000 */
    ccr_value = ((arr + 1) * duty_1000) / 1000;

    /* Update CCR register */
    tim_reg->CCR[channel] = ccr_value;

    /* Store current duty */
    pwm_states[tim][channel].current_duty = duty_1000;

    return HAL_OK;
}

hal_status_t pwm_set_frequency(tim_t tim, uint32_t frequency_hz)
{
    tim_reg_t *tim_reg;
    uint32_t tim_clk;
    uint16_t psc, arr;
    uint32_t old_arr;
    uint32_t new_ccr;
    int i;

    if (tim > TIM4) {
        return HAL_ERROR;
    }

    if (frequency_hz == 0 || frequency_hz > 20000) {
        return HAL_ERROR;
    }

    tim_reg = tim_instances[tim];
    tim_clk = tim_get_clock_freq(tim);
    old_arr = tim_reg->ARR;

    /* Calculate new prescaler and auto-reload */
    tim_calc_period(tim_clk, frequency_hz, &psc, &arr);

    /* Disable counter temporarily */
    tim_reg->CR1 &= ~TIM_CR1_CEN;

    /* Update prescaler */
    tim_reg->PSC = psc;

    /* Update auto-reload */
    tim_reg->ARR = arr;

    /* Update CCR values proportionally for all channels */
    for (i = 0; i < 4; i++) {
        if (pwm_states[tim][i].current_duty > 0) {
            /* Scale CCR proportionally: new_CCR = old_CCR * new_ARR / old_ARR */
            new_ccr = (tim_reg->CCR[i] * (arr + 1)) / (old_arr + 1);
            if (new_ccr > arr) {
                new_ccr = arr;
            }
            tim_reg->CCR[i] = new_ccr;
        }
        pwm_states[tim][i].current_arr = arr;
    }

    /* Generate update event */
    tim_reg->EGR = TIM_EGR_UG;

    /* Re-enable counter if any channel is running */
    for (i = 0; i < 4; i++) {
        if (pwm_states[tim][i].is_running) {
            tim_reg->CR1 |= TIM_CR1_CEN;
            break;
        }
    }

    return HAL_OK;
}

hal_status_t pwm_start(tim_t tim, tim_channel_t channel)
{
    tim_reg_t *tim_reg;
    uint16_t ccer_bit;

    if (tim > TIM4 || channel > TIM_CH4) {
        return HAL_ERROR;
    }

    tim_reg = tim_instances[tim];
    ccer_bit = get_ccer_bit(channel);

    /* Enable output for this channel */
    tim_reg->CCER |= ccer_bit;

    /* For TIM1, enable main output */
    if (tim == TIM1) {
        tim_reg->BDTR |= TIM_BDTR_MOE;
    }

    /* Enable counter */
    tim_reg->CR1 |= TIM_CR1_CEN;

    /* Mark as running */
    pwm_states[tim][channel].is_running = 1;

    return HAL_OK;
}

hal_status_t pwm_stop(tim_t tim, tim_channel_t channel)
{
    tim_reg_t *tim_reg;
    uint16_t ccer_bit;
    int i;
    int any_running = 0;

    if (tim > TIM4 || channel > TIM_CH4) {
        return HAL_ERROR;
    }

    tim_reg = tim_instances[tim];
    ccer_bit = get_ccer_bit(channel);

    /* Disable output for this channel */
    tim_reg->CCER &= ~ccer_bit;

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
        tim_reg->CR1 &= ~TIM_CR1_CEN;

        /* For TIM1, disable main output */
        if (tim == TIM1) {
            tim_reg->BDTR &= ~TIM_BDTR_MOE;
        }
    }

    return HAL_OK;
}
