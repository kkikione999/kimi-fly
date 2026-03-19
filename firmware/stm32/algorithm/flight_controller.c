/**
 * @file flight_controller.c
 * @brief 飞行控制核心实现
 *
 * @note 实现飞行控制状态机、级联PID控制和电机混控
 *
 * @author Drone Control System
 * @version 1.0
 */

#include "flight_controller.h"
#include <math.h>
#include <string.h>

/* ============================================================================
 * 静态常量
 * ============================================================================ */

/* X型四旋翼电机布局 (俯视):
 *      前
 *   M1   M2
 *     \ /
 *     / \
 *   M4   M3
 *
 * M1: 前左, CW  (顺时针旋转)
 * M2: 前右, CCW (逆时针旋转)
 * M3: 后右, CW  (顺时针旋转)
 * M4: 后左, CCW (逆时针旋转)
 */

/* ============================================================================
 * 静态函数声明
 * ============================================================================ */

static void update_attitude(flight_controller_t *fc);
static void calculate_setpoints(flight_controller_t *fc);
static void run_attitude_controller(flight_controller_t *fc, float *roll_rate_sp, float *pitch_rate_sp);
static void run_rate_controller(flight_controller_t *fc, float roll_rate_sp, float pitch_rate_sp, float *roll_out, float *pitch_out, float *yaw_out);
static void update_motors_with_outputs(flight_controller_t *fc, float roll_out, float pitch_out, float yaw_out);
static void reset_controllers(flight_controller_t *fc);

/* ============================================================================
 * API实现 - 初始化和配置
 * ============================================================================ */

hal_status_t flight_controller_init(flight_controller_t *fc, const flight_controller_init_t *params)
{
    if (fc == NULL || params == NULL) {
        return HAL_ERROR;
    }

    /* 清空结构体 */
    memset(fc, 0, sizeof(flight_controller_t));

    /* 初始化AHRS */
    hal_status_t status = ahrs_init(&fc->ahrs, params->sample_rate > 0 ? params->sample_rate : 1000.0f);
    if (status != HAL_OK) {
        return status;
    }

    /* 初始化PID集合 */
    status = flight_pid_init(&fc->pid_set);
    if (status != HAL_OK) {
        ahrs_deinit(&fc->ahrs);
        return status;
    }

    /* 设置默认PID参数 */
    flight_pid_set_defaults(&fc->pid_set);

    /* 初始状态 */
    fc->mode = FLIGHT_MODE_DISARMED;
    fc->state = FLIGHT_STATE_IDLE;
    fc->initialized = true;
    fc->motors_armed = false;

    return HAL_OK;
}

void flight_controller_deinit(flight_controller_t *fc)
{
    if (fc == NULL) {
        return;
    }

    /* 先锁定电机 */
    flight_controller_disarm(fc);

    /* 反初始化子系统 */
    ahrs_deinit(&fc->ahrs);
    flight_pid_reset_all(&fc->pid_set);

    fc->initialized = false;
}

bool flight_controller_arm(flight_controller_t *fc)
{
    if (fc == NULL || !fc->initialized) {
        return false;
    }

    /* 安全检查: 油门必须在最低位置 */
    if (fc->rc_input.throttle > 0.05f) {
        return false;
    }

    /* 复位所有PID控制器 */
    reset_controllers(fc);

    fc->motors_armed = true;
    fc->mode = FLIGHT_MODE_ARMED;
    fc->state = FLIGHT_STATE_IDLE;

    return true;
}

void flight_controller_disarm(flight_controller_t *fc)
{
    if (fc == NULL) {
        return;
    }

    fc->motors_armed = false;
    fc->mode = FLIGHT_MODE_DISARMED;
    fc->state = FLIGHT_STATE_IDLE;

    /* 停止所有电机 */
    fc->motors.motor1 = 0;
    fc->motors.motor2 = 0;
    fc->motors.motor3 = 0;
    fc->motors.motor4 = 0;

    /* 复位PID */
    if (fc->initialized) {
        reset_controllers(fc);
    }
}

bool flight_controller_is_armed(const flight_controller_t *fc)
{
    if (fc == NULL) {
        return false;
    }
    return fc->motors_armed;
}

