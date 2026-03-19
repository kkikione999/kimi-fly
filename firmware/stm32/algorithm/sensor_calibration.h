/**
 * @file sensor_calibration.h
 * @brief 传感器校准算法头文件
 *
 * @note 提供加速度计、陀螺仪、磁力计的校准功能
 *       - 零偏校准 (Zero-offset/Bias)
 *       - 尺度因子校准 (Scale factor)
 *       - 硬铁/软铁补偿 (Hard/Soft iron compensation for magnetometer)
 *       - 温度补偿接口
 *
 * @author Drone Control System
 * @version 1.0
 */

#ifndef SENSOR_CALIBRATION_H
#define SENSOR_CALIBRATION_H

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

#define CALIBRATION_SAMPLES_DEFAULT 1000    /**< 默认校准采样次数 */
#define CALIBRATION_SAMPLES_FAST    100     /**< 快速校准采样次数 */
#define CALIBRATION_TEMP_BINS       8       /**< 温度分档数量 */

/* 地球磁场参考值 (用于磁力计校准验证) */
#define EARTH_MAG_FIELD_MIN         25.0f   /**< 最小地磁场强度 (uT) */
#define EARTH_MAG_FIELD_MAX         65.0f   /**< 最大地磁场强度 (uT) */
#define EARTH_MAG_FIELD_NOMINAL     50.0f   /**< 标称地磁场强度 (uT) */

/* 重力加速度参考值 */
#define GRAVITY_STANDARD            9.80665f    /**< 标准重力加速度 (m/s^2) */

/* ============================================================================
 * 数据类型定义
 * ============================================================================ */

/**
 * @brief 三轴传感器数据 (float)
 */
typedef struct {
    float x;
    float y;
    float z;
} vec3f_t;

/**
 * @brief 三轴传感器数据 (int16原始值)
 */
typedef struct {
    int16_t x;
    int16_t y;
    int16_t z;
} vec3i_t;

/**
 * @brief 校准矩阵 (3x3)
 * @note 用于软铁补偿和尺度因子校准
 */
typedef struct {
    float m[3][3];
} cal_matrix3_t;

/**
 * @brief 加速度计校准参数
 */
typedef struct {
    vec3f_t bias;           /**< 零偏 (m/s^2) */
    vec3f_t scale;          /**< 尺度因子 (无量纲) */
    float   temp_coeff[3];  /**< 温度系数 */
    bool    calibrated;     /**< 校准标志 */
} accel_cal_t;

/**
 * @brief 陀螺仪校准参数
 */
typedef struct {
    vec3f_t bias;           /**< 零偏 (rad/s) */
    float   temp_coeff[3];  /**< 温度系数 */
    bool    calibrated;     /**< 校准标志 */
} gyro_cal_t;

/**
 * @brief 磁力计校准参数 (硬铁+软铁)
 */
typedef struct {
    vec3f_t     hard_iron;      /**< 硬铁补偿 (uT) */
    cal_matrix3_t soft_iron;    /**< 软铁补偿矩阵 */
    float       field_strength; /**< 参考磁场强度 (uT) */
    bool        calibrated;     /**< 校准标志 */
} mag_cal_t;

/**
 * @brief 综合传感器校准句柄
 */
typedef struct {
    accel_cal_t accel;      /**< 加速度计校准 */
    gyro_cal_t  gyro;       /**< 陀螺仪校准 */
    mag_cal_t   mag;        /**< 磁力计校准 */
} sensor_cal_handle_t;

/**
 * @brief 校准状态
 */
typedef enum {
    CAL_STATE_IDLE = 0,         /**< 空闲 */
    CAL_STATE_ACCEL_6POS,       /**< 加速度计6面校准中 */
    CAL_STATE_GYRO_STATIONARY,  /**< 陀螺仪静止校准中 */
    CAL_STATE_MAG_ROTATION,     /**< 磁力计旋转校准中 */
    CAL_STATE_COMPLETE,         /**< 校准完成 */
    CAL_STATE_ERROR             /**< 校准错误 */
} cal_state_t;

/* ============================================================================
 * API函数声明 - 初始化与管理
 * ============================================================================ */

/**
 * @brief 初始化校准模块
 * @param cal 校准句柄指针
 * @return HAL_OK成功
 */
hal_status_t cal_init(sensor_cal_handle_t *cal);

/**
 * @brief 加载校准参数 (从非易失存储)
 * @param cal 校准句柄指针
 * @param data 存储的数据指针
 * @param size 数据大小
 * @return HAL_OK成功
 */
hal_status_t cal_load_params(sensor_cal_handle_t *cal, const uint8_t *data, uint16_t size);

/**
 * @brief 保存校准参数 (到非易失存储)
 * @param cal 校准句柄指针
 * @param data 存储缓冲区指针
 * @param size 缓冲区大小
 * @return 实际写入大小
 */
uint16_t cal_save_params(const sensor_cal_handle_t *cal, uint8_t *data, uint16_t size);

/* ============================================================================
 * API函数声明 - 校准过程
 * ============================================================================ */

/**
 * @brief 开始加速度计6面校准
 * @param cal 校准句柄指针
 * @return HAL_OK成功
 * @note 需要将传感器依次放置于6个静止面
 */
hal_status_t cal_accel_6pos_start(sensor_cal_handle_t *cal);

/**
 * @brief 添加加速度计6面校准采样点
 * @param cal 校准句柄指针
 * @param raw 原始加速度数据 (当前面朝上的读数)
 * @return true采样完成, false继续采样
 */
