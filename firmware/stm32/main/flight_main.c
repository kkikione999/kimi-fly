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
#include <math.h>

/* 磁力计按 200 Hz 节奏轮询，避免在 1 kHz 控制循环中反复读取同一转换结果。 */
#define MAG_POLL_INTERVAL_MS 5U

/* ============================================================================
 * 静态函数声明
 * ============================================================================ */

static hal_status_t init_sensor_drivers(void);
static void read_all_sensors(flight_main_handle_t *handle);
static void update_telemetry(flight_main_handle_t *handle);
static void send_status_report(flight_main_handle_t *handle);
static void handle_errors(flight_main_handle_t *handle);
static void update_leds(flight_main_handle_t *handle);
static void log_attitude_sample(const flight_main_handle_t *handle);
static const char *flight_mode_name(flight_mode_t mode);
static void log_i2c_devices(void);
static void reset_mag_runtime_calibration(void);
static vec3f_t apply_mag_runtime_calibration(const qmc5883p_data_t *mag_data);
static void map_imu_to_body_frame(const icm42688_data_t *imu_data, vec3f_t *accel, vec3f_t *gyro);
static void reset_body_trim(void);
static bool update_body_trim_from_level_accel(const vec3f_t *level_accel);
static void apply_body_trim(vec3f_t *vector);

/* ============================================================================
 * 外部变量 (传感器句柄，在驱动中定义)
 * ============================================================================ */

extern i2c_handle_t hi2c1;      /**< I2C1句柄 */
extern spi_handle_t hspi3;      /**< SPI3句柄 */

/* 传感器设备实例 */
static icm42688_handle_t g_imu;
static lps22hb_handle_t g_baro;
static qmc5883p_handle_t g_mag;
static qmc5883p_data_t g_last_mag_sample;
static bool g_last_mag_valid = false;
static bool g_mag_filter_ready = false;
static vec3f_t g_mag_filtered = {0.0f, 0.0f, 0.0f};
static uint32_t g_last_mag_poll_ms = 0U;
static quaternion_t g_body_trim_q = {1.0f, 0.0f, 0.0f, 0.0f};
static euler_angle_t g_body_trim_euler = {0.0f, 0.0f, 0.0f};
static bool g_body_trim_ready = false;

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
    reset_body_trim();

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
        if (flight_main_calibrate_sensors(handle) != HAL_OK) {
            platform_debug_print("[FLIGHT] Sensor calibration FAILED!\r\n");
        }
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

    log_i2c_devices();

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

    /* 初始化磁力计 (QMC5883P, I2C1, 自动探测地址/寄存器布局) */
    status = qmc5883p_init(&g_mag, &hi2c1);
    if (status != HAL_OK) {
        handle->error = ERR_MAG_INIT;
        platform_debug_print("[SENSOR] Magnetometer init failed!\r\n");
        /* 磁力计非关键，继续 */
    } else {
        platform_debug_print("[SENSOR] Magnetometer init OK addr=0x%02X layout=%u chip=0x%02X ctrl1=0x%02X ctrl2=0x%02X\r\n",
                             g_mag.dev_addr,
                             (unsigned)g_mag.layout,
                             g_mag.chip_id,
                             g_mag.reg_ctrl1,
                             g_mag.reg_ctrl2);
    }

    return HAL_OK;
}

