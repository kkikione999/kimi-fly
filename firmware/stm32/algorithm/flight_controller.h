/**
 * @file flight_controller.h
 * @brief 飞行控制核心头文件
 *
 * @note 实现飞行控制状态机、级联PID控制和电机混控
 *       整合AHRS姿态解算和PID控制器
 *
 * @author Drone Control System
 * @version 1.0
 */

#ifndef FLIGHT_CONTROLLER_H
#define FLIGHT_CONTROLLER_H

#include "pid_controller.h"
#include "ahrs.h"
#include "../hal/pwm.h"
#include "../hal/hal_common.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * 配置常量
 * ============================================================================ */

#define FLIGHT_CTRL_RATE_HZ         1000    /**< 控制频率 (Hz) */
#define FLIGHT_CTRL_DT_MS           1       /**< 控制周期 (ms) */
#define FLIGHT_CTRL_DT              0.001f  /**< 控制周期 (s) */

#define MOTOR_MIN_THROTTLE          0       /**< 最小油门 (PWM) */
#define MOTOR_MAX_THROTTLE          999     /**< 最大油门 (PWM) */
#define MOTOR_IDLE_THROTTLE         50      /**< 怠速油门 (解锁后最小值) */

#define MAX_ANGLE_SETPOINT          30.0f   /**< 最大角度设定值 (度) */
#define MAX_YAW_RATE_SETPOINT       120.0f  /**< 最大偏航角速度 (度/秒) */

/* 姿态环输出限制 (作为角速度环设定值) */
#define ROLL_RATE_LIMIT             250.0f  /**< 横滚角速度限制 (度/秒) */
#define PITCH_RATE_LIMIT            250.0f  /**< 俯仰角速度限制 (度/秒) */
#define YAW_RATE_LIMIT              200.0f  /**< 偏航角速度限制 (度/秒) */

/* ============================================================================
 * 数据类型定义
 * ============================================================================ */

/**
 * @brief 飞行模式
 */
typedef enum {
    FLIGHT_MODE_DISARMED = 0,   /**< 未解锁，电机停止 */
    FLIGHT_MODE_ARMED,          /**< 已解锁，怠速 */
    FLIGHT_MODE_STABILIZE,      /**< 自稳模式 */
    FLIGHT_MODE_ACRO,           /**< 特技模式 (角速度直接控制) */
} flight_mode_t;

/**
 * @brief 飞行状态
 */
typedef enum {
    FLIGHT_STATE_IDLE = 0,      /**< 空闲 */
    FLIGHT_STATE_TAKEOFF,       /**< 起飞 */
    FLIGHT_STATE_FLYING,        /**< 飞行中 */
    FLIGHT_STATE_LANDING,       /**< 降落 */
    FLIGHT_STATE_ERROR          /**< 错误状态 */
} flight_state_t;

/**
 * @brief 遥控器/命令输入 (归一化值 -1.0 ~ 1.0)
 */
typedef struct {
    float throttle;     /**< 油门 (0.0 ~ 1.0) */
    float roll;         /**< 横滚 (-1.0 ~ 1.0) */
    float pitch;        /**< 俯仰 (-1.0 ~ 1.0) */
    float yaw;          /**< 偏航 (-1.0 ~ 1.0) */
    bool  armed;        /**< 解锁请求 */
    bool  mode_switch;  /**< 模式切换 */
} rc_command_t;

/**
 * @brief 电机输出
 */
typedef struct {
    uint16_t motor1;    /**< 电机1 (前左, CW) */
    uint16_t motor2;    /**< 电机2 (前右, CCW) */
    uint16_t motor3;    /**< 电机3 (后右, CW) */
    uint16_t motor4;    /**< 电机4 (后左, CCW) */
} motor_outputs_t;

/**
 * @brief 控制设定值
 */
typedef struct {
    float roll_angle;   /**< 横滚角度设定值 (度) */
    float pitch_angle;  /**< 俯仰角度设定值 (度) */
    float yaw_rate;     /**< 偏航角速度设定值 (度/秒) */
    float throttle;     /**< 油门设定值 (0.0-1.0) */
} control_setpoint_t;

/**
 * @brief 飞行控制器句柄
 */
typedef struct {
    /* 子系统 */
    ahrs_handle_t       ahrs;           /**< AHRS姿态解算 */
    flight_pid_set_t    pid_set;        /**< PID控制器集合 */

    /* 状态 */
    flight_mode_t       mode;           /**< 当前飞行模式 */
    flight_state_t      state;          /**< 当前飞行状态 */
    bool                initialized;    /**< 初始化标志 */
    bool                motors_armed;   /**< 电机解锁标志 */

    /* 输入 */
    rc_command_t        rc_input;       /**< 遥控器输入 */
    control_setpoint_t  setpoint;       /**< 控制设定值 */

    /* 输出 */
    motor_outputs_t     motors;         /**< 电机输出 */

    /* 传感器数据 */
    vec3f_t             gyro;           /**< 角速度 (rad/s) */
    vec3f_t             accel;          /**< 加速度 (m/s^2) */
    vec3f_t             mag;            /**< 磁力计 */
    bool                mag_valid;      /**< 磁力计数据有效 */

    /* 姿态反馈 */
    euler_angle_t       attitude;       /**< 当前姿态 (度) */
    vec3f_t             attitude_rad;   /**< 当前姿态 (rad) */

    /* 运行统计 */
    uint32_t            update_count;   /**< 更新计数 */
    uint32_t            error_count;    /**< 错误计数 */
} flight_controller_t;

/**
 * @brief 初始化参数
 */
typedef struct {
    float sample_rate;          /**< AHRS采样率 (Hz) */
    bool  use_mag;              /**< 是否使用磁力计 */
} flight_controller_init_t;

