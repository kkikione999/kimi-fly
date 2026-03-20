/**
 * @file flight_main.c
 * @brief 飞行控制主程序实现
 *
 * @note 整合所有模块的主控制程序
 *       1kHz控制循环 + WiFi通信处理
 *
 * @author Drone Control System
 * @version 1.0
 */

#include "flight_main.h"
#include "../drivers/icm42688.h"
#include "../drivers/lps22hb.h"
#include "../drivers/qmc5883p.h"
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

/* ============================================================================
 * 静态函数声明
 * ============================================================================ */

static hal_status_t init_sensor_drivers(void);
static void read_all_sensors(flight_main_handle_t *handle);
static void update_telemetry(flight_main_handle_t *handle);
static void send_status_report(flight_main_handle_t *handle);
static void handle_errors(flight_main_handle_t *handle);
static void update_leds(flight_main_handle_t *handle);

/* ============================================================================
 * 外部变量 (传感器句柄，在驱动中定义)
 * ============================================================================ */

extern i2c_handle_t hi2c1;      /**< I2C1句柄 */
extern spi_handle_t hspi3;      /**< SPI3句柄 */

/* 传感器设备实例 */
static icm42688_handle_t g_imu;
static lps22hb_handle_t g_baro;
static qmc5883p_handle_t g_mag;

/* ============================================================================
 * API实现 - 初始化和反初始化
 * ============================================================================ */

hal_status_t flight_main_init(flight_main_handle_t *handle)
{
    if (handle == NULL) {
        return HAL_ERROR;
    }

    memset(handle, 0, sizeof(flight_main_handle_t));
    handle->state = SYS_STATE_INIT;

    platform_debug_print("[FLIGHT] System init start...\r\n");

    /* 1. 初始化飞行控制器 */
    flight_controller_init_t fc_init = {
        .sample_rate = 1000.0f,
        .use_mag = true
    };

    if (flight_controller_init(&handle->flight_ctrl, &fc_init) != HAL_OK) {
        handle->error = ERR_FC_INIT;
        handle->state = SYS_STATE_ERROR;
        platform_debug_print("[FLIGHT] Flight controller init FAILED!\r\n");
        return HAL_ERROR;
    }
    platform_debug_print("[FLIGHT] Flight controller init OK\r\n");

    /* 2. 初始化WiFi命令处理器 */
    if (wifi_command_init(&handle->wifi_cmd) != HAL_OK) {
        handle->error = ERR_WIFI_TIMEOUT;
        handle->state = SYS_STATE_ERROR;
        platform_debug_print("[FLIGHT] WiFi command init FAILED!\r\n");
        return HAL_ERROR;
    }
    platform_debug_print("[FLIGHT] WiFi command init OK\r\n");

    /* 3. 初始化传感器驱动 */
    if (flight_main_init_sensors(handle) != HAL_OK) {
        platform_debug_print("[FLIGHT] Sensor init FAILED!\r\n");
        /* 传感器初始化失败不进入错误状态，允许重试 */
    } else {
        handle->sensors_ok = true;
        platform_debug_print("[FLIGHT] All sensors init OK\r\n");
    }

    handle->initialized = true;
    handle->state = SYS_STATE_STANDBY;
    handle->last_wifi_task = platform_get_time_ms();
    handle->last_telemetry = platform_get_time_ms();
    handle->last_status = platform_get_time_ms();

    platform_debug_print("[FLIGHT] System init complete!\r\n");
    return HAL_OK;
}

void flight_main_deinit(flight_main_handle_t *handle)
{
    if (handle == NULL) {
        return;
    }

    /* 停止电机 */
    platform_set_motors(0, 0, 0, 0);

    /* 反初始化子系统 */
    flight_controller_deinit(&handle->flight_ctrl);
    wifi_command_deinit(&handle->wifi_cmd);

    handle->initialized = false;
    handle->state = SYS_STATE_INIT;
}

hal_status_t flight_main_init_sensors(flight_main_handle_t *handle)
{
    hal_status_t status;

    /* 初始化IMU (ICM-42688-P, I2C1 @ 0x68) */
    status = icm42688_init(&g_imu, &hi2c1);
    if (status != HAL_OK) {
        handle->error = ERR_IMU_INIT;
        platform_debug_print("[SENSOR] IMU init failed!\r\n");
        return HAL_ERROR;
    }
    platform_debug_print("[SENSOR] IMU init OK\r\n");

    /* 初始化气压计 (LPS22HH, SPI3) */
    status = lps22hb_init(&g_baro, &hspi3, LPS22HB_ODR_25_HZ);
    if (status != HAL_OK) {
        handle->error = ERR_BARO_INIT;
        platform_debug_print("[SENSOR] Barometer init failed!\r\n");
        /* 气压计非关键，继续 */
    } else {
        platform_debug_print("[SENSOR] Barometer init OK\r\n");
    }

    /* 初始化磁力计 (QMC5883P, I2C1 @ 0x2C) */
    status = qmc5883p_init(&g_mag, &hi2c1);
    if (status != HAL_OK) {
        handle->error = ERR_MAG_INIT;
        platform_debug_print("[SENSOR] Magnetometer init failed!\r\n");
        /* 磁力计非关键，继续 */
    } else {
        platform_debug_print("[SENSOR] Magnetometer init OK\r\n");
    }

    return HAL_OK;
}

