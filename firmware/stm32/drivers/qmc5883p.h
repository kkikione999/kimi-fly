/**
 * @file qmc5883p.h
 * @brief QMC5883P 3-axis Magnetometer Driver
 *
 * @note 本文件为Ralph-loop v2.0 传感器驱动层实现
 *       基于I2C1接口，与气压计共享总线
 *
 * @hardware
 *   - QMC5883P: I2C地址 0x0D
 *   - I2C1: PB6 (SCL), PB7 (SDA)
 *   - 数据格式: 小端序 (Little Endian)
 */

#ifndef QMC5883P_H
#define QMC5883P_H

#include "hal_common.h"
#include "i2c.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * QMC5883P 寄存器地址定义
 * ============================================================================ */

#define QMC5883P_REG_DATA_OUT_X_L   0x00U   /**< X轴数据低字节 */
#define QMC5883P_REG_DATA_OUT_X_H   0x01U   /**< X轴数据高字节 */
#define QMC5883P_REG_DATA_OUT_Y_L   0x02U   /**< Y轴数据低字节 */
#define QMC5883P_REG_DATA_OUT_Y_H   0x03U   /**< Y轴数据高字节 */
#define QMC5883P_REG_DATA_OUT_Z_L   0x04U   /**< Z轴数据低字节 */
#define QMC5883P_REG_DATA_OUT_Z_H   0x05U   /**< Z轴数据高字节 */
#define QMC5883P_REG_STATUS         0x06U   /**< 状态寄存器 */
#define QMC5883P_REG_TEMP_OUT_L     0x07U   /**< 温度数据低字节 */
#define QMC5883P_REG_TEMP_OUT_H     0x08U   /**< 温度数据高字节 */
#define QMC5883P_REG_CTRL_REG1      0x09U   /**< 控制寄存器1 */
#define QMC5883P_REG_CTRL_REG2      0x0AU   /**< 控制寄存器2 */
#define QMC5883P_REG_SET_PERIOD     0x0BU   /**< 设置/复位周期 */
#define QMC5883P_REG_CHIP_ID        0x0DU   /**< 芯片ID寄存器 */

/* ============================================================================
 * 状态寄存器位定义
 * ============================================================================ */

#define QMC5883P_STATUS_DRDY        (1U << 0)   /**< 数据就绪标志 */
#define QMC5883P_STATUS_OVL         (1U << 1)   /**< 溢出标志 */
#define QMC5883P_STATUS_DOR         (1U << 2)   /**< 数据跳读标志 */

/* ============================================================================
 * 控制寄存器1位定义 (CTRL_REG1)
 * ============================================================================ */

/* 过采样率 OSR[7:6] */
#define QMC5883P_CTRL1_OSR_Pos      6U
#define QMC5883P_CTRL1_OSR_Msk      (0x3U << QMC5883P_CTRL1_OSR_Pos)

/* 量程 RNG[5:4] */
#define QMC5883P_CTRL1_RNG_Pos      4U
#define QMC5883P_CTRL1_RNG_Msk      (0x3U << QMC5883P_CTRL1_RNG_Pos)

/* 输出数据率 ODR[3:2] */
#define QMC5883P_CTRL1_ODR_Pos      2U
#define QMC5883P_CTRL1_ODR_Msk      (0x3U << QMC5883P_CTRL1_ODR_Pos)

/* 模式 MODE[1:0] */
#define QMC5883P_CTRL1_MODE_Pos     0U
#define QMC5883P_CTRL1_MODE_Msk     (0x3U << QMC5883P_CTRL1_MODE_Pos)

/* ============================================================================
 * 控制寄存器2位定义 (CTRL_REG2)
 * ============================================================================ */

#define QMC5883P_CTRL2_SOFT_RST     (1U << 7)   /**< 软复位 */
#define QMC5883P_CTRL2_ROL_PNT      (1U << 6)   /**< 指针回滚 */
#define QMC5883P_CTRL2_INT_ENB      (1U << 0)   /**< 中断使能 (0=使能, 1=禁用) */

/* ============================================================================
 * 量程 (Range) 枚举
 * ============================================================================ */

typedef enum {
    QMC5883P_RANGE_2G = 0x00U,   /**< ±2 Gauss, 分辨率 12000 LSB/G */
    QMC5883P_RANGE_8G = 0x01U    /**< ±8 Gauss, 分辨率 3000 LSB/G */
} qmc5883p_range_t;

/* ============================================================================
 * 输出数据率 (ODR) 枚举
 * ============================================================================ */