hal_status_t flight_main_calibrate_sensors(flight_main_handle_t *handle)
{
    uint32_t valid_samples = 0U;
    vec3f_t level_accel = {0.0f, 0.0f, 0.0f};
    vec3f_t avg_gyro = {0.0f, 0.0f, 0.0f};

    if (handle == NULL || !handle->sensors_ok) {
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
            valid_samples++;
        }

        platform_delay_us(1000); /* 1ms延时 */
    }

    if (valid_samples == 0U) {
        handle->error = ERR_SENSOR_FAIL;
        handle->state = SYS_STATE_ERROR;
        platform_debug_print("[CAL] No valid IMU samples\r\n");
        return HAL_ERROR;
    }

    icm42688_data_t avg_imu = {
        .accel_x = accel_sum.x / (float)valid_samples,
        .accel_y = accel_sum.y / (float)valid_samples,
        .accel_z = accel_sum.z / (float)valid_samples,
        .gyro_x = gyro_sum.x / (float)valid_samples,
        .gyro_y = gyro_sum.y / (float)valid_samples,
        .gyro_z = gyro_sum.z / (float)valid_samples,
        .temp = 0.0f
    };

    map_imu_to_body_frame(&avg_imu, &level_accel, &avg_gyro);

    if (update_body_trim_from_level_accel(&level_accel)) {
        apply_body_trim(&level_accel);
        apply_body_trim(&avg_gyro);
        platform_debug_print("[CAL] Body trim deg: %.2f %.2f\r\n",
                             g_body_trim_euler.roll * 57.2957795f,
                             g_body_trim_euler.pitch * 57.2957795f);
    } else {
        platform_debug_print("[CAL] Body trim skipped\r\n");
    }

    handle->gyro_bias = avg_gyro;

    if (flight_controller_align_to_gravity(&handle->flight_ctrl, &level_accel) == HAL_OK) {
        platform_debug_print("[CAL] Level accel: %.3f %.3f %.3f g\r\n",
                             level_accel.x, level_accel.y, level_accel.z);
    } else {
        platform_debug_print("[CAL] Level align skipped\r\n");
    }

    platform_debug_print("[CAL] Gyro bias: %.3f, %.3f, %.3f\r\n",
                         handle->gyro_bias.x, handle->gyro_bias.y, handle->gyro_bias.z);

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

    /* 额外输出 10Hz 整数姿态日志，便于真机稳定性验证。 */
    log_attitude_sample(handle);

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
        "[STATUS] State: %s, Armed: %d, Mode: %s, "
        "Att: R=%.1f P=%.1f Y=%.1f, "
        "Loops: %lu, IMU: %lu\r\n",
        state_str[state],
        flight_controller_is_armed(&handle->flight_ctrl),
        flight_mode_name(flight_controller_get_mode(&handle->flight_ctrl)),
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

static const char *flight_mode_name(flight_mode_t mode)
{
    switch (mode) {
        case FLIGHT_MODE_DISARMED:
            return "DISARMED";
        case FLIGHT_MODE_ARMED:
            return "ARMED";
        case FLIGHT_MODE_STABILIZE:
            return "STABILIZE";
        case FLIGHT_MODE_ACRO:
            return "ACRO";
        default:
            return "UNKNOWN";
    }
}

static void log_i2c_devices(void)
{
    uint8_t found_addr[8];
    uint8_t found_count = 0U;

    if (i2c_scan(&hi2c1, found_addr, (uint8_t)(sizeof(found_addr) / sizeof(found_addr[0])), &found_count) != HAL_OK) {
        platform_debug_print("[I2C] scan failed\r\n");
        return;
    }

    platform_debug_print("[I2C] found=%u", found_count);
    for (uint8_t i = 0; i < found_count && i < (uint8_t)(sizeof(found_addr) / sizeof(found_addr[0])); i++) {
        platform_debug_print(" 0x%02X", found_addr[i]);
    }
    platform_debug_print("\r\n");
}

static void reset_mag_runtime_calibration(void)
{
    g_mag_filter_ready = false;
    g_mag_filtered.x = 0.0f;
    g_mag_filtered.y = 0.0f;
    g_mag_filtered.z = 0.0f;
    g_last_mag_poll_ms = 0U;
}

static void reset_body_trim(void)
{
    g_body_trim_q.w = 1.0f;
    g_body_trim_q.x = 0.0f;
    g_body_trim_q.y = 0.0f;
    g_body_trim_q.z = 0.0f;
    g_body_trim_euler.roll = 0.0f;
    g_body_trim_euler.pitch = 0.0f;
    g_body_trim_euler.yaw = 0.0f;
    g_body_trim_ready = false;
}

static bool update_body_trim_from_level_accel(const vec3f_t *level_accel)
{
    if (level_accel == NULL || !ahrs_accel_valid(level_accel)) {
        reset_body_trim();
        return false;
    }

    g_body_trim_euler.roll = atan2f(level_accel->y, level_accel->z);
    g_body_trim_euler.pitch = asinf(fmaxf(-1.0f, fminf(1.0f, -level_accel->x)));
    g_body_trim_euler.yaw = 0.0f;
    euler_to_quaternion(&g_body_trim_q, &g_body_trim_euler);
    quat_normalize(&g_body_trim_q);
    g_body_trim_ready = true;
    return true;
}

