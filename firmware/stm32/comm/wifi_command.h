/**
 * @file wifi_command.h
 * @brief WiFi远程控制命令处理器
 *
 * @note 处理从ESP32-C3接收的WiFi命令，转换为飞控指令
 *       同时负责将遥测数据发送到ESP32
 *
 * @author Drone Control System
 * @version 1.0
 */

#ifndef WIFI_COMMAND_H
#define WIFI_COMMAND_H

#include "protocol.h"
#include "../algorithm/flight_controller.h"
#include "../hal/hal_common.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * 配置常量
 * ============================================================================ */

#define WIFI_CMD_BUFFER_SIZE        256     /**< 接收缓冲区大小 */
#define WIFI_TX_BUFFER_SIZE         128     /**< 发送缓冲区大小 */
#define WIFI_CMD_TIMEOUT_MS         500     /**< 命令超时时间 (ms) */
#define WIFI_HEARTBEAT_INTERVAL_MS  100     /**< 心跳间隔 (ms) */
#define WIFI_TELEMETRY_INTERVAL_MS  20      /**< 遥测发送间隔 (ms, 50Hz) */

/* ============================================================================
 * 数据类型定义
 * ============================================================================ */

/**
 * @brief WiFi连接状态
 */
typedef enum {
    WIFI_STATE_DISCONNECTED = 0,    /**< 未连接 */
    WIFI_STATE_CONNECTED,           /**< 已连接 (WiFi) */
    WIFI_STATE_LINK_OK,             /**< 通信链路正常 */
    WIFI_STATE_TIMEOUT              /**< 超时 */
} wifi_state_t;

/**
 * @brief WiFi命令处理器句柄
 */
typedef struct {
    /* 缓冲区 */
    uint8_t rx_buffer[WIFI_CMD_BUFFER_SIZE];    /**< 接收缓冲区 */
    uint8_t tx_buffer[WIFI_TX_BUFFER_SIZE];     /**< 发送缓冲区 */
    uint16_t rx_head;                           /**< 接收缓冲区写入位置 */
    uint16_t rx_tail;                           /**< 接收缓冲区读取位置 */

    /* 状态 */
    wifi_state_t state;                         /**< 连接状态 */
    uint32_t last_rx_time;                      /**< 上次接收时间 */
    uint32_t last_tx_time;                      /**< 上次发送时间 */
    uint32_t last_heartbeat;                    /**< 上次心跳时间 */

    /* 统计 */
    uint32_t rx_frames;                         /**< 接收帧计数 */
    uint32_t tx_frames;                         /**< 发送帧计数 */
    uint32_t rx_errors;                         /**< 接收错误计数 */
    uint32_t timeout_count;                     /**< 超时计数 */

    /* 控制输出 */
    rc_command_t rc_output;                     /**< 解析后的RC命令 */
    bool new_command;                           /**< 新命令标志 */

    /* 初始化标志 */
    bool initialized;
} wifi_command_handle_t;

/**
 * @brief WiFi遥测配置
 */
typedef struct {
    bool enable_attitude;       /**< 发送姿态数据 */
    bool enable_motor;          /**< 发送电机数据 */
    bool enable_battery;        /**< 发送电池数据 */
    uint16_t attitude_rate_hz;  /**< 姿态发送频率 */
    uint16_t motor_rate_hz;     /**< 电机发送频率 */
} wifi_telemetry_config_t;

/* ============================================================================
 * API函数声明 - 初始化和配置
 * ============================================================================ */

/**
 * @brief 初始化WiFi命令处理器
 * @param handle WiFi命令处理器句柄
 * @return HAL_OK成功
 */
hal_status_t wifi_command_init(wifi_command_handle_t *handle);

/**
 * @brief 反初始化WiFi命令处理器
 * @param handle WiFi命令处理器句柄
 */
void wifi_command_deinit(wifi_command_handle_t *handle);

/**
 * @brief 配置遥测输出
 * @param handle WiFi命令处理器句柄
 * @param config 遥测配置
 */
void wifi_command_config_telemetry(wifi_command_handle_t *handle,
                                    const wifi_telemetry_config_t *config);

/* ============================================================================
 * API函数声明 - 数据接收处理
 * ============================================================================ */

