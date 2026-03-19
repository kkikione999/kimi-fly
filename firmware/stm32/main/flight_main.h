/**
 * @file flight_main.h
 * @brief 飞行控制主程序头文件
 *
 * @note 整合所有模块的主控制程序
 *       包含1kHz控制循环和WiFi通信处理
 *
 * @author Drone Control System
 * @version 1.0
 */

#ifndef FLIGHT_MAIN_H
#define FLIGHT_MAIN_H

#include "../hal/hal_common.h"
#include "../comm/wifi_command.h"
#include "../algorithm/flight_controller.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * 系统配置常量
 * ============================================================================ */

#define MAIN_LOOP_RATE_HZ           1000    /**< 主循环频率 (Hz) */
#define MAIN_LOOP_PERIOD_US         1000    /**< 主循环周期 (us) */

#define WIFI_TASK_INTERVAL_MS       5       /**< WiFi任务间隔 (ms, 200Hz) */
#define TELEMETRY_INTERVAL_MS       20      /**< 遥测发送间隔 (ms, 50Hz) */
#define STATUS_INTERVAL_MS          1000    /**< 状态发送间隔 (ms, 1Hz) */

#define SENSOR_INIT_RETRY_MAX       3       /**< 传感器初始化重试次数 */
#define CALIBRATION_SAMPLES         1000    /**< 校准采样次数 */

/* ============================================================================
 * 系统状态
 * ============================================================================ */

/**
 * @brief 系统运行状态
 */
typedef enum {
    SYS_STATE_INIT = 0,         /**< 初始化中 */
    SYS_STATE_CALIBRATION,      /**< 传感器校准中 */
    SYS_STATE_STANDBY,          /**< 待机 (未解锁) */
    SYS_STATE_ACTIVE,           /**< 运行中 (已解锁) */
    SYS_STATE_ERROR             /**< 错误状态 */
} system_state_t;

/**
 * @brief 错误代码
 */
typedef enum {
    ERR_NONE = 0,               /**< 无错误 */
    ERR_IMU_INIT,               /**< IMU初始化失败 */
    ERR_BARO_INIT,              /**< 气压计初始化失败 */
    ERR_MAG_INIT,               /**< 磁力计初始化失败 */
    ERR_AHRS_INIT,              /**< AHRS初始化失败 */
    ERR_FC_INIT,                /**< 飞控初始化失败 */
    ERR_WIFI_TIMEOUT,           /**< WiFi通信超时 */
    ERR_LOW_BATTERY,            /**< 低电压 */
    ERR_SENSOR_FAIL             /**< 传感器故障 */
} error_code_t;

/* ============================================================================
 * 主控制句柄
 * ============================================================================ */

/**
 * @brief 飞行控制主句柄
 */
typedef struct {
    /* 子系统 */
    flight_controller_t flight_ctrl;        /**< 飞行控制器 */
    wifi_command_handle_t wifi_cmd;         /**< WiFi命令处理器 */

    /* 状态 */
    system_state_t state;                   /**< 系统状态 */
    error_code_t error;                     /**< 错误代码 */
    bool initialized;                       /**< 初始化完成标志 */
    bool sensors_ok;                        /**< 传感器正常标志 */

    /* 传感器数据 */
    vec3f_t accel;                          /**< 加速度计数据 */
    vec3f_t gyro;                           /**< 陀螺仪数据 */
    vec3f_t mag;                            /**< 磁力计数据 */
    bool mag_valid;                         /**< 磁力计数据有效 */

    /* 时间跟踪 */
    uint32_t loop_count;                    /**< 循环计数 */
    uint32_t last_wifi_task;                /**< 上次WiFi任务时间 */
    uint32_t last_telemetry;                /**< 上次遥测时间 */
    uint32_t last_status;                   /**< 上次状态时间 */
    uint32_t last_1s_tick;                  /**< 上次1秒刻度 */

    /* 统计 */
    uint32_t imu_read_count;                /**< IMU读取计数 */
    uint32_t wifi_rx_count;                 /**< WiFi接收计数 */
    uint32_t wifi_tx_count;                 /**< WiFi发送计数 */
} flight_main_handle_t;

