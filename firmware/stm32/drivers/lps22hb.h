/**
 * @file lps22hb.h
 * @brief LPS22HBTR 气压计驱动头文件 (SPI版本)
 *
 * @note 本文件为Ralph-loop v2.0 传感器驱动层实现
 *       提供气压和温度数据读取功能
 *
 * @hardware
 *   - 芯片型号: LPS22HBTR
 *   - 接口: SPI3 (PA15=CS, PB3=SCK, PB4=MISO, PB5=MOSI)
 *   - SPI配置: Mode 0 (CPOL=0, CPHA=0), 8-bit, MSB first
 *   - WHO_AM_I: 0xB3 (LPS22HH variant, confirmed by hardware)
 */

#ifndef LPS22HB_H
#define LPS22HB_H

#include "hal_common.h"
#include "spi.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * LPS22HBTR 设备常量
 * ============================================================================ */

#define LPS22HB_WHO_AM_I_VALUE      0xB3U   /**< WHO_AM_I预期值 (LPS22HH = 0xB3, LPS22HB = 0xB1) */

/* ============================================================================
 * LPS22HBTR 寄存器地址定义
 * ============================================================================ */

#define LPS22HB_REG_WHO_AM_I        0x0FU   /**< 设备ID寄存器 */
#define LPS22HB_REG_CTRL_REG1       0x10U   /**< 控制寄存器1 (ODR, BDU, EN_LPFP) */
#define LPS22HB_REG_CTRL_REG2       0x11U   /**< 控制寄存器2 (ONE_SHOT, SWRESET) */
#define LPS22HB_REG_CTRL_REG3       0x12U   /**< 控制寄存器3 (中断配置) */
#define LPS22HB_REG_FIFO_CTRL       0x14U   /**< FIFO控制寄存器 */
#define LPS22HB_REG_REF_P_XL        0x15U   /**< 参考气压低字节 */
#define LPS22HB_REG_REF_P_L         0x16U   /**< 参考气压中字节 */
#define LPS22HB_REG_REF_P_H         0x17U   /**< 参考气压高字节 */
#define LPS22HB_REG_RPDS_L          0x18U   /**< 气压偏移低字节 */
#define LPS22HB_REG_RPDS_H          0x19U   /**< 气压偏移高字节 */
#define LPS22HB_REG_RES_CONF        0x1AU   /**< 分辨率配置 */
#define LPS22HB_REG_INT_SOURCE      0x25U   /**< 中断源寄存器 */
#define LPS22HB_REG_FIFO_STATUS     0x26U   /**< FIFO状态寄存器 */
#define LPS22HB_REG_STATUS          0x27U   /**< 状态寄存器 */
#define LPS22HB_REG_PRESS_OUT_XL    0x28U   /**< 气压数据低字节 */
#define LPS22HB_REG_PRESS_OUT_L     0x29U   /**< 气压数据中字节 */
#define LPS22HB_REG_PRESS_OUT_H     0x2AU   /**< 气压数据高字节 */
#define LPS22HB_REG_TEMP_OUT_L      0x2BU   /**< 温度数据低字节 */
#define LPS22HB_REG_TEMP_OUT_H      0x2CU   /**< 温度数据高字节 */
#define LPS22HB_REG_LPFP_RES        0x33U   /**< LPFP滤波器输出 */

/* ============================================================================
 * CTRL_REG1 寄存器位定义
 * ============================================================================ */

#define LPS22HB_CTRL_REG1_SIM       (1U << 0)   /**< SPI接口模式: 0=4线, 1=3线 */
#define LPS22HB_CTRL_REG1_BDU       (1U << 1)   /**< 块数据更新: 0=连续, 1=读取前不更新 */
#define LPS22HB_CTRL_REG1_LPFP_CFG  (1U << 2)   /**< LPFP配置: 0=ODR/2, 1=ODR/9 */
#define LPS22HB_CTRL_REG1_LPFP_EN   (1U << 3)   /**< LPFP使能 */
#define LPS22HB_CTRL_REG1_ODR_Pos   4U
#define LPS22HB_CTRL_REG1_ODR_Msk   (0x7U << LPS22HB_CTRL_REG1_ODR_Pos)

