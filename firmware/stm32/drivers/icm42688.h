/**
 * @file icm42688.h
 * @brief ICM-42688-P 6轴IMU传感器驱动头文件 (I2C版本)
 *
 * @note 基于TDK InvenSense ICM-42688-P数据手册
 *       接口: I2C1 (PB6=SCL, PB7=SDA)
 *       I2C地址: 0x69 (AD0=VCC) (7-bit), 0xD2 (write), 0xD3 (read)
 *
 * @hardware
 *   - 芯片型号: ICM-42688-P
 *   - 接口: I2C1 (PB6=SCL, PB7=SDA)
 *   - I2C地址: 0x69
 *   - WHO_AM_I: 0x47
 */

#ifndef ICM42688_H
#define ICM42688_H

#include "hal_common.h"
#include "i2c.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * ICM-42688-P 设备常量
 * ============================================================================ */

/**
 * @brief ICM-42688-P I2C设备地址 (7-bit)
 * @note AD0=GND -> 0x68, AD0=VCC -> 0x69
 */
#define ICM42688_I2C_ADDR           0x69U

#define ICM42688_WHO_AM_I_VALUE     0x47U   /**< WHO_AM_I寄存器预期值 */

/* ============================================================================
 * ICM-42688-P 寄存器地址定义 (Bank 0)
 * ============================================================================ */

/* 传感器数据寄存器 */
#define ICM42688_REG_TEMP_DATA1     0x1DU   /**< 温度数据高字节 */
#define ICM42688_REG_TEMP_DATA0     0x1EU   /**< 温度数据低字节 */
#define ICM42688_REG_ACCEL_DATA_X1  0x1FU   /**< 加速度X轴高字节 */
#define ICM42688_REG_ACCEL_DATA_X0  0x20U   /**< 加速度X轴低字节 */
#define ICM42688_REG_ACCEL_DATA_Y1  0x21U   /**< 加速度Y轴高字节 */
#define ICM42688_REG_ACCEL_DATA_Y0  0x22U   /**< 加速度Y轴低字节 */
#define ICM42688_REG_ACCEL_DATA_Z1  0x23U   /**< 加速度Z轴高字节 */
#define ICM42688_REG_ACCEL_DATA_Z0  0x24U   /**< 加速度Z轴低字节 */
#define ICM42688_REG_GYRO_DATA_X1   0x25U   /**< 陀螺仪X轴高字节 */
#define ICM42688_REG_GYRO_DATA_X0   0x26U   /**< 陀螺仪X轴低字节 */
#define ICM42688_REG_GYRO_DATA_Y1   0x27U   /**< 陀螺仪Y轴高字节 */
#define ICM42688_REG_GYRO_DATA_Y0   0x28U   /**< 陀螺仪Y轴低字节 */
#define ICM42688_REG_GYRO_DATA_Z1   0x29U   /**< 陀螺仪Z轴高字节 */
#define ICM42688_REG_GYRO_DATA_Z0   0x2AU   /**< 陀螺仪Z轴低字节 */

/* 时间戳寄存器 */
#define ICM42688_REG_TMST_FSYNCH    0x2BU   /**< 时间戳高字节 */
#define ICM42688_REG_TMST_FSYNCL    0x2CU   /**< 时间戳低字节 */

/* 状态和中断寄存器 */
#define ICM42688_REG_INT_STATUS     0x2DU   /**< 中断状态 */
#define ICM42688_REG_FIFO_COUNTH    0x2EU   /**< FIFO计数高字节 */
#define ICM42688_REG_FIFO_COUNTL    0x2FU   /**< FIFO计数低字节 */
#define ICM42688_REG_FIFO_DATA      0x30U   /**< FIFO数据 */
#define ICM42688_REG_INT_STATUS2    0x37U   /**< 中断状态2 */
#define ICM42688_REG_INT_STATUS3    0x38U   /**< 中断状态3 */

/* 传感器配置寄存器 */
#define ICM42688_REG_PWR_MGMT0      0x4EU   /**< 电源管理0 */
#define ICM42688_REG_GYRO_CONFIG0   0x4FU   /**< 陀螺仪配置0 */
#define ICM42688_REG_ACCEL_CONFIG0  0x50U   /**< 加速度计配置0 */
#define ICM42688_REG_GYRO_CONFIG1   0x51U   /**< 陀螺仪配置1 */
#define ICM42688_REG_GYRO_ACCEL_CONFIG0 0x52U   /**< 陀螺仪/加速度计共享配置 */
#define ICM42688_REG_ACCEL_CONFIG1  0x53U   /**< 加速度计配置1 */
#define ICM42688_REG_TMST_CONFIG    0x54U   /**< 时间戳配置 */