/* ============================================================================
 * API函数声明 - 初始化和配置
 * ============================================================================ */

/**
 * @brief 初始化飞行控制器
 * @param fc 飞行控制器句柄
 * @param params 初始化参数
 * @return HAL_OK成功
 */
hal_status_t flight_controller_init(flight_controller_t *fc,
                                     const flight_controller_init_t *params);

/**
 * @brief 反初始化飞行控制器
 * @param fc 飞行控制器句柄
 */
void flight_controller_deinit(flight_controller_t *fc);

/**
 * @brief 解锁电机
 * @param fc 飞行控制器句柄
 * @return true解锁成功
 */
bool flight_controller_arm(flight_controller_t *fc);

/**
 * @brief 锁定电机
 * @param fc 飞行控制器句柄
 */
void flight_controller_disarm(flight_controller_t *fc);

/**
 * @brief 检查是否已解锁
 * @param fc 飞行控制器句柄
 * @return true已解锁
 */
bool flight_controller_is_armed(const flight_controller_t *fc);

/**
 * @brief 设置飞行模式
 * @param fc 飞行控制器句柄
 * @param mode 目标模式
 * @return HAL_OK成功
 */
hal_status_t flight_controller_set_mode(flight_controller_t *fc, flight_mode_t mode);

/**
 * @brief 获取当前飞行模式
 * @param fc 飞行控制器句柄
 * @return 当前模式
 */
flight_mode_t flight_controller_get_mode(const flight_controller_t *fc);

/* ============================================================================
 * API函数声明 - 控制输入
 * ============================================================================ */

/**
 * @brief 更新遥控器输入
 * @param fc 飞行控制器句柄
 * @param cmd 遥控器命令
 */
void flight_controller_set_rc_input(flight_controller_t *fc, const rc_command_t *cmd);

/**
 * @brief 直接设置控制设定值
 * @param fc 飞行控制器句柄
 * @param setpoint 控制设定值
 */
void flight_controller_set_setpoint(flight_controller_t *fc,
                                     const control_setpoint_t *setpoint);

/* ============================================================================
 * API函数声明 - 传感器输入
 * ============================================================================ */

/**
 * @brief 更新IMU数据 (加速度和陀螺仪)
 * @param fc 飞行控制器句柄
 * @param accel 加速度 (m/s^2)
 * @param gyro 角速度 (rad/s)
 */
void flight_controller_update_imu(flight_controller_t *fc,
                                   const vec3f_t *accel,
                                   const vec3f_t *gyro);

/**
 * @brief 更新磁力计数据
 * @param fc 飞行控制器句柄
 * @param mag 磁场强度 (归一化或原始值)
 */
void flight_controller_update_mag(flight_controller_t *fc, const vec3f_t *mag);

/* ============================================================================
 * API函数声明 - 主控制循环
 * ============================================================================ */

/**
 * @brief 飞行控制主循环 (应在1kHz定时器中调用)
 * @param fc 飞行控制器句柄
 * @return HAL_OK成功
 *
 * @note 调用频率必须与FLIGHT_CTRL_RATE_HZ匹配
 *       内部执行: AHRS更新 -> PID计算 -> 电机混控
 */
hal_status_t flight_controller_update(flight_controller_t *fc);

/**
 * @brief 获取电机输出
 * @param fc 飞行控制器句柄
 * @param outputs 电机输出结构体
 */
void flight_controller_get_motors(const flight_controller_t *fc,
                                   motor_outputs_t *outputs);

/* ============================================================================
 * API函数声明 - 状态获取
 * ============================================================================ */

/**
 * @brief 获取当前姿态 (欧拉角)
 * @param fc 飞行控制器句柄
 * @param attitude 姿态输出 (度)
 * @return true有效
 */
bool flight_controller_get_attitude(const flight_controller_t *fc,
                                     euler_angle_t *attitude);

/**
 * @brief 获取当前角速度
 * @param fc 飞行控制器句柄
 * @param gyro 角速度输出 (rad/s)
 * @return true有效
 */
bool flight_controller_get_gyro(const flight_controller_t *fc, vec3f_t *gyro);

/**
 * @brief 获取控制器运行状态
 * @param fc 飞行控制器句柄
 * @param update_count 更新计数输出 (可为NULL)
 * @param error_count 错误计数输出 (可为NULL)
 */
void flight_controller_get_stats(const flight_controller_t *fc,
                                  uint32_t *update_count,
                                  uint32_t *error_count);

/* ============================================================================
 * 工具函数
 * ============================================================================ */

/**
 * @brief 将归一化RC输入转换为角度设定值
 * @param rc_input 归一化输入 (-1.0 ~ 1.0)
 * @param max_angle 最大角度 (度)
 * @return 角度设定值 (度)
 */
float rc_to_angle(float rc_input, float max_angle);

/**
 * @brief 将归一化RC输入转换为角速度设定值
 * @param rc_input 归一化输入 (-1.0 ~ 1.0)
 * @param max_rate 最大角速度 (度/秒)
 * @return 角速度设定值 (度/秒)
 */
float rc_to_rate(float rc_input, float max_rate);

/**
 * @brief X型四旋翼电机混控
 * @param throttle 油门 (0.0 ~ 1.0)
 * @param roll 横滚控制量
 * @param pitch 俯仰控制量
 * @param yaw 偏航控制量
 * @param outputs 电机输出
 */
void mixer_quad_x(float throttle, float roll, float pitch, float yaw,
                  motor_outputs_t *outputs);

#ifdef __cplusplus
}
#endif

#endif /* FLIGHT_CONTROLLER_H */