static void apply_body_trim(vec3f_t *vector)
{
    vec3f_t rotated;

    if (vector == NULL || !g_body_trim_ready) {
        return;
    }

    quat_rotate_vector(&rotated, &g_body_trim_q, vector);
    *vector = rotated;
}

static vec3f_t apply_mag_runtime_calibration(const qmc5883p_data_t *mag_data)
{
    vec3f_t corrected = {
        mag_data->mag_x_gauss,
        mag_data->mag_y_gauss,
        mag_data->mag_z_gauss
    };

    /* Do not calibrate min/max online during flight. A partial XY sweep makes
     * heading nonlinear and can inflate yaw rotation well beyond the real angle.
     * Keep raw Gauss values here; hard/soft iron compensation must come from a
     * dedicated calibration procedure with fixed parameters.
     */
    return corrected;
}

static void map_imu_to_body_frame(const icm42688_data_t *imu_data, vec3f_t *accel, vec3f_t *gyro)
{
    if (imu_data == NULL || accel == NULL || gyro == NULL) {
        return;
    }

    /* Reverse-engineered from bench motion tests:
     * body +X (forward) = IMU +Y
     * body +Y (left)    = IMU -X
     * body +Z (up)      = IMU +Z
     */
    accel->x = imu_data->accel_y;
    accel->y = -imu_data->accel_x;
    accel->z = imu_data->accel_z;

    gyro->x = imu_data->gyro_y * 3.14159f / 180.0f;
    gyro->y = -imu_data->gyro_x * 3.14159f / 180.0f;
    gyro->z = imu_data->gyro_z * 3.14159f / 180.0f;
}

