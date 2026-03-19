/**
 * @file wifi_command.c
 * @brief WiFi远程控制命令处理器实现
 *
 * @note 处理从ESP32-C3接收的WiFi命令
 *
 * @author Drone Control System
 * @version 1.0
 */

#include "wifi_command.h"
#include "string.h"

/* ============================================================================
 * 静态函数声明
 * ============================================================================ */

static void handle_arm_command(wifi_command_handle_t *handle, flight_controller_t *fc);
static void handle_disarm_command(wifi_command_handle_t *handle, flight_controller_t *fc);
static void handle_mode_command(wifi_command_handle_t *handle, flight_controller_t *fc, const uint8_t *data, uint8_t len);
static void handle_rc_input_command(wifi_command_handle_t *handle, const uint8_t *data, uint8_t len);
static void handle_pid_get_command(wifi_command_handle_t *handle, flight_controller_t *fc, const uint8_t *data, uint8_t len);
static void handle_pid_set_command(wifi_command_handle_t *handle, flight_controller_t *fc, const uint8_t *data, uint8_t len);
static void send_ack(wifi_command_handle_t *handle, uint8_t cmd);
static void send_nack(wifi_command_handle_t *handle, uint8_t cmd, uint8_t error);
static uint16_t transmit_data(wifi_command_handle_t *handle, const uint8_t *data, uint16_t len);

/* ============================================================================
 * 遥测配置默认值
 * ============================================================================ */

static const wifi_telemetry_config_t default_telemetry_config = {
    .enable_attitude = true,
    .enable_motor = true,
    .enable_battery = false,
    .attitude_rate_hz = 50,
    .motor_rate_hz = 10
};

/* ============================================================================
 * API实现 - 初始化和配置
 * ============================================================================ */

hal_status_t wifi_command_init(wifi_command_handle_t *handle)
{
    if (handle == NULL) {
        return HAL_ERROR;
    }

    memset(handle, 0, sizeof(wifi_command_handle_t));

    handle->state = WIFI_STATE_DISCONNECTED;
    handle->initialized = true;

    /* 设置默认遥测配置 */
    wifi_command_config_telemetry(handle, &default_telemetry_config);

    return HAL_OK;
}

void wifi_command_deinit(wifi_command_handle_t *handle)
{
    if (handle == NULL) {
        return;
    }

    handle->initialized = false;
    handle->state = WIFI_STATE_DISCONNECTED;
}

void wifi_command_config_telemetry(wifi_command_handle_t *handle,
                                    const wifi_telemetry_config_t *config)
{
    if (handle == NULL || config == NULL) {
        return;
    }

    /* 实际应用中应该保存配置，这里简化处理 */
    (void)config;
}

/* ============================================================================
 * API实现 - 数据接收处理
 * ============================================================================ */

void wifi_command_rx_byte(wifi_command_handle_t *handle, uint8_t data)
{
    if (handle == NULL || !handle->initialized) {
        return;
    }

    uint16_t next_head = (handle->rx_head + 1) % WIFI_CMD_BUFFER_SIZE;

    /* 检查缓冲区溢出 */
    if (next_head == handle->rx_tail) {
        /* 缓冲区满，丢弃最老的数据 */
        handle->rx_tail = (handle->rx_tail + 1) % WIFI_CMD_BUFFER_SIZE;
        handle->rx_errors++;
    }

    handle->rx_buffer[handle->rx_head] = data;
    handle->rx_head = next_head;
}

void wifi_command_rx_bytes(wifi_command_handle_t *handle,
                            const uint8_t *data,
                            uint16_t len)
{
    if (handle == NULL || data == NULL || len == 0) {
        return;
    }

    for (uint16_t i = 0; i < len; i++) {
        wifi_command_rx_byte(handle, data[i]);
    }
}

