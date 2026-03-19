/**
 * @file sensor_test.c
 * @brief 综合传感器测试程序实现
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

#include "sensor_test.h"
#include <string.h>
#include <stdarg.h>

/* ============================================================================
 * 私有变量
 * ============================================================================ */

static uart_handle_t g_uart1;       /**< UART1句柄 (调试输出) */
static i2c_handle_t g_i2c1;         /**< I2C1句柄 */
static spi_handle_t g_spi3;         /**< SPI3句柄 */
static uint8_t g_test_initialized = 0;

/* 发送缓冲区 */
static char g_tx_buffer[256];

/* ============================================================================
 * 私有函数声明
 * ============================================================================ */

static hal_status_t uart1_init_for_debug(void);
static hal_status_t i2c1_init_for_sensors(void);
static hal_status_t spi3_init_for_barometer(void);

/* ============================================================================
 * 基础函数实现
 * ============================================================================ */

/**
 * @brief 简单延时函数
 */
void sensor_delay_ms(uint32_t ms)
{
    volatile uint32_t count;
    while (ms--) {
        for (count = 0; count < 10000; count++) {
            __asm__ volatile("nop");
        }
    }
}

/**
 * @brief 格式化输出到UART1
 */
void sensor_printf(const char *format, ...)
{
    va_list args;
    int len;

    va_start(args, format);
    /* 简单实现：使用vsnprintf格式化到缓冲区 */
    /* 注意：实际项目中应使用更完善的printf实现 */
    len = vsnprintf(g_tx_buffer, sizeof(g_tx_buffer), format, args);
    va_end(args);

    if (len > 0 && len < (int)sizeof(g_tx_buffer)) {
        uart_send(&g_uart1, (uint8_t *)g_tx_buffer, len, 100);
    }
}

/* ============================================================================
 * 测试环境初始化
 * ============================================================================ */

/**
 * @brief 初始化UART1用于调试输出
 */
static hal_status_t uart1_init_for_debug(void)
{
    uart_config_t config;

    config.baudrate = SENSOR_TEST_UART_BAUDRATE;
    config.databits = UART_DATABITS_8;
    config.stopbits = UART_STOPBITS_1;
    config.parity = UART_PARITY_NONE;
    config.hwcontrol = UART_HWCONTROL_NONE;
    config.mode = UART_MODE_TX_RX;

    return uart_init(&g_uart1, UART_INSTANCE_1, &config);
}

/**
 * @brief 初始化I2C1用于传感器通信
 */
static hal_status_t i2c1_init_for_sensors(void)
{
    i2c_config_t config;

    config.clock_speed = I2C_SPEED_FAST;  /* 400kHz */
    config.addr_mode = I2C_ADDR_MODE_7BIT;
    config.own_address = 0;

    return i2c_init(&g_i2c1, &config);
}

/**
 * @brief 初始化SPI3用于气压计通信
 */
static hal_status_t spi3_init_for_barometer(void)
{
    spi_config_t config;

    /* 配置SPI3用于气压计 */
    config.cpol = SPI_CPOL_LOW;         /* Mode 0: CPOL=0 */
    config.cpha = SPI_CPHA_1EDGE;       /* Mode 0: CPHA=0 */
    config.data_size = SPI_DATA_SIZE_8BIT;
    config.nss_mode = SPI_NSS_MODE_SOFT;     /* 软件NSS控制 */
    config.baudrate_prescaler = SPI_PRESCALER_8;  /* 5.25MHz */
    config.msb_first = 1;               /* MSB在前 */

    g_spi3.periph = SPI_PERIPH_3;
    return spi_init(&g_spi3, &config);
}

/**
 * @brief 初始化测试环境
 */