/* ============================================================================
 * API函数声明
 * ============================================================================ */

/**
 * @brief 初始化飞行控制系统
 * @param handle 主控制句柄
 * @return HAL_OK成功
 */
hal_status_t flight_main_init(flight_main_handle_t *handle);

/**
 * @brief 反初始化飞行控制系统
 * @param handle 主控制句柄
 */
void flight_main_deinit(flight_main_handle_t *handle);

/**
 * @brief 传感器初始化 (单独调用，可重试)
 * @param handle 主控制句柄
 * @return HAL_OK成功
 */
hal_status_t flight_main_init_sensors(flight_main_handle_t *handle);

/**
 * @brief 执行传感器校准
 * @param handle 主控制句柄
 * @return HAL_OK成功
 */
hal_status_t flight_main_calibrate_sensors(flight_main_handle_t *handle);

/**
 * @brief 主控制循环 (1kHz)
 * @param handle 主控制句柄
 * @note 此函数应在1kHz定时器中断或主循环中调用
 */
void flight_main_control_loop(flight_main_handle_t *handle);

/**
 * @brief WiFi任务处理 (200Hz)
 * @param handle 主控制句柄
 */
void flight_main_wifi_task(flight_main_handle_t *handle);

/**
 * @brief 获取系统状态
 * @param handle 主控制句柄
 * @return 系统状态
 */
system_state_t flight_main_get_state(const flight_main_handle_t *handle);

/**
 * @brief 获取错误代码
 * @param handle 主控制句柄
 * @return 错误代码
 */
error_code_t flight_main_get_error(const flight_main_handle_t *handle);

/**
 * @brief 清除错误
 * @param handle 主控制句柄
 */
void flight_main_clear_error(flight_main_handle_t *handle);

/**
 * @brief 获取循环计数 (可用于计算运行时间)
 * @param handle 主控制句柄
 * @return 循环计数
 */
uint32_t flight_main_get_loop_count(const flight_main_handle_t *handle);

/**
 * @brief 打印系统状态到调试串口
 * @param handle 主控制句柄
 */
void flight_main_print_status(const flight_main_handle_t *handle);

/* ============================================================================
 * 平台相关接口 (弱引用，用户可覆盖)
 * ============================================================================ */

/**
 * @brief 获取当前时间 (毫秒)
 * @return 时间戳 (ms)
 */
__attribute__((weak))
uint32_t platform_get_time_ms(void);

/**
 * @brief 获取当前时间 (微秒)
 * @return 时间戳 (us)
 */
__attribute__((weak))
uint32_t platform_get_time_us(void);

/**
 * @brief 微秒延时
 * @param us 延时时间
 */
__attribute__((weak))
void platform_delay_us(uint32_t us);

/**
 * @brief 调试输出 (类似printf)
 * @param fmt 格式字符串
 * @param ... 可变参数
 */
__attribute__((weak))
void platform_debug_print(const char *fmt, ...);

/**
 * @brief 读取IMU数据 (用户实现)
 * @param accel 加速度输出
 * @param gyro 角速度输出
 * @return true成功
 */
__attribute__((weak))
bool platform_read_imu(vec3f_t *accel, vec3f_t *gyro);

/**
 * @brief 读取磁力计数据 (用户实现)
 * @param mag 磁场输出
 * @return true成功
 */
__attribute__((weak))
bool platform_read_mag(vec3f_t *mag);

/**
 * @brief 设置电机输出 (用户实现)
 * @param m1 电机1输出
 * @param m2 电机2输出
 * @param m3 电机3输出
 * @param m4 电机4输出
 */
__attribute__((weak))
void platform_set_motors(uint16_t m1, uint16_t m2, uint16_t m3, uint16_t m4);

#ifdef __cplusplus
}
#endif

#endif /* FLIGHT_MAIN_H */
