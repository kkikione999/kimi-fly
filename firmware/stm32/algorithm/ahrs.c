/**
 * @file ahrs.c
 * @brief AHRS (Attitude Heading Reference System) 姿态解算算法实现
 *
 * @note 实现基于Mahony互补滤波器的姿态解算
 *       融合加速度计、陀螺仪、磁力计数据
 *       经典算法，计算量小，适合嵌入式实时应用
 *
 * @reference
 *   - Mahony, R. et al. "Nonlinear complementary filters on the special orthogonal group"
 *   - http://www.x-io.co.uk/open-source-imu-and-ahrs-algorithms/
 *
 * @author Drone Control System
 * @version 1.0
 */

#include "ahrs.h"
#include <string.h>

/* ============================================================================
 * 私有函数声明
 * ============================================================================ */

static void update_imu(ahrs_handle_t *ahrs, float gx, float gy, float gz,
                       float ax, float ay, float az);
static void update_marg(ahrs_handle_t *ahrs, float gx, float gy, float gz,
                        float ax, float ay, float az,
                        float mx, float my, float mz);

/* ============================================================================
 * 初始化与配置
 * ============================================================================ */

/**
 * @brief 初始化AHRS模块
 */
hal_status_t ahrs_init(ahrs_handle_t *ahrs, float sample_rate)
{
    if (ahrs == NULL || sample_rate <= 0.0f) {
        return HAL_ERROR;
    }

    /* 清零 */
    memset(ahrs, 0, sizeof(ahrs_handle_t));

    /* 设置采样参数 */
    ahrs->sample_rate = sample_rate;
    ahrs->dt = 1.0f / sample_rate;

    /* 设置默认增益 */
    ahrs->kp = AHRS_DEFAULT_KP;
    ahrs->ki = AHRS_DEFAULT_KI;
    ahrs->kp_mag = AHRS_KP_MAG;
    ahrs->ki_mag = AHRS_KI_MAG;

    /* 初始化四元数为单位四元数 (无旋转) */
    ahrs->q.w = 1.0f;
    ahrs->q.x = 0.0f;
    ahrs->q.y = 0.0f;
    ahrs->q.z = 0.0f;

    /* 初始化参考磁场 (假设磁北沿X轴，磁倾角向下) */
    ahrs->mag_ref.x = 1.0f;  /* 磁北分量 */
    ahrs->mag_ref.y = 0.0f;
    ahrs->mag_ref.z = 0.0f;  /* 水平磁场 */

    ahrs->initialized = true;

    return HAL_OK;
}

/**
 * @brief 反初始化AHRS模块
 */
void ahrs_deinit(ahrs_handle_t *ahrs)
{
    if (ahrs == NULL) {
        return;
    }
    ahrs->initialized = false;
}

/**
 * @brief 设置Mahony滤波器增益
 */
void ahrs_set_gains(ahrs_handle_t *ahrs, float kp, float ki)
{
    if (ahrs == NULL) {
        return;
    }
    ahrs->kp = kp;
    ahrs->ki = ki;
}

/**
 * @brief 设置磁力计增益
 */
void ahrs_set_mag_gains(ahrs_handle_t *ahrs, float kp, float ki)
{
    if (ahrs == NULL) {
        return;
    }
    ahrs->kp_mag = kp;
    ahrs->ki_mag = ki;
}

/**
 * @brief 设置参考磁场方向
 */
void ahrs_set_mag_reference(ahrs_handle_t *ahrs, float mag_north, float mag_down)
{
    if (ahrs == NULL) {
        return;
    }
    /* 归一化 */
    float norm = sqrtf(mag_north * mag_north + mag_down * mag_down);
    if (norm > 1e-6f) {
        ahrs->mag_ref.x = mag_north / norm;
        ahrs->mag_ref.y = 0.0f;  /* 假设无东向分量 */
        ahrs->mag_ref.z = mag_down / norm;
    }
}

/* ============================================================================
 * 姿态更新 - Mahony滤波器核心
 * ============================================================================ */

/**
 * @brief 6轴IMU更新 (无磁力计)
 * @note 使用加速度计作为参考向量校正陀螺仪漂移
 *       缺点：偏航角(Yaw)无法确定，会随时间漂移
 */
