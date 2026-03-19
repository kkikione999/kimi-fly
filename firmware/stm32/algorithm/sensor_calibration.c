/**
 * @file sensor_calibration.c
 * @brief 传感器校准算法实现
 *
 * @note 实现加速度计、陀螺仪、磁力计的校准算法
 *       - 6面校准法（加速度计）
 *       - 静止零偏校准（陀螺仪）
 *       - 椭球拟合（磁力计硬铁/软铁补偿）
 *
 * @author Drone Control System
 * @version 1.0
 */

#include "sensor_calibration.h"
#include <string.h>
#include <math.h>

/* 默认灵敏度（需根据实际传感器调整） */
#define ACCEL_SENSITIVITY_DEFAULT   0.00048828125f  /* ±16g: 16/32768 g/LSB */
#define GYRO_SENSITIVITY_DEFAULT    0.001064225f    /* ±2000dps: 2000/32768 * pi/180 rad/s/LSB */
#define MAG_SENSITIVITY_DEFAULT     0.003f          /* QMC5883P典型值 */

/* 磁力计校准相关 */
#define MAG_MIN_SAMPLES             100
#define MAG_MAX_SAMPLES             5000

/* ============================================================================
 * 私有数据结构
 * ============================================================================ */

typedef struct {
    vec3f_t samples[6];     /* 6面采样 */
    uint8_t face_count;     /* 已采样的面数 */
    uint8_t face_flags;     /* 各面是否已采样 */
} accel_6pos_state_t;

typedef struct {
    vec3f_t sum;
    uint16_t count;
    uint16_t target;
} gyro_stationary_state_t;

typedef struct {
    vec3f_t samples[MAG_MAX_SAMPLES];
    uint16_t count;
    uint16_t target;
    vec3f_t min;
    vec3f_t max;
} mag_rotation_state_t;

static struct {
    accel_6pos_state_t accel;
    gyro_stationary_state_t gyro;
    mag_rotation_state_t mag;
} g_cal_state;

/* ============================================================================
 * 私有函数声明
 * ============================================================================ */

static void matrix3_identity(cal_matrix3_t *m);
static void matrix3_mult_vec(const cal_matrix3_t *m, const vec3f_t *v, vec3f_t *out);
static float inv_sqrt(float x);

/* 磁力计椭球拟合 */
static bool ellipsoid_fit(const vec3f_t *samples, uint16_t count,
                         vec3f_t *center, cal_matrix3_t *transform);

/* ============================================================================
 * 初始化与管理
 * ============================================================================ */

/**
 * @brief 初始化校准模块
 */
hal_status_t cal_init(sensor_cal_handle_t *cal)
{
    if (cal == NULL) {
        return HAL_ERROR;
    }

    /* 清零 */
    memset(cal, 0, sizeof(sensor_cal_handle_t));
    memset(&g_cal_state, 0, sizeof(g_cal_state));

    /* 设置默认值：单位矩阵 */
    cal->accel.scale.x = 1.0f;
    cal->accel.scale.y = 1.0f;
    cal->accel.scale.z = 1.0f;

    matrix3_identity(&cal->mag.soft_iron);

    return HAL_OK;
}

/**
 * @brief 复位所有校准参数为默认值
 */
void cal_reset_to_default(sensor_cal_handle_t *cal)
{
    if (cal == NULL) {
        return;
    }

    /* 加速度计 */
    vec3f_zero(&cal->accel.bias);
    cal->accel.scale.x = 1.0f;
    cal->accel.scale.y = 1.0f;
    cal->accel.scale.z = 1.0f;
    memset(cal->accel.temp_coeff, 0, sizeof(cal->accel.temp_coeff));
    cal->accel.calibrated = false;

    /* 陀螺仪 */
    vec3f_zero(&cal->gyro.bias);
    memset(cal->gyro.temp_coeff, 0, sizeof(cal->gyro.temp_coeff));
    cal->gyro.calibrated = false;

    /* 磁力计 */
    vec3f_zero(&cal->mag.hard_iron);
    matrix3_identity(&cal->mag.soft_iron);
    cal->mag.field_strength = EARTH_MAG_FIELD_NOMINAL;
    cal->mag.calibrated = false;

    /* 重置状态 */
    memset(&g_cal_state, 0, sizeof(g_cal_state));
}

/**
 * @brief 检查校准是否有效
 */
bool cal_is_valid(const sensor_cal_handle_t *cal)
{
    if (cal == NULL) {
        return false;
    }

    /* 至少陀螺仪校准有效即可飞行（加速度计和磁力计可用默认值） */
    return cal->gyro.calibrated;
}

