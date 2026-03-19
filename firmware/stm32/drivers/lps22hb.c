/**
 * @file lps22hb.c
 * @brief LPS22HBTR 气压计驱动实现 (SPI版本)
 *
 * @note 本文件为Ralph-loop v2.0 传感器驱动层实现
 *       提供气压和温度数据读取功能
 *
 * @hardware
 *   - 芯片型号: LPS22HBTR
 *   - 接口: SPI3 (PA15=CS, PB3=SCK, PB4=MISO, PB5=MOSI)
 *   - SPI配置: Mode 0 (CPOL=0, CPHA=0), 8-bit, MSB first
 *   - WHO_AM_I: 0xB1
 *
 * @datasheet
 *   - 气压数据: 24位有符号整数, 大端序, 分辨率 0.000244 hPa (1/4096)
 *   - 温度数据: 16位有符号整数, 大端序, 分辨率 0.01 C (1/100)
 */

#include "lps22hb.h"
#include <string.h>

/* ============================================================================
 * 私有宏定义
 * ============================================================================ */

#define LPS22HB_TIMEOUT_DEFAULT     100U    /* 默认超时时间 (ms) */
#define LPS22HB_RESET_DELAY_MS      5U      /* 复位后等待时间 */

/* 数据转换系数 */
#define LPS22HB_PRESSURE_SCALE      4096.0f /* 气压转换系数: raw / 4096 = hPa */
#define LPS22HB_TEMP_SCALE          100.0f  /* 温度转换系数: raw / 100 = C */

/* ============================================================================
 * 私有函数声明
 * ============================================================================ */

static hal_status_t lps22hb_write_reg(lps22hb_handle_t *hlps22hb, uint8_t reg, uint8_t data);
static hal_status_t lps22hb_read_reg(lps22hb_handle_t *hlps22hb, uint8_t reg, uint8_t *data);
static hal_status_t lps22hb_read_regs(lps22hb_handle_t *hlps22hb, uint8_t reg, uint8_t *data, uint16_t len);
static int32_t lps22hb_sign_extend_24bit(uint32_t value);
static void lps22hb_delay_ms(uint32_t ms);

/* ============================================================================
 * 私有函数实现
 * ============================================================================ */

/**
 * @brief 简单延时函数
 */
static void lps22hb_delay_ms(uint32_t ms)
{
    volatile uint32_t count;
    while (ms--) {
        for (count = 0; count < 10000; count++) {
            __asm__ volatile("nop");
        }
    }
}

/**
 * @brief 向寄存器写入单字节数据 (SPI)
 */
static hal_status_t lps22hb_write_reg(lps22hb_handle_t *hlps22hb, uint8_t reg, uint8_t data)
{
    hal_status_t status;
    uint8_t tx_buf[2];

    if (hlps22hb == NULL || hlps22hb->spi == NULL) {
        return HAL_ERROR;
    }

    /* 准备数据: 寄存器地址(写操作) + 数据 */
    tx_buf[0] = reg | LPS22HB_SPI_WRITE_BIT;
    tx_buf[1] = data;

    /* 拉低CS开始传输 */
    spi_set_nss(hlps22hb->spi, 0);

    /* 发送数据 */
    status = spi_transmit(hlps22hb->spi, tx_buf, 2, hlps22hb->timeout);

    /* 拉高CS结束传输 */
    spi_set_nss(hlps22hb->spi, 1);

    return status;
}

/**
 * @brief 从寄存器读取单字节数据 (SPI)
 */
static hal_status_t lps22hb_read_reg(lps22hb_handle_t *hlps22hb, uint8_t reg, uint8_t *data)
{
    hal_status_t status;
    uint8_t tx_buf[2];
    uint8_t rx_buf[2];

    if (hlps22hb == NULL || hlps22hb->spi == NULL || data == NULL) {
        return HAL_ERROR;
    }

    /* 准备数据: 寄存器地址(读操作) + 空字节 */
    tx_buf[0] = reg | LPS22HB_SPI_READ_BIT;
    tx_buf[1] = 0x00;

    /* 拉低CS开始传输 */
    spi_set_nss(hlps22hb->spi, 0);

    /* 发送并接收数据 */
    status = spi_transmit_receive(hlps22hb->spi, tx_buf, rx_buf, 2, hlps22hb->timeout);

    /* 拉高CS结束传输 */
    spi_set_nss(hlps22hb->spi, 1);

    if (status == HAL_OK) {
        *data = rx_buf[1];
    }

    return status;
}

