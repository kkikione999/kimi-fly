/**
 * @file icm42688.c
 * @brief ICM-42688-P 6轴IMU传感器驱动实现 (I2C版本)
 *
 * @note 基于TDK InvenSense ICM-42688-P数据手册
 *       接口: I2C1 (PB6=SCL, PB7=SDA)
 *       I2C地址: 0x68
 *
 * @hardware
 *   - 芯片型号: ICM-42688-P
 *   - 接口: I2C1 (PB6=SCL, PB7=SDA)
 *   - I2C地址: 0x68
 *   - WHO_AM_I: 0x47
 */

#include "icm42688.h"
#include <string.h>

/* ============================================================================
 * 私有宏定义
 * ============================================================================ */

#define ICM42688_TIMEOUT_MS         100U    /**< I2C超时时间(ms) */
#define ICM42688_RESET_DELAY_MS     10U     /**< 复位后延时 */
#define ICM42688_STARTUP_DELAY_MS   10U     /**< 启动延时 */

/* ============================================================================
 * 私有函数声明
 * ============================================================================ */

static hal_status_t icm42688_write_reg(icm42688_handle_t *himu, uint8_t reg, uint8_t value);
static hal_status_t icm42688_read_reg(icm42688_handle_t *himu, uint8_t reg, uint8_t *p_value);
static hal_status_t icm42688_read_burst(icm42688_handle_t *himu, uint8_t start_reg, uint8_t *p_data, uint16_t len);
static float icm42688_get_gyro_sensitivity(icm42688_gyro_range_t range);
static float icm42688_get_accel_sensitivity(icm42688_accel_range_t range);
static void icm42688_delay_ms(uint32_t ms);

/* ============================================================================
 * 私有函数实现
 * ============================================================================ */

/**
 * @brief 简单延时函数
 */
static void icm42688_delay_ms(uint32_t ms)
{
    volatile uint32_t count;
    while (ms--) {
        for (count = 0; count < 10000; count++) {
            __asm__ volatile("nop");
        }
    }
}

/**
 * @brief 向寄存器写入一个字节 (I2C)
 */
static hal_status_t icm42688_write_reg(icm42688_handle_t *himu, uint8_t reg, uint8_t value)
{
    if (himu == NULL || himu->i2c_handle == NULL) {
        return HAL_ERROR;
    }
    return i2c_mem_write(himu->i2c_handle, himu->dev_addr, reg, 1U, &value, 1U, himu->timeout);
}

/**
 * @brief 从寄存器读取一个字节 (I2C)
 */
static hal_status_t icm42688_read_reg(icm42688_handle_t *himu, uint8_t reg, uint8_t *p_value)
{
    if (himu == NULL || himu->i2c_handle == NULL || p_value == NULL) {
        return HAL_ERROR;
    }
    return i2c_mem_read(himu->i2c_handle, himu->dev_addr, reg, 1U, p_value, 1U, himu->timeout);
}

/**
 * @brief 从连续寄存器读取多个字节 (I2C burst read)
 */
static hal_status_t icm42688_read_burst(icm42688_handle_t *himu, uint8_t start_reg, uint8_t *p_data, uint16_t len)
{
    if (himu == NULL || himu->i2c_handle == NULL || p_data == NULL || len == 0) {
        return HAL_ERROR;
    }
    return i2c_mem_read(himu->i2c_handle, himu->dev_addr, start_reg, 1U, p_data, len, himu->timeout);
}

/**
 * @brief 获取当前陀螺仪灵敏度系数
 */
static float icm42688_get_gyro_sensitivity(icm42688_gyro_range_t range)
{
    switch (range) {
        case ICM42688_GYRO_RANGE_2000DPS:
            return ICM42688_GYRO_SENSITIVITY_2000DPS;
        case ICM42688_GYRO_RANGE_1000DPS:
            return ICM42688_GYRO_SENSITIVITY_1000DPS;
        case ICM42688_GYRO_RANGE_500DPS:
            return ICM42688_GYRO_SENSITIVITY_500DPS;
        case ICM42688_GYRO_RANGE_250DPS:
            return ICM42688_GYRO_SENSITIVITY_250DPS;
        case ICM42688_GYRO_RANGE_125DPS:
            return ICM42688_GYRO_SENSITIVITY_125DPS;
        default:
            return ICM42688_GYRO_SENSITIVITY_2000DPS;
    }
}