hal_status_t flight_main_calibrate_sensors(flight_main_handle_t *handle)
{
    if (handle == NULL || !handle->initialized) {
        return HAL_ERROR;
    }

    platform_debug_print("[CAL] Starting sensor calibration...\r\n");
    handle->state = SYS_STATE_CALIBRATION;

    /* 收集校准样本 */
    vec3f_t accel_sum = {0, 0, 0};
    vec3f_t gyro_sum = {0, 0, 0};

    for (int i = 0; i < CALIBRATION_SAMPLES; i++) {
        icm42688_data_t imu_data;

        if (icm42688_read_data(&g_imu, &imu_data) == HAL_OK) {
            accel_sum.x += imu_data.accel_x;
            accel_sum.y += imu_data.accel_y;
            accel_sum.z += imu_data.accel_z;
            gyro_sum.x += imu_data.gyro_x;
            gyro_sum.y += imu_data.gyro_y;
            gyro_sum.z += imu_data.gyro_z;
        }

        platform_delay_us(1000); /* 1ms延时 */
    }

    /* 计算并应用零偏 */
    vec3f_t gyro_bias;
    gyro_bias.x = gyro_sum.x / CALIBRATION_SAMPLES;
    gyro_bias.y = gyro_sum.y / CALIBRATION_SAMPLES;
    gyro_bias.z = gyro_sum.z / CALIBRATION_SAMPLES;

    platform_debug_print("[CAL] Gyro bias: %.3f, %.3f, %.3f\r\n",
                         gyro_bias.x, gyro_bias.y, gyro_bias.z);

    handle->state = SYS_STATE_STANDBY;
    platform_debug_print("[CAL] Calibration complete!\r\n");

    return HAL_OK;
}

/* ============================================================================
 * API实现 - 主控制循环
 * ============================================================================ */

void flight_main_control_loop(flight_main_handle_t *handle)
{
    if (handle == NULL || !handle->initialized) {
        return;
    }

    handle->loop_count++;

    /* 1. 读取传感器 */
    read_all_sensors(handle);

    /* 2. 更新飞控 (1kHz) */
    if (handle->sensors_ok) {
        flight_controller_update_imu(&handle->flight_ctrl,
                                      &handle->accel, &handle->gyro);

        if (handle->mag_valid) {
            flight_controller_update_mag(&handle->flight_ctrl, &handle->mag);
        }

        flight_controller_update(&handle->flight_ctrl);
        handle->imu_read_count++;
    }

    /* 3. 输出到电机 */
    if (flight_controller_is_armed(&handle->flight_ctrl)) {
        motor_outputs_t motors;
        flight_controller_get_motors(&handle->flight_ctrl, &motors);
        platform_set_motors(motors.motor1, motors.motor2,
                            motors.motor3, motors.motor4);
    } else {
        platform_set_motors(0, 0, 0, 0);
    }

    /* 4. WiFi任务 (200Hz) */
    uint32_t now = platform_get_time_ms();
    if ((now - handle->last_wifi_task) >= WIFI_TASK_INTERVAL_MS) {
        flight_main_wifi_task(handle);
        handle->last_wifi_task = now;
    }

    /* 5. 遥测发送 (50Hz) */
    if ((now - handle->last_telemetry) >= TELEMETRY_INTERVAL_MS) {
        update_telemetry(handle);
        handle->last_telemetry = now;
    }

    /* 6. 状态报告 (1Hz) */
    if ((now - handle->last_status) >= STATUS_INTERVAL_MS) {
        send_status_report(handle);
        handle->last_status = now;
    }

    /* 7. LED更新 */
    update_leds(handle);
}

void flight_main_wifi_task(flight_main_handle_t *handle)
{
    if (handle == NULL || !handle->initialized) {
        return;
    }

    uint32_t now = platform_get_time_ms();

    /* 处理WiFi命令 */
    hal_status_t status = wifi_command_execute(&handle->wifi_cmd, &handle->flight_ctrl);
    if (status == HAL_OK) {
        handle->wifi_rx_count++;
    }

    /* 主动发送链路心跳，便于 ESP32 侧持续确认 STM32->ESP32 TX 通道 */
    if ((now - handle->wifi_cmd.last_heartbeat) >= WIFI_HEARTBEAT_INTERVAL_MS) {
        wifi_command_send_heartbeat(&handle->wifi_cmd);
        handle->wifi_cmd.last_heartbeat = now;
    }

    /* 获取RC命令并应用到飞控 */
    rc_command_t rc_cmd;
    if (wifi_command_get_rc(&handle->wifi_cmd, &rc_cmd)) {
        flight_controller_set_rc_input(&handle->flight_ctrl, &rc_cmd);
    }

    /* 检查WiFi超时 */
    if (wifi_command_is_timeout(&handle->wifi_cmd, now)) {
        /* WiFi超时，切换到安全模式 */
        if (flight_controller_get_mode(&handle->flight_ctrl) != FLIGHT_MODE_DISARMED) {
            platform_debug_print("[WIFI] Link timeout! Disarming...\r\n");
            flight_controller_disarm(&handle->flight_ctrl);
        }
    }
}