hal_status_t flight_controller_set_mode(flight_controller_t *fc, flight_mode_t mode)
{
    if (fc == NULL || !fc->initialized) {
        return HAL_ERROR;
    }

    if (mode == FLIGHT_MODE_DISARMED) {
        flight_controller_disarm(fc);
        return HAL_OK;
    }

    if (!fc->motors_armed) {
        return HAL_ERROR;
    }

    fc->mode = mode;
    return HAL_OK;
}

flight_mode_t flight_controller_get_mode(const flight_controller_t *fc)
{
    if (fc == NULL) {
        return FLIGHT_MODE_DISARMED;
    }
    return fc->mode;
}

/* ============================================================================
 * API实现 - 控制输入
 * ============================================================================ */

void flight_controller_set_rc_input(flight_controller_t *fc, const rc_command_t *cmd)
{
    if (fc == NULL || cmd == NULL) {
        return;
    }

    fc->rc_input = *cmd;

    /* 处理解锁/锁定命令 */
    if (cmd->armed && !fc->motors_armed) {
        flight_controller_arm(fc);
    } else if (!cmd->armed && fc->motors_armed) {
        flight_controller_disarm(fc);
    }

    /* 转换RC输入为控制设定值 */
    calculate_setpoints(fc);
}

void flight_controller_set_setpoint(flight_controller_t *fc, const control_setpoint_t *setpoint)
{
    if (fc == NULL || setpoint == NULL) {
        return;
    }

    fc->setpoint = *setpoint;
}

/* ============================================================================
 * API实现 - 传感器输入
 * ============================================================================ */

void flight_controller_update_imu(flight_controller_t *fc, const vec3f_t *accel, const vec3f_t *gyro)
{
    if (fc == NULL || accel == NULL || gyro == NULL) {
        return;
    }

    fc->accel = *accel;
    fc->gyro = *gyro;
}

void flight_controller_update_mag(flight_controller_t *fc, const vec3f_t *mag)
{
    if (fc == NULL || mag == NULL) {
        return;
    }

    fc->mag = *mag;
    fc->mag_valid = true;
}

/* ============================================================================
 * API实现 - 主控制循环
 * ============================================================================ */

hal_status_t flight_controller_update(flight_controller_t *fc)
{
    if (fc == NULL || !fc->initialized) {
        return HAL_ERROR;
    }

    float roll_rate_sp = 0.0f, pitch_rate_sp = 0.0f;
    float roll_out = 0.0f, pitch_out = 0.0f, yaw_out = 0.0f;

    /* 1. 更新姿态解算 */
    update_attitude(fc);

    /* 2. 根据模式运行控制律 */
    if (fc->mode == FLIGHT_MODE_STABILIZE) {
        /* 自稳模式: 角度环 + 角速度环 */
        run_attitude_controller(fc, &roll_rate_sp, &pitch_rate_sp);
        run_rate_controller(fc, roll_rate_sp, pitch_rate_sp, &roll_out, &pitch_out, &yaw_out);
    }
    else if (fc->mode == FLIGHT_MODE_ACRO) {
        /* 特技模式: 直接角速度控制 */
        roll_rate_sp = rc_to_rate(fc->rc_input.roll, ROLL_RATE_LIMIT);
        pitch_rate_sp = rc_to_rate(fc->rc_input.pitch, PITCH_RATE_LIMIT);
        float yaw_rate_sp = rc_to_rate(fc->rc_input.yaw, YAW_RATE_LIMIT);
        run_rate_controller(fc, roll_rate_sp, pitch_rate_sp, &roll_out, &pitch_out, &yaw_out);
        /* 偏航直接给前馈 */
        yaw_out = yaw_rate_sp * 0.1f;
    }
    else if (fc->mode == FLIGHT_MODE_ARMED) {
        /* 怠速模式: 复位控制器，只给怠速油门 */
        reset_controllers(fc);
    }

    /* 3. 电机输出 */
    if (fc->motors_armed) {
        update_motors_with_outputs(fc, roll_out, pitch_out, yaw_out);
    } else {
        /* 未解锁，电机停止 */
        fc->motors.motor1 = 0;
        fc->motors.motor2 = 0;
        fc->motors.motor3 = 0;
        fc->motors.motor4 = 0;
    }

    fc->update_count++;
    return HAL_OK;
}

void flight_controller_get_motors(const flight_controller_t *fc, motor_outputs_t *outputs)
{
    if (fc == NULL || outputs == NULL) {
        return;
    }

    *outputs = fc->motors;
}

/* ============================================================================
 * API实现 - 状态获取
 * ============================================================================ */