/* ============================================================================
 * CTRL_REG2 寄存器位定义
 * ============================================================================ */

#define LPS22HB_CTRL_REG2_ONE_SHOT  (1U << 0)   /**< 单次测量触发 */
#define LPS22HB_CTRL_REG2_AUTO_ZERO (1U << 1)   /**< 自动归零 */
#define LPS22HB_CTRL_REG2_SWRESET   (1U << 2)   /**< 软件复位 */
#define LPS22HB_CTRL_REG2_IF_ADD_INC (1U << 4)  /**< I2C地址自动递增 */
#define LPS22HB_CTRL_REG2_I2C_DIS   (1U << 3)   /**< 禁用I2C（SPI模式） */
#define LPS22HB_CTRL_REG2_FIFO_EN   (1U << 6)   /**< FIFO使能 */
#define LPS22HB_CTRL_REG2_BOOT      (1U << 7)   /**< 重启内存内容 */

/* ============================================================================
 * STATUS 寄存器位定义
 * ============================================================================ */

#define LPS22HB_STATUS_T_DA         (1U << 0)   /**< 温度数据可用 */
#define LPS22HB_STATUS_P_DA         (1U << 1)   /**< 气压数据可用 */
#define LPS22HB_STATUS_T_OR         (1U << 4)   /**< 温度数据溢出 */
#define LPS22HB_STATUS_P_OR         (1U << 5)   /**< 气压数据溢出 */

/* ============================================================================
 * SPI读写命令位
 * ============================================================================ */

#define LPS22HB_SPI_READ_BIT        0x80U   /**< 读操作位 (bit7=1) */
#define LPS22HB_SPI_WRITE_BIT       0x00U   /**< 写操作位 (bit7=0) */
#define LPS22HB_SPI_AUTO_INC_BIT    0x40U   /**< 地址自动递增位 (bit6=1, multi-byte) */
#define LPS22HB_SPI_READ_MULTI      (LPS22HB_SPI_READ_BIT | LPS22HB_SPI_AUTO_INC_BIT)   /**< 多字节读: bit7=1, bit6=1 = 0xC0 */

/* ============================================================================
 * 输出数据率 (ODR) 枚举
 * ============================================================================ */

typedef enum {
    LPS22HB_ODR_ONE_SHOT = 0x00U,   /**< 单次测量模式 */
    LPS22HB_ODR_1_HZ     = 0x01U,   /**< 1 Hz */
    LPS22HB_ODR_10_HZ    = 0x02U,   /**< 10 Hz */
    LPS22HB_ODR_25_HZ    = 0x03U,   /**< 25 Hz (推荐) */
    LPS22HB_ODR_50_HZ    = 0x04U,   /**< 50 Hz */
    LPS22HB_ODR_75_HZ    = 0x05U    /**< 75 Hz */
} lps22hb_odr_t;

/* ============================================================================
 * 低通滤波器 (LPFP) 配置枚举
 * ============================================================================ */

typedef enum {
    LPS22HB_LPFP_DISABLE = 0x00U,   /**< 禁用LPFP */
    LPS22HB_LPFP_ODR_2   = 0x01U,   /**< ODR/2 */
    LPS22HB_LPFP_ODR_9   = 0x03U    /**< ODR/9 */
} lps22hb_lpfp_t;

/* ============================================================================
 * LPS22HBTR 句柄结构体
 * ============================================================================ */

typedef struct {
    spi_handle_t   *spi;            /**< SPI句柄指针 */
    lps22hb_odr_t   odr;            /**< 当前输出数据率 */
    uint32_t        timeout;        /**< 默认超时时间 (ms) */
    uint8_t         initialized;    /**< 初始化标志 */
} lps22hb_handle_t;

/* ============================================================================
 * 数据读取结构体
 * ============================================================================ */

typedef struct {
    int32_t  pressure_raw;          /**< 气压原始值 (24位有符号) */
    int16_t  temperature_raw;       /**< 温度原始值 (16位有符号) */
    float    pressure_hpa;          /**< 气压值 (hPa) */
    float    temperature_c;         /**< 温度值 (摄氏度) */
} lps22hb_data_t;