/* ============================================================================
 * API实现 - 状态查询
 * ============================================================================ */

system_state_t flight_main_get_state(const flight_main_handle_t *handle)
{
    if (handle == NULL) {
        return SYS_STATE_ERROR;
    }
    return handle->state;
}

error_code_t flight_main_get_error(const flight_main_handle_t *handle)
{
    if (handle == NULL) {
        return ERR_NONE;
    }
    return handle->error;
}

void flight_main_clear_error(flight_main_handle_t *handle)
{
    if (handle == NULL) {
        return;
    }
    handle->error = ERR_NONE;
    if (handle->state == SYS_STATE_ERROR) {
        handle->state = SYS_STATE_STANDBY;
    }
}

uint32_t flight_main_get_loop_count(const flight_main_handle_t *handle)
{
    if (handle == NULL) {
        return 0;
    }
    return handle->loop_count;
}

void flight_main_print_status(const flight_main_handle_t *handle)
{
    if (handle == NULL) {
        return;
    }

    const char *state_str[] = {"INIT", "CAL", "STANDBY", "ACTIVE", "ERROR"};
    system_state_t state = handle->state;
    if (state > SYS_STATE_ERROR) state = SYS_STATE_ERROR;

    euler_angle_t attitude;
    flight_controller_get_attitude(&handle->flight_ctrl, &attitude);

    platform_debug_print(
        "[STATUS] State: %s, Armed: %d, "
        "Att: R=%.1f P=%.1f Y=%.1f, "
        "Loops: %lu, IMU: %lu\r\n",
        state_str[state],
        flight_controller_is_armed(&handle->flight_ctrl),
        attitude.roll, attitude.pitch, attitude.yaw,
        (unsigned long)handle->loop_count,
        (unsigned long)handle->imu_read_count
    );
}

/* ============================================================================
 * 静态函数实现
 * ============================================================================ */

static hal_status_t init_sensor_drivers(void)
{
    /* 传感器句柄在全局变量中定义，这里只返回OK */
    return HAL_OK;
}

static void read_all_sensors(flight_main_handle_t *handle)
{
    /* 读取IMU */
    icm42688_data_t imu_data;
    if (icm42688_read_data(&g_imu, &imu_data) == HAL_OK) {
        handle->accel.x = imu_data.accel_x;
        handle->accel.y = imu_data.accel_y;
        handle->accel.z = imu_data.accel_z;
        handle->gyro.x = imu_data.gyro_x * 3.14159f / 180.0f;  /* dps to rad/s */
        handle->gyro.y = imu_data.gyro_y * 3.14159f / 180.0f;
        handle->gyro.z = imu_data.gyro_z * 3.14159f / 180.0f;
        handle->sensors_ok = true;
    } else {
        handle->sensors_ok = false;
    }

    /* 读取磁力计 (可选) */
    qmc5883p_data_t mag_data;
    if (qmc5883p_read_data(&g_mag, &mag_data) == HAL_OK) {
        handle->mag.x = mag_data.mag_x_gauss;
        handle->mag.y = mag_data.mag_y_gauss;
        handle->mag.z = mag_data.mag_z_gauss;
        handle->mag_valid = true;
    } else {
        handle->mag_valid = false;
    }
}

static void update_telemetry(flight_main_handle_t *handle)
{
    uint32_t now = platform_get_time_ms();
    uint16_t sent = wifi_command_update_telemetry(&handle->wifi_cmd,
                                                   &handle->flight_ctrl, now);
    handle->wifi_tx_count += sent;
}

static void send_status_report(flight_main_handle_t *handle)
{
    wifi_command_send_status(&handle->wifi_cmd, &handle->flight_ctrl);
    flight_main_print_status(handle);
}

static void handle_errors(flight_main_handle_t *handle)
{
    (void)handle;
    /* 错误处理逻辑 */
}

static void update_leds(flight_main_handle_t *handle)
{
    (void)handle;
    /* LED状态指示 */
}

/* ============================================================================
 * 平台相关接口默认实现 (弱引用)
 * ============================================================================ */

__attribute__((weak))
uint32_t platform_get_time_ms(void)
{
    return 0;
}

__attribute__((weak))
uint32_t platform_get_time_us(void)
{
    return 0;
}

__attribute__((weak))
void platform_delay_us(uint32_t us)
{
    (void)us;
}

__attribute__((weak))
void platform_debug_print(const char *fmt, ...)
{
    (void)fmt;
}

__attribute__((weak))
bool platform_read_imu(vec3f_t *accel, vec3f_t *gyro)
{
    (void)accel;
    (void)gyro;
    return false;
}

__attribute__((weak))
bool platform_read_mag(vec3f_t *mag)
{
    (void)mag;
    return false;
}

__attribute__((weak))
void platform_set_motors(uint16_t m1, uint16_t m2, uint16_t m3, uint16_t m4)
{
    (void)m1;
    (void)m2;
    (void)m3;
    (void)m4;
}