/**
 * @brief 从寄存器连续读取多字节数据 (SPI)
 */
static hal_status_t lps22hb_read_regs(lps22hb_handle_t *hlps22hb, uint8_t reg, uint8_t *data, uint16_t len)
{
    hal_status_t status;
    uint8_t tx_byte;

    if (hlps22hb == NULL || hlps22hb->spi == NULL || data == NULL || len == 0) {
        return HAL_ERROR;
    }

    /* 准备起始地址 (多字节读操作: bit7=1读, bit6=1地址自动递增) */
    tx_byte = reg | LPS22HB_SPI_READ_MULTI;

    /* 拉低CS开始传输 */
    spi_set_nss(hlps22hb->spi, 0);

    /* 发送起始地址 */
    status = spi_transmit(hlps22hb->spi, &tx_byte, 1, hlps22hb->timeout);
    if (status != HAL_OK) {
        spi_set_nss(hlps22hb->spi, 1);
        return status;
    }

    /* 接收数据 */
    status = spi_receive(hlps22hb->spi, data, len, hlps22hb->timeout);

    /* 拉高CS结束传输 */
    spi_set_nss(hlps22hb->spi, 1);

    return status;
}

/**
 * @brief 24位有符号数符号扩展
 */
static int32_t lps22hb_sign_extend_24bit(uint32_t value)
{
    /* 检查符号位 (第23位) */
    if (value & 0x00800000U) {
        /* 负数: 高8位置1 */
        return (int32_t)(value | 0xFF000000U);
    } else {
        /* 正数: 直接返回 */
        return (int32_t)value;
    }
}

/* ============================================================================
 * API 实现
 * ============================================================================ */

/**
 * @brief 初始化LPS22HBTR传感器 (SPI版本)
 */
hal_status_t lps22hb_init(lps22hb_handle_t *hlps22hb, spi_handle_t *hspi, lps22hb_odr_t odr)
{
    hal_status_t status;
    uint8_t id;
    uint8_t ctrl_reg1;

    if (hlps22hb == NULL || hspi == NULL) {
        return HAL_ERROR;
    }

    /* 初始化句柄 */
    memset(hlps22hb, 0, sizeof(lps22hb_handle_t));
    hlps22hb->spi = hspi;
    hlps22hb->odr = odr;
    hlps22hb->timeout = LPS22HB_TIMEOUT_DEFAULT;

    /* 验证WHO_AM_I */
    status = lps22hb_read_id(hlps22hb, &id);
    if (status != HAL_OK) {
        return status;
    }

    if (id != LPS22HB_WHO_AM_I_VALUE) {
        return HAL_ERROR;
    }

    /* 软件复位 */
    status = lps22hb_reset(hlps22hb);
    if (status != HAL_OK) {
        return status;
    }

    lps22hb_delay_ms(LPS22HB_RESET_DELAY_MS);

    /* 重启NVM加载校准参数 (参考代码关键步骤) */
    status = lps22hb_write_reg(hlps22hb, LPS22HB_REG_CTRL_REG2, LPS22HB_CTRL_REG2_BOOT);
    if (status != HAL_OK) {
        return status;
    }
    /* 等待BOOT位自清零 (约20ms) */
    lps22hb_delay_ms(20);

    /* 配置CTRL_REG2: 启用地址自增, 禁用I2C (纯SPI模式) */
    status = lps22hb_write_reg(hlps22hb, LPS22HB_REG_CTRL_REG2,
                               LPS22HB_CTRL_REG2_IF_ADD_INC | LPS22HB_CTRL_REG2_I2C_DIS);
    if (status != HAL_OK) {
        return status;
    }

    /* 配置CTRL_REG1: BDU + ODR */
    ctrl_reg1 = LPS22HB_CTRL_REG1_BDU;  /* 启用BDU确保数据一致性 */

    if (odr != LPS22HB_ODR_ONE_SHOT) {
        ctrl_reg1 |= (uint8_t)((odr << LPS22HB_CTRL_REG1_ODR_Pos) & LPS22HB_CTRL_REG1_ODR_Msk);
    }

    status = lps22hb_write_reg(hlps22hb, LPS22HB_REG_CTRL_REG1, ctrl_reg1);
    if (status != HAL_OK) {
        return status;
    }

    hlps22hb->initialized = 1;

    return HAL_OK;
}