bool flight_controller_get_attitude(const flight_controller_t *fc, euler_angle_t *attitude)
{
    if (fc == NULL || attitude == NULL || !fc->initialized) {
        return false;
    }

    *attitude = fc->attitude;
    return true;
}

bool flight_controller_get_gyro(const flight_controller_t *fc, vec3f_t *gyro)
{
    if (fc == NULL || gyro == NULL) {
        return false;
    }

    *gyro = fc->gyro;
    return true;
}

void flight_controller_get_stats(const flight_controller_t *fc, uint32_t *update_count, uint32_t *error_count)
{
    if (fc == NULL) {
        return;
    }

    if (update_count != NULL) {
        *update_count = fc->update_count;
    }
    if (error_count != NULL) {
        *error_count = fc->error_count;
    }
}

/* ============================================================================
 * 工具函数实现
 * ============================================================================ */

float rc_to_angle(float rc_input, float max_angle)
{
    /* 限制输入范围 */
    if (rc_input > 1.0f) rc_input = 1.0f;
    if (rc_input < -1.0f) rc_input = -1.0f;

    /* 小死区处理 */
    if (fabsf(rc_input) < 0.02f) {
        rc_input = 0.0f;
    }

    return rc_input * max_angle;
}

float rc_to_rate(float rc_input, float max_rate)
{
    /* 限制输入范围 */
    if (rc_input > 1.0f) rc_input = 1.0f;
    if (rc_input < -1.0f) rc_input = -1.0f;

    /* 小死区处理 */
    if (fabsf(rc_input) < 0.02f) {
        rc_input = 0.0f;
    }

    return rc_input * max_rate;
}

void mixer_quad_x(float throttle, float roll, float pitch, float yaw, motor_outputs_t *outputs)
{
    if (outputs == NULL) {
        return;
    }

    /* X型混控:
     * M1 (前左, CW)  = throttle + roll - pitch - yaw
     * M2 (前右, CCW) = throttle - roll - pitch + yaw
     * M3 (后右, CW)  = throttle - roll + pitch - yaw
     * M4 (后左, CCW) = throttle + roll + pitch + yaw
     */

    float m1 = throttle + roll - pitch - yaw;
    float m2 = throttle - roll - pitch + yaw;
    float m3 = throttle - roll + pitch - yaw;
    float m4 = throttle + roll + pitch + yaw;

    /* 限制在有效范围内 */
    if (m1 < 0) m1 = 0; if (m1 > 1) m1 = 1;
    if (m2 < 0) m2 = 0; if (m2 > 1) m2 = 1;
    if (m3 < 0) m3 = 0; if (m3 > 1) m3 = 1;
    if (m4 < 0) m4 = 0; if (m4 > 1) m4 = 1;

    /* 转换为PWM值 (0-999) */
    outputs->motor1 = (uint16_t)(m1 * MOTOR_MAX_THROTTLE);
    outputs->motor2 = (uint16_t)(m2 * MOTOR_MAX_THROTTLE);
    outputs->motor3 = (uint16_t)(m3 * MOTOR_MAX_THROTTLE);
    outputs->motor4 = (uint16_t)(m4 * MOTOR_MAX_THROTTLE);
}

/* ============================================================================
 * 静态函数实现
 * ============================================================================ */

/**
 * @brief 更新姿态解算
 */
static void update_attitude(flight_controller_t *fc)
{
    if (fc->mag_valid) {
        /* 9轴更新 */
        ahrs_update_9axis(&fc->ahrs, &fc->gyro, &fc->accel, &fc->mag);
    } else {
        /* 6轴更新 */
        ahrs_update_6axis(&fc->ahrs, &fc->gyro, &fc->accel);
    }

    /* 获取欧拉角 (度) */
    ahrs_get_euler_deg(&fc->ahrs, &fc->attitude);

    /* 转换为弧度供PID使用 */
    fc->attitude_rad.x = fc->attitude.roll * (float)M_PI / 180.0f;
    fc->attitude_rad.y = fc->attitude.pitch * (float)M_PI / 180.0f;
    fc->attitude_rad.z = fc->attitude.yaw * (float)M_PI / 180.0f;
}

/**
 * @brief 计算控制设定值
 */