hal_status_t ahrs_update_6axis(ahrs_handle_t *ahrs,
                                const vec3f_t *gyro,
                                const vec3f_t *accel)
{
    if (ahrs == NULL || gyro == NULL || accel == NULL || !ahrs->initialized) {
        return HAL_ERROR;
    }

    if (!ahrs_accel_valid(accel)) {
        /* 加速度数据无效，仅使用陀螺仪积分 */
        /* 四元数微分方程积分 */
        quaternion_t q = ahrs->q;
        float gx = gyro->x * 0.5f * ahrs->dt;
        float gy = gyro->y * 0.5f * ahrs->dt;
        float gz = gyro->z * 0.5f * ahrs->dt;

        ahrs->q.w += -q.x * gx - q.y * gy - q.z * gz;
        ahrs->q.x +=  q.w * gx + q.y * gz - q.z * gy;
        ahrs->q.y +=  q.w * gy - q.x * gz + q.z * gx;
        ahrs->q.z +=  q.w * gz + q.x * gy - q.y * gx;

        quat_normalize(&ahrs->q);
    } else {
        /* 使用加速度计校正 */
        update_imu(ahrs, gyro->x, gyro->y, gyro->z,
                   accel->x, accel->y, accel->z);
    }

    ahrs->update_count++;
    return HAL_OK;
}

/**
 * @brief 9轴MARG更新 (加速度计+陀螺仪+磁力计)
 * @note 使用加速度和磁场作为参考向量校正陀螺仪漂移
 *       可以稳定确定偏航角
 */
hal_status_t ahrs_update_9axis(ahrs_handle_t *ahrs,
                                const vec3f_t *gyro,
                                const vec3f_t *accel,
                                const vec3f_t *mag)
{
    if (ahrs == NULL || gyro == NULL || accel == NULL || mag == NULL || !ahrs->initialized) {
        return HAL_ERROR;
    }

    if (!ahrs_accel_valid(accel) || !ahrs_mag_valid(mag)) {
        /* 退化为6轴模式 */
        return ahrs_update_6axis(ahrs, gyro, accel);
    }

    update_marg(ahrs, gyro->x, gyro->y, gyro->z,
                accel->x, accel->y, accel->z,
                mag->x, mag->y, mag->z);

    ahrs->update_count++;
    return HAL_OK;
}

/**
 * @brief 统一更新接口
 */
hal_status_t ahrs_update(ahrs_handle_t *ahrs, const ahrs_input_t *input)
{
    if (ahrs == NULL || input == NULL) {
        return HAL_ERROR;
    }

    if (input->mag_valid) {
        return ahrs_update_9axis(ahrs, &input->gyro, &input->accel, &input->mag);
    } else {
        return ahrs_update_6axis(ahrs, &input->gyro, &input->accel);
    }
}

/* ============================================================================
 * Mahony滤波器核心实现 (私有)
 * ============================================================================ */

/**
 * @brief Mahony IMU更新 (6轴)
 * @note 核心算法：使用加速度计校正陀螺仪
 */
static void update_imu(ahrs_handle_t *ahrs, float gx, float gy, float gz,
                       float ax, float ay, float az)
{
    quaternion_t *q = &ahrs->q;
    vec3f_t *eInt = &ahrs->integral_error;

    float q0 = q->w, q1 = q->x, q2 = q->y, q3 = q->z;

    /* 归一化加速度计测量值 */
    float norm = sqrtf(ax * ax + ay * ay + az * az);
    if (norm < 1e-6f) return;  /* 避免除零 */
    norm = 1.0f / norm;
    ax *= norm;
    ay *= norm;
    az *= norm;

    /* 从四元数估计重力方向 (机体坐标系) */
    /* g_est = q * [0,0,0,1] * q^-1 的重力分量 */
    float vx = 2.0f * (q1 * q3 - q0 * q2);
    float vy = 2.0f * (q0 * q1 + q2 * q3);
    float vz = q0 * q0 - q1 * q1 - q2 * q2 + q3 * q3;

    /* 计算误差 (测量重力方向与估计重力方向的叉积) */
    float ex = ay * vz - az * vy;
    float ey = az * vx - ax * vz;
    float ez = ax * vy - ay * vx;

    /* 积分误差 */
    eInt->x += ex * ahrs->ki * ahrs->dt;
    eInt->y += ey * ahrs->ki * ahrs->dt;
    eInt->z += ez * ahrs->ki * ahrs->dt;

    /* PI控制器校正陀螺仪零偏 */
    gx += ahrs->kp * ex + eInt->x;
    gy += ahrs->kp * ey + eInt->y;
    gz += ahrs->kp * ez + eInt->z;

    /* 四元数微分方程积分 */
    /* dq/dt = 0.5 * q * [0, gx, gy, gz] */
    float qDot0 = 0.5f * (-q1 * gx - q2 * gy - q3 * gz);
    float qDot1 = 0.5f * ( q0 * gx + q2 * gz - q3 * gy);
    float qDot2 = 0.5f * ( q0 * gy - q1 * gz + q3 * gx);
    float qDot3 = 0.5f * ( q0 * gz + q1 * gy - q2 * gx);

    /* 数值积分 */
    q->w += qDot0 * ahrs->dt;
    q->x += qDot1 * ahrs->dt;
    q->y += qDot2 * ahrs->dt;
    q->z += qDot3 * ahrs->dt;

    /* 归一化 */
    quat_normalize(q);
}