/**
 * @brief 反初始化LPS22HBTR传感器
 */
hal_status_t lps22hb_deinit(lps22hb_handle_t *hlps22hb)
{
    hal_status_t status;

    if (hlps22hb == NULL || hlps22hb->spi == NULL) {
        return HAL_ERROR;
    }

    /* 关闭传感器 (ODR=0, 进入掉电模式) */
    status = lps22hb_write_reg(hlps22hb, LPS22HB_REG_CTRL_REG1, 0x00U);
    if (status != HAL_OK) {
        return status;
    }

    hlps22hb->initialized = 0;

    return HAL_OK;
}

/**
 * @brief 读取WHO_AM_I寄存器
 */
hal_status_t lps22hb_read_id(lps22hb_handle_t *hlps22hb, uint8_t *id)
{
    if (hlps22hb == NULL || hlps22hb->spi == NULL || id == NULL) {
        return HAL_ERROR;
    }

    return lps22hb_read_reg(hlps22hb, LPS22HB_REG_WHO_AM_I, id);
}

/**
 * @brief 读取气压和温度数据
 */
hal_status_t lps22hb_read_data(lps22hb_handle_t *hlps22hb, lps22hb_data_t *data)
{
    hal_status_t status;
    uint8_t buffer[5];  /* PRESS_OUT_XL, PRESS_OUT_L, PRESS_OUT_H, TEMP_OUT_L, TEMP_OUT_H */
    uint32_t pressure_u24;
    int32_t pressure_raw;
    int16_t temp_raw;

    if (hlps22hb == NULL || hlps22hb->spi == NULL || data == NULL) {
        return HAL_ERROR;
    }

    if (!hlps22hb->initialized) {
        return HAL_ERROR;
    }

    /* 读取气压和温度数据 (5字节连续读取) */
    status = lps22hb_read_regs(hlps22hb, LPS22HB_REG_PRESS_OUT_XL, buffer, 5U);
    if (status != HAL_OK) {
        return status;
    }

    /* 解析气压数据 (24位有符号, 小端序) */
    pressure_u24 = ((uint32_t)buffer[2] << 16) |
                   ((uint32_t)buffer[1] << 8)  |
                   ((uint32_t)buffer[0]);

    /* 24位有符号数符号扩展 */
    pressure_raw = lps22hb_sign_extend_24bit(pressure_u24);

    /* 解析温度数据 (16位有符号, 小端序) */
    temp_raw = (int16_t)(((uint16_t)buffer[4] << 8) | (uint16_t)buffer[3]);

    /* 填充数据结构体 */
    data->pressure_raw = pressure_raw;
    data->temperature_raw = temp_raw;
    data->pressure_hpa = lps22hb_pressure_to_hpa(pressure_raw);
    data->temperature_c = lps22hb_temp_to_celsius(temp_raw);

    return HAL_OK;
}

/**
 * @brief 设置输出数据率
 */
hal_status_t lps22hb_set_odr(lps22hb_handle_t *hlps22hb, lps22hb_odr_t odr)
{
    hal_status_t status;
    uint8_t ctrl_reg1;

    if (hlps22hb == NULL || hlps22hb->spi == NULL) {
        return HAL_ERROR;
    }

    /* 读取当前CTRL_REG1 */
    status = lps22hb_read_reg(hlps22hb, LPS22HB_REG_CTRL_REG1, &ctrl_reg1);
    if (status != HAL_OK) {
        return status;
    }

    /* 清除ODR位, 保留其他配置 */
    ctrl_reg1 &= ~LPS22HB_CTRL_REG1_ODR_Msk;

    /* 设置新的ODR */
    if (odr != LPS22HB_ODR_ONE_SHOT) {
        ctrl_reg1 |= (uint8_t)((odr << LPS22HB_CTRL_REG1_ODR_Pos) & LPS22HB_CTRL_REG1_ODR_Msk);
    }

    /* 写入CTRL_REG1 */
    status = lps22hb_write_reg(hlps22hb, LPS22HB_REG_CTRL_REG1, ctrl_reg1);
    if (status != HAL_OK) {
        return status;
    }

    hlps22hb->odr = odr;

    return HAL_OK;
}

