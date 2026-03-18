#ifndef PWM_H
#define PWM_H

#include "hal_common.h"

/* TIM peripheral definitions */
typedef enum {
    TIM1 = 0,
    TIM2 = 1,
    TIM3 = 2,
    TIM4 = 3
} tim_t;

/* TIM channel definitions */
typedef enum {
    TIM_CH1 = 0,
    TIM_CH2 = 1,
    TIM_CH3 = 2,
    TIM_CH4 = 3
} tim_channel_t;

/* PWM initialization structure */
typedef struct {
    tim_t tim;              /* TIM peripheral selection */
    tim_channel_t channel;  /* TIM channel selection */
    uint32_t frequency_hz;  /* PWM frequency in Hz */
    uint16_t duty_1000;     /* Duty cycle: 0-1000 = 0-100.0% */
} pwm_init_t;

/* PWM state structure (internal use) */
typedef struct {
    uint32_t current_arr;   /* Current auto-reload value */
    uint16_t current_duty;  /* Current duty cycle */
    uint8_t is_running;     /* PWM output state */
} pwm_state_t;

/* Function declarations */
hal_status_t pwm_init(const pwm_init_t *init);
hal_status_t pwm_set_duty(tim_t tim, tim_channel_t channel, uint16_t duty_1000);
hal_status_t pwm_set_frequency(tim_t tim, uint32_t frequency_hz);
hal_status_t pwm_start(tim_t tim, tim_channel_t channel);
hal_status_t pwm_stop(tim_t tim, tim_channel_t channel);

#endif /* PWM_H */