/**
 * @brief 接收字节数据 (从中断或DMA回调调用)
 * @param handle WiFi命令处理器句柄
 * @param data 接收到的字节
 */
void wifi_command_rx_byte(wifi_command_handle_t *handle, uint8_t data);

/**
 * @brief 批量接收数据
 * @param handle WiFi命令处理器句柄
 * @param data 数据缓冲区
 * @param len 数据长度
 */
void wifi_command_rx_bytes(wifi_command_handle_t *handle,
                            const uint8_t *data,
                            uint16_t len);

/**
 * @brief 处理接收到的数据 (兼容入口, 复用 execute 的单一解析路径)
 * @param handle WiFi命令处理器句柄
 * @return HAL_OK成功
 */
hal_status_t wifi_command_process_rx(wifi_command_handle_t *handle);

/* ============================================================================
 * API函数声明 - 命令执行
 * ============================================================================ */

/**
 * @brief 执行解析后的命令
 * @param handle WiFi命令处理器句柄
 * @param fc 飞行控制器句柄
 * @return HAL_OK成功
 */
hal_status_t wifi_command_execute(wifi_command_handle_t *handle,
                                   flight_controller_t *fc);

/**
 * @brief 获取解析后的RC命令
 * @param handle WiFi命令处理器句柄
 * @param rc RC命令输出
 * @return true有新命令
 */
bool wifi_command_get_rc(wifi_command_handle_t *handle, rc_command_t *rc);

/* ============================================================================
 * API函数声明 - 遥测发送
 * ============================================================================ */

/**
 * @brief 更新遥测数据 (定期调用)
 * @param handle WiFi命令处理器句柄
 * @param fc 飞行控制器句柄
 * @param current_time_ms 当前时间戳 (ms)
 * @return 发送的字节数
 */
uint16_t wifi_command_update_telemetry(wifi_command_handle_t *handle,
                                         flight_controller_t *fc,
                                         uint32_t current_time_ms);

/**
 * @brief 发送心跳包
 * @param handle WiFi命令处理器句柄
 */
void wifi_command_send_heartbeat(wifi_command_handle_t *handle);

/**
 * @brief 发送状态响应
 * @param handle WiFi命令处理器句柄
 * @param fc 飞行控制器句柄
 */
void wifi_command_send_status(wifi_command_handle_t *handle,
                               flight_controller_t *fc);

/* ============================================================================
 * API函数声明 - 状态查询
 * ============================================================================ */

/**
 * @brief 获取WiFi连接状态
 * @param handle WiFi命令处理器句柄
 * @return 连接状态
 */
wifi_state_t wifi_command_get_state(const wifi_command_handle_t *handle);

/**
 * @brief 检查链路是否超时
 * @param handle WiFi命令处理器句柄
 * @param current_time_ms 当前时间戳 (ms)
 * @return true已超时
 */
bool wifi_command_is_timeout(const wifi_command_handle_t *handle,
                              uint32_t current_time_ms);

/**
 * @brief 获取统计信息
 * @param handle WiFi命令处理器句柄
 * @param rx_frames 接收帧计数 (可为NULL)
 * @param tx_frames 发送帧计数 (可为NULL)
 * @param errors 错误计数 (可为NULL)
 */
void wifi_command_get_stats(const wifi_command_handle_t *handle,
                             uint32_t *rx_frames,
                             uint32_t *tx_frames,
                             uint32_t *errors);

/**
 * @brief 复位统计信息
 * @param handle WiFi命令处理器句柄
 */
void wifi_command_reset_stats(wifi_command_handle_t *handle);

/* ============================================================================
 * API函数声明 - 底层发送接口
 * ============================================================================ */

/**
 * @brief 发送原始帧 (需要用户实现底层发送)
 * @param handle WiFi命令处理器句柄
 * @param frame 要发送的帧
 * @return 发送的字节数
 */
uint16_t wifi_command_send_frame(wifi_command_handle_t *handle,
                                  const protocol_frame_t *frame);

/**
 * @brief 平台相关的底层发送函数
 * @param data 数据指针
 * @param len 数据长度
 * @return 发送的字节数
 */
uint16_t wifi_platform_send(const uint8_t *data, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif /* WIFI_COMMAND_H */