/**
 * @brief 获取当前加速度计灵敏度系数
 */
static float icm42688_get_accel_sensitivity(icm42688_accel_range_t range)
{
    switch (range) {
        case ICM42688_ACCEL_RANGE_16G:
            return ICM42688_ACCEL_SENSITIVITY_16G;
        case ICM42688_ACCEL_RANGE_8G:
            return ICM42688_ACCEL_SENSITIVITY_8G;
        case ICM42688_ACCEL_RANGE_4G:
            return ICM42688_ACCEL_SENSITIVITY_4G;
        case ICM42688_ACCEL_RANGE_2G:
            return ICM42688_ACCEL_SENSITIVITY_2G;
        default:
            return ICM42688_ACCEL_SENSITIVITY_16G;
    }
}

/* ============================================================================
 * API函数实现
 * ============================================================================ */

/**
 * @brief 初始化ICM-42688-P传感器 (I2C版本)
 */
hal_status_t icm42688_init(icm42688_handle_t *himu, i2c_handle_t *hi2c)
{
    hal_status_t status;
    uint8_t id;
    uint8_t reg_val;

    if (himu == NULL || hi2c == NULL) {
        return HAL_ERROR;
    }

    /* 清零句柄 */
    memset(himu, 0, sizeof(icm42688_handle_t));

    /* 保存I2C句柄 */
    himu->i2c_handle = hi2c;
    himu->dev_addr = ICM42688_I2C_ADDR;
    himu->timeout = ICM42688_TIMEOUT_MS;

    /* 延时等待传感器上电稳定 */
    icm42688_delay_ms(ICM42688_STARTUP_DELAY_MS);

    /* 读取并验证WHO_AM_I */
    status = icm42688_read_id(himu, &id);
    if (status != HAL_OK) {
        return status;
    }

    if (id != ICM42688_WHO_AM_I_VALUE) {
        return HAL_ERROR;
    }

    /* 软复位 */
    status = icm42688_reset(himu);
    if (status != HAL_OK) {
        return status;
    }

    /* 配置电源管理 - 开启陀螺仪和加速度计的低噪声模式 */
    reg_val = (ICM42688_PWR_MODE_LOW_NOISE << ICM42688_PWR_MGMT0_GYRO_MODE_Pos) |
              (ICM42688_PWR_MODE_LOW_NOISE << ICM42688_PWR_MGMT0_ACCEL_MODE_Pos);
    status = icm42688_write_reg(himu, ICM42688_REG_PWR_MGMT0, reg_val);
    if (status != HAL_OK) {
        return status;
    }

    /* 延时等待传感器启动 */
    icm42688_delay_ms(ICM42688_STARTUP_DELAY_MS);

    /* 设置默认量程和ODR */
    status = icm42688_set_gyro_range(himu, ICM42688_GYRO_RANGE_2000DPS);
    if (status != HAL_OK) {
        return status;
    }

    status = icm42688_set_accel_range(himu, ICM42688_ACCEL_RANGE_8G);
    if (status != HAL_OK) {
        return status;
    }

    status = icm42688_set_odr(himu, ICM42688_ODR_1KHZ, ICM42688_ODR_1KHZ);
    if (status != HAL_OK) {
        return status;
    }

    himu->initialized = 1;

    return HAL_OK;
}

/**
 * @brief 反初始化ICM-42688-P传感器
 */
hal_status_t icm42688_deinit(icm42688_handle_t *himu)
{
    hal_status_t status;

    if (himu == NULL || himu->i2c_handle == NULL) {
        return HAL_ERROR;
    }

    /* 关闭陀螺仪和加速度计 */
    status = icm42688_write_reg(himu, ICM42688_REG_PWR_MGMT0, 0x00);
    if (status != HAL_OK) {
        return status;
    }

    himu->initialized = 0;

    return HAL_OK;
}

/**
 * @brief 读取WHO_AM_I寄存器
 */