/**
 * @brief 配置低通滤波器
 */
hal_status_t lps22hb_set_lpfp(lps22hb_handle_t *hlps22hb, lps22hb_lpfp_t lpfp)
{
    hal_status_t status;
    uint8_t ctrl_reg1;

    if (hlps22hb == NULL || hlps22hb->spi == NULL) {
        return HAL_ERROR;
    }

    /* 读取当前CTRL_REG1 */
    status = lps22hb_read_reg(hlps22hb, LPS22HB_REG_CTRL_REG1, &ctrl_reg1);
    if (status != HAL_OK) {
        return status;
    }

    /* 清除LPFP相关位 */
    ctrl_reg1 &= ~(LPS22HB_CTRL_REG1_LPFP_EN | LPS22HB_CTRL_REG1_LPFP_CFG);

    /* 设置LPFP配置 */
    switch (lpfp) {
        case LPS22HB_LPFP_DISABLE:
            break;
        case LPS22HB_LPFP_ODR_2:
            ctrl_reg1 |= LPS22HB_CTRL_REG1_LPFP_EN;
            break;
        case LPS22HB_LPFP_ODR_9:
            ctrl_reg1 |= LPS22HB_CTRL_REG1_LPFP_EN | LPS22HB_CTRL_REG1_LPFP_CFG;
            break;
        default:
            return HAL_ERROR;
    }

    /* 写入CTRL_REG1 */
    status = lps22hb_write_reg(hlps22hb, LPS22HB_REG_CTRL_REG1, ctrl_reg1);
    if (status != HAL_OK) {
        return status;
    }

    return HAL_OK;
}

/**
 * @brief 触发单次测量
 */
hal_status_t lps22hb_one_shot(lps22hb_handle_t *hlps22hb)
{
    hal_status_t status;
    uint8_t ctrl_reg2;

    if (hlps22hb == NULL || hlps22hb->spi == NULL) {
        return HAL_ERROR;
    }

    /* 读取当前CTRL_REG2 */
    status = lps22hb_read_reg(hlps22hb, LPS22HB_REG_CTRL_REG2, &ctrl_reg2);
    if (status != HAL_OK) {
        return status;
    }

    /* 设置ONE_SHOT位 */
    ctrl_reg2 |= LPS22HB_CTRL_REG2_ONE_SHOT;

    /* 写入CTRL_REG2 */
    status = lps22hb_write_reg(hlps22hb, LPS22HB_REG_CTRL_REG2, ctrl_reg2);
    if (status != HAL_OK) {
        return status;
    }

    return HAL_OK;
}

/**
 * @brief 检查数据是否就绪
 */
hal_status_t lps22hb_data_ready(lps22hb_handle_t *hlps22hb, uint8_t *pressure_ready, uint8_t *temp_ready)
{
    hal_status_t status;
    uint8_t status_reg;

    if (hlps22hb == NULL || hlps22hb->spi == NULL) {
        return HAL_ERROR;
    }

    status = lps22hb_read_reg(hlps22hb, LPS22HB_REG_STATUS, &status_reg);
    if (status != HAL_OK) {
        return status;
    }

    if (pressure_ready != NULL) {
        *pressure_ready = (status_reg & LPS22HB_STATUS_P_DA) ? 1U : 0U;
    }

    if (temp_ready != NULL) {
        *temp_ready = (status_reg & LPS22HB_STATUS_T_DA) ? 1U : 0U;
    }

    return HAL_OK;
}

/**
 * @brief 软件复位
 */
