/**
 * @file qmc5883p.c
 * @brief QMC5883P 磁力计驱动实现
 *
 * @note 本文件为Ralph-loop v2.0 传感器驱动层实现
 *       提供三轴磁场数据读取功能
 *
 * @hardware
 *   - 芯片型号: QMC5883P
 *   - 接口: I2C1 (PB6=SCL, PB7=SDA)
 *   - I2C地址: 运行时自动探测
 */

#include "qmc5883p.h"
#include <string.h>

/* ============================================================================
 * 私有宏定义
 * ============================================================================ */

#define QMC5883P_TIMEOUT_DEFAULT    100U    /* 默认超时时间 (ms) */
#define QMC5883P_RESET_DELAY_MS     10U     /* 与已验证 sensor_test_main.c 保持一致 */
#define QMC5883P_STARTUP_DELAY_MS   20U     /* 模式切换后的启动等待 */
#define QMC5883P_DRDY_TIMEOUT_MS    50U     /* 首次数据就绪等待超时 */
#define QMC5883P_AXIS_SIGN_VALUE    0x06U   /* Rev.C 推荐值 */

/* 温度转换系数 */
#define QMC5883P_TEMP_SCALE         100.0f  /* 温度转换系数 */

/* ============================================================================
 * 私有函数声明
 * ============================================================================ */

static hal_status_t qmc5883p_write_reg(qmc5883p_handle_t *hqmc, uint8_t reg, uint8_t data);
static hal_status_t qmc5883p_read_reg(qmc5883p_handle_t *hqmc, uint8_t reg, uint8_t *data);
static hal_status_t qmc5883p_read_regs(qmc5883p_handle_t *hqmc, uint8_t reg, uint8_t *data, uint16_t len);
static hal_status_t qmc5883p_config(qmc5883p_handle_t *hqmc, qmc5883p_odr_t odr, qmc5883p_osr_t osr);
static hal_status_t qmc5883p_try_probe(qmc5883p_handle_t *hqmc, uint16_t addr, qmc5883p_layout_t layout);
static hal_status_t qmc5883p_wait_data_ready(qmc5883p_handle_t *hqmc, uint32_t timeout_ms);
static uint8_t qmc5883p_build_official_ctrl1(qmc5883p_odr_t odr);
static void qmc5883p_delay_ms(uint32_t ms);

/* ============================================================================
 * 私有函数实现
 * ============================================================================ */

/**
 * @brief 简单延时函数
 */
static void qmc5883p_delay_ms(uint32_t ms)
{
    volatile uint32_t count;
    while (ms--) {
        for (count = 0; count < 10000; count++) {
            __asm__ volatile("nop");
        }
    }
}

/**
 * @brief 向寄存器写入单字节数据
 */
static hal_status_t qmc5883p_write_reg(qmc5883p_handle_t *hqmc, uint8_t reg, uint8_t data)
{
    return i2c_mem_write(hqmc->i2c, hqmc->dev_addr, reg, 1U, &data, 1U, hqmc->timeout);
}

/**
 * @brief 从寄存器读取单字节数据
 */
static hal_status_t qmc5883p_read_reg(qmc5883p_handle_t *hqmc, uint8_t reg, uint8_t *data)
{
    return i2c_mem_read(hqmc->i2c, hqmc->dev_addr, reg, 1U, data, 1U, hqmc->timeout);
}

/**
 * @brief 从寄存器连续读取多字节数据
 */
static hal_status_t qmc5883p_read_regs(qmc5883p_handle_t *hqmc, uint8_t reg, uint8_t *data, uint16_t len)
{
    return i2c_mem_read(hqmc->i2c, hqmc->dev_addr, reg, 1U, data, len, hqmc->timeout);
}

static uint8_t qmc5883p_build_official_ctrl1(qmc5883p_odr_t odr)
{
    /* Rev.C example uses 0xCD for normal mode at 200 Hz. Keep the upper nibble
     * from the documented example and only vary the public ODR bits.
     */
    return (uint8_t)(0xC1U | (((uint8_t)odr << 2) & 0x0CU));
}

static hal_status_t qmc5883p_wait_data_ready(qmc5883p_handle_t *hqmc, uint32_t timeout_ms)
{
    while (timeout_ms-- > 0U) {
        uint8_t status_reg = 0U;
        hal_status_t status = qmc5883p_read_reg(hqmc, hqmc->reg_status, &status_reg);
        if (status != HAL_OK) {
            return status;
        }
        if ((status_reg & QMC5883P_STATUS_DRDY) != 0U) {
            return HAL_OK;
        }
        qmc5883p_delay_ms(1U);
    }

    return HAL_TIMEOUT;
}