typedef enum {
    QMC5883P_ODR_10HZ  = 0x00U,  /**< 10 Hz */
    QMC5883P_ODR_50HZ  = 0x01U,  /**< 50 Hz */
    QMC5883P_ODR_100HZ = 0x02U,  /**< 100 Hz (推荐) */
    QMC5883P_ODR_200HZ = 0x03U   /**< 200 Hz */
} qmc5883p_odr_t;

/* ============================================================================
 * 工作模式枚举
 * ============================================================================ */

typedef enum {
    QMC5883P_MODE_STANDBY   = 0x00U,  /**< 待机模式 */
    QMC5883P_MODE_CONTINUOUS = 0x01U, /**< 连续测量模式 (推荐) */
    QMC5883P_MODE_SINGLE    = 0x02U   /**< 单次测量模式 */
} qmc5883p_mode_t;

/* ============================================================================
 * 过采样率 (OSR) 枚举
 * ============================================================================ */

typedef enum {
    QMC5883P_OSR_512 = 0x00U,    /**< 512 (最高精度，最慢) */
    QMC5883P_OSR_256 = 0x01U,    /**< 256 (推荐，平衡精度和速度) */
    QMC5883P_OSR_128 = 0x02U,    /**< 128 */
    QMC5883P_OSR_64  = 0x03U     /**< 64 (最低精度，最快) */
} qmc5883p_osr_t;

/* ============================================================================
 * 转换系数定义 (LSB/Gauss)
 * ============================================================================ */

#define QMC5883P_SENSITIVITY_2G     12000.0f    /**< ±2G量程灵敏度 (LSB/Gauss) */
#define QMC5883P_SENSITIVITY_8G     3000.0f     /**< ±8G量程灵敏度 (LSB/Gauss) */

#define QMC5883P_SCALE_2G           (1.0f / QMC5883P_SENSITIVITY_2G)   /**< ±2G量程转换系数 (Gauss/LSB) */
#define QMC5883P_SCALE_8G           (1.0f / QMC5883P_SENSITIVITY_8G)    /**< ±8G量程转换系数 (Gauss/LSB) */

/* ============================================================================
 * 默认配置
 * ============================================================================ */

#define QMC5883P_DEFAULT_RANGE      QMC5883P_RANGE_8G
#define QMC5883P_DEFAULT_ODR        QMC5883P_ODR_100HZ
#define QMC5883P_DEFAULT_OSR        QMC5883P_OSR_256
#define QMC5883P_DEFAULT_MODE       QMC5883P_MODE_CONTINUOUS

/* I2C超时时间 (ms) */
#define QMC5883P_I2C_TIMEOUT        100U

/* ============================================================================
 * 数据结构定义
 * ============================================================================ */

/**
 * @brief QMC5883P 句柄结构体
 */
typedef struct {
    i2c_handle_t    *i2c_handle;    /**< I2C句柄指针 */
    uint16_t         dev_addr;      /**< 设备I2C地址 */
    qmc5883p_range_t range;         /**< 当前量程设置 */
    uint32_t         timeout;       /**< I2C通信超时时间 */
} qmc5883p_handle_t;

/**
 * @brief QMC5883P 原始数据 (16位有符号)
 */
typedef struct {
    int16_t x;      /**< X轴原始数据 */
    int16_t y;      /**< Y轴原始数据 */
    int16_t z;      /**< Z轴原始数据 */
} qmc5883p_raw_data_t;

/**
 * @brief QMC5883P 磁场数据 (高斯单位)
 */
typedef struct {
    float x;        /**< X轴磁场 (Gauss) */
    float y;        /**< Y轴磁场 (Gauss) */
    float z;        /**< Z轴磁场 (Gauss) */
} qmc5883p_mag_data_t;

/**
 * @brief QMC5883P 温度数据
 */
typedef struct {
    int16_t raw;    /**< 原始温度数据 */
    float   celsius; /**< 摄氏度 */
} qmc5883p_temp_data_t;

/**
 * @brief QMC5883P 完整数据
 */
typedef struct {
    qmc5883p_mag_data_t mag;    /**< 磁场数据 (Gauss) */
    qmc5883p_temp_data_t temp;  /**< 温度数据 (可选) */
    uint8_t status;             /**< 状态寄存器 */
} qmc5883p_data_t;

/* ============================================================================
 * API 函数声明
 * ============================================================================ */

/**
 * @brief 初始化QMC5883P传感器
 * @param handle QMC5883P句柄
 * @param hi2c I2C句柄
 * @return HAL状态
 * @note 执行软复位并配置为默认参数 (连续模式, 100Hz ODR, 8G量程)
 */