/**
 * @brief Mahony MARG更新 (9轴)
 * @note 核心算法：使用加速度和磁力计校正陀螺仪
 */
static void update_marg(ahrs_handle_t *ahrs, float gx, float gy, float gz,
                        float ax, float ay, float az,
                        float mx, float my, float mz)
{
    quaternion_t *q = &ahrs->q;
    vec3f_t *eInt = &ahrs->integral_error;

    float q0 = q->w, q1 = q->x, q2 = q->y, q3 = q->z;

    /* 归一化加速度计 */
    float norm = sqrtf(ax * ax + ay * ay + az * az);
    if (norm < 1e-6f) return;
    norm = 1.0f / norm;
    ax *= norm;
    ay *= norm;
    az *= norm;

    /* 归一化磁力计 */
    norm = sqrtf(mx * mx + my * my + mz * mz);
    if (norm < 1e-6f) return;
    norm = 1.0f / norm;
    mx *= norm;
    my *= norm;
    mz *= norm;

    /* 将磁力计旋转到地球坐标系 (参考系) */
    /* h = q * m * q^-1 */
    float hx = 2.0f * (mx * (0.5f - q2 * q2 - q3 * q3) + my * (q1 * q2 - q0 * q3) + mz * (q1 * q3 + q0 * q2));
    float hy = 2.0f * (mx * (q1 * q2 + q0 * q3) + my * (0.5f - q1 * q1 - q3 * q3) + mz * (q2 * q3 - q0 * q1));
    float hz = 2.0f * (mx * (q1 * q3 - q0 * q2) + my * (q2 * q3 + q0 * q1) + mz * (0.5f - q1 * q1 - q2 * q2));

    /* 计算参考磁场方向 (在水平面内) */
    float bx = sqrtf(hx * hx + hy * hy);
    float bz = hz;

    /* 从四元数估计地球坐标系的磁场方向 */
    float wx = bx * (0.5f - q2 * q2 - q3 * q3) + bz * (q1 * q3 - q0 * q2);
    float wy = bx * (q1 * q2 - q0 * q3) + bz * (q0 * q1 + q2 * q3);
    float wz = bx * (q0 * q2 + q1 * q3) + bz * (0.5f - q1 * q1 - q2 * q2);

    /* 估计重力方向 */
    float vx = 2.0f * (q1 * q3 - q0 * q2);
    float vy = 2.0f * (q0 * q1 + q2 * q3);
    float vz = q0 * q0 - q1 * q1 - q2 * q2 + q3 * q3;

    /* 误差计算 (叉积) */
    /* 加速度误差 */
    float ex1 = ay * vz - az * vy;
    float ey1 = az * vx - ax * vz;
    float ez1 = ax * vy - ay * vx;

    /* 磁力计误差 */
    float ex2 = my * wz - mz * wy;
    float ey2 = mz * wx - mx * wz;
    float ez2 = mx * wy - my * wx;

    /* 合并误差 (加权平均) */
    float ex = ex1 + ex2;
    float ey = ey1 + ey2;
    float ez = ez1 + ez2;

    /* 积分误差 */
    eInt->x += ex * ahrs->ki_mag * ahrs->dt;
    eInt->y += ey * ahrs->ki_mag * ahrs->dt;
    eInt->z += ez * ahrs->ki_mag * ahrs->dt;

    /* PI控制器校正 */
    gx += ahrs->kp_mag * ex + eInt->x;
    gy += ahrs->kp_mag * ey + eInt->y;
    gz += ahrs->kp_mag * ez + eInt->z;

    /* 四元数积分 */
    float qDot0 = 0.5f * (-q1 * gx - q2 * gy - q3 * gz);
    float qDot1 = 0.5f * ( q0 * gx + q2 * gz - q3 * gy);
    float qDot2 = 0.5f * ( q0 * gy - q1 * gz + q3 * gx);
    float qDot3 = 0.5f * ( q0 * gz + q1 * gy - q2 * gx);

    q->w += qDot0 * ahrs->dt;
    q->x += qDot1 * ahrs->dt;
    q->y += qDot2 * ahrs->dt;
    q->z += qDot3 * ahrs->dt;

    quat_normalize(q);
}

