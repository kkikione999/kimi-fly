/**
 * @file protocol.h
 * @brief WiFi通信协议定义
 *
 * @note 定义STM32与ESP32-C3之间的通信协议帧格式
 *       采用二进制帧格式，包含帧头、长度、命令、数据、CRC校验
 *
 * @author Drone Control System
 * @version 1.0
 */

#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "../hal/hal_common.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * 协议常量定义
 * ============================================================================ */

#define PROTOCOL_FRAME_HEADER       0xAA55  /**< 帧头 (2字节，大端序) */
#define PROTOCOL_MAX_PAYLOAD_SIZE   64      /**< 最大负载字节数 */
#define PROTOCOL_MAX_FRAME_SIZE     (PROTOCOL_MAX_PAYLOAD_SIZE + 8) /**< 最大帧大小 */

#define PROTOCOL_VERSION            0x01    /**< 协议版本 */

/* ============================================================================
 * 命令定义 (CMD)
 * ============================================================================ */

/**
 * @brief 命令类型枚举
 */
typedef enum {
    /* 系统命令 (0x00-0x0F) */
    CMD_HEARTBEAT = 0x00,       /**< 心跳 */
    CMD_ACK = 0x01,             /**< 确认响应 */
    CMD_NACK = 0x02,            /**< 否定响应 */
    CMD_VERSION = 0x03,         /**< 获取版本 */
    CMD_STATUS = 0x04,          /**< 获取状态 */

    /* 飞控命令 (0x10-0x1F) */
    CMD_ARM = 0x10,             /**< 解锁 */
    CMD_DISARM = 0x11,          /**< 锁定 */
    CMD_MODE = 0x12,            /**< 设置飞行模式 */
    CMD_RC_INPUT = 0x13,        /**< RC输入 */

    /* 遥测命令 (0x20-0x2F) */
    CMD_TELEMETRY_ATT = 0x20,   /**< 姿态遥测 */
    CMD_TELEMETRY_IMU = 0x21,   /**< IMU原始数据 */
    CMD_TELEMETRY_MOTOR = 0x22, /**< 电机输出 */
    CMD_TELEMETRY_BATTERY = 0x23, /**< 电池电压 */

    /* 配置命令 (0x30-0x3F) */
    CMD_PID_GET = 0x30,         /**< 获取PID参数 */
    CMD_PID_SET = 0x31,         /**< 设置PID参数 */
    CMD_CALIBRATE = 0x32,       /**< 校准命令 */

    CMD_MAX = 0x3F
} protocol_cmd_t;

/**
 * @brief 响应状态码
 */
typedef enum {
    RESP_OK = 0x00,             /**< 成功 */
    RESP_ERR_INVALID_CMD = 0x01, /**< 无效命令 */
    RESP_ERR_INVALID_PARAM = 0x02, /**< 无效参数 */
    RESP_ERR_BUSY = 0x03,       /**< 忙 */
    RESP_ERR_NOT_ARMED = 0x04,  /**< 未解锁 */
    RESP_ERR_ARMED = 0x05,      /**< 已解锁 */
    RESP_ERR_CRC = 0x06,        /**< CRC错误 */
    RESP_ERR_TIMEOUT = 0x07,    /**< 超时 */
    RESP_ERR_INTERNAL = 0x08,   /**< 内部错误 */
} protocol_resp_t;

/* ============================================================================
 * 数据类型定义
 * ============================================================================ */

/**
 * @brief 协议帧结构
 * @note 帧格式: [HEADER:2][LEN:1][CMD:1][DATA:N][CRC:2]
 *       总长度 = N + 6
 */
typedef struct {
    uint8_t cmd;                                    /**< 命令字节 */
    uint8_t len;                                    /**< 数据长度 */
    uint8_t data[PROTOCOL_MAX_PAYLOAD_SIZE];        /**< 数据负载 */
} protocol_frame_t;

/**
 * @brief RC输入数据包 (用于CMD_RC_INPUT)
 */
typedef struct __attribute__((packed)) {
    int16_t throttle;   /**< 油门: 0-1000 */
    int16_t roll;       /**< 横滚: -500 ~ +500 */
    int16_t pitch;      /**< 俯仰: -500 ~ +500 */
    int16_t yaw;        /**< 偏航: -500 ~ +500 */
    uint8_t buttons;    /**< 按钮位图 */
} protocol_rc_data_t;

/**
 * @brief 姿态遥测数据包 (用于CMD_TELEMETRY_ATT)
 */