hal_status_t icm42688_read_id(icm42688_handle_t *himu, uint8_t *p_id)
{
    return icm42688_read_reg(himu, ICM42688_REG_WHO_AM_I, p_id);
}

/**
 * @brief 读取所有传感器原始数据
 * @note 使用burst read从TEMP_DATA1开始连续读取14字节
 *       数据格式: TEMP(2) + ACCEL_X(2) + ACCEL_Y(2) + ACCEL_Z(2) + GYRO_X(2) + GYRO_Y(2) + GYRO_Z(2)
 */
hal_status_t icm42688_read_raw_data(icm42688_handle_t *himu, icm42688_raw_data_t *p_raw_data)
{
    hal_status_t status;
    uint8_t buf[14];

    if (himu == NULL || p_raw_data == NULL) {
        return HAL_ERROR;
    }

    if (!himu->initialized) {
        return HAL_ERROR;
    }

    /* Burst read从TEMP_DATA1开始，连续读取14字节 */
    status = icm42688_read_burst(himu, ICM42688_REG_TEMP_DATA1, buf, 14);
    if (status != HAL_OK) {
        return status;
    }

    /* 解析数据 (大端序) */
    p_raw_data->temp    = (int16_t)((buf[0]  << 8) | buf[1]);
    p_raw_data->accel_x = (int16_t)((buf[2]  << 8) | buf[3]);
    p_raw_data->accel_y = (int16_t)((buf[4]  << 8) | buf[5]);
    p_raw_data->accel_z = (int16_t)((buf[6]  << 8) | buf[7]);
    p_raw_data->gyro_x  = (int16_t)((buf[8]  << 8) | buf[9]);
    p_raw_data->gyro_y  = (int16_t)((buf[10] << 8) | buf[11]);
    p_raw_data->gyro_z  = (int16_t)((buf[12] << 8) | buf[13]);

    return HAL_OK;
}

/**
 * @brief 读取并转换为物理单位的传感器数据
 */
hal_status_t icm42688_read_data(icm42688_handle_t *himu, icm42688_data_t *p_data)
{
    hal_status_t status;
    icm42688_raw_data_t raw_data;
    float gyro_sens;
    float accel_sens;

    if (himu == NULL || p_data == NULL) {
        return HAL_ERROR;
    }

    /* 读取原始数据 */
    status = icm42688_read_raw_data(himu, &raw_data);
    if (status != HAL_OK) {
        return status;
    }

    /* 获取当前灵敏度系数 */
    gyro_sens = icm42688_get_gyro_sensitivity(himu->gyro_range);
    accel_sens = icm42688_get_accel_sensitivity(himu->accel_range);

    /* 转换为物理单位 */
    p_data->gyro_x  = (float)raw_data.gyro_x * gyro_sens;
    p_data->gyro_y  = (float)raw_data.gyro_y * gyro_sens;
    p_data->gyro_z  = (float)raw_data.gyro_z * gyro_sens;
    p_data->accel_x = (float)raw_data.accel_x * accel_sens;
    p_data->accel_y = (float)raw_data.accel_y * accel_sens;
    p_data->accel_z = (float)raw_data.accel_z * accel_sens;
    p_data->temp    = icm42688_convert_temp_celsius(raw_data.temp);

    return HAL_OK;
}

/**
 * @brief 设置陀螺仪量程
 */
hal_status_t icm42688_set_gyro_range(icm42688_handle_t *himu, icm42688_gyro_range_t range)
{
    hal_status_t status;
    uint8_t reg_val;

    if (himu == NULL) {
        return HAL_ERROR;
    }

    /* 读取当前配置 */
    status = icm42688_read_reg(himu, ICM42688_REG_GYRO_CONFIG0, &reg_val);
    if (status != HAL_OK) {
        return status;
    }

    /* 清除FS_SEL位并设置新的量程 */
    reg_val &= ~ICM42688_GYRO_CONFIG0_FS_SEL_Msk;
    reg_val |= (range << ICM42688_GYRO_CONFIG0_FS_SEL_Pos) & ICM42688_GYRO_CONFIG0_FS_SEL_Msk;

    /* 写入配置 */
    status = icm42688_write_reg(himu, ICM42688_REG_GYRO_CONFIG0, reg_val);
    if (status != HAL_OK) {
        return status;
    }

    himu->gyro_range = range;

    return HAL_OK;
}

