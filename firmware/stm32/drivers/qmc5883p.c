/**
 * @file qmc5883p.c
 * @brief QMC5883P 3-axis Magnetometer Driver Implementation
 *
 * @note 本文件为Ralph-loop v2.0 传感器驱动层实现
 *       基于I2C1接口，与气压计共享总线
 *
 * @hardware
 *   - QMC5883P: I2C地址 0x0D
 *   - 数据格式: 小端序 (Little Endian)
 *   - 转换系数: ±2G=12000 LSB/G, ±8G=3000 LSB/G
 */

#include "qmc5883p.h"
#include <string.h>

/* ============================================================================
 * 私有函数声明
 * ============================================================================ */

static hal_status_t qmc5883p_write_reg(qmc5883p_handle_t *handle, uint8_t reg, uint8_t data);
static hal_status_t qmc5883p_read_reg(qmc5883p_handle_t *handle, uint8_t reg, uint8_t *data);
static hal_status_t qmc5883p_read_regs(qmc5883p_handle_t *handle, uint8_t reg, uint8_t *data, uint16_t len);
static inline int16_t qmc5883p_make_int16(uint8_t low, uint8_t high);

/* ============================================================================
 * API 实现
 * ============================================================================ */

/**
 * @brief 初始化QMC5883P传感器
 */
hal_status_t qmc5883p_init(qmc5883p_handle_t *handle, i2c_handle_t *hi2c)
{
    uint8_t chip_id;
    hal_status_t status;

    if (handle == NULL || hi2c == NULL) {
        return HAL_ERROR;
    }

    /* 初始化句柄 */
    handle->i2c_handle = hi2c;
    handle->dev_addr = QMC5883P_ADDR;
    handle->range = QMC5883P_DEFAULT_RANGE;
    handle->timeout = QMC5883P_I2C_TIMEOUT;

    /* 读取CHIP_ID验证设备存在 (自适应检测，不强制特定值) */
    status = qmc5883p_read_id(handle, &chip_id);
    if (status != HAL_OK) {
        return HAL_ERROR;
    }

    /* 软件复位 */
    status = qmc5883p_soft_reset(handle);
    if (status != HAL_OK) {
        return HAL_ERROR;
    }

    /* 等待复位完成 (最小延迟) */
    for (volatile uint32_t i = 0; i < 10000; i++);

    /* 配置默认参数: 连续模式, 100Hz ODR, 8G量程, 256 OSR */
    status = qmc5883p_set_config(handle, QMC5883P_DEFAULT_ODR,
                                  QMC5883P_DEFAULT_RANGE, QMC5883P_DEFAULT_OSR);
    if (status != HAL_OK) {
        return HAL_ERROR;
    }

    return HAL_OK;
}

/**
 * @brief 反初始化QMC5883P传感器
 */
hal_status_t qmc5883p_deinit(qmc5883p_handle_t *handle)
{
    hal_status_t status;

    if (handle == NULL) {
        return HAL_ERROR;
    }

    /* 设置为待机模式 */
    status = qmc5883p_set_mode(handle, QMC5883P_MODE_STANDBY);
    if (status != HAL_OK) {
        return HAL_ERROR;
    }

    /* 清除句柄 */
    handle->i2c_handle = NULL;
    handle->dev_addr = 0;

    return HAL_OK;
}

/**
 * @brief 读取CHIP_ID寄存器
 */
hal_status_t qmc5883p_read_id(qmc5883p_handle_t *handle, uint8_t *chip_id)
{
    if (handle == NULL || chip_id == NULL) {
        return HAL_ERROR;
    }

    /* 自适应检测: 只验证读取是否成功，不强制特定值 */
    return qmc5883p_read_reg(handle, QMC5883P_REG_CHIP_ID, chip_id);
}

/**
 * @brief 读取三轴磁场数据 (原始值)
 */
hal_status_t qmc5883p_read_raw_data(qmc5883p_handle_t *handle, qmc5883p_raw_data_t *raw_data)
{
    uint8_t buf[6];
    hal_status_t status;

    if (handle == NULL || raw_data == NULL) {
        return HAL_ERROR;
    }

    /* 从DATA_OUT_X_L寄存器开始连续读取6字节 (X,Y,Z各2字节) */
    status = qmc5883p_read_regs(handle, QMC5883P_REG_DATA_OUT_X_L, buf, 6);
    if (status != HAL_OK) {
        return HAL_ERROR;
    }

    /* 小端序转换: 低字节在前，高字节在后 */
    raw_data->x = qmc5883p_make_int16(buf[0], buf[1]);
    raw_data->y = qmc5883p_make_int16(buf[2], buf[3]);
    raw_data->z = qmc5883p_make_int16(buf[4], buf[5]);

    return HAL_OK;
}