hal_status_t sensor_test_init(void)
{
    hal_status_t status;

    /* 初始化UART1用于调试输出 */
    status = uart1_init_for_debug();
    if (status != HAL_OK) {
        return status;
    }

    sensor_printf("\r\n");
    sensor_printf("========================================\r\n");
    sensor_printf("    无人机传感器综合测试程序\r\n");
    sensor_printf("    Sensor Test Suite v1.0\r\n");
    sensor_printf("========================================\r\n");
    sensor_printf("\r\n");

    /* 初始化I2C1 */
    sensor_printf("[INIT] Initializing I2C1...\r\n");
    status = i2c1_init_for_sensors();
    if (status != HAL_OK) {
        sensor_printf("[ERROR] I2C1 init failed!\r\n");
        return status;
    }
    sensor_printf("[OK] I2C1 initialized (400kHz)\r\n");

    /* 初始化SPI3 */
    sensor_printf("[INIT] Initializing SPI3...\r\n");
    status = spi3_init_for_barometer();
    if (status != HAL_OK) {
        sensor_printf("[ERROR] SPI3 init failed!\r\n");
        return status;
    }
    sensor_printf("[OK] SPI3 initialized (Mode 0, 5.25MHz)\r\n");

    sensor_printf("\r\n");
    g_test_initialized = 1;

    return HAL_OK;
}

/**
 * @brief 反初始化测试环境
 */
hal_status_t sensor_test_deinit(void)
{
    if (!g_test_initialized) {
        return HAL_OK;
    }

    uart_deinit(&g_uart1);
    i2c_deinit(&g_i2c1);
    spi_deinit(&g_spi3);

    g_test_initialized = 0;
    return HAL_OK;
}

/* ============================================================================
 * I2C总线扫描测试
 * ============================================================================ */

/**
 * @brief I2C总线扫描测试
 */
hal_status_t sensor_test_i2c_scan(sensor_test_results_t *results)
{
    hal_status_t status;
    uint8_t found_count = 0;

    sensor_printf("--- Test 1: I2C Bus Scan ---\r\n");
    sensor_printf("Scanning I2C1 bus (PB6=SCL, PB7=SDA)...\r\n");
    sensor_printf("Expected devices:\r\n");
    sensor_printf("  - ICM-42688-P @ 0x68 (IMU)\r\n");
    sensor_printf("  - QMC5883P @ 0x2C (Magnetometer)\r\n");
    sensor_printf("\r\n");

    /* 扫描所有可能的7位I2C地址 */
    sensor_printf("Scanning addresses 0x08-0x77:\r\n");
    for (uint16_t addr = 0x08; addr <= 0x77; addr++) {
        status = i2c_is_device_ready(&g_i2c1, addr);
        if (status == HAL_OK) {
            sensor_printf("  [FOUND] Device at 0x%02X\r\n", addr);
            if (found_count < 16) {
                results->i2c_device_addrs[found_count] = (uint8_t)addr;
            }
            found_count++;
        }
    }

    results->i2c_devices_found = found_count;
    sensor_printf("\r\nTotal devices found: %d\r\n", found_count);

    /* 检查是否找到预期的设备 */
    uint8_t found_icm = 0;
    uint8_t found_mag = 0;

    for (uint8_t i = 0; i < found_count && i < 16; i++) {
        if (results->i2c_device_addrs[i] == ICM42688_I2C_ADDR) {
            found_icm = 1;
        }
        if (results->i2c_device_addrs[i] == QMC5883P_I2C_ADDR) {
            found_mag = 1;
        }
    }

    if (found_icm) {
        sensor_printf("[PASS] ICM-42688-P (0x68) detected\r\n");
    } else {
        sensor_printf("[WARN] ICM-42688-P (0x68) NOT detected\r\n");
    }

    if (found_mag) {
        sensor_printf("[PASS] QMC5883P (0x2C) detected\r\n");
    } else {
        sensor_printf("[WARN] QMC5883P (0x2C) NOT detected\r\n");
    }

    results->i2c_scan = (found_count >= 2) ? TEST_RESULT_PASS : TEST_RESULT_FAIL;

    sensor_printf("\r\n");
    return HAL_OK;
}

/* ============================================================================
 * ICM-42688-P IMU测试
 * ============================================================================ */

/**
 * @brief ICM-42688-P IMU测试
 */