static void read_all_sensors(flight_main_handle_t *handle)
{
    uint32_t now;

    /* 读取IMU */
    icm42688_data_t imu_data;
    if (icm42688_read_data(&g_imu, &imu_data) == HAL_OK) {
        map_imu_to_body_frame(&imu_data, &handle->accel, &handle->gyro);
        apply_body_trim(&handle->accel);
        apply_body_trim(&handle->gyro);
        handle->gyro.x -= handle->gyro_bias.x;
        handle->gyro.y -= handle->gyro_bias.y;
        handle->gyro.z -= handle->gyro_bias.z;
        handle->sensors_ok = true;
    } else {
        handle->sensors_ok = false;
    }

    if (g_mag_filter_ready) {
        handle->mag = g_mag_filtered;
        apply_body_trim(&handle->mag);
        handle->mag_valid = true;
    } else {
        handle->mag_valid = false;
    }

    /* 读取磁力计 (可选) */
    now = platform_get_time_ms();
    if (g_last_mag_poll_ms != 0U &&
        (uint32_t)(now - g_last_mag_poll_ms) < MAG_POLL_INTERVAL_MS) {
        return;
    }
    g_last_mag_poll_ms = now;

    qmc5883p_data_t mag_data;
    hal_status_t mag_status = qmc5883p_read_data(&g_mag, &mag_data);
    if (mag_status == HAL_OK) {
        vec3f_t calibrated_mag = apply_mag_runtime_calibration(&mag_data);
        if (!g_mag_filter_ready) {
            g_mag_filtered = calibrated_mag;
            g_mag_filter_ready = true;
        } else {
            const float alpha = 0.02f;
            g_mag_filtered.x += alpha * (calibrated_mag.x - g_mag_filtered.x);
            g_mag_filtered.y += alpha * (calibrated_mag.y - g_mag_filtered.y);
            g_mag_filtered.z += alpha * (calibrated_mag.z - g_mag_filtered.z);
        }

        handle->mag = g_mag_filtered;
        apply_body_trim(&handle->mag);
        handle->mag_valid = true;
        g_last_mag_sample = mag_data;
        g_last_mag_valid = true;
    } else if (mag_status == HAL_BUSY) {
        /* 没有新样本时继续沿用上一次有效磁场数据。 */
    } else {
        handle->mag_valid = false;
        g_last_mag_valid = false;
        reset_mag_runtime_calibration();
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

static void log_attitude_sample(const flight_main_handle_t *handle)
{
    static uint32_t last_log_ms = 0U;
    static uint32_t last_mag_log_ms = 0U;
    uint32_t now;
    euler_angle_t attitude;
    vec3f_t gyro;
    motor_outputs_t motors = {0U, 0U, 0U, 0U};
    bool armed;
    flight_mode_t mode;
    int32_t roll_cdeg;
    int32_t pitch_cdeg;
    int32_t yaw_cdeg;
    int32_t roll_rate_ddeg;
    int32_t pitch_rate_ddeg;
    int32_t yaw_rate_ddeg;
    int32_t trim_roll_cdeg;
    int32_t trim_pitch_cdeg;
    int32_t roll_rate_sp_ddeg;
    int32_t pitch_rate_sp_ddeg;
    int32_t roll_out_raw;
    int32_t pitch_out_raw;

    if (handle == NULL || !handle->initialized) {
        return;
    }

    now = platform_get_time_ms();
    if ((now - last_log_ms) < 100U) {
        return;
    }
    last_log_ms = now;

    if (!flight_controller_get_attitude(&handle->flight_ctrl, &attitude)) {
        return;
    }

    armed = flight_controller_is_armed(&handle->flight_ctrl);
    mode = flight_controller_get_mode(&handle->flight_ctrl);
    flight_controller_get_motors(&handle->flight_ctrl, &motors);

    roll_cdeg = (int32_t)(attitude.roll * 100.0f);
    pitch_cdeg = (int32_t)(attitude.pitch * 100.0f);
    yaw_cdeg = (int32_t)(attitude.yaw * 100.0f);
    roll_rate_ddeg = 0;
    pitch_rate_ddeg = 0;
    yaw_rate_ddeg = 0;

    if (flight_controller_get_gyro(&handle->flight_ctrl, &gyro)) {
        roll_rate_ddeg = (int32_t)(gyro.x * 57.2957795f * 10.0f);
        pitch_rate_ddeg = (int32_t)(gyro.y * 57.2957795f * 10.0f);
        yaw_rate_ddeg = (int32_t)(gyro.z * 57.2957795f * 10.0f);
    }

    trim_roll_cdeg = (int32_t)(handle->flight_ctrl.attitude_trim.roll * 100.0f);
    trim_pitch_cdeg = (int32_t)(handle->flight_ctrl.attitude_trim.pitch * 100.0f);
    roll_rate_sp_ddeg = (int32_t)(handle->flight_ctrl.debug_roll_rate_sp * 10.0f);
    pitch_rate_sp_ddeg = (int32_t)(handle->flight_ctrl.debug_pitch_rate_sp * 10.0f);
    roll_out_raw = (int32_t)handle->flight_ctrl.debug_roll_out;
    pitch_out_raw = (int32_t)handle->flight_ctrl.debug_pitch_out;

    platform_debug_print("[ATT_CDEG] t=%lu r=%ld p=%ld y=%ld rr=%ld pr=%ld yr=%ld tr=%ld tp=%ld rrs=%ld prs=%ld ro=%ld po=%ld m=%d arm=%d md=%u m1=%u m2=%u m3=%u m4=%u\r\n",
                         (unsigned long)now,
                         (long)roll_cdeg,
                         (long)pitch_cdeg,
                         (long)yaw_cdeg,
                         (long)roll_rate_ddeg,
                         (long)pitch_rate_ddeg,
                         (long)yaw_rate_ddeg,
                         (long)trim_roll_cdeg,
                         (long)trim_pitch_cdeg,
                         (long)roll_rate_sp_ddeg,
                         (long)pitch_rate_sp_ddeg,
                         (long)roll_out_raw,
                         (long)pitch_out_raw,
                         handle->mag_valid ? 1 : 0,
                         armed ? 1 : 0,
                         (unsigned)mode,
                         (unsigned)motors.motor1,
                         (unsigned)motors.motor2,
                         (unsigned)motors.motor3,
                         (unsigned)motors.motor4);

    if (handle->mag_valid && g_last_mag_valid && (now - last_mag_log_ms) >= 1000U) {
        float heading_deg = atan2f(-handle->mag.y, handle->mag.x) * 57.2957795f;
        int32_t heading_cdeg = (int32_t)(heading_deg * 100.0f);
        last_mag_log_ms = now;
        platform_debug_print("[MAG_DBG] t=%lu addr=0x%02X lay=%u mx=%d my=%d mz=%d hx=%ld\r\n",
                             (unsigned long)now,
                             g_mag.dev_addr,
                             (unsigned)g_mag.layout,
                             (int)g_last_mag_sample.mag_x,
                             (int)g_last_mag_sample.mag_y,
                             (int)g_last_mag_sample.mag_z,
                             (long)heading_cdeg);
    }
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