static void calculate_setpoints(flight_controller_t *fc)
{
    /* 角度设定值 */
    fc->setpoint.roll_angle = rc_to_angle(fc->rc_input.roll, MAX_ANGLE_SETPOINT);
    fc->setpoint.pitch_angle = rc_to_angle(fc->rc_input.pitch, MAX_ANGLE_SETPOINT);

    /* 偏航角速度设定值 */
    fc->setpoint.yaw_rate = rc_to_rate(fc->rc_input.yaw, MAX_YAW_RATE_SETPOINT);

    /* 油门设定值 (直接传递) */
    fc->setpoint.throttle = fc->rc_input.throttle;
}

/**
 * @brief 运行角度环控制器
 */
static void run_attitude_controller(flight_controller_t *fc, float *roll_rate_sp, float *pitch_rate_sp)
{
    pid_controller_t *roll_angle_pid = flight_pid_get_channel(&fc->pid_set, PID_ROLL_ANGLE);
    pid_controller_t *pitch_angle_pid = flight_pid_get_channel(&fc->pid_set, PID_PITCH_ANGLE);

    /* 外环: 角度 -> 角速度设定值 */
    *roll_rate_sp = pid_update(roll_angle_pid, fc->setpoint.roll_angle, fc->attitude.roll, FLIGHT_CTRL_DT);
    *pitch_rate_sp = pid_update(pitch_angle_pid, fc->setpoint.pitch_angle, fc->attitude.pitch, FLIGHT_CTRL_DT);

    /* 限制角速度设定值 */
    if (*roll_rate_sp > ROLL_RATE_LIMIT) *roll_rate_sp = ROLL_RATE_LIMIT;
    if (*roll_rate_sp < -ROLL_RATE_LIMIT) *roll_rate_sp = -ROLL_RATE_LIMIT;
    if (*pitch_rate_sp > PITCH_RATE_LIMIT) *pitch_rate_sp = PITCH_RATE_LIMIT;
    if (*pitch_rate_sp < -PITCH_RATE_LIMIT) *pitch_rate_sp = -PITCH_RATE_LIMIT;
}

/**
 * @brief 运行角速度环控制器
 */
static void run_rate_controller(flight_controller_t *fc, float roll_rate_sp, float pitch_rate_sp,
                                 float *roll_out, float *pitch_out, float *yaw_out)
{
    pid_controller_t *roll_rate_pid = flight_pid_get_channel(&fc->pid_set, PID_ROLL_RATE);
    pid_controller_t *pitch_rate_pid = flight_pid_get_channel(&fc->pid_set, PID_PITCH_RATE);
    pid_controller_t *yaw_rate_pid = flight_pid_get_channel(&fc->pid_set, PID_YAW_RATE);

    /* 当前角速度 (度/秒) */
    float roll_rate = fc->gyro.x * 180.0f / (float)M_PI;
    float pitch_rate = fc->gyro.y * 180.0f / (float)M_PI;
    float yaw_rate = fc->gyro.z * 180.0f / (float)M_PI;

    /* 内环: 角速度 -> 控制输出 */
    *roll_out = pid_update(roll_rate_pid, roll_rate_sp, roll_rate, FLIGHT_CTRL_DT);
    *pitch_out = pid_update(pitch_rate_pid, pitch_rate_sp, pitch_rate, FLIGHT_CTRL_DT);
    *yaw_out = pid_update(yaw_rate_pid, fc->setpoint.yaw_rate, yaw_rate, FLIGHT_CTRL_DT);
}

/**
 * @brief 使用预计算的控制输出更新电机
 */
static void update_motors_with_outputs(flight_controller_t *fc, float roll_out, float pitch_out, float yaw_out)
{
    float throttle = fc->setpoint.throttle;

    /* 怠速模式处理 */
    if (fc->mode == FLIGHT_MODE_ARMED || throttle <= 0.02f) {
        throttle = (float)MOTOR_IDLE_THROTTLE / MOTOR_MAX_THROTTLE;
        roll_out = pitch_out = yaw_out = 0.0f;
    }

    /* 归一化控制输出到合适范围 */
    roll_out *= 0.001f;   /* 缩放以适应混控 */
    pitch_out *= 0.001f;
    yaw_out *= 0.001f;

    /* 混控计算 */
    mixer_quad_x(throttle, roll_out, pitch_out, yaw_out, &fc->motors);
}

/**
 * @brief 复位所有控制器
 */
static void reset_controllers(flight_controller_t *fc)
{
    flight_pid_reset_all(&fc->pid_set);
}