/**
 * @brief 设置加速度计量程
 */
hal_status_t icm42688_set_accel_range(icm42688_handle_t *himu, icm42688_accel_range_t range)
{
    hal_status_t status;
    uint8_t reg_val;

    if (himu == NULL) {
        return HAL_ERROR;
    }

    /* 读取当前配置 */
    status = icm42688_read_reg(himu, ICM42688_REG_ACCEL_CONFIG0, &reg_val);
    if (status != HAL_OK) {
        return status;
    }

    /* 清除FS_SEL位并设置新的量程 */
    reg_val &= ~ICM42688_ACCEL_CONFIG0_FS_SEL_Msk;
    reg_val |= (range << ICM42688_ACCEL_CONFIG0_FS_SEL_Pos) & ICM42688_ACCEL_CONFIG0_FS_SEL_Msk;

    /* 写入配置 */
    status = icm42688_write_reg(himu, ICM42688_REG_ACCEL_CONFIG0, reg_val);
    if (status != HAL_OK) {
        return status;
    }

    himu->accel_range = range;

    return HAL_OK;
}

/**
 * @brief 设置输出数据率
 */
hal_status_t icm42688_set_odr(icm42688_handle_t *himu, icm42688_odr_t gyro_odr, icm42688_odr_t accel_odr)
{
    hal_status_t status;
    uint8_t reg_val;

    if (himu == NULL) {
        return HAL_ERROR;
    }

    /* 配置陀螺仪ODR */
    status = icm42688_read_reg(himu, ICM42688_REG_GYRO_CONFIG0, &reg_val);
    if (status != HAL_OK) {
        return status;
    }

    reg_val &= ~ICM42688_GYRO_CONFIG0_ODR_Msk;
    reg_val |= (gyro_odr << ICM42688_GYRO_CONFIG0_ODR_Pos) & ICM42688_GYRO_CONFIG0_ODR_Msk;

    status = icm42688_write_reg(himu, ICM42688_REG_GYRO_CONFIG0, reg_val);
    if (status != HAL_OK) {
        return status;
    }

    /* 配置加速度计ODR */
    status = icm42688_read_reg(himu, ICM42688_REG_ACCEL_CONFIG0, &reg_val);
    if (status != HAL_OK) {
        return status;
    }

    reg_val &= ~ICM42688_ACCEL_CONFIG0_ODR_Msk;
    reg_val |= (accel_odr << ICM42688_ACCEL_CONFIG0_ODR_Pos) & ICM42688_ACCEL_CONFIG0_ODR_Msk;

    status = icm42688_write_reg(himu, ICM42688_REG_ACCEL_CONFIG0, reg_val);
    if (status != HAL_OK) {
        return status;
    }

    return HAL_OK;
}

/**
 * @brief 软复位传感器
 */
hal_status_t icm42688_reset(icm42688_handle_t *himu)
{
    hal_status_t status;

    if (himu == NULL) {
        return HAL_ERROR;
    }

    /* 关闭所有传感器 */
    status = icm42688_write_reg(himu, ICM42688_REG_PWR_MGMT0, 0x00);
    if (status != HAL_OK) {
        return status;
    }

    /* 延时等待 */
    icm42688_delay_ms(ICM42688_RESET_DELAY_MS);

    /* 重新配置为默认值 */
    status = icm42688_write_reg(himu, ICM42688_REG_GYRO_CONFIG0, 0x06);  /* 2000dps, 1kHz */
    if (status != HAL_OK) {
        return status;
    }

    status = icm42688_write_reg(himu, ICM42688_REG_ACCEL_CONFIG0, 0x06); /* 16g, 1kHz */
    if (status != HAL_OK) {
        return status;
    }

    /* 重置句柄中的量程设置 */
    himu->gyro_range = ICM42688_GYRO_RANGE_2000DPS;
    himu->accel_range = ICM42688_ACCEL_RANGE_16G;

    return HAL_OK;
}

/**
 * @brief 将原始陀螺仪数据转换为dps
 */