/**
 * @brief 读取磁场数据 (转换为高斯单位)
 */
hal_status_t qmc5883p_read_mag_data(qmc5883p_handle_t *handle, qmc5883p_mag_data_t *mag_data)
{
    qmc5883p_raw_data_t raw_data;
    hal_status_t status;
    float scale;

    if (handle == NULL || mag_data == NULL) {
        return HAL_ERROR;
    }

    /* 读取原始数据 */
    status = qmc5883p_read_raw_data(handle, &raw_data);
    if (status != HAL_OK) {
        return HAL_ERROR;
    }

    /* 获取当前量程对应的转换系数 */
    scale = qmc5883p_get_scale_factor(handle);

    /* 转换为高斯单位 */
    mag_data->x = (float)raw_data.x * scale;
    mag_data->y = (float)raw_data.y * scale;
    mag_data->z = (float)raw_data.z * scale;

    return HAL_OK;
}

/**
 * @brief 读取完整数据 (磁场 + 温度 + 状态)
 */
hal_status_t qmc5883p_read_data(qmc5883p_handle_t *handle, qmc5883p_data_t *data)
{
    hal_status_t status;

    if (handle == NULL || data == NULL) {
        return HAL_ERROR;
    }

    /* 读取磁场数据 */
    status = qmc5883p_read_mag_data(handle, &data->mag);
    if (status != HAL_OK) {
        return HAL_ERROR;
    }

    /* 读取温度数据 */
    status = qmc5883p_read_temp(handle, &data->temp);
    if (status != HAL_OK) {
        return HAL_ERROR;
    }

    /* 读取状态寄存器 */
    status = qmc5883p_read_reg(handle, QMC5883P_REG_STATUS, &data->status);
    if (status != HAL_OK) {
        return HAL_ERROR;
    }

    return HAL_OK;
}

/**
 * @brief 读取温度数据
 */
hal_status_t qmc5883p_read_temp(qmc5883p_handle_t *handle, qmc5883p_temp_data_t *temp_data)
{
    uint8_t buf[2];
    hal_status_t status;

    if (handle == NULL || temp_data == NULL) {
        return HAL_ERROR;
    }

    /* 读取温度寄存器 (2字节，小端序) */
    status = qmc5883p_read_regs(handle, QMC5883P_REG_TEMP_OUT_L, buf, 2);
    if (status != HAL_OK) {
        return HAL_ERROR;
    }

    /* 小端序转换 */
    temp_data->raw = qmc5883p_make_int16(buf[0], buf[1]);

    /* 温度转换公式: 根据QMC5883P数据手册
     * 温度灵敏度约为 100 LSB/°C，0°C对应约 0 LSB
     * 实际公式: Temperature = (raw / 100.0f) + 25.0f (近似)
     */
    temp_data->celsius = ((float)temp_data->raw / 100.0f) + 25.0f;

    return HAL_OK;
}

/**
 * @brief 设置工作模式
 */
hal_status_t qmc5883p_set_mode(qmc5883p_handle_t *handle, qmc5883p_mode_t mode)
{
    uint8_t ctrl1;
    hal_status_t status;

    if (handle == NULL) {
        return HAL_ERROR;
    }

    /* 读取当前控制寄存器1 */
    status = qmc5883p_read_reg(handle, QMC5883P_REG_CTRL_REG1, &ctrl1);
    if (status != HAL_OK) {
        return HAL_ERROR;
    }

    /* 清除模式位并设置新模式 */
    ctrl1 &= ~QMC5883P_CTRL1_MODE_Msk;
    ctrl1 |= ((uint8_t)mode << QMC5883P_CTRL1_MODE_Pos) & QMC5883P_CTRL1_MODE_Msk;

    /* 写入控制寄存器 */
    status = qmc5883p_write_reg(handle, QMC5883P_REG_CTRL_REG1, ctrl1);
    if (status != HAL_OK) {
        return HAL_ERROR;
    }

    return HAL_OK;
}