/**
 * @brief 配置传感器参数
 * @note 参考代码结构: CTRL1=ODR+OSR1+OSR2+mode, CTRL2=RNG
 */
static hal_status_t qmc5883p_config(qmc5883p_handle_t *hqmc, qmc5883p_odr_t odr, qmc5883p_osr_t osr)
{
    hal_status_t status;
    uint8_t ctrl1;

    if (hqmc->layout == QMC5883P_LAYOUT_OFFICIAL) {
        if (hqmc->reg_axis_sign != 0xFFU) {
            status = qmc5883p_write_reg(hqmc, hqmc->reg_axis_sign, QMC5883P_AXIS_SIGN_VALUE);
            if (status != HAL_OK) {
                return status;
            }
        }

        status = qmc5883p_write_reg(hqmc, hqmc->reg_ctrl2, 0x08U);
        if (status != HAL_OK) {
            return status;
        }

        ctrl1 = qmc5883p_build_official_ctrl1(odr);
        hqmc->range = QMC5883P_RNG_8G;
        hqmc->osr = QMC5883P_OSR_512;
    } else {
        ctrl1 = 0x79U;
        hqmc->osr = osr;
    }

    status = qmc5883p_write_reg(hqmc, hqmc->reg_ctrl1, ctrl1);
    if (status != HAL_OK) {
        return status;
    }

    /* 保存配置 */
    hqmc->odr = odr;

    return HAL_OK;
}

static hal_status_t qmc5883p_try_probe(qmc5883p_handle_t *hqmc, uint16_t addr, qmc5883p_layout_t layout)
{
    hal_status_t status;
    uint8_t chip_id = 0U;
    uint8_t sample[6];

    hqmc->dev_addr = addr;
    hqmc->layout = layout;
    hqmc->reg_set_reset = 0xFFU;
    hqmc->reg_axis_sign = 0xFFU;

    if (layout == QMC5883P_LAYOUT_OFFICIAL) {
        hqmc->reg_ctrl1 = QMC5883P_REG_CTRL1;
        hqmc->reg_ctrl2 = QMC5883P_REG_CTRL2;
        hqmc->reg_status = QMC5883P_REG_STATUS;
        hqmc->reg_data_start = QMC5883P_REG_XOUT_L;
        hqmc->reg_axis_sign = QMC5883P_REG_AXIS_SIGN;
        status = qmc5883p_read_reg(hqmc, QMC5883P_REG_CHIP_ID, &chip_id);
        if (status != HAL_OK || chip_id != QMC5883P_CHIP_ID_VALUE) {
            return HAL_ERROR;
        }
    } else {
        hqmc->reg_ctrl1 = QMC5883L_REG_CTRL1;
        hqmc->reg_ctrl2 = QMC5883L_REG_CTRL2;
        hqmc->reg_status = QMC5883L_REG_STATUS;
        hqmc->reg_data_start = QMC5883L_REG_XOUT_L;
        hqmc->reg_set_reset = QMC5883L_REG_SET_RESET;
        status = qmc5883p_read_reg(hqmc, QMC5883L_REG_CHIP_ID, &chip_id);
        if (status != HAL_OK || chip_id != QMC5883L_CHIP_ID_VALUE) {
            return HAL_ERROR;
        }
    }

    hqmc->chip_id = chip_id;

    status = qmc5883p_reset(hqmc);
    if (status != HAL_OK) {
        return status;
    }

    qmc5883p_delay_ms(QMC5883P_RESET_DELAY_MS);

    if (hqmc->reg_set_reset != 0xFFU) {
        status = qmc5883p_write_reg(hqmc, hqmc->reg_set_reset, 0x01U);
        if (status != HAL_OK) {
            return status;
        }
    }

    hqmc->range = QMC5883P_RNG_8G;
    hqmc->odr = QMC5883P_ODR_200_HZ;
    hqmc->osr = QMC5883P_OSR_512;
    status = qmc5883p_config(hqmc, hqmc->odr, hqmc->osr);
    if (status != HAL_OK) {
        return status;
    }

    qmc5883p_delay_ms(QMC5883P_STARTUP_DELAY_MS);

    status = qmc5883p_wait_data_ready(hqmc, QMC5883P_DRDY_TIMEOUT_MS);
    if (status != HAL_OK) {
        return status;
    }

    status = qmc5883p_read_regs(hqmc, hqmc->reg_data_start, sample, sizeof(sample));
    if (status != HAL_OK) {
        return status;
    }

    return HAL_OK;
}

/* ============================================================================
 * API 实现
 * ============================================================================ */

/**
 * @brief 初始化QMC5883P传感器
 */
