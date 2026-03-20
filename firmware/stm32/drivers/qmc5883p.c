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
 *   - I2C地址: 0x0D
 */

#include "qmc5883p.h"
#include <string.h>

/* ============================================================================
 * 私有宏定义
 * ============================================================================ */

#define QMC5883P_TIMEOUT_DEFAULT    100U    /* 默认超时时间 (ms) */
#define QMC5883P_RESET_DELAY_MS     70U     /* 复位后等待时间,参考代码使用70ms */

/* 温度转换系数 */
#define QMC5883P_TEMP_SCALE         100.0f  /* 温度转换系数 */

/* ============================================================================
 * 私有函数声明
 * ============================================================================ */

static hal_status_t qmc5883p_write_reg(qmc5883p_handle_t *hqmc, uint8_t reg, uint8_t data);
static hal_status_t qmc5883p_read_reg(qmc5883p_handle_t *hqmc, uint8_t reg, uint8_t *data);
static hal_status_t qmc5883p_read_regs(qmc5883p_handle_t *hqmc, uint8_t reg, uint8_t *data, uint16_t len);
static hal_status_t qmc5883p_config(qmc5883p_handle_t *hqmc, qmc5883p_odr_t odr, qmc5883p_osr_t osr);
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

/**
 * @brief 配置传感器参数
 * @note 参考代码结构: CTRL1=ODR+OSR1+OSR2+mode, CTRL2=RNG
 */
static hal_status_t qmc5883p_config(qmc5883p_handle_t *hqmc, qmc5883p_odr_t odr, qmc5883p_osr_t osr)
{
    hal_status_t status;
    uint8_t ctrl1;

    /* 构建CTRL1寄存器值: ODR + OSR + mode */
    /* 注意: RNG在CTRL2设置, 不在CTRL1 */
    ctrl1 = 0;
    ctrl1 |= (odr << QMC5883P_CTRL1_ODR_Pos) & QMC5883P_CTRL1_ODR_Msk;
    ctrl1 |= (osr << QMC5883P_CTRL1_OSR_Pos) & QMC5883P_CTRL1_OSR_Msk;
    ctrl1 |= QMC5883P_CTRL1_MODE_CONT;  /* 连续测量模式 */

    status = qmc5883p_write_reg(hqmc, QMC5883P_REG_CTRL1, ctrl1);
    if (status != HAL_OK) {
        return status;
    }

    /* 保存配置 */
    hqmc->odr = odr;
    hqmc->osr = osr;

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
    hqmc->dev_addr = QMC5883P_I2C_ADDR;
    hqmc->timeout = QMC5883P_TIMEOUT_DEFAULT;

    /* 软件复位 */
    status = qmc5883p_reset(hqmc);
    if (status != HAL_OK) {
        return status;
    }

    qmc5883p_delay_ms(QMC5883P_RESET_DELAY_MS);

    /* 配置CTRL2量程: ±2G (bits[3:2]=0x00) */
    /* 注意: 参考代码中量程在CTRL2设置, 不是CTRL1 */
    status = qmc5883p_write_reg(hqmc, QMC5883P_REG_CTRL2, QMC5883P_CTRL2_RNG_2G);
    if (status != HAL_OK) {
        return status;
    }
    hqmc->range = QMC5883P_RNG_2G;

    /* 使用参考代码配置: 200Hz ODR, ±8G量程, OSR=4/2 */
    /* 参考代码: mode=0x01, ODR=0x03<<2, OSR1=0x01<<4, OSR2=0x01<<6 = 0x5D */
    /* CTRL1 = 0x5D (200Hz, normal mode, OSR1=4, OSR2=2) */

    /* 先进入suspend模式 (参考代码第一步) */
    status = qmc5883p_write_reg(hqmc, QMC5883P_REG_CTRL1, 0x00);
    if (status != HAL_OK) {
        return status;
    }

    /* 配置CTRL1: 参考代码值 0x5D */
    /* 0x5D = 0b01011101 = mode=01, ODR=11(200Hz), OSR1=01, OSR2=01 */
    status = qmc5883p_write_reg(hqmc, QMC5883P_REG_CTRL1, 0x5D);
    if (status != HAL_OK) {
        return status;
    }
    hqmc->odr = QMC5883P_ODR_200_HZ;
    hqmc->osr = QMC5883P_OSR_256;

    /* 标记初始化完成, 不在init中等待DRDY -- 数据就绪检查在read_data中进行 */
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
    status = qmc5883p_write_reg(hqmc, QMC5883P_REG_CTRL1, 0x00);
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
    uint8_t buffer[6];  /* X_L, X_H, Y_L, Y_H, Z_L, Z_H (参考代码读6字节) */
    int16_t mag_x, mag_y, mag_z;

    if (hqmc == NULL || data == NULL || !hqmc->initialized) {
        return HAL_ERROR;
    }

    /* 检查数据就绪状态，未就绪则返回HAL_BUSY让调用者重试 */
    {
        uint8_t ready = 0U;
        status = qmc5883p_data_ready(hqmc, &ready);
        if (status != HAL_OK) {
            return status;
        }
        if (!ready) {
            return HAL_BUSY;
        }
    }

    /* 使用单独寄存器读取测试,参考代码使用6字节burst read */
    /* 如果burst read有问题,单独读取可以验证 */
    uint8_t xl, xh, yl, yh, zl, zh;
    qmc5883p_read_reg(hqmc, QMC5883P_REG_XOUT_L, &xl);
    qmc5883p_read_reg(hqmc, QMC5883P_REG_XOUT_H, &xh);
    qmc5883p_read_reg(hqmc, QMC5883P_REG_YOUT_L, &yl);
    qmc5883p_read_reg(hqmc, QMC5883P_REG_YOUT_H, &yh);
    qmc5883p_read_reg(hqmc, QMC5883P_REG_ZOUT_L, &zl);
    qmc5883p_read_reg(hqmc, QMC5883P_REG_ZOUT_H, &zh);

    mag_x = (int16_t)((xh << 8) | xl);
    mag_y = (int16_t)((yh << 8) | yl);
    mag_z = (int16_t)((zh << 8) | zl);

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
    status = qmc5883p_write_reg(hqmc, QMC5883P_REG_CTRL2, ctrl2);
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

    status = qmc5883p_read_reg(hqmc, QMC5883P_REG_STATUS, &status_reg);
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
    status = qmc5883p_write_reg(hqmc, QMC5883P_REG_CTRL2, QMC5883P_CTRL2_SOFTRST);
    if (status != HAL_OK) {
        return status;
    }

    /* 等待复位完成 - 参考代码使用70ms固定延迟 */
    qmc5883p_delay_ms(70U);

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