hal_status_t sensor_test_icm42688(sensor_test_results_t *results)
{
    hal_status_t status;
    icm42688_handle_t himu;
    uint8_t id;
    icm42688_data_t data;

    sensor_printf("--- Test 2: ICM-42688-P IMU ---\r\n");
    sensor_printf("Interface: I2C1, Address: 0x%02X\r\n", ICM42688_I2C_ADDR);

    /* 初始化IMU */
    sensor_printf("[INIT] Initializing IMU...\r\n");
    status = icm42688_init(&himu, &g_i2c1);
    if (status != HAL_OK) {
        sensor_printf("[FAIL] IMU init failed!\r\n");
        results->icm42688 = TEST_RESULT_FAIL;
        return status;
    }
    sensor_printf("[OK] IMU initialized\r\n");

    /* 读取WHO_AM_I */
    status = icm42688_read_id(&himu, &id);
    if (status != HAL_OK || id != ICM42688_WHO_AM_I_VALUE) {
        sensor_printf("[FAIL] WHO_AM_I check failed! Expected 0x%02X, got 0x%02X\r\n",
                      ICM42688_WHO_AM_I_VALUE, id);
        results->icm42688 = TEST_RESULT_FAIL;
        return HAL_ERROR;
    }
    sensor_printf("[PASS] WHO_AM_I = 0x%02X\r\n", id);

    /* 读取数据样本 */
    sensor_printf("\r\nReading %d samples:\r\n", SENSOR_TEST_SAMPLE_COUNT);
    sensor_printf("%-4s %-10s %-10s %-10s %-12s %-12s %-12s %-8s\r\n",
                  "#", "AccX(g)", "AccY(g)", "AccZ(g)", "GyroX(dps)", "GyroY(dps)", "GyroZ(dps)", "Temp(C)");

    for (uint8_t i = 0; i < SENSOR_TEST_SAMPLE_COUNT; i++) {
        status = icm42688_read_data(&himu, &data);
        if (status != HAL_OK) {
            sensor_printf("Sample %d: Read failed\r\n", i + 1);
            continue;
        }

        sensor_printf("%-4d %-10.3f %-10.3f %-10.3f %-12.2f %-12.2f %-12.2f %-8.1f\r\n",
                      i + 1,
                      data.accel_x, data.accel_y, data.accel_z,
                      data.gyro_x, data.gyro_y, data.gyro_z,
                      data.temp);

        sensor_delay_ms(SENSOR_TEST_SAMPLE_DELAY_MS);
    }

    /* 反初始化 */
    icm42688_deinit(&himu);

    sensor_printf("\r\n[PASS] IMU test completed\r\n");
    results->icm42688 = TEST_RESULT_PASS;
    sensor_printf("\r\n");

    return HAL_OK;
}

/* ============================================================================
 * LPS22HBTR气压计测试
 * ============================================================================ */

/**
 * @brief LPS22HBTR气压计测试
 */
hal_status_t sensor_test_lps22hb(sensor_test_results_t *results)
{
    hal_status_t status;
    lps22hb_handle_t hlps22;
    uint8_t id;
    lps22hb_data_t data;

    sensor_printf("--- Test 3: LPS22HBTR Barometer ---\r\n");
    sensor_printf("Interface: SPI3, CS=PA15, SCK=PB3, MISO=PB4, MOSI=PB5\r\n");

    /* 初始化气压计 */
    sensor_printf("[INIT] Initializing barometer...\r\n");
    status = lps22hb_init(&hlps22, &g_spi3, LPS22HB_ODR_25_HZ);
    if (status != HAL_OK) {
        sensor_printf("[FAIL] Barometer init failed!\r\n");
        results->lps22hb = TEST_RESULT_FAIL;
        return status;
    }
    sensor_printf("[OK] Barometer initialized\r\n");

    /* 读取WHO_AM_I */
    status = lps22hb_read_id(&hlps22, &id);
    if (status != HAL_OK || id != LPS22HB_WHO_AM_I_VALUE) {
        sensor_printf("[FAIL] WHO_AM_I check failed! Expected 0x%02X, got 0x%02X\r\n",
                      LPS22HB_WHO_AM_I_VALUE, id);
        results->lps22hb = TEST_RESULT_FAIL;
        return HAL_ERROR;
    }
    sensor_printf("[PASS] WHO_AM_I = 0x%02X\r\n", id);

    /* 读取数据样本 */
    sensor_printf("\r\nReading %d samples:\r\n", SENSOR_TEST_SAMPLE_COUNT);
    sensor_printf("%-4s %-14s %-10s\r\n", "#", "Pressure(hPa)", "Temp(C)");

    for (uint8_t i = 0; i < SENSOR_TEST_SAMPLE_COUNT; i++) {
        status = lps22hb_read_data(&hlps22, &data);
        if (status != HAL_OK) {
            sensor_printf("Sample %d: Read failed\r\n", i + 1);
            continue;
        }

        sensor_printf("%-4d %-14.3f %-10.2f\r\n",
                      i + 1,
                      data.pressure_hpa,
                      data.temperature_c);

        sensor_delay_ms(SENSOR_TEST_SAMPLE_DELAY_MS);
    }

    /* 反初始化 */
    lps22hb_deinit(&hlps22);

    sensor_printf("\r\n[PASS] Barometer test completed\r\n");
    results->lps22hb = TEST_RESULT_PASS;
    sensor_printf("\r\n");

    return HAL_OK;
}