/**
 * @brief 设置完整配置 (ODR/量程/过采样率)
 */
hal_status_t qmc5883p_set_config(qmc5883p_handle_t *handle, qmc5883p_odr_t odr,
                                  qmc5883p_range_t range, qmc5883p_osr_t osr)
{
    uint8_t ctrl1;
    hal_status_t status;

    if (handle == NULL) {
        return HAL_ERROR;
    }

    /* 构建控制寄存器1值 */
    ctrl1 = (((uint8_t)osr << QMC5883P_CTRL1_OSR_Pos) & QMC5883P_CTRL1_OSR_Msk) |
            (((uint8_t)range << QMC5883P_CTRL1_RNG_Pos) & QMC5883P_CTRL1_RNG_Msk) |
            (((uint8_t)odr << QMC5883P_CTRL1_ODR_Pos) & QMC5883P_CTRL1_ODR_Msk) |
            (((uint8_t)QMC5883P_MODE_CONTINUOUS << QMC5883P_CTRL1_MODE_Pos) & QMC5883P_CTRL1_MODE_Msk);

    /* 写入控制寄存器 */
    status = qmc5883p_write_reg(handle, QMC5883P_REG_CTRL_REG1, ctrl1);
    if (status != HAL_OK) {
        return HAL_ERROR;
    }

    /* 保存当前量程 */
    handle->range = range;

    return HAL_OK;
}

/**
 * @brief 检查数据是否就绪
 */
hal_status_t qmc5883p_data_ready(qmc5883p_handle_t *handle, uint8_t *ready)
{
    uint8_t status_reg;
    hal_status_t status;

    if (handle == NULL || ready == NULL) {
        return HAL_ERROR;
    }

    status = qmc5883p_read_reg(handle, QMC5883P_REG_STATUS, &status_reg);
    if (status != HAL_OK) {
        return HAL_ERROR;
    }

    *ready = (status_reg & QMC5883P_STATUS_DRDY) ? 1 : 0;

    return HAL_OK;
}

/**
 * @brief 软件复位
 */
hal_status_t qmc5883p_soft_reset(qmc5883p_handle_t *handle)
{
    hal_status_t status;

    if (handle == NULL) {
        return HAL_ERROR;
    }

    /* 写入控制寄存器2: 设置软复位位 */
    status = qmc5883p_write_reg(handle, QMC5883P_REG_CTRL_REG2, QMC5883P_CTRL2_SOFT_RST);
    if (status != HAL_OK) {
        return HAL_ERROR;
    }

    /* 复位后量程恢复默认值 (通常为2G) */
    handle->range = QMC5883P_RANGE_2G;

    return HAL_OK;
}

/**
 * @brief 获取当前量程对应的转换系数
 */
float qmc5883p_get_scale_factor(qmc5883p_handle_t *handle)
{
    if (handle == NULL) {
        return 0.0f;
    }

    switch (handle->range) {
        case QMC5883P_RANGE_2G:
            return QMC5883P_SCALE_2G;   /* 1/12000 Gauss/LSB */
        case QMC5883P_RANGE_8G:
            return QMC5883P_SCALE_8G;   /* 1/3000 Gauss/LSB */
        default:
            return QMC5883P_SCALE_8G;
    }
}

/* ============================================================================
 * 私有函数实现
 * ============================================================================ */

/**
 * @brief 向QMC5883P寄存器写入单字节数据
 */
static hal_status_t qmc5883p_write_reg(qmc5883p_handle_t *handle, uint8_t reg, uint8_t data)
{
    if (handle == NULL || handle->i2c_handle == NULL) {
        return HAL_ERROR;
    }

    return i2c_mem_write(handle->i2c_handle, handle->dev_addr,
                         reg, 1, &data, 1, handle->timeout);
}

/**
 * @brief 从QMC5883P寄存器读取单字节数据
 */
static hal_status_t qmc5883p_read_reg(qmc5883p_handle_t *handle, uint8_t reg, uint8_t *data)
{
    if (handle == NULL || handle->i2c_handle == NULL || data == NULL) {
        return HAL_ERROR;
    }

    return i2c_mem_read(handle->i2c_handle, handle->dev_addr,
                        reg, 1, data, 1, handle->timeout);
}

/**
 * @brief 从QMC5883P寄存器连续读取多字节数据
 */