/* 其他配置寄存器 */
#define ICM42688_REG_WHO_AM_I       0x75U   /**< 设备ID寄存器 */
#define ICM42688_REG_BANK_SEL       0x76U   /**< 寄存器Bank选择 */

/* ============================================================================
 * 寄存器位定义
 * ============================================================================ */

/* PWR_MGMT0 (0x4E) */
#define ICM42688_PWR_MGMT0_ACCEL_MODE_Pos   0
#define ICM42688_PWR_MGMT0_ACCEL_MODE_Msk   (0x3U << ICM42688_PWR_MGMT0_ACCEL_MODE_Pos)
#define ICM42688_PWR_MGMT0_GYRO_MODE_Pos    2
#define ICM42688_PWR_MGMT0_GYRO_MODE_Msk    (0x3U << ICM42688_PWR_MGMT0_GYRO_MODE_Pos)
#define ICM42688_PWR_MGMT0_TEMP_DIS_Pos     5
#define ICM42688_PWR_MGMT0_TEMP_DIS_Msk     (0x1U << ICM42688_PWR_MGMT0_TEMP_DIS_Pos)
#define ICM42688_PWR_MGMT0_IDLE_Pos         6
#define ICM42688_PWR_MGMT0_IDLE_Msk         (0x1U << ICM42688_PWR_MGMT0_IDLE_Pos)

/* 电源模式 */
#define ICM42688_PWR_MODE_OFF       0x00U   /**< 关闭 */
#define ICM42688_PWR_MODE_STANDBY   0x01U   /**< 待机模式 */
#define ICM42688_PWR_MODE_LOW_NOISE 0x03U   /**< 低噪声模式 */

/* GYRO_CONFIG0 (0x4F) */
#define ICM42688_GYRO_CONFIG0_FS_SEL_Pos    5
#define ICM42688_GYRO_CONFIG0_FS_SEL_Msk    (0x7U << ICM42688_GYRO_CONFIG0_FS_SEL_Pos)
#define ICM42688_GYRO_CONFIG0_ODR_Pos       0
#define ICM42688_GYRO_CONFIG0_ODR_Msk       (0xFU << ICM42688_GYRO_CONFIG0_ODR_Pos)

/* ACCEL_CONFIG0 (0x50) */
#define ICM42688_ACCEL_CONFIG0_FS_SEL_Pos   5
#define ICM42688_ACCEL_CONFIG0_FS_SEL_Msk   (0x7U << ICM42688_ACCEL_CONFIG0_FS_SEL_Pos)
#define ICM42688_ACCEL_CONFIG0_ODR_Pos      0
#define ICM42688_ACCEL_CONFIG0_ODR_Msk      (0xFU << ICM42688_ACCEL_CONFIG0_ODR_Pos)

/* ============================================================================
 * 陀螺仪量程枚举
 * ============================================================================ */

typedef enum {
    ICM42688_GYRO_RANGE_2000DPS = 0x00U,    /**< ±2000 dps (默认) */
    ICM42688_GYRO_RANGE_1000DPS = 0x01U,    /**< ±1000 dps */
    ICM42688_GYRO_RANGE_500DPS  = 0x02U,    /**< ±500 dps */
    ICM42688_GYRO_RANGE_250DPS  = 0x03U,    /**< ±250 dps */
    ICM42688_GYRO_RANGE_125DPS  = 0x04U,    /**< ±125 dps */
    ICM42688_GYRO_RANGE_62_5DPS = 0x05U,    /**< ±62.5 dps */
    ICM42688_GYRO_RANGE_31_25DPS= 0x06U,    /**< ±31.25 dps */
    ICM42688_GYRO_RANGE_15_625DPS=0x07U     /**< ±15.625 dps */
} icm42688_gyro_range_t;

/* ============================================================================
 * 加速度计量程枚举
 * ============================================================================ */

typedef enum {
    ICM42688_ACCEL_RANGE_16G    = 0x00U,    /**< ±16g (默认) */
    ICM42688_ACCEL_RANGE_8G     = 0x01U,    /**< ±8g */
    ICM42688_ACCEL_RANGE_4G     = 0x02U,    /**< ±4g */
    ICM42688_ACCEL_RANGE_2G     = 0x03U     /**< ±2g */
} icm42688_accel_range_t;

/* ============================================================================
 * 输出数据率 (ODR) 枚举
 * ============================================================================ */