hal_status_t lps22hb_reset(lps22hb_handle_t *hlps22hb)
{
    hal_status_t status;
    uint8_t ctrl_reg2;
    uint32_t timeout;

    if (hlps22hb == NULL || hlps22hb->spi == NULL) {
        return HAL_ERROR;
    }

    /* 读取当前CTRL_REG2 */
    status = lps22hb_read_reg(hlps22hb, LPS22HB_REG_CTRL_REG2, &ctrl_reg2);
    if (status != HAL_OK) {
        return status;
    }

    /* 设置SWRESET位 */
    ctrl_reg2 |= LPS22HB_CTRL_REG2_SWRESET;

    /* 写入CTRL_REG2 */
    status = lps22hb_write_reg(hlps22hb, LPS22HB_REG_CTRL_REG2, ctrl_reg2);
    if (status != HAL_OK) {
        return status;
    }

    /* 等待复位完成 (SWRESET位自动清零) */
    timeout = LPS22HB_RESET_DELAY_MS * 1000U;
    while (timeout > 0U) {
        status = lps22hb_read_reg(hlps22hb, LPS22HB_REG_CTRL_REG2, &ctrl_reg2);
        if (status != HAL_OK) {
            return status;
        }

        if ((ctrl_reg2 & LPS22HB_CTRL_REG2_SWRESET) == 0U) {
            break;
        }

        timeout--;
    }

    if (timeout == 0U) {
        return HAL_TIMEOUT;
    }

    return HAL_OK;
}

/**
 * @brief 将原始气压值转换为hPa
 */
float lps22hb_pressure_to_hpa(int32_t raw)
{
    return (float)raw / LPS22HB_PRESSURE_SCALE;
}

/**
 * @brief 将原始温度值转换为摄氏度
 */
float lps22hb_temp_to_celsius(int16_t raw)
{
    return (float)raw / LPS22HB_TEMP_SCALE;
}

/* ============================================================================
 * 测试函数实现
 * ============================================================================ */

#ifdef LPS22HB_ENABLE_TEST

#include <stdio.h>

/**
 * @brief LPS22HBTR驱动测试函数 (SPI版本)
 */
hal_status_t lps22hb_test(spi_handle_t *hspi)
{
    hal_status_t status;
    lps22hb_handle_t hlps22hb;
    lps22hb_data_t data;
    uint8_t id;
    uint8_t pressure_ready, temp_ready;
    uint32_t sample_count = 0U;
    const uint32_t max_samples = 10U;

    printf("=== LPS22HBTR SPI Test ===\n");

    /* 初始化传感器 (25Hz ODR) */
    status = lps22hb_init(&hlps22hb, hspi, LPS22HB_ODR_25_HZ);
    if (status != HAL_OK) {
        printf("[FAIL] Init failed\n");
        return status;
    }
    printf("[PASS] Initialized\n");

    /* 测试1: 读取WHO_AM_I */
    status = lps22hb_read_id(&hlps22hb, &id);
    if (status != HAL_OK || id != LPS22HB_WHO_AM_I_VALUE) {
        printf("[FAIL] WHO_AM_I failed, expected 0x%02X, got 0x%02X\n", LPS22HB_WHO_AM_I_VALUE, id);
        return HAL_ERROR;
    }
    printf("[PASS] WHO_AM_I = 0x%02X\n", id);

    /* 测试2: 循环读取数据 */
    printf("\nReading %u samples:\n", max_samples);
    printf("%-4s %-12s %-12s\n", "#", "Pressure(hPa)", "Temp(C)");

    while (sample_count < max_samples) {
        status = lps22hb_data_ready(&hlps22hb, &pressure_ready, &temp_ready);
        if (status != HAL_OK) {
            return status;
        }

        if (pressure_ready && temp_ready) {
            status = lps22hb_read_data(&hlps22hb, &data);
            if (status != HAL_OK) {
                return status;
            }

            printf("%-4u %-12.3f %-12.2f\n",
                   sample_count + 1,
                   data.pressure_hpa,
                   data.temperature_c);

            sample_count++;
        }

        lps22hb_delay_ms(50);
    }

    /* 反初始化 */
    lps22hb_deinit(&hlps22hb);

    printf("\n=== Test Complete ===\n");
    return HAL_OK;
}

#endif /* LPS22HB_ENABLE_TEST */
