/**
 * @file pid_controller.c
 * @brief PID控制器实现
 *
 * @note 实现位置式和增量式PID控制器
 *       包含抗积分饱和、微分滤波、输出限幅等功能
 *
 * @author Drone Control System
 * @version 1.0
 */

#include "pid_controller.h"
#include <string.h>

/* ============================================================================
 * 静态函数声明
 * ============================================================================ */

static float pid_update_position(pid_controller_t *pid, float error, float dt);
static float pid_update_incremental(pid_controller_t *pid, float error, float dt);

/* ============================================================================
 * 单PID控制器API实现
 * ============================================================================ */

hal_status_t pid_init(pid_controller_t *pid, pid_mode_t mode)
{
    if (pid == NULL) {
        return HAL_ERROR;
    }

    /* 清空所有状态 */
    memset(pid, 0, sizeof(pid_controller_t));

    /* 设置模式 */
    pid->mode = mode;

    /* 设置默认值 */
    pid->gains.kp = 0.0f;
    pid->gains.ki = 0.0f;
    pid->gains.kd = 0.0f;

    pid->integral_limit = PID_DEFAULT_INTEGRAL_LIMIT;
    pid->output_limit = PID_DEFAULT_OUTPUT_LIMIT;
    pid->d_filter_coef = PID_DEFAULT_D_FILTER_COEF;

    pid->last_error = 0.0f;
    pid->last_derivative = 0.0f;
    pid->last_input = 0.0f;
    pid->integral = 0.0f;

    pid->update_count = 0;
    pid->initialized = true;

    return HAL_OK;
}

void pid_deinit(pid_controller_t *pid)
{
    if (pid == NULL) {
        return;
    }

    pid->initialized = false;
    pid_reset(pid);
}

void pid_set_gains(pid_controller_t *pid, float kp, float ki, float kd)
{
    if (pid == NULL || !pid->initialized) {
        return;
    }

    pid->gains.kp = kp;
    pid->gains.ki = ki;
    pid->gains.kd = kd;
}

void pid_get_gains(const pid_controller_t *pid, pid_gains_t *gains)
{
    if (pid == NULL || gains == NULL || !pid->initialized) {
        return;
    }

    *gains = pid->gains;
}

void pid_set_integral_limit(pid_controller_t *pid, float limit)
{
    if (pid == NULL || !pid->initialized || limit <= 0.0f) {
        return;
    }

    pid->integral_limit = limit;

    /* 立即限制当前积分值 */
    pid->integral = pid_constrain(pid->integral, limit);
}

void pid_set_output_limit(pid_controller_t *pid, float limit)
{
    if (pid == NULL || !pid->initialized || limit <= 0.0f) {
        return;
    }

    pid->output_limit = limit;
}

void pid_set_d_filter(pid_controller_t *pid, float coef)
{
    if (pid == NULL || !pid->initialized) {
        return;
    }

    /* 限制滤波系数在有效范围内 [0.001, 1.0] */
    if (coef < 0.001f) coef = 0.001f;
    if (coef > 1.0f) coef = 1.0f;

    pid->d_filter_coef = coef;
}

void pid_reset(pid_controller_t *pid)
{
    if (pid == NULL) {
        return;
    }

    pid->integral = 0.0f;
    pid->last_error = 0.0f;
    pid->last_derivative = 0.0f;
    pid->last_input = 0.0f;
    pid->update_count = 0;
}

float pid_update(pid_controller_t *pid, float setpoint, float input, float dt)
{
    if (pid == NULL || !pid->initialized || dt <= 0.0f) {
        return 0.0f;
    }

    float error = setpoint - input;
    float output = 0.0f;

    /* 根据模式选择计算方式 */
    switch (pid->mode) {
        case PID_MODE_POSITION:
            output = pid_update_position(pid, error, dt);
            break;

        case PID_MODE_INCREMENTAL:
            output = pid_update_incremental(pid, error, dt);
            break;

        default:
            output = 0.0f;
            break;
    }

    pid->update_count++;
    return output;
}