/* ============================================================================
 * API 声明
 * ============================================================================ */

/**
 * @brief 初始化LPS22HBTR传感器 (SPI版本)
 * @param hlps22hb LPS22HB句柄
 * @param hspi SPI句柄指针 (必须已初始化)
 * @param odr 输出数据率
 * @return HAL状态
 * @note 验证WHO_AM_I并配置默认ODR和BDU
 */
hal_status_t lps22hb_init(lps22hb_handle_t *hlps22hb, spi_handle_t *hspi, lps22hb_odr_t odr);

/**
 * @brief 反初始化LPS22HBTR传感器
 * @param hlps22hb LPS22HB句柄
 * @return HAL状态
 */
hal_status_t lps22hb_deinit(lps22hb_handle_t *hlps22hb);

/**
 * @brief 读取WHO_AM_I寄存器
 * @param hlps22hb LPS22HB句柄
 * @param id 设备ID输出 (应为0xB3)
 * @return HAL状态
 */
hal_status_t lps22hb_read_id(lps22hb_handle_t *hlps22hb, uint8_t *id);

/**
 * @brief 读取气压和温度数据
 * @param hlps22hb LPS22HB句柄
 * @param data 数据输出结构体
 * @return HAL状态
 * @note 自动进行数据转换 (hPa和摄氏度)
 */
hal_status_t lps22hb_read_data(lps22hb_handle_t *hlps22hb, lps22hb_data_t *data);

/**
 * @brief 设置输出数据率
 * @param hlps22hb LPS22HB句柄
 * @param odr 输出数据率
 * @return HAL状态
 */
hal_status_t lps22hb_set_odr(lps22hb_handle_t *hlps22hb, lps22hb_odr_t odr);

/**
 * @brief 配置低通滤波器
 * @param hlps22hb LPS22HB句柄
 * @param lpfp LPFP配置
 * @return HAL状态
 */
hal_status_t lps22hb_set_lpfp(lps22hb_handle_t *hlps22hb, lps22hb_lpfp_t lpfp);

/**
 * @brief 触发单次测量
 * @param hlps22hb LPS22HB句柄
 * @return HAL状态
 * @note 仅在单次测量模式下有效
 */
hal_status_t lps22hb_one_shot(lps22hb_handle_t *hlps22hb);

/**
 * @brief 检查数据是否就绪
 * @param hlps22hb LPS22HB句柄
 * @param pressure_ready 气压数据就绪标志 (输出)
 * @param temp_ready 温度数据就绪标志 (输出)
 * @return HAL状态
 */
hal_status_t lps22hb_data_ready(lps22hb_handle_t *hlps22hb, uint8_t *pressure_ready, uint8_t *temp_ready);

/**
 * @brief 软件复位
 * @param hlps22hb LPS22HB句柄
 * @return HAL状态
 */
hal_status_t lps22hb_reset(lps22hb_handle_t *hlps22hb);

/**
 * @brief 将原始气压值转换为hPa
 * @param raw 原始气压值 (24位有符号)
 * @return 气压值 (hPa)
 * @note 转换公式: hPa = raw / 4096.0f
 */
float lps22hb_pressure_to_hpa(int32_t raw);

/**
 * @brief 将原始温度值转换为摄氏度
 * @param raw 原始温度值 (16位有符号)
 * @return 温度值 (摄氏度)
 * @note 转换公式: C = raw / 100.0f
 */
float lps22hb_temp_to_celsius(int16_t raw);

/* ============================================================================
 * 测试函数声明
 * ============================================================================ */

#ifdef LPS22HB_ENABLE_TEST

/**
 * @brief LPS22HBTR驱动测试函数
 * @param hspi SPI句柄指针
 * @return HAL状态
 * @note 测试WHO_AM_I读取和数据读取功能
 */
hal_status_t lps22hb_test(spi_handle_t *hspi);

#endif /* LPS22HB_ENABLE_TEST */

#ifdef __cplusplus
}
#endif

#endif /* LPS22HB_H */
