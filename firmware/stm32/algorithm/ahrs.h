/**
 * @file ahrs.h
 * @brief AHRS (Attitude Heading Reference System) 姿态解算算法头文件
 *
 * @note 实现基于Mahony互补滤波器的姿态解算
 *       融合加速度计、陀螺仪、磁力计数据
 *       输出四元数和欧拉角 (Roll/Pitch/Yaw)
 *
 * @author Drone Control System
 * @version 1.0
 */

#ifndef AHRS_H
#define AHRS_H

#include "sensor_calibration.h"
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

#define AHRS_DEFAULT_SAMPLE_RATE    1000.0f     /**< 默认采样率 (Hz) */
#define AHRS_DEFAULT_KP             2.0f        /**< 默认比例增益 */
#define AHRS_DEFAULT_KI             0.005f      /**< 默认积分增益 */
#define AHRS_KP_MAG                 2.0f        /**< 磁力计比例增益 */
#define AHRS_KI_MAG                 0.005f      /**< 磁力计积分增益 */

/* 欧拉角限制 */
#define AHRS_ROLL_MAX               90.0f       /**< 最大横滚角 (度) */
#define AHRS_PITCH_MAX              90.0f       /**< 最大俯仰角 (度) */

/* 数据有效性阈值 */
#define AHRS_ACCEL_NORM_MIN         0.7f        /**< 加速度模长最小值 (g) */
#define AHRS_ACCEL_NORM_MAX         1.3f        /**< 加速度模长最大值 (g) */
#define AHRS_MAG_NORM_MIN           0.3f        /**< 磁场模长最小值 (相对值) */
#define AHRS_MAG_NORM_MAX           3.0f        /**< 磁场模长最大值 (相对值) */

/* ============================================================================
 * 数据类型定义
 * ============================================================================ */

/**
 * @brief 四元数结构体
 * @note 表示为 q = [w, x, y, z]，其中w为实部
 */
typedef struct {
    float w;    /**< 实部 */
    float x;    /**< i分量 */
    float y;    /**< j分量 */
    float z;    /**< k分量 */
} quaternion_t;

/**
 * @brief 欧拉角结构体 (单位: 弧度)
 * @note 按照Z-Y-X顺序 (Yaw-Pitch-Roll)
 */
typedef struct {
    float roll;     /**< 横滚角 (绕X轴) */
    float pitch;    /**< 俯仰角 (绕Y轴) */
    float yaw;      /**< 偏航角 (绕Z轴) */
} euler_angle_t;

/**
 * @brief 旋转矩阵 (3x3)
 * @note 从机体坐标系到地理坐标系的旋转
 */
typedef struct {
    float m[3][3];
} rotation_matrix_t;

/**
 * @brief Mahony AHRS算法句柄
 */
typedef struct {
    /* 四元数状态 */
    quaternion_t q;         /**< 当前姿态四元数 */

    /* 积分误差项 */
    vec3f_t integral_error; /**< 积分误差累计 */

    /* 增益参数 */
    float kp;               /**< 比例增益 */
    float ki;               /**< 积分增益 */
    float kp_mag;           /**< 磁力计比例增益 */
    float ki_mag;           /**< 磁力计积分增益 */

    /* 采样参数 */
    float sample_rate;      /**< 采样率 (Hz) */
    float dt;               /**< 采样周期 (s) */

    /* 参考向量 */
    vec3f_t mag_ref;        /**< 归一化参考磁场向量 (地理坐标系) */

    /* 状态标志 */
    bool initialized;       /**< 初始化标志 */
    uint32_t update_count;  /**< 更新计数 */
} ahrs_handle_t;

/**
 * @brief 传感器原始数据输入
 */
typedef struct {
    vec3f_t accel;  /**< 加速度 (m/s^2) */
    vec3f_t gyro;   /**< 角速度 (rad/s) */
    vec3f_t mag;    /**< 磁场 (可选, 归一化值) */
    bool mag_valid; /**< 磁力计数据有效标志 */
} ahrs_input_t;

/* ============================================================================
 * API函数声明 - 初始化和配置
 * ============================================================================ */

/**
 * @brief 初始化AHRS模块
 * @param ahrs AHRS句柄指针
 * @param sample_rate 采样率 (Hz), 推荐100-1000Hz
 * @return HAL_OK成功
 */
hal_status_t ahrs_init(ahrs_handle_t *ahrs, float sample_rate);

/**
 * @brief 反初始化AHRS模块
 * @param ahrs AHRS句柄指针
 */
void ahrs_deinit(ahrs_handle_t *ahrs);

/**
 * @brief 设置Mahony滤波器增益
 * @param ahrs AHRS句柄指针
 * @param kp 比例增益 (推荐0.5-5.0)
 * @param ki 积分增益 (推荐0.0-0.01)
 */
void ahrs_set_gains(ahrs_handle_t *ahrs, float kp, float ki);

/**
 * @brief 设置磁力计增益
 * @param ahrs AHRS句柄指针
 * @param kp 比例增益
 * @param ki 积分增益
 */
void ahrs_set_mag_gains(ahrs_handle_t *ahrs, float kp, float ki);

/**
 * @brief 设置参考磁场方向
 * @param ahrs AHRS句柄指针
 * @param mag_north 地理北向磁场分量 (归一化)
 * @param mag_down 地理下向磁场分量 (归一化)
 * @note 通常磁北沿X轴正方向，磁倾角向下为负
 */