hal_status_t qmc5883p_init(qmc5883p_handle_t *handle, i2c_handle_t *hi2c);

/**
 * @brief 反初始化QMC5883P传感器
 * @param handle QMC5883P句柄
 * @return HAL状态
 */
hal_status_t qmc5883p_deinit(qmc5883p_handle_t *handle);

/**
 * @brief 读取CHIP_ID寄存器
 * @param handle QMC5883P句柄
 * @param chip_id 芯片ID输出缓冲区
 * @return HAL状态
 * @note 实现自适应检测，不强制特定值，只验证读取是否成功
 */
hal_status_t qmc5883p_read_id(qmc5883p_handle_t *handle, uint8_t *chip_id);

/**
 * @brief 读取三轴磁场数据 (原始值)
 * @param handle QMC5883P句柄
 * @param raw_data 原始数据输出结构体
 * @return HAL状态
 * @note 数据格式为小端序 (Little Endian)
 */
hal_status_t qmc5883p_read_raw_data(qmc5883p_handle_t *handle, qmc5883p_raw_data_t *raw_data);

/**
 * @brief 读取磁场数据 (转换为高斯单位)
 * @param handle QMC5883P句柄
 * @param mag_data 磁场数据输出结构体 (Gauss)
 * @return HAL状态
 */
hal_status_t qmc5883p_read_mag_data(qmc5883p_handle_t *handle, qmc5883p_mag_data_t *mag_data);

/**
 * @brief 读取完整数据 (磁场 + 温度 + 状态)
 * @param handle QMC5883P句柄
 * @param data 完整数据输出结构体
 * @return HAL状态
 */
hal_status_t qmc5883p_read_data(qmc5883p_handle_t *handle, qmc5883p_data_t *data);

/**
 * @brief 读取温度数据
 * @param handle QMC5883P句柄
 * @param temp_data 温度数据输出结构体
 * @return HAL状态
 * @note 温度数据用于温度补偿 (如果需要)
 */
hal_status_t qmc5883p_read_temp(qmc5883p_handle_t *handle, qmc5883p_temp_data_t *temp_data);

/**
 * @brief 设置工作模式
 * @param handle QMC5883P句柄
 * @param mode 工作模式
 * @return HAL状态
 */
hal_status_t qmc5883p_set_mode(qmc5883p_handle_t *handle, qmc5883p_mode_t mode);

/**
 * @brief 设置完整配置 (ODR/量程/过采样率)
 * @param handle QMC5883P句柄
 * @param odr 输出数据率
 * @param range 量程
 * @param osr 过采样率
 * @return HAL状态
 * @note 设置后会自动进入连续测量模式
 */
hal_status_t qmc5883p_set_config(qmc5883p_handle_t *handle, qmc5883p_odr_t odr,
                                  qmc5883p_range_t range, qmc5883p_osr_t osr);

/**
 * @brief 检查数据是否就绪
 * @param handle QMC5883P句柄
 * @param ready 数据就绪标志输出 (1=就绪, 0=未就绪)
 * @return HAL状态
 */
hal_status_t qmc5883p_data_ready(qmc5883p_handle_t *handle, uint8_t *ready);

/**
 * @brief 软件复位
 * @param handle QMC5883P句柄
 * @return HAL状态
 * @note 复位后传感器进入待机模式，需要重新配置
 */
hal_status_t qmc5883p_soft_reset(qmc5883p_handle_t *handle);

/**
 * @brief 获取当前量程对应的转换系数
 * @param handle QMC5883P句柄
 * @return 转换系数 (Gauss/LSB)
 */
float qmc5883p_get_scale_factor(qmc5883p_handle_t *handle);

/* ============================================================================
 * 测试函数
 * ============================================================================ */

#ifdef QMC5883P_ENABLE_TEST
/**
 * @brief QMC5883P 自检函数
 * @param hi2c I2C句柄
 * @return HAL状态
 * @note 测试I2C通信、CHIP_ID读取和数据读取
 */
hal_status_t qmc5883p_self_test(i2c_handle_t *hi2c);

/**
 * @brief QMC5883P 数据读取测试
 * @param hi2c I2C句柄
 * @param sample_count 采样次数
 * @return HAL状态
 * @note 连续读取并输出磁场数据，用于验证传感器工作正常
 */
hal_status_t qmc5883p_read_test(i2c_handle_t *hi2c, uint16_t sample_count);
#endif /* QMC5883P_ENABLE_TEST */

#ifdef __cplusplus
}
#endif

#endif /* QMC5883P_H */
