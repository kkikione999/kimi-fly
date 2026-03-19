/**
 * @file pid_controller.h
 * @brief PID控制器头文件
 *
 * @note 实现通用PID控制器，支持位置式和增量式
 *       包含抗积分饱和、微分滤波、输出限幅等功能
 *
 * @author Drone Control System
 * @version 1.0
 */

#ifndef PID_CONTROLLER_H
#define PID_CONTROLLER_H

#include "../hal/hal_common.h"
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * 配置常量
 * ============================================================================ */

#define PID_DEFAULT_INTEGRAL_LIMIT    1000.0f    /**积分限幅默认值 */
#define PID_DEFAULT_OUTPUT_LIMIT      1000.0f    /**输出限幅默认值 */
#define PID_DEFAULT_D_FILTER_COEF     0.1f       /**微分滤波系数 (0-1) */

/* PID通道数量 */
#define PID_CHANNELS_MAX              8          /**最大PID通道数 */

/* ============================================================================
 * 数据类型定义
 * ============================================================================ */

/**
 * @brief PID控制模式
 */
typedef enum {
    PID_MODE_POSITION = 0,      /**位置式PID (推荐) */
    PID_MODE_INCREMENTAL = 1    /**增量式PID */
} pid_mode_t;

/**
 * @brief PID增益参数
 */
typedef struct {
    float kp;       /**比例增益 */
    float ki;       /**积分增益 */
    float kd;       /**微分增益 */
} pid_gains_t;

/**
 * @brief PID控制器状态
 */
typedef struct {
    pid_gains_t gains;          /**PID增益 */

    /**内部状态 */
    float integral;             /**积分累计 */
    float last_error;           /**上次误差 */
    float last_derivative;      /**上次微分项 (用于滤波) */
    float last_input;           /**上次输入值 */

    /**限制参数 */
    float integral_limit;       /**积分限幅 */
    float output_limit;         /**输出限幅 */
    float d_filter_coef;        /**微分滤波系数 (0-1, 越小滤波越强) */

    /**控制模式 */
    pid_mode_t mode;            /**PID模式 */

    /**状态标志 */
    bool initialized;           /**初始化标志 */
    uint32_t update_count;      /**更新计数 */
} pid_controller_t;

/**
 * @brief 多轴PID管理器 (用于飞行控制)
 */
typedef enum {
    PID_ROLL_ANGLE = 0,         /**横滚角度环 */
    PID_ROLL_RATE = 1,          /**横滚角速度环 */
    PID_PITCH_ANGLE = 2,        /**俯仰角度环 */
    PID_PITCH_RATE = 3,         /**俯仰角速度环 */
    PID_YAW_ANGLE = 4,          /**偏航角度环 */
    PID_YAW_RATE = 5,           /**偏航角速度环 */
    PID_ALTITUDE = 6,           /**< 高度环 */
    PID_CHANNEL_COUNT = 7       /**通道总数 (不含ALTITUDE) */
} pid_channel_id_t;

/**
 * @brief 飞行控制PID集合
 */
typedef struct {
    pid_controller_t channels[PID_CHANNEL_COUNT];   /**PID通道数组 */
    bool initialized;
} flight_pid_set_t;

/* ============================================================================
 * API函数声明 - 单PID控制器
 * ============================================================================ */

/**
 * @brief 初始化PID控制器
 * @param pid PID控制器指针
 * @param mode PID模式 (位置式/增量式)
 * @return HAL_OK成功
 */
hal_status_t pid_init(pid_controller_t *pid, pid_mode_t mode);

/**
 * @brief 反初始化PID控制器
 * @param pid PID控制器指针
 */
void pid_deinit(pid_controller_t *pid);

/**
 * @brief 设置PID增益
 * @param pid PID控制器指针
 * @param kp 比例增益
 * @param ki 积分增益
 * @param kd 微分增益
 */
void pid_set_gains(pid_controller_t *pid, float kp, float ki, float kd);

/**
 * @brief 获取PID增益
 * @param pid PID控制器指针
 * @param gains 增益输出结构体
 */
void pid_get_gains(const pid_controller_t *pid, pid_gains_t *gains);

/**
 * @brief 设置积分限幅
 * @param pid PID控制器指针
 * @param limit 限幅值 (必须为正)
 */
void pid_set_integral_limit(pid_controller_t *pid, float limit);

