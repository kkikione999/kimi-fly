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

/* ============================================================================
 * Motor Control API
 * Hardware: 4x 空心杯电机 @ 42kHz PWM
 * ============================================================================ */

typedef enum {
    MOTOR_1 = 0,    /* TIM1_CH1 (PA8) */
    MOTOR_2 = 1,    /* TIM1_CH4 (PA11) */
    MOTOR_3 = 2,    /* TIM3_CH4 (PB1) */
    MOTOR_4 = 3     /* TIM2_CH3 (PB10) */
} motor_id_t;

/**
 * @brief 初始化所有电机PWM
 * @param frequency_hz PWM频率(推荐42000=42kHz)
 * @return HAL状态
 * @note 初始化后电机处于停止状态，需调用motor_start()启动
 */
hal_status_t motor_init_all(uint32_t frequency_hz);

/**
 * @brief 设置电机转速
 * @param motor 电机ID
 * @param throttle 油门值 0-1000 (对应0-100%)
 * @return HAL状态
 */
hal_status_t motor_set_throttle(motor_id_t motor, uint16_t throttle);

/**
 * @brief 设置所有电机相同油门
 * @param throttle 油门值 0-1000
 * @return HAL状态
 */
hal_status_t motor_set_all_throttle(uint16_t throttle);

/**
 * @brief 启动指定电机
 * @param motor 电机ID
 * @return HAL状态
 */
hal_status_t motor_start(motor_id_t motor);

/**
 * @brief 启动所有电机
 * @return HAL状态
 */
hal_status_t motor_start_all(void);

/**
 * @brief 停止指定电机
 * @param motor 电机ID
 * @return HAL状态
 */
hal_status_t motor_stop(motor_id_t motor);

/**
 * @brief 停止所有电机
 * @return HAL状态
 */
hal_status_t motor_stop_all(void);

/**
 * @brief 紧急停止所有电机(油门归零并停止)
 * @return HAL状态
 */
hal_status_t motor_emergency_stop(void);

#endif /* PWM_H */