hal_status_t wifi_command_process_rx(wifi_command_handle_t *handle)
{
    if (handle == NULL || !handle->initialized) {
        return HAL_ERROR;
    }

    /* 计算缓冲区中可用数据 */
    uint16_t available;
    if (handle->rx_head >= handle->rx_tail) {
        available = handle->rx_head - handle->rx_tail;
    } else {
        available = WIFI_CMD_BUFFER_SIZE - handle->rx_tail + handle->rx_head;
    }

    if (available < 6) {
        return HAL_OK;  /* 数据不足，等待更多数据 */
    }

    /* 创建线性缓冲区 */
    uint8_t temp_buffer[WIFI_CMD_BUFFER_SIZE];
    uint16_t temp_len = 0;

    for (uint16_t i = 0; i < available; i++) {
        temp_buffer[i] = handle->rx_buffer[(handle->rx_tail + i) % WIFI_CMD_BUFFER_SIZE];
        temp_len++;
    }

    /* 解析帧 */
    protocol_frame_t frame;
    uint16_t consumed = 0;
    hal_status_t status = protocol_decode(temp_buffer, temp_len, &frame, &consumed);

    if (status == HAL_OK) {
        /* 成功解析一帧 */
        handle->rx_tail = (handle->rx_tail + consumed) % WIFI_CMD_BUFFER_SIZE;
        handle->rx_frames++;
        handle->last_rx_time = 0;  /* 应由调用者设置实际时间 */

        /* 处理命令 */
        switch (frame.cmd) {
            case CMD_HEARTBEAT:
                send_ack(handle, CMD_HEARTBEAT);
                handle->state = WIFI_STATE_LINK_OK;
                break;

            case CMD_VERSION:
                /* 发送版本信息 */
                {
                    protocol_frame_t resp;
                    resp.cmd = CMD_VERSION;
                    resp.len = 1;
                    resp.data[0] = PROTOCOL_VERSION;
                    wifi_command_send_frame(handle, &resp);
                }
                break;

            default:
                /* 保存命令供execute处理 */
                handle->new_command = true;
                break;
        }

        return HAL_OK;
    } else {
        /* 解析失败，更新rx_tail */
        if (consumed > 0) {
            handle->rx_tail = (handle->rx_tail + consumed) % WIFI_CMD_BUFFER_SIZE;
        }
        return HAL_ERROR;
    }
}

/* ============================================================================
 * API实现 - 命令执行
 * ============================================================================ */

hal_status_t wifi_command_execute(wifi_command_handle_t *handle,
                                   flight_controller_t *fc)
{
    if (handle == NULL || fc == NULL || !handle->initialized) {
        return HAL_ERROR;
    }

    /* 计算缓冲区中可用数据 */
    uint16_t available;
    if (handle->rx_head >= handle->rx_tail) {
        available = handle->rx_head - handle->rx_tail;
    } else {
        available = WIFI_CMD_BUFFER_SIZE - handle->rx_tail + handle->rx_head;
    }

    if (available < 6) {
        return HAL_OK;
    }

    /* 创建线性缓冲区 */
    uint8_t temp_buffer[WIFI_CMD_BUFFER_SIZE];
    uint16_t temp_len = 0;

    for (uint16_t i = 0; i < available && i < WIFI_CMD_BUFFER_SIZE; i++) {
        temp_buffer[i] = handle->rx_buffer[(handle->rx_tail + i) % WIFI_CMD_BUFFER_SIZE];
        temp_len++;
    }

    /* 解析帧 */
    protocol_frame_t frame;
    uint16_t consumed = 0;
    hal_status_t status = protocol_decode(temp_buffer, temp_len, &frame, &consumed);

    if (status != HAL_OK) {
        if (consumed > 0) {
            handle->rx_tail = (handle->rx_tail + consumed) % WIFI_CMD_BUFFER_SIZE;
        }
        return HAL_ERROR;
    }

    /* 成功解析，更新缓冲区 */
    handle->rx_tail = (handle->rx_tail + consumed) % WIFI_CMD_BUFFER_SIZE;
    handle->rx_frames++;

    /* 执行命令 */
    switch (frame.cmd) {
        case CMD_ARM:
            handle_arm_command(handle, fc);
            break;

        case CMD_DISARM:
            handle_disarm_command(handle, fc);
            break;

        case CMD_MODE:
            handle_mode_command(handle, fc, frame.data, frame.len);
            break;

        case CMD_RC_INPUT:
            handle_rc_input_command(handle, frame.data, frame.len);
            break;

        case CMD_PID_GET:
            handle_pid_get_command(handle, fc, frame.data, frame.len);
            break;

        case CMD_PID_SET:
            handle_pid_set_command(handle, fc, frame.data, frame.len);
            break;

        case CMD_STATUS:
            wifi_command_send_status(handle, fc);
            break;

        case CMD_HEARTBEAT:
            send_ack(handle, CMD_HEARTBEAT);
            handle->state = WIFI_STATE_LINK_OK;
            break;

        case CMD_VERSION:
            {
                protocol_frame_t resp;
                resp.cmd = CMD_VERSION;
                resp.len = 1;
                resp.data[0] = PROTOCOL_VERSION;
                wifi_command_send_frame(handle, &resp);
            }
            break;

        default:
            send_nack(handle, frame.cmd, RESP_ERR_INVALID_CMD);
            break;
    }

    return HAL_OK;
}