/**
 * @brief 设置输出限幅
 * @param pid PID控制器指针
 * @param limit 限幅值 (必须为正)
 */
void pid_set_output_limit(pid_controller_t *pid, float limit);

/**
 * @brief 设置微分滤波系数
 * @param pid PID控制器指针
 * @param coef 滤波系数 (0-1, 越小滤波越强)
 */
void pid_set_d_filter(pid_controller_t *pid, float coef);

/**
 * @brief 复位PID控制器 (清空积分和误差历史)
 * @param pid PID控制器指针
 */
void pid_reset(pid_controller_t *pid);

/**
 * @brief PID更新计算 (主函数)
 * @param pid PID控制器指针
 * @param setpoint 目标值
 * @param input 当前测量值
 * @param dt 时间间隔 (秒)
 * @return PID输出值
 */
float pid_update(pid_controller_t *pid, float setpoint, float input, float dt);

/**
 * @brief PID更新计算 (带前馈)
 * @param pid PID控制器指针
 * @param setpoint 目标值
 * @param input 当前测量值
 * @param feedforward 前馈值
 * @param dt 时间间隔 (秒)
 * @return PID输出值
 */
float pid_update_with_feedforward(pid_controller_t *pid, float setpoint,
                                   float input, float feedforward, float dt);

/* ============================================================================
 * API函数声明 - 飞行控制PID集合
 * ============================================================================ */

/**
 * @brief 初始化飞行控制PID集合
 * @param pid_set PID集合指针
 * @return HAL_OK成功
 * @note 初始化所有7个PID通道为位置式模式
 */
hal_status_t flight_pid_init(flight_pid_set_t *pid_set);

/**
 * @brief 设置默认飞行PID参数
 * @param pid_set PID集合指针
 * @note 设置一组经验值作为初始参数
 */
void flight_pid_set_defaults(flight_pid_set_t *pid_set);

/**
 * @brief 复位所有飞行PID
 * @param pid_set PID集合指针
 */
void flight_pid_reset_all(flight_pid_set_t *pid_set);

/**
 * @brief 获取指定PID通道
 * @param pid_set PID集合指针
 * @param channel 通道ID
 * @return PID控制器指针 (NULL如果ID无效)
 */
pid_controller_t* flight_pid_get_channel(flight_pid_set_t *pid_set,
                                          pid_channel_id_t channel);

/**
 * @brief 级联PID更新 (角度环+角速度环)
 * @param outer_pid 外环PID (角度环)
 * @param inner_pid 内环PID (角速度环)
 * @param angle_setpoint 角度目标值 (度)
 * @param angle_input 当前角度 (度)
 * @param rate_input 当前角速度 (度/秒)
 * @param dt 时间间隔
 * @return 内环PID输出 (电机控制量)
 */
float pid_cascade_update(pid_controller_t *outer_pid,
                          pid_controller_t *inner_pid,
                          float angle_setpoint,
                          float angle_input,
                          float rate_input,
                          float dt);

/* ============================================================================
 * 工具函数
 * ============================================================================ */

/**
 * @brief 限幅函数
 * @param value 输入值
 * @param limit 限幅值 (正负限幅相同)
 * @return 限幅后的值
 */
static inline float pid_constrain(float value, float limit)
{
    if (value > limit) return limit;
    if (value < -limit) return -limit;
    return value;
}

/**
 * @brief 一阶低通滤波
 * @param input 当前输入
 * @param last_output 上次输出
 * @param alpha 滤波系数 (0-1)
 * @return 滤波后输出
 */
static inline float pid_lpf(float input, float last_output, float alpha)
{
    return last_output + alpha * (input - last_output);
}

/**
 * @brief 死区处理
 * @param value 输入值
 * @param deadband 死区大小
 * @return 处理后的值
 */
static inline float pid_deadband(float value, float deadband)
{
    if (value > deadband) return value - deadband;
    if (value < -deadband) return value + deadband;
    return 0.0f;
}

/**
 * @brief 检查PID是否初始化
 * @param pid PID控制器指针
 * @return true已初始化
 */
static inline bool pid_is_initialized(const pid_controller_t *pid)
{
    return (pid != NULL && pid->initialized);
}

#ifdef __cplusplus
}
#endif

#endif /* PID_CONTROLLER_H */
