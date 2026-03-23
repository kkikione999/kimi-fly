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
 *   - I2C地址: 官方 QMC5883P 为 0x2C
 *   - 兼容历史工程中误用的 QMC5883L 风格寄存器表
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
 * @brief QMC5883P 官方 I2C 设备地址
 * @note QST QMC5883P Datasheet Rev.C 给出的 7-bit 地址为 0x2C
 */
#define QMC5883P_I2C_ADDR           0x2CU

/**
 * @brief 兼容旧版 QMC5883L 风格地址
 */
#define QMC5883P_I2C_ADDR_LEGACY    0x0DU

/**
 * @brief CHIP_ID 预期值
 * @note 当前正式驱动未使用 CHIP_ID 校验，此值保留仅供参考
 */
#define QMC5883P_CHIP_ID_VALUE      0x80U
#define QMC5883L_CHIP_ID_VALUE      0xFFU

/* ============================================================================
 * QMC5883P 寄存器地址定义
 * ============================================================================ */

#define QMC5883P_REG_CHIP_ID        0x00U   /**< 官方手册 CHIP_ID */
#define QMC5883P_REG_XOUT_L         0x01U   /**< X轴数据低字节 */
#define QMC5883P_REG_XOUT_H         0x02U   /**< X轴数据高字节 */
#define QMC5883P_REG_YOUT_L         0x03U   /**< Y轴数据低字节 */
#define QMC5883P_REG_YOUT_H         0x04U   /**< Y轴数据高字节 */
#define QMC5883P_REG_ZOUT_L         0x05U   /**< Z轴数据低字节 */
#define QMC5883P_REG_ZOUT_H         0x06U   /**< Z轴数据高字节 */
#define QMC5883P_REG_STATUS         0x09U   /**< 官方手册状态寄存器 */
#define QMC5883P_REG_CTRL1          0x0AU   /**< 官方手册控制寄存器1 */
#define QMC5883P_REG_CTRL2          0x0BU   /**< 官方手册控制寄存器2 */
#define QMC5883P_REG_AXIS_SIGN      0x29U   /**< 官方手册轴极性寄存器 */

#define QMC5883L_REG_CHIP_ID        0x0DU   /**< QMC5883L 风格 CHIP_ID */
#define QMC5883L_REG_XOUT_L         0x00U   /**< QMC5883L 风格 X轴数据低字节 */
#define QMC5883L_REG_STATUS         0x06U   /**< QMC5883L 风格状态寄存器 */
#define QMC5883L_REG_CTRL1          0x09U   /**< QMC5883L 风格控制寄存器1 */
#define QMC5883L_REG_CTRL2          0x0AU   /**< QMC5883L 风格控制寄存器2 */
#define QMC5883L_REG_SET_RESET      0x0BU   /**< QMC5883L 风格 SET/RESET 周期寄存器 */

typedef enum {
    QMC5883P_LAYOUT_UNKNOWN = 0,
    QMC5883P_LAYOUT_OFFICIAL,
    QMC5883P_LAYOUT_LEGACY
} qmc5883p_layout_t;

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

#define QMC5883P_CTRL2_SOFTRST      (1U << 7)   /**< 软复位 (bit7=1触发, 自动清零, 参考代码0x80) */
#define QMC5883P_CTRL2_RNG_Pos      2U
#define QMC5883P_CTRL2_RNG_Msk      (0x3U << QMC5883P_CTRL2_RNG_Pos)
#define QMC5883P_CTRL2_RNG_2G       0x00U       /**< ±2G量程 (CTRL2 bits[3:2]=0x00) */
#define QMC5883P_CTRL2_RNG_8G       0x02U       /**< ±8G量程 (CTRL2 bits[3:2]=0x02) */
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
    QMC5883P_RNG_2G = 0x00U,        /**< ±2G (默认, CTRL2 bits[3:2]=0x00) */
    QMC5883P_RNG_8G = 0x02U         /**< ±8G (CTRL2 bits[3:2]=0x02) */
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

#define QMC5883P_SENSITIVITY_2G     15000.0f    /**< Rev.C ±2G量程灵敏度: 15000 LSB/G */
#define QMC5883P_SENSITIVITY_8G     3750.0f     /**< Rev.C ±8G量程灵敏度: 3750 LSB/G */

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
    qmc5883p_layout_t layout;       /**< 探测到的寄存器布局 */
    uint8_t         chip_id;        /**< 启动时读到的 CHIP_ID */
    uint8_t         reg_data_start; /**< 当前数据起始寄存器地址 */
    uint8_t         reg_status;     /**< 当前状态寄存器地址 */
    uint8_t         reg_ctrl1;      /**< 当前控制寄存器1地址 */
    uint8_t         reg_ctrl2;      /**< 当前控制寄存器2地址 */
    uint8_t         reg_set_reset;  /**< 当前 SET/RESET 寄存器地址, 0xFF 表示无 */
    uint8_t         reg_axis_sign;  /**< 当前轴符号寄存器地址, 0xFF 表示无 */
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