float pid_update_with_feedforward(pid_controller_t *pid, float setpoint,
                                   float input, float feedforward, float dt)
{
    if (pid == NULL || !pid->initialized || dt <= 0.0f) {
        return 0.0f;
    }

    /* 计算PID输出 */
    float pid_output = pid_update(pid, setpoint, input, dt);

    /* 添加前馈 */
    float output = pid_output + feedforward;

    /* 输出限幅 */
    return pid_constrain(output, pid->output_limit);
}

/* ============================================================================
 * 飞行控制PID集合API实现
 * ============================================================================ */

hal_status_t flight_pid_init(flight_pid_set_t *pid_set)
{
    if (pid_set == NULL) {
        return HAL_ERROR;
    }

    memset(pid_set, 0, sizeof(flight_pid_set_t));

    /* 初始化所有7个PID通道为位置式模式 */
    for (int i = 0; i < PID_CHANNEL_COUNT; i++) {
        hal_status_t status = pid_init(&pid_set->channels[i], PID_MODE_POSITION);
        if (status != HAL_OK) {
            return status;
        }
    }

    pid_set->initialized = true;
    return HAL_OK;
}

void flight_pid_set_defaults(flight_pid_set_t *pid_set)
{
    if (pid_set == NULL || !pid_set->initialized) {
        return;
    }

    /* 角度环PID参数 - 响应较慢，积分小 */
    /* 横滚角度环 */
    pid_set_gains(&pid_set->channels[PID_ROLL_ANGLE],  4.0f, 0.05f, 0.0f);
    pid_set_integral_limit(&pid_set->channels[PID_ROLL_ANGLE], 20.0f);
    pid_set_output_limit(&pid_set->channels[PID_ROLL_ANGLE], 200.0f);

    /* 俯仰角度环 */
    pid_set_gains(&pid_set->channels[PID_PITCH_ANGLE], 4.0f, 0.05f, 0.0f);
    pid_set_integral_limit(&pid_set->channels[PID_PITCH_ANGLE], 20.0f);
    pid_set_output_limit(&pid_set->channels[PID_PITCH_ANGLE], 200.0f);

    /* 偏航角度环 */
    pid_set_gains(&pid_set->channels[PID_YAW_ANGLE],  3.0f, 0.02f, 0.0f);
    pid_set_integral_limit(&pid_set->channels[PID_YAW_ANGLE], 30.0f);
    pid_set_output_limit(&pid_set->channels[PID_YAW_ANGLE], 150.0f);

    /* 角速度环PID参数 - 响应快，D项抑制震荡 */
    /* 横滚角速度环 */
    pid_set_gains(&pid_set->channels[PID_ROLL_RATE],  0.15f, 0.3f, 0.001f);
    pid_set_integral_limit(&pid_set->channels[PID_ROLL_RATE], 200.0f);
    pid_set_output_limit(&pid_set->channels[PID_ROLL_RATE], 400.0f);
    pid_set_d_filter(&pid_set->channels[PID_ROLL_RATE], 0.1f);

    /* 俯仰角速度环 */
    pid_set_gains(&pid_set->channels[PID_PITCH_RATE], 0.15f, 0.3f, 0.001f);
    pid_set_integral_limit(&pid_set->channels[PID_PITCH_RATE], 200.0f);
    pid_set_output_limit(&pid_set->channels[PID_PITCH_RATE], 400.0f);
    pid_set_d_filter(&pid_set->channels[PID_PITCH_RATE], 0.1f);

    /* 偏航角速度环 */
    pid_set_gains(&pid_set->channels[PID_YAW_RATE],   0.2f, 0.4f, 0.0f);
    pid_set_integral_limit(&pid_set->channels[PID_YAW_RATE], 150.0f);
    pid_set_output_limit(&pid_set->channels[PID_YAW_RATE], 300.0f);

    /* 高度环PID参数 */
    pid_set_gains(&pid_set->channels[PID_ALTITUDE],   2.0f, 0.1f, 0.5f);
    pid_set_integral_limit(&pid_set->channels[PID_ALTITUDE], 100.0f);
    pid_set_output_limit(&pid_set->channels[PID_ALTITUDE], 300.0f);
}

