/**
 * @file qmc5883p.h
 * @brief QMC5883P 磁力计驱动头文件
 *
 * @note 本文件为Ralph-loop v2.0 传感器驱动层实现
 *       提供三轴磁场数据读取功能
 *
 * @hardware
 *   - 芯片型号: QMC5883P (三轴磁力计)
 *   - 接口: I2C1 (PB6=SCL, PB7=SDA)
 *   - I2C地址: 0x0D
 *   - WHO_AM_I: 0xFF
 *
 * @datasheet
 *   - 磁场数据: 16位有符号整数, 小端序
 *   - 温度数据: 16位有符号整数, 小端序
 *   - 量程: ±2G (默认) 或 ±8G
 */

#ifndef QMC5883P_H
#define QMC5883P_H

#include "hal_common.h"
#include "i2c.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * QMC5883P 设备常量
 * ============================================================================ */

/**
 * @brief QMC5883P I2C设备地址
 * @note 固定地址 0x2C (7-bit)
 *       0x2C = 0x16 << 1 (与pinout.md一致)
 */
#define QMC5883P_I2C_ADDR           0x2CU

/**
 * @brief WHO_AM_I 预期值
 * @note QMC5883P没有标准的WHO_AM_I寄存器，
 *       使用芯片ID寄存器0x0D读取
 */
#define QMC5883P_CHIP_ID_VALUE      0xFFU

/* ============================================================================
 * QMC5883P 寄存器地址定义
 * ============================================================================ */

#define QMC5883P_REG_XOUT_L         0x00U   /**< X轴数据低字节 */
#define QMC5883P_REG_XOUT_H         0x01U   /**< X轴数据高字节 */
#define QMC5883P_REG_YOUT_L         0x02U   /**< Y轴数据低字节 */
#define QMC5883P_REG_YOUT_H         0x03U   /**< Y轴数据高字节 */
#define QMC5883P_REG_ZOUT_L         0x04U   /**< Z轴数据低字节 */
#define QMC5883P_REG_ZOUT_H         0x05U   /**< Z轴数据高字节 */
#define QMC5883P_REG_TOUT_L         0x06U   /**< 温度数据低字节 */
#define QMC5883P_REG_TOUT_H         0x07U   /**< 温度数据高字节 */
#define QMC5883P_REG_STATUS         0x08U   /**< 状态寄存器 */
#define QMC5883P_REG_TEMP_L         0x09U   /**< 温度低字节(备用) */
#define QMC5883P_REG_TEMP_H         0x0AU   /**< 温度高字节(备用) */
#define QMC5883P_REG_CTRL1          0x0BU   /**< 控制寄存器1 (模式/ODR/量程/OSR) */
#define QMC5883P_REG_CTRL2          0x0CU   /**< 控制寄存器2 (软复位/中断) */
#define QMC5883P_REG_SR_PERIOD      0x0DU   /**< SET/RESET周期寄存器 */
#define QMC5883P_REG_CHIP_ID        0x0DU   /**< 芯片ID寄存器 (与SR_PERIOD共用) */
#define QMC5883P_REG_RESERVED1      0x0EU   /**< 保留 */
#define QMC5883P_REG_RESERVED2      0x0FU   /**< 保留 */

/* ============================================================================
 * STATUS 寄存器位定义
 * ============================================================================ */

#define QMC5883P_STATUS_DRDY        (1U << 0)   /**< 数据就绪标志 */
#define QMC5883P_STATUS_OVL         (1U << 1)   /**< 数据溢出标志 */
#define QMC5883P_STATUS_DOR         (1U << 2)   /**< 数据跳读标志 */

/* ============================================================================
 * CTRL1 寄存器位定义
 * ============================================================================ */

#define QMC5883P_CTRL1_MODE_Pos     0U
#define QMC5883P_CTRL1_MODE_Msk     (0x3U << QMC5883P_CTRL1_MODE_Pos)
#define QMC5883P_CTRL1_MODE_STANDBY 0x00U       /**< 待机模式 */
#define QMC5883P_CTRL1_MODE_CONT    0x01U       /**< 连续测量模式 */

#define QMC5883P_CTRL1_ODR_Pos      2U
#define QMC5883P_CTRL1_ODR_Msk      (0x3U << QMC5883P_CTRL1_ODR_Pos)

#define QMC5883P_CTRL1_RNG_Pos      4U
#define QMC5883P_CTRL1_RNG_Msk      (0x3U << QMC5883P_CTRL1_RNG_Pos)

#define QMC5883P_CTRL1_OSR_Pos      6U
#define QMC5883P_CTRL1_OSR_Msk      (0x3U << QMC5883P_CTRL1_OSR_Pos)

/* ============================================================================
 * CTRL2 寄存器位定义
 * ============================================================================ */

#define QMC5883P_CTRL2_SOFTRST      (1U << 6)   /**< 软复位 */
#define QMC5883P_CTRL2_INT_ENB      (1U << 0)   /**< 中断使能 */

/* ============================================================================
 * 输出数据率 (ODR) 枚举
 * ============================================================================ */

typedef enum {
    QMC5883P_ODR_10_HZ   = 0x00U,   /**< 10 Hz */
    QMC5883P_ODR_50_HZ   = 0x01U,   /**< 50 Hz */
    QMC5883P_ODR_100_HZ  = 0x02U,   /**< 100 Hz (推荐) */
    QMC5883P_ODR_200_HZ  = 0x03U    /**< 200 Hz */
} qmc5883p_odr_t;