/* ============================================================================
 * 姿态输出
 * ============================================================================ */

/**
 * @brief 获取当前四元数
 */
bool ahrs_get_quaternion(const ahrs_handle_t *ahrs, quaternion_t *q)
{
    if (ahrs == NULL || q == NULL || !ahrs->initialized) {
        return false;
    }
    *q = ahrs->q;
    return true;
}

/**
 * @brief 获取当前欧拉角 (弧度)
 */
bool ahrs_get_euler_rad(const ahrs_handle_t *ahrs, euler_angle_t *euler)
{
    if (ahrs == NULL || euler == NULL || !ahrs->initialized) {
        return false;
    }
    quaternion_to_euler(euler, &ahrs->q);
    return true;
}

/**
 * @brief 获取当前欧拉角 (角度)
 */
bool ahrs_get_euler_deg(const ahrs_handle_t *ahrs, euler_angle_t *euler)
{
    if (!ahrs_get_euler_rad(ahrs, euler)) {
        return false;
    }
    const float RAD_TO_DEG = 57.295779513082320876798154814105f;
    euler->roll  *= RAD_TO_DEG;
    euler->pitch *= RAD_TO_DEG;
    euler->yaw   *= RAD_TO_DEG;
    return true;
}

/**
 * @brief 获取旋转矩阵
 */
bool ahrs_get_rotation_matrix(const ahrs_handle_t *ahrs, rotation_matrix_t *R)
{
    if (ahrs == NULL || R == NULL || !ahrs->initialized) {
        return false;
    }

    float q0 = ahrs->q.w, q1 = ahrs->q.x, q2 = ahrs->q.y, q3 = ahrs->q.z;

    /* 从四元数构造旋转矩阵 */
    R->m[0][0] = q0*q0 + q1*q1 - q2*q2 - q3*q3;
    R->m[0][1] = 2.0f * (q1*q2 - q0*q3);
    R->m[0][2] = 2.0f * (q1*q3 + q0*q2);

    R->m[1][0] = 2.0f * (q1*q2 + q0*q3);
    R->m[1][1] = q0*q0 - q1*q1 + q2*q2 - q3*q3;
    R->m[1][2] = 2.0f * (q2*q3 - q0*q1);

    R->m[2][0] = 2.0f * (q1*q3 - q0*q2);
    R->m[2][1] = 2.0f * (q2*q3 + q0*q1);
    R->m[2][2] = q0*q0 - q1*q1 - q2*q2 + q3*q3;

    return true;
}

/**
 * @brief 获取重力向量
 */
bool ahrs_get_gravity(const ahrs_handle_t *ahrs, vec3f_t *gravity)
{
    if (ahrs == NULL || gravity == NULL || !ahrs->initialized) {
        return false;
    }

    float q0 = ahrs->q.w, q1 = ahrs->q.x, q2 = ahrs->q.y, q3 = ahrs->q.z;

    /* 机体坐标系中的重力方向 */
    gravity->x = 2.0f * (q1 * q3 - q0 * q2);
    gravity->y = 2.0f * (q0 * q1 + q2 * q3);
    gravity->z = q0 * q0 - q1 * q1 - q2 * q2 + q3 * q3;

    return true;
}

/* ============================================================================
 * 四元数工具函数
 * ============================================================================ */

/**
 * @brief 四元数归一化
 */
void quat_normalize(quaternion_t *q)
{
    float norm = sqrtf(q->w * q->w + q->x * q->x + q->y * q->y + q->z * q->z);
    if (norm > 1e-6f) {
        norm = 1.0f / norm;
        q->w *= norm;
        q->x *= norm;
        q->y *= norm;
        q->z *= norm;
    }
}

/**
 * @brief 四元数乘法
 */
void quat_multiply(quaternion_t *out, const quaternion_t *a, const quaternion_t *b)
{
    quaternion_t temp;
    temp.w = a->w * b->w - a->x * b->x - a->y * b->y - a->z * b->z;
    temp.x = a->w * b->x + a->x * b->w + a->y * b->z - a->z * b->y;
    temp.y = a->w * b->y - a->x * b->z + a->y * b->w + a->z * b->x;
    temp.z = a->w * b->z + a->x * b->y - a->y * b->x + a->z * b->w;
    *out = temp;
}

/**
 * @brief 四元数共轭
 */