typedef struct __attribute__((packed)) {
    int16_t roll;       /**< 横滚角 (度 * 100) */
    int16_t pitch;      /**< 俯仰角 (度 * 100) */
    int16_t yaw;        /**< 偏航角 (度 * 100) */
    int16_t roll_rate;  /**< 横滚角速度 (度/秒 * 10) */
    int16_t pitch_rate; /**< 俯仰角速度 (度/秒 * 10) */
    int16_t yaw_rate;   /**< 偏航角速度 (度/秒 * 10) */
} protocol_attitude_t;

/**
 * @brief 电机遥测数据包 (用于CMD_TELEMETRY_MOTOR)
 */
typedef struct __attribute__((packed)) {
    uint16_t motor1;    /**< 电机1输出 */
    uint16_t motor2;    /**< 电机2输出 */
    uint16_t motor3;    /**< 电机3输出 */
    uint16_t motor4;    /**< 电机4输出 */
    uint8_t armed;      /**< 解锁状态 */
    uint8_t mode;       /**< 飞行模式 */
} protocol_motor_t;

/**
 * @brief 状态响应数据包 (用于CMD_STATUS)
 */
typedef struct __attribute__((packed)) {
    uint8_t version;    /**< 协议版本 */
    uint8_t armed;      /**< 解锁状态 */
    uint8_t mode;       /**< 飞行模式 */
    uint8_t status;     /**< 系统状态 */
    uint16_t error_flags; /**< 错误标志位 */
} protocol_status_t;

/**
 * @brief PID参数数据包 (用于CMD_PID_GET/SET)
 */
typedef struct __attribute__((packed)) {
    uint8_t channel;    /**< PID通道ID */
    int16_t kp;         /**< Kp * 1000 */
    int16_t ki;         /**< Ki * 1000 */
    int16_t kd;         /**< Kd * 1000 */
} protocol_pid_data_t;

/* ============================================================================
 * API函数声明 - 帧编码/解码
 * ============================================================================ */

/**
 * @brief 编码帧为字节流
 * @param frame 输入帧结构
 * @param buffer 输出缓冲区
 * @param buf_size 缓冲区大小
 * @param out_len 输出长度指针
 * @return HAL_OK成功
 */
hal_status_t protocol_encode(const protocol_frame_t *frame,
                              uint8_t *buffer,
                              uint16_t buf_size,
                              uint16_t *out_len);

/**
 * @brief 解码字节流为帧
 * @param buffer 输入缓冲区
 * @param len 输入长度
 * @param frame 输出帧结构
 * @param consumed 消耗的字节数 (可为NULL)
 * @return HAL_OK成功，HAL_ERROR校验失败
 */
hal_status_t protocol_decode(const uint8_t *buffer,
                              uint16_t len,
                              protocol_frame_t *frame,
                              uint16_t *consumed);

/**
 * @brief 计算CRC16校验
 * @param data 数据指针
 * @param len 数据长度
 * @return CRC16值
 */
uint16_t protocol_calc_crc16(const uint8_t *data, uint16_t len);

/**
 * @brief 验证帧CRC
 * @param frame 帧数据 (包含CRC)
 * @param len 帧长度
 * @return true CRC校验通过
 */
bool protocol_verify_crc(const uint8_t *frame, uint16_t len);

/* ============================================================================
 * API函数声明 - 数据打包/解包辅助函数
 * ============================================================================ */

/**
 * @brief 打包RC输入数据
 * @param rc RC数据结构
 * @param frame 输出帧
 */
void protocol_pack_rc(const protocol_rc_data_t *rc, protocol_frame_t *frame);

/**
 * @brief 解包RC输入数据
 * @param frame 输入帧
 * @param rc 输出RC数据结构
 * @return true成功
 */
bool protocol_unpack_rc(const protocol_frame_t *frame, protocol_rc_data_t *rc);

/**
 * @brief 打包姿态遥测数据
 * @param att 姿态数据结构
 * @param frame 输出帧
 */
void protocol_pack_attitude(const protocol_attitude_t *att, protocol_frame_t *frame);

/**
 * @brief 打包电机遥测数据
 * @param motor 电机数据结构
 * @param frame 输出帧
 */
void protocol_pack_motor(const protocol_motor_t *motor, protocol_frame_t *frame);

/**
 * @brief 打包状态数据
 * @param status 状态数据结构
 * @param frame 输出帧
 */
void protocol_pack_status(const protocol_status_t *status, protocol_frame_t *frame);

/**
 * @brief 创建ACK响应帧
 * @param cmd 被确认的命令
 * @param frame 输出帧
 */
void protocol_make_ack(uint8_t cmd, protocol_frame_t *frame);

/**
 * @brief 创建NACK响应帧
 * @param cmd 被拒绝的命令
 * @param error 错误码
 * @param frame 输出帧
 */
void protocol_make_nack(uint8_t cmd, uint8_t error, protocol_frame_t *frame);

#ifdef __cplusplus
}
#endif

#endif /* PROTOCOL_H */