typedef enum {
    ICM42688_ODR_32KHZ          = 0x01U,    /**< 32 kHz (陀螺仪LN模式) */
    ICM42688_ODR_16KHZ          = 0x02U,    /**< 16 kHz (陀螺仪LN模式) */
    ICM42688_ODR_8KHZ           = 0x03U,    /**< 8 kHz */
    ICM42688_ODR_4KHZ           = 0x04U,    /**< 4 kHz */
    ICM42688_ODR_2KHZ           = 0x05U,    /**< 2 kHz */
    ICM42688_ODR_1KHZ           = 0x06U,    /**< 1 kHz (推荐默认) */
    ICM42688_ODR_500HZ          = 0x0FU,    /**< 500 Hz (低功耗加速度) */
    ICM42688_ODR_200HZ          = 0x07U,    /**< 200 Hz */
    ICM42688_ODR_100HZ          = 0x08U,    /**< 100 Hz */
    ICM42688_ODR_50HZ           = 0x09U,    /**< 50 Hz */
    ICM42688_ODR_25HZ           = 0x0AU,    /**< 25 Hz */
    ICM42688_ODR_12_5HZ         = 0x0BU,    /**< 12.5 Hz */
    ICM42688_ODR_6_25HZ         = 0x0CU,    /**< 6.25 Hz */
    ICM42688_ODR_3_125HZ        = 0x0DU,    /**< 3.125 Hz */
    ICM42688_ODR_1_5625HZ       = 0x0EU     /**< 1.5625 Hz */
} icm42688_odr_t;

/* ============================================================================
 * 灵敏度系数 (LSB到物理单位转换)
 * ============================================================================ */

/* 陀螺仪灵敏度系数: dps/LSB (基于±2000dps范围) */
#define ICM42688_GYRO_SENSITIVITY_2000DPS   0.06103515625f   /**< 2000/32768 */
#define ICM42688_GYRO_SENSITIVITY_1000DPS   0.030517578125f  /**< 1000/32768 */
#define ICM42688_GYRO_SENSITIVITY_500DPS    0.0152587890625f /**< 500/32768 */
#define ICM42688_GYRO_SENSITIVITY_250DPS    0.00762939453125f
#define ICM42688_GYRO_SENSITIVITY_125DPS    0.003814697265625f

/* 加速度计灵敏度系数: g/LSB (基于±16g范围) */
#define ICM42688_ACCEL_SENSITIVITY_16G      0.00048828125f   /**< 16/32768 */
#define ICM42688_ACCEL_SENSITIVITY_8G       0.000244140625f  /**< 8/32768 */
#define ICM42688_ACCEL_SENSITIVITY_4G       0.0001220703125f /**< 4/32768 */
#define ICM42688_ACCEL_SENSITIVITY_2G       0.00006103515625f

/* 温度转换系数 */
#define ICM42688_TEMP_SENSITIVITY           132.48f          /**< LSB/°C */
#define ICM42688_TEMP_OFFSET                25.0f            /**< 偏移温度 */

/* ============================================================================
 * 数据结构定义
 * ============================================================================ */

/**
 * @brief ICM-42688-P 句柄结构体 (I2C版本)
 */
typedef struct {
    i2c_handle_t *i2c_handle;           /**< I2C句柄指针 */
    uint16_t dev_addr;                  /**< 设备I2C地址 */
    icm42688_gyro_range_t gyro_range;   /**< 当前陀螺仪量程 */
    icm42688_accel_range_t accel_range; /**< 当前加速度计量程 */
    uint32_t timeout;                   /**< 超时时间(ms) */
    uint8_t initialized;                /**< 初始化标志 */
} icm42688_handle_t;

/**
 * @brief 原始传感器数据结构体 (16位有符号整数)
 */
typedef struct {
    int16_t accel_x;    /**< 加速度X轴原始值 */
    int16_t accel_y;    /**< 加速度Y轴原始值 */
    int16_t accel_z;    /**< 加速度Z轴原始值 */
    int16_t gyro_x;     /**< 陀螺仪X轴原始值 */
    int16_t gyro_y;     /**< 陀螺仪Y轴原始值 */
    int16_t gyro_z;     /**< 陀螺仪Z轴原始值 */
    int16_t temp;       /**< 温度原始值 */
} icm42688_raw_data_t;

/**
 * @brief 物理单位传感器数据结构体
 */
