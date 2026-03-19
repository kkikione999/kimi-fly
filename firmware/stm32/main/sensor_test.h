/**
 * @file sensor_test.h
 * @brief 综合传感器测试程序头文件
 *
 * @note 测试所有传感器:
 *       - ICM-42688-P (IMU, I2C1, 0x68)
 *       - LPS22HBTR (气压计, SPI3)
 *       - QMC5883P (磁力计, I2C1, 0x2C)
 *
 * @hardware
 *   - UART1: PA9(TX), PA10(RX) - 调试输出 (460800 baud)
 *   - I2C1: PB6(SCL), PB7(SDA) - IMU + 磁力计
 *   - SPI3: PA15(CS), PB3(SCK), PB4(MISO), PB5(MOSI) - 气压计
 */

#ifndef SENSOR_TEST_H
#define SENSOR_TEST_H

#include "hal_common.h"
#include "uart.h"
#include "i2c.h"
#include "spi.h"
#include "../drivers/icm42688.h"
#include "../drivers/lps22hb.h"
#include "../drivers/qmc5883p.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * 测试配置
 * ============================================================================ */

#define SENSOR_TEST_UART_BAUDRATE   460800U     /**< 调试串口波特率 */
#define SENSOR_TEST_SAMPLE_COUNT    10U         /**< 每个传感器采样次数 */
#define SENSOR_TEST_SAMPLE_DELAY_MS 100U        /**< 采样间隔(ms) */

/* ============================================================================
 * 测试结果枚举
 * ============================================================================ */

typedef enum {
    TEST_RESULT_PASS = 0,   /**< 测试通过 */
    TEST_RESULT_FAIL = 1,   /**< 测试失败 */
    TEST_RESULT_SKIP = 2    /**< 测试跳过 */
} test_result_t;

/* ============================================================================
 * 测试统计结构体
 * ============================================================================ */

typedef struct {
    test_result_t i2c_scan;         /**< I2C总线扫描结果 */
    test_result_t icm42688;         /**< IMU测试结果 */
    test_result_t lps22hb;          /**< 气压计测试结果 */
    test_result_t qmc5883;          /**< 磁力计测试结果 */
    uint8_t i2c_devices_found;      /**< 发现的I2C设备数量 */
    uint8_t i2c_device_addrs[16];   /**< 发现的I2C设备地址 */
} sensor_test_results_t;

/* ============================================================================
 * API函数声明
 * ============================================================================ */

/**
 * @brief 运行完整的传感器测试
 * @return HAL状态
 * @note 此函数会初始化所有外设，运行测试，输出结果
 */
hal_status_t sensor_test_run_all(void);

/**
 * @brief 初始化测试环境
 * @return HAL状态
 * @note 初始化UART1、I2C1、SPI3
 */
hal_status_t sensor_test_init(void);

/**
 * @brief 反初始化测试环境
 * @return HAL状态
 */
hal_status_t sensor_test_deinit(void);

/**
 * @brief I2C总线扫描测试
 * @param results 测试结果结构体
 * @return HAL状态
 */
hal_status_t sensor_test_i2c_scan(sensor_test_results_t *results);

/**
 * @brief ICM-42688-P IMU测试
 * @param results 测试结果结构体
 * @return HAL状态
 */
hal_status_t sensor_test_icm42688(sensor_test_results_t *results);

/**
 * @brief LPS22HBTR气压计测试
 * @param results 测试结果结构体
 * @return HAL状态
 */
hal_status_t sensor_test_lps22hb(sensor_test_results_t *results);

/**
 * @brief QMC5883P磁力计测试
 * @param results 测试结果结构体
 * @return HAL状态
 */
hal_status_t sensor_test_qmc5883(sensor_test_results_t *results);

/**
 * @brief 打印测试报告
 * @param results 测试结果结构体
 */
void sensor_test_print_report(const sensor_test_results_t *results);

/**
 * @brief 简单的printf函数，输出到UART1
 * @param format 格式字符串
 * @param ... 可变参数
 */
void sensor_printf(const char *format, ...);

/**
 * @brief 延时函数
 * @param ms 毫秒
 */
void sensor_delay_ms(uint32_t ms);

#ifdef __cplusplus
}
#endif

#endif /* SENSOR_TEST_H */