/* ============================================================================
 * 快速校准
 * ============================================================================ */

/**
 * @brief 快速零偏校准（假设传感器水平静止）
 * @note 适用于简单场景，假设Z轴指向重力方向
 */
hal_status_t cal_quick_bias(sensor_cal_handle_t *cal,
                            const vec3i_t *accel_raw,
                            const vec3i_t *gyro_raw)
{
    if (cal == NULL || accel_raw == NULL || gyro_raw == NULL) {
        return HAL_ERROR;
    }

    /* 陀螺仪零偏：假设静止时读数为零 */
    cal->gyro.bias.x = (float)gyro_raw->x * GYRO_SENSITIVITY_DEFAULT;
    cal->gyro.bias.y = (float)gyro_raw->y * GYRO_SENSITIVITY_DEFAULT;
    cal->gyro.bias.z = (float)gyro_raw->z * GYRO_SENSITIVITY_DEFAULT;
    cal->gyro.calibrated = true;

    /* 加速度计：假设水平静止，X=Y=0, Z=+g */
    cal->accel.bias.x = (float)accel_raw->x * ACCEL_SENSITIVITY_DEFAULT;
    cal->accel.bias.y = (float)accel_raw->y * ACCEL_SENSITIVITY_DEFAULT;
    cal->accel.bias.z = ((float)accel_raw->z * ACCEL_SENSITIVITY_DEFAULT) - GRAVITY_STANDARD;
    cal->accel.calibrated = true;

    return HAL_OK;
}

/* ============================================================================
 * 加速度计6面校准
 * ============================================================================ */

/**
 * @brief 开始加速度计6面校准
 */
hal_status_t cal_accel_6pos_start(sensor_cal_handle_t *cal)
{
    if (cal == NULL) {
        return HAL_ERROR;
    }

    memset(&g_cal_state.accel, 0, sizeof(accel_6pos_state_t));
    return HAL_OK;
}

/**
 * @brief 添加加速度计6面校准采样点
 * @return true采样完成, false继续采样
 */
bool cal_accel_6pos_sample(sensor_cal_handle_t *cal, const vec3i_t *raw)
{
    accel_6pos_state_t *state = &g_cal_state.accel;
    vec3f_t sample;

    if (cal == NULL || raw == NULL || state->face_count >= 6) {
        return true; /* 已完成或错误 */
    }

    /* 转换为物理单位 */
    sample.x = (float)raw->x * ACCEL_SENSITIVITY_DEFAULT;
    sample.y = (float)raw->y * ACCEL_SENSITIVITY_DEFAULT;
    sample.z = (float)raw->z * ACCEL_SENSITIVITY_DEFAULT;

    /* 判断当前朝向（基于加速度方向） */
    /* 简化：直接存储，后续处理 */
    state->samples[state->face_count] = sample;
    state->face_count++;

    if (state->face_count >= 6) {
        /* 计算校准参数 */
        /* 6面法：+X, -X, +Y, -Y, +Z, -Z 应对应 ±g */
        /* 简化实现：使用最大最小值计算bias和scale */

        vec3f_t min_val = {1e6f, 1e6f, 1e6f};
        vec3f_t max_val = {-1e6f, -1e6f, -1e6f};

        for (int i = 0; i < 6; i++) {
            if (state->samples[i].x < min_val.x) min_val.x = state->samples[i].x;
            if (state->samples[i].x > max_val.x) max_val.x = state->samples[i].x;
            if (state->samples[i].y < min_val.y) min_val.y = state->samples[i].y;
            if (state->samples[i].y > max_val.y) max_val.y = state->samples[i].y;
            if (state->samples[i].z < min_val.z) min_val.z = state->samples[i].z;
            if (state->samples[i].z > max_val.z) max_val.z = state->samples[i].z;
        }

        /* 零偏 = (最大值 + 最小值) / 2 */
        cal->accel.bias.x = (max_val.x + min_val.x) * 0.5f;
        cal->accel.bias.y = (max_val.y + min_val.y) * 0.5f;
        cal->accel.bias.z = (max_val.z + min_val.z) * 0.5f;

        /* 尺度因子 = 2*g / (最大值 - 最小值) */
        float span_x = max_val.x - min_val.x;
        float span_y = max_val.y - min_val.y;
        float span_z = max_val.z - min_val.z;

        if (span_x > 1.0f) cal->accel.scale.x = (2.0f * GRAVITY_STANDARD) / span_x;
        if (span_y > 1.0f) cal->accel.scale.y = (2.0f * GRAVITY_STANDARD) / span_y;
        if (span_z > 1.0f) cal->accel.scale.z = (2.0f * GRAVITY_STANDARD) / span_z;

        cal->accel.calibrated = true;
        return true;
    }

    return false;
}