bool wifi_command_get_rc(wifi_command_handle_t *handle, rc_command_t *rc)
{
    if (handle == NULL || rc == NULL || !handle->initialized) {
        return false;
    }

    if (!handle->new_command) {
        return false;
    }

    *rc = handle->rc_output;
    handle->new_command = false;
    return true;
}

/* ============================================================================
 * API实现 - 遥测发送
 * ============================================================================ */

uint16_t wifi_command_update_telemetry(wifi_command_handle_t *handle,
                                         flight_controller_t *fc,
                                         uint32_t current_time_ms)
{
    if (handle == NULL || fc == NULL || !handle->initialized) {
        return 0;
    }

    uint16_t total_sent = 0;

    /* 发送姿态遥测 (50Hz) */
    if ((current_time_ms - handle->last_tx_time) >= WIFI_TELEMETRY_INTERVAL_MS) {
        euler_angle_t attitude;
        if (flight_controller_get_attitude(fc, &attitude)) {
            protocol_frame_t frame;
            protocol_attitude_t att_data;

            att_data.roll = (int16_t)(attitude.roll * 100.0f);
            att_data.pitch = (int16_t)(attitude.pitch * 100.0f);
            att_data.yaw = (int16_t)(attitude.yaw * 100.0f);

            vec3f_t gyro;
            if (flight_controller_get_gyro(fc, &gyro)) {
                att_data.roll_rate = (int16_t)(gyro.x * 180.0f / 3.14159f * 10.0f);
                att_data.pitch_rate = (int16_t)(gyro.y * 180.0f / 3.14159f * 10.0f);
                att_data.yaw_rate = (int16_t)(gyro.z * 180.0f / 3.14159f * 10.0f);
            } else {
                att_data.roll_rate = 0;
                att_data.pitch_rate = 0;
                att_data.yaw_rate = 0;
            }

            protocol_pack_attitude(&att_data, &frame);
            total_sent += wifi_command_send_frame(handle, &frame);
        }

        handle->last_tx_time = current_time_ms;
    }

    return total_sent;
}

void wifi_command_send_heartbeat(wifi_command_handle_t *handle)
{
    if (handle == NULL || !handle->initialized) {
        return;
    }

    protocol_frame_t frame;
    frame.cmd = CMD_HEARTBEAT;
    frame.len = 0;

    wifi_command_send_frame(handle, &frame);
}

void wifi_command_send_status(wifi_command_handle_t *handle,
                               flight_controller_t *fc)
{
    if (handle == NULL || fc == NULL || !handle->initialized) {
        return;
    }

    protocol_frame_t frame;
    protocol_status_t status;

    status.version = PROTOCOL_VERSION;
    status.armed = flight_controller_is_armed(fc) ? 1 : 0;
    status.mode = (uint8_t)flight_controller_get_mode(fc);
    status.status = (uint8_t)handle->state;
    status.error_flags = 0;

    protocol_pack_status(&status, &frame);
    wifi_command_send_frame(handle, &frame);
}

/* ============================================================================
 * API实现 - 状态查询
 * ============================================================================ */

wifi_state_t wifi_command_get_state(const wifi_command_handle_t *handle)
{
    if (handle == NULL) {
        return WIFI_STATE_DISCONNECTED;
    }
    return handle->state;
}