bool cal_accel_6pos_sample(sensor_cal_handle_t *cal, const vec3i_t *raw);

/**
 * @brief 开始陀螺仪静止校准
 * @param cal 校准句柄指针
 * @param sample_count 采样次数 (推荐1000)
 * @return HAL_OK成功
 */
hal_status_t cal_gyro_stationary_start(sensor_cal_handle_t *cal, uint16_t sample_count);

/**
 * @brief 添加陀螺仪静止校准样本
 * @param cal 校准句柄指针
 * @param raw 原始陀螺仪数据
 * @return true采样完成, false继续采样
 */
bool cal_gyro_stationary_sample(sensor_cal_handle_t *cal, const vec3i_t *raw);

/**
 * @brief 开始磁力计旋转校准
 * @param cal 校准句柄指针
 * @param sample_count 采样次数 (推荐2000+)
 * @return HAL_OK成功
 * @note 需要在多个方向旋转传感器以覆盖整个球面
 */
hal_status_t cal_mag_rotation_start(sensor_cal_handle_t *cal, uint16_t sample_count);

/**
 * @brief 添加磁力计旋转校准样本
 * @param cal 校准句柄指针
 * @param raw 原始磁力计数据
 * @return true采样完成, false继续采样
 */
bool cal_mag_rotation_sample(sensor_cal_handle_t *cal, const vec3i_t *raw);

/* ============================================================================
 * API函数声明 - 应用校准
 * ============================================================================ */

/**
 * @brief 应用加速度计校准
 * @param cal 校准句柄指针
 * @param raw 原始数据输入
 * @param out 校准后数据输出 (m/s^2)
 */
void cal_accel_apply(const sensor_cal_handle_t *cal, const vec3i_t *raw, vec3f_t *out);

/**
 * @brief 应用陀螺仪校准
 * @param cal 校准句柄指针
 * @param raw 原始数据输入
 * @param out 校准后数据输出 (rad/s)
 */
void cal_gyro_apply(const sensor_cal_handle_t *cal, const vec3i_t *raw, vec3f_t *out);

/**
 * @brief 应用磁力计校准
 * @param cal 校准句柄指针
 * @param raw 原始数据输入
 * @param out 校准后数据输出 (uT或Gauss)
 */
void cal_mag_apply(const sensor_cal_handle_t *cal, const vec3i_t *raw, vec3f_t *out);

/* ============================================================================
 * API函数声明 - 快速校准/复位
 * ============================================================================ */

/**
 * @brief 快速零偏校准 (假设传感器水平静止)
 * @param cal 校准句柄指针
 * @param accel_raw 当前加速度原始值
 * @param gyro_raw 当前陀螺仪原始值
 * @return HAL_OK成功
 * @note 适用于简单场景，假设Z轴指向重力方向
 */
hal_status_t cal_quick_bias(sensor_cal_handle_t *cal,
                            const vec3i_t *accel_raw,
                            const vec3i_t *gyro_raw);

/**
 * @brief 复位所有校准参数为默认值
 * @param cal 校准句柄指针
 */
void cal_reset_to_default(sensor_cal_handle_t *cal);

/**
 * @brief 检查校准是否有效
 * @param cal 校准句柄指针
 * @return true校准有效
 */
bool cal_is_valid(const sensor_cal_handle_t *cal);

/* ============================================================================
 * 内联辅助函数
 * ============================================================================ */

/**
 * @brief 向量赋值
 */
static inline void vec3f_set(vec3f_t *v, float x, float y, float z)
{
    v->x = x; v->y = y; v->z = z;
}

/**
 * @brief 向量归零
 */
static inline void vec3f_zero(vec3f_t *v)
{
    v->x = v->y = v->z = 0.0f;
}

/**
 * @brief 向量加法
 */
static inline void vec3f_add(vec3f_t *out, const vec3f_t *a, const vec3f_t *b)
{
    out->x = a->x + b->x;
    out->y = a->y + b->y;
    out->z = a->z + b->z;
}

/**
 * @brief 向量减法
 */
static inline void vec3f_sub(vec3f_t *out, const vec3f_t *a, const vec3f_t *b)
{
    out->x = a->x - b->x;
    out->y = a->y - b->y;
    out->z = a->z - b->z;
}

/**
 * @brief 向量数乘
 */
static inline void vec3f_scale(vec3f_t *out, const vec3f_t *v, float s)
{
    out->x = v->x * s;
    out->y = v->y * s;
    out->z = v->z * s;
}

/**
 * @brief 向量点积
 */
static inline float vec3f_dot(const vec3f_t *a, const vec3f_t *b)
{
    return a->x * b->x + a->y * b->y + a->z * b->z;
}

/**
 * @brief 向量叉积
 */
static inline void vec3f_cross(vec3f_t *out, const vec3f_t *a, const vec3f_t *b)
{
    out->x = a->y * b->z - a->z * b->y;
    out->y = a->z * b->x - a->x * b->z;
    out->z = a->x * b->y - a->y * b->x;
}

/**
 * @brief 向量模长
 */
static inline float vec3f_norm(const vec3f_t *v)
{
    float x = v->x, y = v->y, z = v->z;
    return sqrtf(x*x + y*y + z*z);
}

/**
 * @brief 向量归一化
 */
static inline void vec3f_normalize(vec3f_t *v)
{
    float norm = vec3f_norm(v);
    if (norm > 1e-6f) {
        v->x /= norm;
        v->y /= norm;
        v->z /= norm;
    }
}

#ifdef __cplusplus
}
#endif

#endif /* SENSOR_CALIBRATION_H */