float icm42688_convert_gyro_dps(icm42688_handle_t *himu, int16_t raw_value)
{
    float sensitivity;

    if (himu == NULL) {
        return 0.0f;
    }

    sensitivity = icm42688_get_gyro_sensitivity(himu->gyro_range);
    return (float)raw_value * sensitivity;
}

/**
 * @brief 将原始加速度计数据转换为g
 */
float icm42688_convert_accel_g(icm42688_handle_t *himu, int16_t raw_value)
{
    float sensitivity;

    if (himu == NULL) {
        return 0.0f;
    }

    sensitivity = icm42688_get_accel_sensitivity(himu->accel_range);
    return (float)raw_value * sensitivity;
}

/**
 * @brief 将原始温度数据转换为摄氏度
 * @note 根据数据手册: Temperature = (TEMP_DATA / 132.48) + 25
 */
float icm42688_convert_temp_celsius(int16_t raw_value)
{
    return ((float)raw_value / ICM42688_TEMP_SENSITIVITY) + ICM42688_TEMP_OFFSET;
}

/**
 * @brief 自检函数 - 验证WHO_AM_I并读取一次数据
 */
hal_status_t icm42688_self_test(icm42688_handle_t *himu)
{
    hal_status_t status;
    uint8_t id;
    icm42688_data_t data;

    if (himu == NULL) {
        return HAL_ERROR;
    }

    /* 验证WHO_AM_I */
    status = icm42688_read_id(himu, &id);
    if (status != HAL_OK) {
        return status;
    }

    if (id != ICM42688_WHO_AM_I_VALUE) {
        return HAL_ERROR;
    }

    /* 读取一次数据验证通信正常 */
    status = icm42688_read_data(himu, &data);
    if (status != HAL_OK) {
        return status;
    }

    return HAL_OK;
}

/* ============================================================================
 * 测试代码
 * ============================================================================ */

#if defined(ICM42688_ENABLE_TEST)

#include <stdio.h>

/**
 * @brief ICM-42688-P 测试函数 (I2C版本)
 */
hal_status_t icm42688_test(i2c_handle_t *hi2c)
{
    hal_status_t status;
    icm42688_handle_t himu;
    uint8_t id;
    icm42688_data_t data;
    int i;

    printf("=== ICM-42688-P I2C Test ===\n");

    /* 初始化IMU */
    status = icm42688_init(&himu, hi2c);
    if (status != HAL_OK) {
        printf("[FAIL] IMU init failed\n");
        return status;
    }
    printf("[PASS] IMU initialized\n");

    /* 测试1: 读取WHO_AM_I */
    status = icm42688_read_id(&himu, &id);
    if (status == HAL_OK && id == ICM42688_WHO_AM_I_VALUE) {
        printf("[PASS] WHO_AM_I = 0x%02X\n", id);
    } else {
        printf("[FAIL] WHO_AM_I read failed, expected 0x%02X, got 0x%02X\n",
               ICM42688_WHO_AM_I_VALUE, id);
        return HAL_ERROR;
    }

    /* 测试2: 连续读取10次数据 */
    printf("\nReading 10 samples:\n");
    printf("%-4s %-10s %-10s %-10s %-10s %-10s %-10s %-8s\n",
           "#", "AccX(g)", "AccY(g)", "AccZ(g)", "GyroX(dps)", "GyroY(dps)", "GyroZ(dps)", "Temp(C)");

    for (i = 0; i < 10; i++) {
        status = icm42688_read_data(&himu, &data);
        if (status == HAL_OK) {
            printf("%-4d %-10.3f %-10.3f %-10.3f %-10.2f %-10.2f %-10.2f %-8.1f\n",
                   i + 1,
                   data.accel_x, data.accel_y, data.accel_z,
                   data.gyro_x, data.gyro_y, data.gyro_z,
                   data.temp);
        } else {
            printf("Sample %d: Read failed\n", i + 1);
        }
        icm42688_delay_ms(100);
    }

    /* 反初始化 */
    icm42688_deinit(&himu);

    printf("\n=== Test Complete ===\n");
    return HAL_OK;
}

#endif /* ICM42688_ENABLE_TEST */