bool wifi_command_is_timeout(const wifi_command_handle_t *handle,
                              uint32_t current_time_ms)
{
    if (handle == NULL) {
        return true;
    }

    if (handle->state == WIFI_STATE_DISCONNECTED) {
        return false;
    }

    return (current_time_ms - handle->last_rx_time) > WIFI_CMD_TIMEOUT_MS;
}

void wifi_command_get_stats(const wifi_command_handle_t *handle,
                             uint32_t *rx_frames,
                             uint32_t *tx_frames,
                             uint32_t *errors)
{
    if (handle == NULL) {
        return;
    }

    if (rx_frames != NULL) {
        *rx_frames = handle->rx_frames;
    }
    if (tx_frames != NULL) {
        *tx_frames = handle->tx_frames;
    }
    if (errors != NULL) {
        *errors = handle->rx_errors;
    }
}

void wifi_command_reset_stats(wifi_command_handle_t *handle)
{
    if (handle == NULL) {
        return;
    }

    handle->rx_frames = 0;
    handle->tx_frames = 0;
    handle->rx_errors = 0;
    handle->timeout_count = 0;
}

/* ============================================================================
 * API实现 - 底层发送接口
 * ============================================================================ */

uint16_t wifi_command_send_frame(wifi_command_handle_t *handle,
                                  const protocol_frame_t *frame)
{
    if (handle == NULL || frame == NULL || !handle->initialized) {
        return 0;
    }

    uint16_t len = 0;
    if (protocol_encode(frame, handle->tx_buffer, WIFI_TX_BUFFER_SIZE, &len) != HAL_OK) {
        return 0;
    }

    uint16_t sent = transmit_data(handle, handle->tx_buffer, len);
    if (sent == len) {
        handle->tx_frames++;
    }

    return sent;
}

/* 弱引用默认实现 */
__attribute__((weak))
uint16_t wifi_platform_send(const uint8_t *data, uint16_t len)
{
    /* 用户应覆盖此函数以提供实际的发送功能 */
    (void)data;
    return len;
}

/* ============================================================================
 * 静态函数实现
 * ============================================================================ */

static void handle_arm_command(wifi_command_handle_t *handle, flight_controller_t *fc)
{
    if (flight_controller_arm(fc)) {
        send_ack(handle, CMD_ARM);
        handle->state = WIFI_STATE_LINK_OK;
    } else {
        send_nack(handle, CMD_ARM, RESP_ERR_BUSY);
    }
}

static void handle_disarm_command(wifi_command_handle_t *handle, flight_controller_t *fc)
{
    flight_controller_disarm(fc);
    send_ack(handle, CMD_DISARM);
}

static void handle_mode_command(wifi_command_handle_t *handle, flight_controller_t *fc,
                                 const uint8_t *data, uint8_t len)
{
    if (len < 1) {
        send_nack(handle, CMD_MODE, RESP_ERR_INVALID_PARAM);
        return;
    }

    flight_mode_t mode = (flight_mode_t)data[0];
    if (flight_controller_set_mode(fc, mode) == HAL_OK) {
        send_ack(handle, CMD_MODE);
    } else {
        send_nack(handle, CMD_MODE, RESP_ERR_INVALID_PARAM);
    }
}