/* ============================================================================
 * 陀螺仪静止校准
 * ============================================================================ */

/**
 * @brief 开始陀螺仪静止校准
 */
hal_status_t cal_gyro_stationary_start(sensor_cal_handle_t *cal, uint16_t sample_count)
{
    if (cal == NULL) {
        return HAL_ERROR;
    }

    memset(&g_cal_state.gyro, 0, sizeof(gyro_stationary_state_t));
    g_cal_state.gyro.target = sample_count;

    return HAL_OK;
}

/**
 * @brief 添加陀螺仪静止校准样本
 * @return true采样完成, false继续采样
 */
bool cal_gyro_stationary_sample(sensor_cal_handle_t *cal, const vec3i_t *raw)
{
    gyro_stationary_state_t *state = &g_cal_state.gyro;

    if (cal == NULL || raw == NULL) {
        return true;
    }

    /* 累加 */
    state->sum.x += (float)raw->x * GYRO_SENSITIVITY_DEFAULT;
    state->sum.y += (float)raw->y * GYRO_SENSITIVITY_DEFAULT;
    state->sum.z += (float)raw->z * GYRO_SENSITIVITY_DEFAULT;
    state->count++;

    if (state->count >= state->target) {
        /* 计算平均值作为零偏 */
        cal->gyro.bias.x = state->sum.x / (float)state->count;
        cal->gyro.bias.y = state->sum.y / (float)state->count;
        cal->gyro.bias.z = state->sum.z / (float)state->count;
        cal->gyro.calibrated = true;
        return true;
    }

    return false;
}

/* ============================================================================
 * 磁力计旋转校准
 * ============================================================================ */

/**
 * @brief 开始磁力计旋转校准
 */
hal_status_t cal_mag_rotation_start(sensor_cal_handle_t *cal, uint16_t sample_count)
{
    if (cal == NULL || sample_count < MAG_MIN_SAMPLES || sample_count > MAG_MAX_SAMPLES) {
        return HAL_ERROR;
    }

    memset(&g_cal_state.mag, 0, sizeof(mag_rotation_state_t));
    g_cal_state.mag.target = sample_count;

    /* 初始化min/max */
    g_cal_state.mag.min.x = g_cal_state.mag.min.y = g_cal_state.mag.min.z = 1e6f;
    g_cal_state.mag.max.x = g_cal_state.mag.max.y = g_cal_state.mag.max.z = -1e6f;

    return HAL_OK;
}

/**
 * @brief 添加磁力计旋转校准样本
 * @return true采样完成, false继续采样
 */
bool cal_mag_rotation_sample(sensor_cal_handle_t *cal, const vec3i_t *raw)
{
    mag_rotation_state_t *state = &g_cal_state.mag;
    vec3f_t sample;

    if (cal == NULL || raw == NULL || state->count >= state->target) {
        return true;
    }

    /* 转换为物理单位 */
    sample.x = (float)raw->x * MAG_SENSITIVITY_DEFAULT;
    sample.y = (float)raw->y * MAG_SENSITIVITY_DEFAULT;
    sample.z = (float)raw->z * MAG_SENSITIVITY_DEFAULT;

    /* 存储样本 */
    state->samples[state->count] = sample;
    state->count++;

    /* 更新min/max */
    if (sample.x < state->min.x) state->min.x = sample.x;
    if (sample.x > state->max.x) state->max.x = sample.x;
    if (sample.y < state->min.y) state->min.y = sample.y;
    if (sample.y > state->max.y) state->max.y = sample.y;
    if (sample.z < state->min.z) state->min.z = sample.z;
    if (sample.z > state->max.z) state->max.z = sample.z;

    if (state->count >= state->target) {
        /* 简化实现：使用椭球中心作为硬铁补偿 */
        cal->mag.hard_iron.x = (state->max.x + state->min.x) * 0.5f;
        cal->mag.hard_iron.y = (state->max.y + state->min.y) * 0.5f;
        cal->mag.hard_iron.z = (state->max.z + state->min.z) * 0.5f;

        /* 软铁补偿简化为尺度调整 */
        float span_x = state->max.x - state->min.x;
        float span_y = state->max.y - state->min.y;
        float span_z = state->max.z - state->min.z;
        float avg_span = (span_x + span_y + span_z) / 3.0f;

        if (span_x > 1.0f) cal->mag.soft_iron.m[0][0] = avg_span / span_x;
        if (span_y > 1.0f) cal->mag.soft_iron.m[1][1] = avg_span / span_y;
        if (span_z > 1.0f) cal->mag.soft_iron.m[2][2] = avg_span / span_z;

        cal->mag.field_strength = avg_span * 0.5f;
        cal->mag.calibrated = true;

        return true;
    }

    return false;
}