hal_status_t qmc5883p_init(qmc5883p_handle_t *hqmc, i2c_handle_t *hi2c)
{
    hal_status_t status;

    if (hqmc == NULL || hi2c == NULL) {
        return HAL_ERROR;
    }

    /* 初始化句柄 */
    memset(hqmc, 0, sizeof(qmc5883p_handle_t));
    hqmc->i2c = hi2c;
    hqmc->timeout = QMC5883P_TIMEOUT_DEFAULT;
    hqmc->reg_set_reset = 0xFFU;

    status = qmc5883p_try_probe(hqmc, QMC5883P_I2C_ADDR, QMC5883P_LAYOUT_OFFICIAL);
    if (status != HAL_OK) {
        status = qmc5883p_try_probe(hqmc, QMC5883P_I2C_ADDR_LEGACY, QMC5883P_LAYOUT_LEGACY);
        if (status != HAL_OK) {
            return status;
        }
    }

    hqmc->initialized = 1;

    return HAL_OK;
}

/**
 * @brief 反初始化QMC5883P传感器
 */
hal_status_t qmc5883p_deinit(qmc5883p_handle_t *hqmc)
{
    hal_status_t status;

    if (hqmc == NULL || hqmc->i2c == NULL) {
        return HAL_ERROR;
    }

    /* 进入待机模式 */
    status = qmc5883p_write_reg(hqmc, hqmc->reg_ctrl1, 0x00);
    if (status != HAL_OK) {
        return status;
    }

    hqmc->initialized = 0;

    return HAL_OK;
}

/**
 * @brief 读取磁力计和温度数据
 */
hal_status_t qmc5883p_read_data(qmc5883p_handle_t *hqmc, qmc5883p_data_t *data)
{
    hal_status_t status;
    uint8_t buffer[6];
    int16_t mag_x, mag_y, mag_z;

    if (hqmc == NULL || data == NULL || !hqmc->initialized) {
        return HAL_ERROR;
    }

    {
        uint8_t ready = 0U;
        status = qmc5883p_data_ready(hqmc, &ready);
        if (status != HAL_OK) {
            return status;
        }
        if (ready == 0U) {
            return HAL_BUSY;
        }
    }

    status = qmc5883p_read_regs(hqmc, hqmc->reg_data_start, buffer, sizeof(buffer));
    if (status != HAL_OK) {
        return status;
    }

    mag_x = (int16_t)((buffer[1] << 8) | buffer[0]);
    mag_y = (int16_t)((buffer[3] << 8) | buffer[2]);
    mag_z = (int16_t)((buffer[5] << 8) | buffer[4]);

    /* 填充数据结构体 */
    data->mag_x = mag_x;
    data->mag_y = mag_y;
    data->mag_z = mag_z;
    data->mag_x_gauss = qmc5883p_mag_to_gauss(mag_x, hqmc->range);
    data->mag_y_gauss = qmc5883p_mag_to_gauss(mag_y, hqmc->range);
    data->mag_z_gauss = qmc5883p_mag_to_gauss(mag_z, hqmc->range);

    /* QMC5883P没有内置温度传感器，温度读取可选 */
    data->temperature = 0;
    data->temperature_c = 0.0f;

    return HAL_OK;
}

/**
 * @brief 设置输出数据率
 */
hal_status_t qmc5883p_set_odr(qmc5883p_handle_t *hqmc, qmc5883p_odr_t odr)
{
    if (hqmc == NULL || !hqmc->initialized) {
        return HAL_ERROR;
    }

    return qmc5883p_config(hqmc, odr, hqmc->osr);
}

/**
 * @brief 设置量程
 */
hal_status_t qmc5883p_set_range(qmc5883p_handle_t *hqmc, qmc5883p_rng_t rng)
{
    hal_status_t status;
    uint8_t ctrl2;

    if (hqmc == NULL || !hqmc->initialized) {
        return HAL_ERROR;
    }

    /* 量程在CTRL2设置, bits[3:2]: 0x00=±2G, 0x02=±8G */
    ctrl2 = (uint8_t)(rng << QMC5883P_CTRL2_RNG_Pos);
    if (hqmc->layout == QMC5883P_LAYOUT_OFFICIAL) {
        if (rng != QMC5883P_RNG_8G) {
            return HAL_ERROR;
        }
        hqmc->range = rng;
        return qmc5883p_config(hqmc, hqmc->odr, hqmc->osr);
    }

    status = qmc5883p_write_reg(hqmc, hqmc->reg_ctrl2, ctrl2);
    if (status != HAL_OK) {
        return status;
    }

    hqmc->range = rng;
    return HAL_OK;
}

/**
 * @brief 设置过采样率
 */