/* ============================================================================
 * QMC5883P磁力计测试
 * ============================================================================ */

/**
 * @brief QMC5883P磁力计测试
 */
hal_status_t sensor_test_qmc5883(sensor_test_results_t *results)
{
    hal_status_t status;
    qmc5883p_handle_t hqmc;
    qmc5883p_data_t data;

    sensor_printf("--- Test 4: QMC5883P Magnetometer ---\r\n");
    sensor_printf("Interface: I2C1, Address: 0x%02X\r\n", QMC5883P_I2C_ADDR);

    /* 初始化磁力计 */
    sensor_printf("[INIT] Initializing magnetometer...\r\n");
    status = qmc5883p_init(&hqmc, &g_i2c1);
    if (status != HAL_OK) {
        sensor_printf("[FAIL] Magnetometer init failed!\r\n");
        results->qmc5883 = TEST_RESULT_FAIL;
        return status;
    }
    sensor_printf("[OK] Magnetometer initialized\r\n");

    /* 读取数据样本 */
    sensor_printf("\r\nReading %d samples:\r\n", SENSOR_TEST_SAMPLE_COUNT);
    sensor_printf("%-4s %-10s %-10s %-10s %-12s %-12s %-12s\r\n",
                  "#", "MagX(G)", "MagY(G)", "MagZ(G)", "MagX(raw)", "MagY(raw)", "MagZ(raw)");

    for (uint8_t i = 0; i < SENSOR_TEST_SAMPLE_COUNT; i++) {
        status = qmc5883p_read_data(&hqmc, &data);
        if (status != HAL_OK) {
            sensor_printf("Sample %d: Read failed\r\n", i + 1);
            continue;
        }

        sensor_printf("%-4d %-10.3f %-10.3f %-10.3f %-12d %-12d %-12d\r\n",
                      i + 1,
                      data.mag_x_gauss, data.mag_y_gauss, data.mag_z_gauss,
                      data.mag_x, data.mag_y, data.mag_z);

        sensor_delay_ms(SENSOR_TEST_SAMPLE_DELAY_MS);
    }

    /* 反初始化 */
    qmc5883p_deinit(&hqmc);

    sensor_printf("\r\n[PASS] Magnetometer test completed\r\n");
    results->qmc5883 = TEST_RESULT_PASS;
    sensor_printf("\r\n");

    return HAL_OK;
}

/* ============================================================================
 * 打印测试报告
 * ============================================================================ */

/**
 * @brief 打印测试报告
 */