void ahrs_set_mag_reference(ahrs_handle_t *ahrs, float mag_north, float mag_down);

/* ============================================================================
 * API函数声明 - 姿态更新
 * ============================================================================ */

/**
 * @brief AHRS姿态更新 (仅加速度计+陀螺仪)
 * @param ahrs AHRS句柄指针
 * @param gyro 角速度 (rad/s), 已校准
 * @param accel 加速度 (m/s^2), 已校准
 * @return HAL_OK成功
 * @note 6轴模式，无偏航参考，会漂移
 */
hal_status_t ahrs_update_6axis(ahrs_handle_t *ahrs,
                                const vec3f_t *gyro,
                                const vec3f_t *accel);

/**
 * @brief AHRS姿态更新 (9轴：加速度计+陀螺仪+磁力计)
 * @param ahrs AHRS句柄指针
 * @param gyro 角速度 (rad/s), 已校准
 * @param accel 加速度 (m/s^2), 已校准
 * @param mag 磁场强度 (归一化值), 已校准
 * @return HAL_OK成功
 * @note 9轴模式，使用磁力计提供偏航参考
 */
hal_status_t ahrs_update_9axis(ahrs_handle_t *ahrs,
                                const vec3f_t *gyro,
                                const vec3f_t *accel,
                                const vec3f_t *mag);

/**
 * @brief AHRS姿态更新 (统一接口)
 * @param ahrs AHRS句柄指针
 * @param input 传感器输入数据
 * @return HAL_OK成功
 */
hal_status_t ahrs_update(ahrs_handle_t *ahrs, const ahrs_input_t *input);

/* ============================================================================
 * API函数声明 - 姿态输出
 * ============================================================================ */

/**
 * @brief 获取当前四元数
 * @param ahrs AHRS句柄指针
 * @param q 四元数输出
 * @return true有效
 */
bool ahrs_get_quaternion(const ahrs_handle_t *ahrs, quaternion_t *q);

/**
 * @brief 获取当前欧拉角 (弧度)
 * @param ahrs AHRS句柄指针
 * @param euler 欧拉角输出
 * @return true有效
 */
bool ahrs_get_euler_rad(const ahrs_handle_t *ahrs, euler_angle_t *euler);

/**
 * @brief 获取当前欧拉角 (角度)
 * @param ahrs AHRS句柄指针
 * @param euler 欧拉角输出
 * @return true有效
 */
bool ahrs_get_euler_deg(const ahrs_handle_t *ahrs, euler_angle_t *euler);

/**
 * @brief 获取旋转矩阵
 * @param ahrs AHRS句柄指针
 * @param R 旋转矩阵输出 (机体->地理)
 * @return true有效
 */
bool ahrs_get_rotation_matrix(const ahrs_handle_t *ahrs, rotation_matrix_t *R);

/**
 * @brief 获取重力向量 (机体坐标系)
 * @param ahrs AHRS句柄指针
 * @param gravity 重力方向输出 (归一化)
 * @return true有效
 */
bool ahrs_get_gravity(const ahrs_handle_t *ahrs, vec3f_t *gravity);

/* ============================================================================
 * API函数声明 - 工具函数
 * ============================================================================ */

/**
 * @brief 四元数归一化
 * @param q 四元数
 */
void quat_normalize(quaternion_t *q);

/**
 * @brief 四元数乘法: out = a * b
 * @param out 结果
 * @param a 左乘数
 * @param b 右乘数
 */
void quat_multiply(quaternion_t *out, const quaternion_t *a, const quaternion_t *b);

/**
 * @brief 四元数共轭
 * @param out 输出
 * @param q 输入
 */
void quat_conjugate(quaternion_t *out, const quaternion_t *q);

/**
 * @brief 用四元数旋转向量: out = q * v * q^-1
 * @param out 旋转后的向量
 * @param q 四元数
 * @param v 输入向量
 */
void quat_rotate_vector(vec3f_t *out, const quaternion_t *q, const vec3f_t *v);

/**
 * @brief 欧拉角转四元数 (Z-Y-X顺序)
 * @param q 输出四元数
 * @param euler 输入欧拉角 (弧度)
 */
void euler_to_quaternion(quaternion_t *q, const euler_angle_t *euler);

/**
 * @brief 四元数转欧拉角 (Z-Y-X顺序)
 * @param euler 输出欧拉角 (弧度)
 * @param q 输入四元数
 */
void quaternion_to_euler(euler_angle_t *euler, const quaternion_t *q);

/**
 * @brief 检查加速度数据有效性
 * @param accel 加速度数据
 * @return true有效
 */
bool ahrs_accel_valid(const vec3f_t *accel);

/**
 * @brief 检查磁力计数据有效性
 * @param mag 磁力计数据
 * @return true有效
 */
bool ahrs_mag_valid(const vec3f_t *mag);

/**
 * @brief 重置姿态到水平
 * @param ahrs AHRS句柄指针
 * @note 将Roll和Pitch重置为0，Yaw保持不变
 */
void ahrs_reset_to_level(ahrs_handle_t *ahrs);

/**
 * @brief 获取AHRS运行状态
 * @param ahrs AHRS句柄指针
 * @return 更新计数
 */
uint32_t ahrs_get_update_count(const ahrs_handle_t *ahrs);

#ifdef __cplusplus
}
#endif

#endif /* AHRS_H */
