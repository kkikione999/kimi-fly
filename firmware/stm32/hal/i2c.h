/**
 * @file i2c.h
 * @brief STM32 I2C HAL接口定义
 *
 * @note 本文件为Ralph-loop v2.0 HAL层实现
 *       支持I2C1与气压计(LPS22HBTR)和磁力计(QMC5883P)通信
 *
 * @hardware
 *   - I2C1: PB6 (SCL), PB7 (SDA)
 *   - 时钟源: APB1 (42MHz)
 *   - 设备地址:
 *     - LPS22HBTR: 0x5C (SA0=0) 或 0x5D (SA0=1)
 *     - QMC5883P: 0x0D
 */

#ifndef I2C_H
#define I2C_H

#include "hal_common.h"
#include "gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * I2C 设备地址定义
 * ============================================================================ */

/**
 * @brief LPS22HBTR 气压计设备地址
 * @note SA0引脚决定地址: SA0=0 -> 0x5C, SA0=1 -> 0x5D
 */
#define LPS22HBTR_ADDR_SA0_0    0x5CU
#define LPS22HBTR_ADDR_SA0_1    0x5DU

/**
 * @brief QMC5883P 磁力计设备地址
 */
#define QMC5883P_ADDR           0x0DU

/* ============================================================================
 * I2C 时钟频率定义
 * ============================================================================ */

typedef enum {
    I2C_SPEED_STANDARD = 100000U,   /**< 标准模式 100kHz */
    I2C_SPEED_FAST     = 400000U    /**< 快速模式 400kHz */
} i2c_speed_t;

/* ============================================================================
 * I2C 地址模式定义
 * ============================================================================ */

typedef enum {
    I2C_ADDR_MODE_7BIT  = 0x00U,    /**< 7位地址模式 */
    I2C_ADDR_MODE_10BIT = 0x01U     /**< 10位地址模式 (本实现不支持) */
} i2c_addr_mode_t;

/* ============================================================================
 * I2C 配置结构体
 * ============================================================================ */

typedef struct {
    i2c_speed_t     clock_speed;    /**< I2C时钟频率 (Hz) */
    i2c_addr_mode_t addr_mode;      /**< 地址模式 */
    uint16_t        own_address;    /**< 本机地址 (从模式使用, 主模式设为0) */
} i2c_config_t;

/* ============================================================================
 * I2C 句柄结构体
 * ============================================================================ */

typedef struct {
    void           *instance;       /**< I2C外设实例指针 (I2C1, I2C2, I2C3) */
    i2c_config_t    config;         /**< I2C配置 */
    uint32_t        timeout;        /**< 默认超时时间 (ms) */
    uint32_t        error_code;     /**< 错误码 */
} i2c_handle_t;

/* ============================================================================
 * I2C 错误码定义
 * ============================================================================ */

#define I2C_ERROR_NONE          0x00000000U /**< 无错误 */
#define I2C_ERROR_BERR          0x00000001U /**< 总线错误 */
#define I2C_ERROR_ARLO          0x00000002U /**< 仲裁丢失 */
#define I2C_ERROR_AF            0x00000004U /**< 应答失败 */
#define I2C_ERROR_OVR           0x00000008U /**< 溢出/欠载 */
#define I2C_ERROR_DMA           0x00000010U /**< DMA传输错误 */
#define I2C_ERROR_TIMEOUT       0x00000020U /**< 超时错误 */
#define I2C_ERROR_SIZE          0x00000040U /**< 数据大小错误 */

/* ============================================================================
 * I2C API 声明
 * ============================================================================ */

/**
 * @brief 初始化I2C外设
 * @param hi2c I2C句柄
 * @param config I2C配置
 * @return HAL状态
 * @note 自动配置GPIO: PB6=SCL, PB7=SDA (开漏+上拉)
 */
hal_status_t i2c_init(i2c_handle_t *hi2c, const i2c_config_t *config);