void sensor_test_print_report(const sensor_test_results_t *results)
{
    int pass_count = 0;
    int total_tests = 4;

    if (results->i2c_scan == TEST_RESULT_PASS) pass_count++;
    if (results->icm42688 == TEST_RESULT_PASS) pass_count++;
    if (results->lps22hb == TEST_RESULT_PASS) pass_count++;
    if (results->qmc5883 == TEST_RESULT_PASS) pass_count++;

    sensor_printf("========================================\r\n");
    sensor_printf("           测试报告 / TEST REPORT         \r\n");
    sensor_printf("========================================\r\n");
    sensor_printf("\r\n");

    /* 各项测试结果 */
    sensor_printf("1. I2C Bus Scan:    %s\r\n",
                  results->i2c_scan == TEST_RESULT_PASS ? "PASS" :
                  results->i2c_scan == TEST_RESULT_FAIL ? "FAIL" : "SKIP");
    sensor_printf("   Devices found: %d\r\n", results->i2c_devices_found);

    sensor_printf("2. ICM-42688-P:     %s\r\n",
                  results->icm42688 == TEST_RESULT_PASS ? "PASS" :
                  results->icm42688 == TEST_RESULT_FAIL ? "FAIL" : "SKIP");

    sensor_printf("3. LPS22HBTR:       %s\r\n",
                  results->lps22hb == TEST_RESULT_PASS ? "PASS" :
                  results->lps22hb == TEST_RESULT_FAIL ? "FAIL" : "SKIP");

    sensor_printf("4. QMC5883P:        %s\r\n",
                  results->qmc5883 == TEST_RESULT_PASS ? "PASS" :
                  results->qmc5883 == TEST_RESULT_FAIL ? "FAIL" : "SKIP");

    sensor_printf("\r\n");
    sensor_printf("Total: %d/%d tests passed\r\n", pass_count, total_tests);
    sensor_printf("\r\n");

    if (pass_count == total_tests) {
        sensor_printf("*** ALL TESTS PASSED ***\r\n");
        sensor_printf("All sensors are working correctly!\r\n");
    } else if (pass_count >= 2) {
        sensor_printf("*** PARTIAL SUCCESS ***\r\n");
        sensor_printf("Some sensors not responding. Check connections.\r\n");
    } else {
        sensor_printf("*** MULTIPLE FAILURES ***\r\n");
        sensor_printf("Most tests failed. Check hardware connections.\r\n");
    }

    sensor_printf("\r\n");
    sensor_printf("========================================\r\n");
}

/* ============================================================================
 * 运行完整测试
 * ============================================================================ */

/**
 * @brief 运行完整的传感器测试
 */
hal_status_t sensor_test_run_all(void)
{
    hal_status_t status;
    sensor_test_results_t results;

    /* 清零结果 */
    memset(&results, 0, sizeof(results));

    /* 初始化测试环境 */
    status = sensor_test_init();
    if (status != HAL_OK) {
        return status;
    }

    /* 运行各项测试 */
    sensor_test_i2c_scan(&results);

    /* 如果找到设备，继续测试 */
    if (results.i2c_devices_found > 0) {
        /* 测试IMU */
        uint8_t icm_found = 0;
        for (uint8_t i = 0; i < results.i2c_devices_found && i < 16; i++) {
            if (results.i2c_device_addrs[i] == ICM42688_I2C_ADDR) {
                icm_found = 1;
                break;
            }
        }
        if (icm_found) {
            sensor_test_icm42688(&results);
        } else {
            sensor_printf("--- Test 2: ICM-42688-P IMU ---\r\n");
            sensor_printf("[SKIP] Device not detected on I2C bus\r\n\r\n");
            results.icm42688 = TEST_RESULT_SKIP;
        }

        /* 测试气压计 (SPI设备) */
        sensor_test_lps22hb(&results);

        /* 测试磁力计 */
        uint8_t mag_found = 0;
        for (uint8_t i = 0; i < results.i2c_devices_found && i < 16; i++) {
            if (results.i2c_device_addrs[i] == QMC5883P_I2C_ADDR) {
                mag_found = 1;
                break;
            }
        }
        if (mag_found) {
            sensor_test_qmc5883(&results);
        } else {
            sensor_printf("--- Test 4: QMC5883P Magnetometer ---\r\n");
            sensor_printf("[SKIP] Device not detected on I2C bus\r\n\r\n");
            results.qmc5883 = TEST_RESULT_SKIP;
        }
    }

    /* 打印测试报告 */
    sensor_test_print_report(&results);

    return HAL_OK;
}

/* ============================================================================
 * 主函数 (测试入口)
 * ============================================================================ */

/**
 * @brief 传感器测试程序主入口
 * @note 编译时定义SENSOR_TEST_STANDALONE可使用此main函数
 */
#ifdef SENSOR_TEST_STANDALONE
int main(void)
{
    /* 运行完整测试 */
    sensor_test_run_all();

    /* 测试完成，挂起 */
    sensor_printf("Test complete. Halting...\r\n");
    while (1) {
        /* 主循环挂起 */
    }

    return 0;
}
#endif /* SENSOR_TEST_STANDALONE */