hal_status_t qmc5883p_set_osr(qmc5883p_handle_t *hqmc, qmc5883p_osr_t osr)
{
    if (hqmc == NULL || !hqmc->initialized) {
        return HAL_ERROR;
    }

    if (hqmc->layout == QMC5883P_LAYOUT_OFFICIAL && osr != QMC5883P_OSR_512) {
        return HAL_ERROR;
    }

    return qmc5883p_config(hqmc, hqmc->odr, osr);
}

/**
 * @brief 检查数据是否就绪
 */
hal_status_t qmc5883p_data_ready(qmc5883p_handle_t *hqmc, uint8_t *ready)
{
    hal_status_t status;
    uint8_t status_reg;

    if (hqmc == NULL || ready == NULL || !hqmc->initialized) {
        return HAL_ERROR;
    }

    status = qmc5883p_read_reg(hqmc, hqmc->reg_status, &status_reg);
    if (status != HAL_OK) {
        return status;
    }

    *ready = (status_reg & QMC5883P_STATUS_DRDY) ? 1U : 0U;

    return HAL_OK;
}

/**
 * @brief 软件复位
 * @note 根据参考代码: 写0x80到CTRL2触发软复位, 等待70ms
 */
hal_status_t qmc5883p_reset(qmc5883p_handle_t *hqmc)
{
    hal_status_t status;

    if (hqmc == NULL || hqmc->i2c == NULL) {
        return HAL_ERROR;
    }

    /* 设置软复位位 (bit7 = 0x80) */
    status = qmc5883p_write_reg(hqmc, hqmc->reg_ctrl2, QMC5883P_CTRL2_SOFTRST);
    if (status != HAL_OK) {
        return status;
    }

    qmc5883p_delay_ms(QMC5883P_RESET_DELAY_MS);

    return HAL_OK;
}

/**
 * @brief 将原始磁场值转换为Gauss
 */
float qmc5883p_mag_to_gauss(int16_t raw, qmc5883p_rng_t range)
{
    float sensitivity;

    if (range == QMC5883P_RNG_8G) {
        sensitivity = QMC5883P_SENSITIVITY_8G;
    } else {
        sensitivity = QMC5883P_SENSITIVITY_2G;
    }

    return (float)raw / sensitivity;
}

/**
 * @brief 将原始温度值转换为摄氏度
 */
float qmc5883p_temp_to_celsius(int16_t raw)
{
    /* QMC5883P温度传感器输出转换 */
    return (float)raw / QMC5883P_TEMP_SCALE;
}

/* ============================================================================
 * 测试函数实现
 * ============================================================================ */

#ifdef QMC5883P_ENABLE_TEST

#include <stdio.h>

/**
 * @brief QMC5883P驱动测试函数
 */
hal_status_t qmc5883p_test(i2c_handle_t *hi2c)
{
    hal_status_t status;
    qmc5883p_handle_t hqmc;
    qmc5883p_data_t data;
    uint8_t ready;
    uint32_t sample_count = 0;
    const uint32_t max_samples = 10;

    printf("=== QMC5883P Magnetometer Test ===\n");

    /* 初始化传感器 */
    status = qmc5883p_init(&hqmc, hi2c);
    if (status != HAL_OK) {
        printf("[FAIL] QMC5883P init failed\n");
        return status;
    }
    printf("[PASS] QMC5883P initialized\n");

    /* 读取10次数据 */
    printf("\nReading %u samples:\n", max_samples);
    printf("%-4s %-10s %-10s %-10s %-10s %-10s %-10s\n",
           "#", "MagX(G)", "MagY(G)", "MagZ(G)", "MagX(raw)", "MagY(raw)", "MagZ(raw)");

    while (sample_count < max_samples) {
        /* 检查数据就绪 */
        status = qmc5883p_data_ready(&hqmc, &ready);
        if (status != HAL_OK) {
            printf("[ERROR] Data ready check failed\n");
            return status;
        }

        if (ready) {
            /* 读取数据 */
            status = qmc5883p_read_data(&hqmc, &data);
            if (status != HAL_OK) {
                printf("[ERROR] Read data failed\n");
                return status;
            }

            printf("%-4u %-10.3f %-10.3f %-10.3f %-10d %-10d %-10d\n",
                   sample_count + 1,
                   data.mag_x_gauss, data.mag_y_gauss, data.mag_z_gauss,
                   data.mag_x, data.mag_y, data.mag_z);

            sample_count++;
        }

        qmc5883p_delay_ms(20);  /* 50Hz采样 */
    }

    /* 反初始化 */
    status = qmc5883p_deinit(&hqmc);
    if (status != HAL_OK) {
        printf("[FAIL] Deinit failed\n");
        return status;
    }

    printf("\n=== Test Complete ===\n");
    return HAL_OK;
}

#endif /* QMC5883P_ENABLE_TEST */