/* ============================================================================
 * 应用校准
 * ============================================================================ */

/**
 * @brief 应用加速度计校准
 */
void cal_accel_apply(const sensor_cal_handle_t *cal, const vec3i_t *raw, vec3f_t *out)
{
    vec3f_t temp;

    if (cal == NULL || raw == NULL || out == NULL) {
        return;
    }

    /* 原始值转物理单位 */
    temp.x = (float)raw->x * ACCEL_SENSITIVITY_DEFAULT;
    temp.y = (float)raw->y * ACCEL_SENSITIVITY_DEFAULT;
    temp.z = (float)raw->z * ACCEL_SENSITIVITY_DEFAULT;

    /* 减去零偏 */
    temp.x -= cal->accel.bias.x;
    temp.y -= cal->accel.bias.y;
    temp.z -= cal->accel.bias.z;

    /* 应用尺度因子 */
    out->x = temp.x * cal->accel.scale.x;
    out->y = temp.y * cal->accel.scale.y;
    out->z = temp.z * cal->accel.scale.z;
}

/**
 * @brief 应用陀螺仪校准
 */
void cal_gyro_apply(const sensor_cal_handle_t *cal, const vec3i_t *raw, vec3f_t *out)
{
    if (cal == NULL || raw == NULL || out == NULL) {
        return;
    }

    /* 原始值转rad/s并减去零偏 */
    out->x = ((float)raw->x * GYRO_SENSITIVITY_DEFAULT) - cal->gyro.bias.x;
    out->y = ((float)raw->y * GYRO_SENSITIVITY_DEFAULT) - cal->gyro.bias.y;
    out->z = ((float)raw->z * GYRO_SENSITIVITY_DEFAULT) - cal->gyro.bias.z;
}

/**
 * @brief 应用磁力计校准
 */
void cal_mag_apply(const sensor_cal_handle_t *cal, const vec3i_t *raw, vec3f_t *out)
{
    vec3f_t temp;

    if (cal == NULL || raw == NULL || out == NULL) {
        return;
    }

    /* 原始值转物理单位 */
    temp.x = (float)raw->x * MAG_SENSITIVITY_DEFAULT;
    temp.y = (float)raw->y * MAG_SENSITIVITY_DEFAULT;
    temp.z = (float)raw->z * MAG_SENSITIVITY_DEFAULT;

    /* 硬铁补偿 */
    temp.x -= cal->mag.hard_iron.x;
    temp.y -= cal->mag.hard_iron.y;
    temp.z -= cal->mag.hard_iron.z;

    /* 软铁补偿（矩阵乘法） */
    matrix3_mult_vec(&cal->mag.soft_iron, &temp, out);
}

/* ============================================================================
 * 私有工具函数
 * ============================================================================ */

/**
 * @brief 设置3x3矩阵为单位矩阵
 */
static void matrix3_identity(cal_matrix3_t *m)
{
    memset(m, 0, sizeof(cal_matrix3_t));
    m->m[0][0] = 1.0f;
    m->m[1][1] = 1.0f;
    m->m[2][2] = 1.0f;
}

/**
 * @brief 矩阵乘向量
 */
static void matrix3_mult_vec(const cal_matrix3_t *m, const vec3f_t *v, vec3f_t *out)
{
    vec3f_t temp;
    temp.x = m->m[0][0] * v->x + m->m[0][1] * v->y + m->m[0][2] * v->z;
    temp.y = m->m[1][0] * v->x + m->m[1][1] * v->y + m->m[1][2] * v->z;
    temp.z = m->m[2][0] * v->x + m->m[2][1] * v->y + m->m[2][2] * v->z;
    *out = temp;
}

/**
 * @brief 快速逆平方根 (Quake III算法)
 */
static float inv_sqrt(float x)
{
    /* 使用标准库实现，嵌入式平台可优化为快速算法 */
    return 1.0f / sqrtf(x);
}