void flight_pid_reset_all(flight_pid_set_t *pid_set)
{
    if (pid_set == NULL || !pid_set->initialized) {
        return;
    }

    for (int i = 0; i < PID_CHANNEL_COUNT; i++) {
        pid_reset(&pid_set->channels[i]);
    }
}

pid_controller_t* flight_pid_get_channel(flight_pid_set_t *pid_set,
                                          pid_channel_id_t channel)
{
    if (pid_set == NULL || !pid_set->initialized) {
        return NULL;
    }

    if (channel < 0 || channel >= PID_CHANNEL_COUNT) {
        return NULL;
    }

    return &pid_set->channels[channel];
}

float pid_cascade_update(pid_controller_t *outer_pid,
                          pid_controller_t *inner_pid,
                          float angle_setpoint,
                          float angle_input,
                          float rate_input,
                          float dt)
{
    if (outer_pid == NULL || inner_pid == NULL) {
        return 0.0f;
    }

    if (!outer_pid->initialized || !inner_pid->initialized) {
        return 0.0f;
    }

    /* 外环：角度环计算，输出作为角速度目标 */
    float rate_setpoint = pid_update(outer_pid, angle_setpoint, angle_input, dt);

    /* 内环：角速度环计算，输出作为最终控制量 */
    float output = pid_update(inner_pid, rate_setpoint, rate_input, dt);

    return output;
}

/* ============================================================================
 * 静态函数实现
 * ============================================================================ */

/**
 * @brief 位置式PID更新
 * @note output = Kp*error + Ki*integral + Kd*derivative
 */
static float pid_update_position(pid_controller_t *pid, float error, float dt)
{
    /* 比例项 */
    float p_term = pid->gains.kp * error;

    /* 积分项 (带抗饱和) */
    pid->integral += error * dt;
    pid->integral = pid_constrain(pid->integral, pid->integral_limit);
    float i_term = pid->gains.ki * pid->integral;

    /* 微分项 (基于测量值的变化，而非误差，避免setpoint突变导致冲击) */
    float derivative = (error - pid->last_error) / dt;

    /* 一阶低通滤波 */
    pid->last_derivative = pid_lpf(derivative, pid->last_derivative, pid->d_filter_coef);

    float d_term = pid->gains.kd * pid->last_derivative;

    /* 保存当前误差 */
    pid->last_error = error;

    /* 计算输出 */
    float output = p_term + i_term + d_term;

    /* 输出限幅 */
    output = pid_constrain(output, pid->output_limit);

    /* 条件积分：仅在输出未饱和时积分 (更严格的抗饱和) */
    float output_before_limit = p_term + i_term + d_term;
    if (output_before_limit > pid->output_limit || output_before_limit < -pid->output_limit) {
        /* 输出饱和，回退积分 */
        pid->integral -= error * dt;
        pid->integral = pid_constrain(pid->integral, pid->integral_limit);
    }

    return output;
}

/**
 * @brief 增量式PID更新
 * @note delta_output = Kp*(error-last_error) + Ki*error + Kd*(error-2*last_error+last_last_error)
 *       实际使用基于测量的微分避免噪声放大
 */
static float pid_update_incremental(pid_controller_t *pid, float error, float dt)
{
    static float prev_error = 0.0f;  /* 上上次误差 */

    /* 比例增量 */
    float p_delta = pid->gains.kp * (error - pid->last_error);

    /* 积分增量 */
    float i_delta = pid->gains.ki * error * dt;

    /* 微分增量 */
    float d_delta = pid->gains.kd * (error - 2.0f * pid->last_error + prev_error) / dt;

    /* 更新历史误差 */
    prev_error = pid->last_error;
    pid->last_error = error;

    /* 计算输出增量 */
    float delta_output = p_delta + i_delta + d_delta;

    /* 累加输出 (增量式PID的输出是连续的) */
    float output = pid->last_input + delta_output;

    /* 输出限幅 */
    output = pid_constrain(output, pid->output_limit);

    /* 保存当前输出 */
    pid->last_input = output;

    return output;
}