/**
 * @brief 反初始化I2C外设
 * @param hi2c I2C句柄
 * @return HAL状态
 */
hal_status_t i2c_deinit(i2c_handle_t *hi2c);

/**
 * @brief I2C主模式发送数据
 * @param hi2c I2C句柄
 * @param dev_addr 设备地址 (7位地址, 无需左移)
 * @param data 数据缓冲区
 * @param size 数据大小 (字节)
 * @param timeout 超时时间 (ms), HAL_MAX_DELAY表示无限等待
 * @return HAL状态
 */
hal_status_t i2c_master_transmit(i2c_handle_t *hi2c, uint16_t dev_addr,
                                  const uint8_t *data, uint16_t size,
                                  uint32_t timeout);

/**
 * @brief I2C主模式接收数据
 * @param hi2c I2C句柄
 * @param dev_addr 设备地址 (7位地址, 无需左移)
 * @param data 数据缓冲区
 * @param size 数据大小 (字节)
 * @param timeout 超时时间 (ms), HAL_MAX_DELAY表示无限等待
 * @return HAL状态
 */
hal_status_t i2c_master_receive(i2c_handle_t *hi2c, uint16_t dev_addr,
                                 uint8_t *data, uint16_t size,
                                 uint32_t timeout);

/**
 * @brief 向I2C设备寄存器写入数据 (带内存地址)
 * @param hi2c I2C句柄
 * @param dev_addr 设备地址 (7位地址, 无需左移)
 * @param mem_addr 寄存器/内存地址
 * @param mem_addr_size 内存地址大小 (1或2字节)
 * @param data 数据缓冲区
 * @param size 数据大小 (字节)
 * @param timeout 超时时间 (ms)
 * @return HAL状态
 */
hal_status_t i2c_mem_write(i2c_handle_t *hi2c, uint16_t dev_addr,
                            uint16_t mem_addr, uint8_t mem_addr_size,
                            const uint8_t *data, uint16_t size,
                            uint32_t timeout);

/**
 * @brief 从I2C设备寄存器读取数据 (带内存地址)
 * @param hi2c I2C句柄
 * @param dev_addr 设备地址 (7位地址, 无需左移)
 * @param mem_addr 寄存器/内存地址
 * @param mem_addr_size 内存地址大小 (1或2字节)
 * @param data 数据缓冲区
 * @param size 数据大小 (字节)
 * @param timeout 超时时间 (ms)
 * @return HAL状态
 */
hal_status_t i2c_mem_read(i2c_handle_t *hi2c, uint16_t dev_addr,
                           uint16_t mem_addr, uint8_t mem_addr_size,
                           uint8_t *data, uint16_t size,
                           uint32_t timeout);

/**
 * @brief 扫描I2C总线上的设备
 * @param hi2c I2C句柄
 * @param found_addr 发现的设备地址数组 (输出)
 * @param max_count 最大检测数量
 * @param found_count 实际发现的设备数量 (输出)
 * @return HAL状态
 * @note 扫描地址范围: 0x08 - 0x77 (7位地址有效范围)
 */
hal_status_t i2c_scan(i2c_handle_t *hi2c, uint8_t *found_addr,
                       uint8_t max_count, uint8_t *found_count);

/**
 * @brief 检查设备是否响应
 * @param hi2c I2C句柄
 * @param dev_addr 设备地址 (7位地址, 无需左移)
 * @return HAL_OK表示设备存在, HAL_ERROR表示无响应
 */
hal_status_t i2c_is_device_ready(i2c_handle_t *hi2c, uint16_t dev_addr);

/**
 * @brief 获取I2C错误码
 * @param hi2c I2C句柄
 * @return 错误码 (I2C_ERROR_xxx)
 */
uint32_t i2c_get_error(i2c_handle_t *hi2c);

/**
 * @brief 清除I2C错误码
 * @param hi2c I2C句柄
 */
void i2c_clear_error(i2c_handle_t *hi2c);

#ifdef __cplusplus
}
#endif

#endif /* I2C_H */