typedef struct {
    float accel_x;      /**< 加速度X轴 (g) */
    float accel_y;      /**< 加速度Y轴 (g) */
    float accel_z;      /**< 加速度Z轴 (g) */
    float gyro_x;       /**< 陀螺仪X轴 (dps) */
    float gyro_y;       /**< 陀螺仪Y轴 (dps) */
    float gyro_z;       /**< 陀螺仪Z轴 (dps) */
    float temp;         /**< 温度 (摄氏度) */
} icm42688_data_t;

/* ============================================================================
 * API函数声明
 * ============================================================================ */

/**
 * @brief 初始化ICM-42688-P传感器 (I2C版本)
 * @param himu ICM-42688-P句柄指针
 * @param hi2c I2C句柄指针 (必须已初始化)
 * @return HAL状态
 * @note 此函数会验证WHO_AM_I并配置默认参数
 */
hal_status_t icm42688_init(icm42688_handle_t *himu, i2c_handle_t *hi2c);

/**
 * @brief 反初始化ICM-42688-P传感器
 * @param himu ICM-42688-P句柄指针
 * @return HAL状态
 */
hal_status_t icm42688_deinit(icm42688_handle_t *himu);

/**
 * @brief 读取WHO_AM_I寄存器
 * @param himu ICM-42688-P句柄指针
 * @param p_id 存储读取到的ID的指针
 * @return HAL状态
 */
hal_status_t icm42688_read_id(icm42688_handle_t *himu, uint8_t *p_id);

/**
 * @brief 读取所有传感器原始数据
 * @param himu ICM-42688-P句柄指针
 * @param p_raw_data 存储原始数据的结构体指针
 * @return HAL状态
 * @note 使用burst read从TEMP_DATA1开始连续读取14字节
 */
hal_status_t icm42688_read_raw_data(icm42688_handle_t *himu, icm42688_raw_data_t *p_raw_data);

/**
 * @brief 读取并转换为物理单位的传感器数据
 * @param himu ICM-42688-P句柄指针
 * @param p_data 存储物理数据的结构体指针
 * @return HAL状态
 */
hal_status_t icm42688_read_data(icm42688_handle_t *himu, icm42688_data_t *p_data);

/**
 * @brief 设置陀螺仪量程
 * @param himu ICM-42688-P句柄指针
 * @param range 陀螺仪量程
 * @return HAL状态
 */
hal_status_t icm42688_set_gyro_range(icm42688_handle_t *himu, icm42688_gyro_range_t range);

/**
 * @brief 设置加速度计量程
 * @param himu ICM-42688-P句柄指针
 * @param range 加速度计量程
 * @return HAL状态
 */
hal_status_t icm42688_set_accel_range(icm42688_handle_t *himu, icm42688_accel_range_t range);

/**
 * @brief 设置输出数据率
 * @param himu ICM-42688-P句柄指针
 * @param gyro_odr 陀螺仪ODR
 * @param accel_odr 加速度计ODR
 * @return HAL状态
 */
hal_status_t icm42688_set_odr(icm42688_handle_t *himu, icm42688_odr_t gyro_odr, icm42688_odr_t accel_odr);

/**
 * @brief 软复位传感器
 * @param himu ICM-42688-P句柄指针
 * @return HAL状态
 */
hal_status_t icm42688_reset(icm42688_handle_t *himu);

/**
 * @brief 将原始陀螺仪数据转换为dps
 * @param himu ICM-42688-P句柄指针
 * @param raw_value 原始值
 * @return 转换后的dps值
 */
float icm42688_convert_gyro_dps(icm42688_handle_t *himu, int16_t raw_value);

/**
 * @brief 将原始加速度计数据转换为g
 * @param himu ICM-42688-P句柄指针
 * @param raw_value 原始值
 * @return 转换后的g值
 */
float icm42688_convert_accel_g(icm42688_handle_t *himu, int16_t raw_value);

/**
 * @brief 将原始温度数据转换为摄氏度
 * @param raw_value 原始值
 * @return 转换后的温度值
 */
float icm42688_convert_temp_celsius(int16_t raw_value);

/**
 * @brief 自检函数 - 验证WHO_AM_I并读取一次数据
 * @param himu ICM-42688-P句柄指针
 * @return HAL状态
 */
hal_status_t icm42688_self_test(icm42688_handle_t *himu);

/* ============================================================================
 * 测试/调试函数
 * ============================================================================ */

#ifdef ICM42688_ENABLE_TEST

/**
 * @brief ICM-42688-P 测试函数
 * @param hi2c I2C句柄指针
 * @return HAL状态
 */
hal_status_t icm42688_test(i2c_handle_t *hi2c);

#endif /* ICM42688_ENABLE_TEST */

#ifdef __cplusplus
}
#endif

#endif /* ICM42688_H */