static void handle_rc_input_command(wifi_command_handle_t *handle, const uint8_t *data, uint8_t len)
{
    if (len != sizeof(protocol_rc_data_t)) {
        send_nack(handle, CMD_RC_INPUT, RESP_ERR_INVALID_PARAM);
        return;
    }

    protocol_rc_data_t rc_data;
    memcpy(&rc_data, data, sizeof(protocol_rc_data_t));

    /* 转换为内部RC命令格式 (归一化到 -1.0 ~ 1.0) */
    handle->rc_output.throttle = rc_data.throttle / 1000.0f;
    handle->rc_output.roll = rc_data.roll / 500.0f;
    handle->rc_output.pitch = rc_data.pitch / 500.0f;
    handle->rc_output.yaw = rc_data.yaw / 500.0f;
    handle->rc_output.armed = (rc_data.buttons & 0x01) ? true : false;
    handle->rc_output.mode_switch = (rc_data.buttons & 0x02) ? true : false;

    /* 限制范围 */
    if (handle->rc_output.throttle > 1.0f) handle->rc_output.throttle = 1.0f;
    if (handle->rc_output.throttle < 0.0f) handle->rc_output.throttle = 0.0f;
    if (handle->rc_output.roll > 1.0f) handle->rc_output.roll = 1.0f;
    if (handle->rc_output.roll < -1.0f) handle->rc_output.roll = -1.0f;
    if (handle->rc_output.pitch > 1.0f) handle->rc_output.pitch = 1.0f;
    if (handle->rc_output.pitch < -1.0f) handle->rc_output.pitch = -1.0f;
    if (handle->rc_output.yaw > 1.0f) handle->rc_output.yaw = 1.0f;
    if (handle->rc_output.yaw < -1.0f) handle->rc_output.yaw = -1.0f;

    handle->new_command = true;
    send_ack(handle, CMD_RC_INPUT);
}

static void handle_pid_get_command(wifi_command_handle_t *handle, flight_controller_t *fc,
                                    const uint8_t *data, uint8_t len)
{
    if (len < 1 || data[0] >= PID_CHANNEL_COUNT) {
        send_nack(handle, CMD_PID_GET, RESP_ERR_INVALID_PARAM);
        return;
    }

    pid_controller_t *pid = flight_pid_get_channel(&fc->pid_set, (pid_channel_id_t)data[0]);
    if (pid == NULL) {
        send_nack(handle, CMD_PID_GET, RESP_ERR_INVALID_PARAM);
        return;
    }

    pid_gains_t gains;
    pid_get_gains(pid, &gains);

    protocol_frame_t resp;
    protocol_pid_data_t pid_data;

    pid_data.channel = data[0];
    pid_data.kp = (int16_t)(gains.kp * 1000.0f);
    pid_data.ki = (int16_t)(gains.ki * 1000.0f);
    pid_data.kd = (int16_t)(gains.kd * 1000.0f);

    resp.cmd = CMD_PID_GET;
    resp.len = sizeof(protocol_pid_data_t);
    memcpy(resp.data, &pid_data, sizeof(protocol_pid_data_t));

    wifi_command_send_frame(handle, &resp);
}

static void handle_pid_set_command(wifi_command_handle_t *handle, flight_controller_t *fc,
                                    const uint8_t *data, uint8_t len)
{
    if (len != sizeof(protocol_pid_data_t)) {
        send_nack(handle, CMD_PID_SET, RESP_ERR_INVALID_PARAM);
        return;
    }

    protocol_pid_data_t pid_data;
    memcpy(&pid_data, data, sizeof(protocol_pid_data_t));

    if (pid_data.channel >= PID_CHANNEL_COUNT) {
        send_nack(handle, CMD_PID_SET, RESP_ERR_INVALID_PARAM);
        return;
    }

    pid_controller_t *pid = flight_pid_get_channel(&fc->pid_set, (pid_channel_id_t)pid_data.channel);
    if (pid == NULL) {
        send_nack(handle, CMD_PID_SET, RESP_ERR_INVALID_PARAM);
        return;
    }

    float kp = pid_data.kp / 1000.0f;
    float ki = pid_data.ki / 1000.0f;
    float kd = pid_data.kd / 1000.0f;

    pid_set_gains(pid, kp, ki, kd);
    send_ack(handle, CMD_PID_SET);
}

static void send_ack(wifi_command_handle_t *handle, uint8_t cmd)
{
    protocol_frame_t frame;
    protocol_make_ack(cmd, &frame);
    wifi_command_send_frame(handle, &frame);
}

static void send_nack(wifi_command_handle_t *handle, uint8_t cmd, uint8_t error)
{
    protocol_frame_t frame;
    protocol_make_nack(cmd, error, &frame);
    wifi_command_send_frame(handle, &frame);
}

static uint16_t transmit_data(wifi_command_handle_t *handle, const uint8_t *data, uint16_t len)
{
    return wifi_platform_send(data, len);
}