void quat_conjugate(quaternion_t *out, const quaternion_t *q)
{
    out->w = q->w;
    out->x = -q->x;
    out->y = -q->y;
    out->z = -q->z;
}

/**
 * @brief 用四元数旋转向量
 */
void quat_rotate_vector(vec3f_t *out, const quaternion_t *q, const vec3f_t *v)
{
    /* q * [0, v] * q^-1 */
    quaternion_t vq = {0, v->x, v->y, v->z};
    quaternion_t q_conj, temp, result;

    quat_conjugate(&q_conj, q);
    quat_multiply(&temp, q, &vq);
    quat_multiply(&result, &temp, &q_conj);

    out->x = result.x;
    out->y = result.y;
    out->z = result.z;
}

/**
 * @brief 欧拉角转四元数 (Z-Y-X顺序)
 */
void euler_to_quaternion(quaternion_t *q, const euler_angle_t *euler)
{
    float cr = cosf(euler->roll * 0.5f);
    float sr = sinf(euler->roll * 0.5f);
    float cp = cosf(euler->pitch * 0.5f);
    float sp = sinf(euler->pitch * 0.5f);
    float cy = cosf(euler->yaw * 0.5f);
    float sy = sinf(euler->yaw * 0.5f);

    q->w = cr * cp * cy + sr * sp * sy;
    q->x = sr * cp * cy - cr * sp * sy;
    q->y = cr * sp * cy + sr * cp * sy;
    q->z = cr * cp * sy - sr * sp * cy;
}

/**
 * @brief 四元数转欧拉角 (Z-Y-X顺序)
 */
void quaternion_to_euler(euler_angle_t *euler, const quaternion_t *q)
{
    float q0 = q->w, q1 = q->x, q2 = q->y, q3 = q->z;

    /* Roll (X轴) */
    float sinr_cosp = 2.0f * (q0 * q1 + q2 * q3);
    float cosr_cosp = 1.0f - 2.0f * (q1 * q1 + q2 * q2);
    euler->roll = atan2f(sinr_cosp, cosr_cosp);

    /* Pitch (Y轴) - 使用 asin 需要限幅 */
    float sinp = 2.0f * (q0 * q2 - q3 * q1);
    if (sinp >= 1.0f) {
        euler->pitch = 3.14159265358979323846f / 2.0f;  /* 90度 */
    } else if (sinp <= -1.0f) {
        euler->pitch = -3.14159265358979323846f / 2.0f; /* -90度 */
    } else {
        euler->pitch = asinf(sinp);
    }

    /* Yaw (Z轴) */
    float siny_cosp = 2.0f * (q0 * q3 + q1 * q2);
    float cosy_cosp = 1.0f - 2.0f * (q2 * q2 + q3 * q3);
    euler->yaw = atan2f(siny_cosp, cosy_cosp);
}

/* ============================================================================
 * 工具函数
 * ============================================================================ */

/**
 * @brief 检查加速度数据有效性
 */
bool ahrs_accel_valid(const vec3f_t *accel)
{
    if (accel == NULL) return false;

    float norm = sqrtf(accel->x * accel->x + accel->y * accel->y + accel->z * accel->z);
    norm /= GRAVITY_STANDARD;  /* 转换为g单位 */

    return (norm >= AHRS_ACCEL_NORM_MIN && norm <= AHRS_ACCEL_NORM_MAX);
}

/**
 * @brief 检查磁力计数据有效性
 */
bool ahrs_mag_valid(const vec3f_t *mag)
{
    if (mag == NULL) return false;

    float norm = sqrtf(mag->x * mag->x + mag->y * mag->y + mag->z * mag->z);

    return (norm >= AHRS_MAG_NORM_MIN && norm <= AHRS_MAG_NORM_MAX);
}

/**
 * @brief 重置姿态到水平
 */
void ahrs_reset_to_level(ahrs_handle_t *ahrs)
{
    if (ahrs == NULL || !ahrs->initialized) {
        return;
    }

    /* 保持当前Yaw，重置Roll和Pitch为0 */
    euler_angle_t euler;
    quaternion_to_euler(&euler, &ahrs->q);

    euler.roll = 0.0f;
    euler.pitch = 0.0f;

    euler_to_quaternion(&ahrs->q, &euler);
    quat_normalize(&ahrs->q);

    /* 清空积分误差 */
    vec3f_zero(&ahrs->integral_error);
}

/**
 * @brief 获取AHRS运行状态
 */
uint32_t ahrs_get_update_count(const ahrs_handle_t *ahrs)
{
    if (ahrs == NULL) {
        return 0;
    }
    return ahrs->update_count;
}