/* ============================================================================
 * 量程 (RNG) 枚举
 * ============================================================================ */

typedef enum {
    QMC5883P_RNG_2G = 0x00U,        /**< ±2G (默认) */
    QMC5883P_RNG_8G = 0x01U         /**< ±8G */
} qmc5883p_rng_t;

/* ============================================================================
 * 过采样率 (OSR) 枚举
 * ============================================================================ */

typedef enum {
    QMC5883P_OSR_512 = 0x00U,       /**< 512 */
    QMC5883P_OSR_256 = 0x01U,       /**< 256 */
    QMC5883P_OSR_128 = 0x02U,       /**< 128 */
    QMC5883P_OSR_64  = 0x03U        /**< 64 */
} qmc5883p_osr_t;

/* ============================================================================
 * 灵敏度系数定义
 * ============================================================================ */

#define QMC5883P_SENSITIVITY_2G     12000.0f    /**< ±2G量程灵敏度: 12000 LSB/G */
#define QMC5883P_SENSITIVITY_8G     3000.0f     /**< ±8G量程灵敏度: 3000 LSB/G */

/* ============================================================================
 * QMC5883P 句柄结构体
 * ============================================================================ */

typedef struct {
    i2c_handle_t   *i2c;            /**< I2C句柄指针 */
    uint16_t        dev_addr;       /**< 设备I2C地址 */
    qmc5883p_rng_t  range;          /**< 当前量程 */
    qmc5883p_odr_t  odr;            /**< 当前输出数据率 */
    qmc5883p_osr_t  osr;            /**< 当前过采样率 */
    uint32_t        timeout;        /**< 默认超时时间 (ms) */
    uint8_t         initialized;    /**< 初始化标志 */
} qmc5883p_handle_t;

/* ============================================================================
 * 数据读取结构体
 * ============================================================================ */

typedef struct {
    int16_t mag_x;                  /**< X轴磁场原始值 */
    int16_t mag_y;                  /**< Y轴磁场原始值 */
    int16_t mag_z;                  /**< Z轴磁场原始值 */
    int16_t temperature;            /**< 温度原始值 */
    float mag_x_gauss;              /**< X轴磁场 (Gauss) */
    float mag_y_gauss;              /**< Y轴磁场 (Gauss) */
    float mag_z_gauss;              /**< Z轴磁场 (Gauss) */
    float temperature_c;            /**< 温度 (摄氏度) */
} qmc5883p_data_t;

/* ============================================================================
 * API 声明
 * ============================================================================ */

/**
 * @brief 初始化QMC5883P传感器
 * @param hqmc QMC5883P句柄
 * @param hi2c I2C句柄指针
 * @return HAL状态
 */
hal_status_t qmc5883p_init(qmc5883p_handle_t *hqmc, i2c_handle_t *hi2c);

/**
 * @brief 反初始化QMC5883P传感器
 * @param hqmc QMC5883P句柄
 * @return HAL状态
 */
hal_status_t qmc5883p_deinit(qmc5883p_handle_t *hqmc);

/**
 * @brief 读取磁力计和温度数据
 * @param hqmc QMC5883P句柄
 * @param data 数据输出结构体
 * @return HAL状态
 */
hal_status_t qmc5883p_read_data(qmc5883p_handle_t *hqmc, qmc5883p_data_t *data);

/**
 * @brief 设置输出数据率
 * @param hqmc QMC5883P句柄
 * @param odr 输出数据率
 * @return HAL状态
 */
hal_status_t qmc5883p_set_odr(qmc5883p_handle_t *hqmc, qmc5883p_odr_t odr);

/**
 * @brief 设置量程
 * @param hqmc QMC5883P句柄
 * @param rng 量程
 * @return HAL状态
 */
hal_status_t qmc5883p_set_range(qmc5883p_handle_t *hqmc, qmc5883p_rng_t rng);

/**
 * @brief 设置过采样率
 * @param hqmc QMC5883P句柄
 * @param osr 过采样率
 * @return HAL状态
 */
hal_status_t qmc5883p_set_osr(qmc5883p_handle_t *hqmc, qmc5883p_osr_t osr);

/**
 * @brief 检查数据是否就绪
 * @param hqmc QMC5883P句柄
 * @param ready 数据就绪标志 (输出)
 * @return HAL状态
 */
hal_status_t qmc5883p_data_ready(qmc5883p_handle_t *hqmc, uint8_t *ready);

/**
 * @brief 软件复位
 * @param hqmc QMC5883P句柄
 * @return HAL状态
 */
hal_status_t qmc5883p_reset(qmc5883p_handle_t *hqmc);

/**
 * @brief 将原始磁场值转换为Gauss
 * @param raw 原始值
 * @param range 当前量程
 * @return 磁场值 (Gauss)
 */
float qmc5883p_mag_to_gauss(int16_t raw, qmc5883p_rng_t range);

/**
 * @brief 将原始温度值转换为摄氏度
 * @param raw 原始温度值
 * @return 温度值 (摄氏度)
 */
float qmc5883p_temp_to_celsius(int16_t raw);

/* ============================================================================
 * 测试函数声明
 * ============================================================================ */

#ifdef QMC5883P_ENABLE_TEST

/**
 * @brief QMC5883P驱动测试函数
 * @param hi2c I2C句柄指针
 * @return HAL状态
 */
hal_status_t qmc5883p_test(i2c_handle_t *hi2c);

#endif /* QMC5883P_ENABLE_TEST */

#ifdef __cplusplus
}
#endif

#endif /* QMC5883P_H */