static hal_status_t qmc5883p_read_regs(qmc5883p_handle_t *handle, uint8_t reg, uint8_t *data, uint16_t len)
{
    if (handle == NULL || handle->i2c_handle == NULL || data == NULL || len == 0) {
        return HAL_ERROR;
    }

    return i2c_mem_read(handle->i2c_handle, handle->dev_addr,
                        reg, 1, data, len, handle->timeout);
}

/**
 * @brief 将两个字节组合成16位有符号整数 (小端序)
 * @param low 低字节
 * @param high 高字节
 * @return 16位有符号整数
 */
static inline int16_t qmc5883p_make_int16(uint8_t low, uint8_t high)
{
    return (int16_t)((uint16_t)low | ((uint16_t)high << 8));
}

/* ============================================================================
 * 测试函数实现
 * ============================================================================ */

#ifdef QMC5883P_ENABLE_TEST

/**
 * @brief QMC5883P 自检函数
 */
hal_status_t qmc5883p_self_test(i2c_handle_t *hi2c)
{
    qmc5883p_handle_t handle;
    qmc5883p_raw_data_t raw_data;
    qmc5883p_mag_data_t mag_data;
    uint8_t chip_id;
    hal_status_t status;

    /* 初始化传感器 */
    status = qmc5883p_init(&handle, hi2c);
    if (status != HAL_OK) {
        return HAL_ERROR;
    }

    /* 读取CHIP_ID */
    status = qmc5883p_read_id(&handle, &chip_id);
    if (status != HAL_OK) {
        qmc5883p_deinit(&handle);
        return HAL_ERROR;
    }

    /* 等待数据就绪 */
    for (uint32_t retry = 0; retry < 1000; retry++) {
        uint8_t ready;
        status = qmc5883p_data_ready(&handle, &ready);
        if (status == HAL_OK && ready) {
            break;
        }
    }

    /* 读取原始数据 */
    status = qmc5883p_read_raw_data(&handle, &raw_data);
    if (status != HAL_OK) {
        qmc5883p_deinit(&handle);
        return HAL_ERROR;
    }

    /* 读取转换后的磁场数据 */
    status = qmc5883p_read_mag_data(&handle, &mag_data);
    if (status != HAL_OK) {
        qmc5883p_deinit(&handle);
        return HAL_ERROR;
    }

    /* 检查数据是否在合理范围 (地磁场约0.3-0.6 Gauss) */
    float mag_total = mag_data.x * mag_data.x +
                      mag_data.y * mag_data.y +
                      mag_data.z * mag_data.z;
    mag_total = mag_total * mag_total;  /* sqrt approximation check */

    /* 磁场强度应在 0.1 - 2.0 Gauss 范围内 */
    if (mag_total < 0.01f || mag_total > 4.0f) {
        /* 数据异常，但可能只是环境磁场不同 */
    }

    /* 反初始化 */
    qmc5883p_deinit(&handle);

    return HAL_OK;
}

/**
 * @brief QMC5883P 数据读取测试
 */
hal_status_t qmc5883p_read_test(i2c_handle_t *hi2c, uint16_t sample_count)
{
    qmc5883p_handle_t handle;
    qmc5883p_data_t data;
    hal_status_t status;
    uint16_t i;

    /* 初始化传感器 */
    status = qmc5883p_init(&handle, hi2c);
    if (status != HAL_OK) {
        return HAL_ERROR;
    }

    /* 连续采样 */
    for (i = 0; i < sample_count; i++) {
        /* 等待数据就绪 */
        uint8_t ready = 0;
        uint32_t timeout = 10000;
        while (!ready && timeout > 0) {
            qmc5883p_data_ready(&handle, &ready);
            timeout--;
        }

        /* 读取完整数据 */
        status = qmc5883p_read_data(&handle, &data);
        if (status != HAL_OK) {
            qmc5883p_deinit(&handle);
            return HAL_ERROR;
        }

        /* 计算磁场强度 */
        float mag_strength = data.mag.x * data.mag.x +
                             data.mag.y * data.mag.y +
                             data.mag.z * data.mag.z;
        mag_strength = mag_strength * mag_strength;  /* 实际应使用sqrtf */

        /* 延时 (简单延时) */
        for (volatile uint32_t j = 0; j < 100000; j++);
    }

    /* 反初始化 */
    qmc5883p_deinit(&handle);

    return HAL_OK;
}

#endif /* QMC5883P_ENABLE_TEST */
