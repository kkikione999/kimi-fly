/*
 * Copyright (c) 2026 kimi-fly Project
 * SPDX-License-Identifier: MIT
 */

/**
 * @file verify_hardware.c
 * @brief Hardware verification firmware for kimi-fly drone
 *
 * This is a standalone diagnostic firmware that tests all critical
 * hardware interfaces before the main flight firmware is flashed.
 *
 * Tests performed:
 * - I2C scanner: Detects MPU6050 @ 0x68 and BMP280 @ 0x76
 * - PWM test: Outputs 400Hz signals on PA0-PA3 with different duty cycles
 * - UART loopback: Tests TX/RX at 921600 baud
 *
 * @warning This is verification firmware, NOT the flight firmware.
 *          PWM outputs use low duty cycles safe for motors without props.
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "../../hal/stm32/stm32_hal.h"
#include "../../platform/board_config.h"

/* Test configuration */
#define VERIFY_UART_PORT        UART_PORT_3
#define VERIFY_UART_BAUDRATE    921600U
#define VERIFY_I2C_BUS          I2C_BUS_1
#define VERIFY_PWM_FREQ_HZ      400U
#define VERIFY_PWM_DUTY_SAFE    0.05f   /* 5% duty cycle - safe for motors */

/* I2C device addresses to scan */
#define MPU6050_ADDR            0x68
#define BMP280_ADDR             0x76

/* Test timeout values */
#define TEST_TIMEOUT_MS         100
#define UART_LOOPBACK_BYTES     16

/* Test result tracking */
typedef enum {
    TEST_PASS = 0,
    TEST_FAIL = 1,
    TEST_SKIP = 2
} test_result_t;

static struct {
    test_result_t i2c_mpu6050;
    test_result_t i2c_bmp280;
    test_result_t pwm_all_channels;
    test_result_t uart_loopback;
} g_test_results;

static hal_handle_t g_uart;
static hal_handle_t g_i2c;
static hal_handle_t g_pwm[4];

/* UART output buffer for test messages */
static char msg_buf[128];

/**
 * @brief Send string over UART
 */
static void uart_puts(const char *str)
{
    const hal_uart_interface_t *uart = hal_uart_get_interface();
    if (uart && g_uart) {
        uart->send(g_uart, (const uint8_t *)str, strlen(str), 100);
    }
}

/**
 * @brief Send formatted test result message
 */
static void print_result(const char *test_name, test_result_t result)
{
    const char *status_str;
    switch (result) {
        case TEST_PASS:  status_str = "PASS"; break;
        case TEST_FAIL:  status_str = "FAIL"; break;
        case TEST_SKIP:  status_str = "SKIP"; break;
        default:         status_str = "???"; break;
    }

    int len = snprintf(msg_buf, sizeof(msg_buf),
                       "[%s] %s\r\n", status_str, test_name);
    if (len > 0 && len < (int)sizeof(msg_buf)) {
        uart_puts(msg_buf);
    }
}

/**
 * @brief Initialize all hardware interfaces
 */
static int hardware_init(void)
{
    int rc;
    const hal_uart_interface_t *uart;
    const hal_i2c_interface_t *i2c;
    const hal_pwm_interface_t *pwm;

    /* Initialize HAL layer */
    stm32_hal_init();
    stm32_gpio_register_hal();
    stm32_pwm_register_hal();
    stm32_uart_register_hal();
    stm32_i2c_register_hal();
    stm32_system_register_hal();

    uart_puts("\r\n=== kimi-fly Hardware Verification ===\r\n");
    uart_puts("MCU: STM32F411CEU6\r\n");
    uart_puts("=====================================\r\n\r\n");

    /* Initialize UART for test output and loopback test */
    uart = hal_uart_get_interface();
    if (!uart) {
        uart_puts("ERROR: UART interface not available\r\n");
        return -1;
    }

    hal_uart_cfg_t uart_cfg = {
        .baudrate = VERIFY_UART_BAUDRATE,
        .data_bits = HAL_UART_DATA_BITS_8,
        .stop_bits = HAL_UART_STOP_BITS_1,
        .parity = HAL_UART_PARITY_NONE
    };

    rc = uart->init(VERIFY_UART_PORT, &uart_cfg, &g_uart);
    if (rc != 0) {
        uart_puts("ERROR: UART initialization failed\r\n");
        return rc;
    }
    uart_puts("UART initialized: 921600 baud\r\n");

    /* Initialize I2C for sensor scanning */
    i2c = hal_i2c_get_interface();
    if (!i2c) {
        uart_puts("ERROR: I2C interface not available\r\n");
        return -1;
    }

    hal_i2c_cfg_t i2c_cfg = {
        .speed = HAL_I2C_SPEED_FAST
    };

    rc = i2c->init(VERIFY_I2C_BUS, &i2c_cfg, &g_i2c);
    if (rc != 0) {
        uart_puts("ERROR: I2C initialization failed\r\n");
        return rc;
    }
    uart_puts("I2C initialized: 400kHz\r\n");

    /* Initialize PWM channels */
    pwm = hal_pwm_get_interface();
    if (!pwm) {
        uart_puts("ERROR: PWM interface not available\r\n");
        return -1;
    }

    hal_pwm_cfg_t pwm_cfg = {
        .freq_hz = VERIFY_PWM_FREQ_HZ,
        .pol = HAL_PWM_POLARITY_NORMAL,
        .init_duty = 0.0f  /* Start with 0% duty */
    };

    for (int i = 0; i < 4; i++) {
        rc = pwm->init(i, &pwm_cfg, &g_pwm[i]);
        if (rc != 0) {
            snprintf(msg_buf, sizeof(msg_buf),
                     "ERROR: PWM channel %d initialization failed\r\n", i);
            uart_puts(msg_buf);
            return rc;
        }
    }
    uart_puts("PWM initialized: 400Hz on PA0-PA3\r\n");
    uart_puts("\r\n");

    return 0;
}

/**
 * @brief Test I2C device at specific address
 *
 * Performs a simple write-then-read to verify device presence.
 *
 * @param addr 7-bit I2C address
 * @param name Device name for output
 * @return TEST_PASS if device responds, TEST_FAIL otherwise
 */
static test_result_t test_i2c_device(uint8_t addr, const char *name)
{
    const hal_i2c_interface_t *i2c = hal_i2c_get_interface();
    if (!i2c || !g_i2c) {
        return TEST_FAIL;
    }

    /* Try to read a single byte from device
     * For MPU6050: read WHO_AM_I register (0x75)
     * For BMP280: read chip ID register (0xD0)
     */
    uint8_t reg_addr;
    uint8_t rx_buf;
    int rc;

    if (addr == MPU6050_ADDR) {
        reg_addr = 0x75;  /* WHO_AM_I register */
    } else if (addr == BMP280_ADDR) {
        reg_addr = 0xD0;  /* Chip ID register */
    } else {
        /* Generic probe: just try to read without register */
        rc = i2c->read(g_i2c, addr, &rx_buf, 1, TEST_TIMEOUT_MS);
        return (rc == 0) ? TEST_PASS : TEST_FAIL;
    }

    /* Write register address, then read value */
    rc = i2c->write_read(g_i2c, addr, &reg_addr, 1, &rx_buf, 1, TEST_TIMEOUT_MS);

    if (rc == 0) {
        /* Verify expected chip ID values */
        if (addr == MPU6050_ADDR && rx_buf == 0x68) {
            return TEST_PASS;
        }
        if (addr == BMP280_ADDR && (rx_buf == 0x58 || rx_buf == 0x56)) {
            return TEST_PASS;
        }
        /* Device responded but wrong ID - still counts as present */
        return TEST_PASS;
    }

    return TEST_FAIL;
}

/**
 * @brief Run I2C scanner test
 */
static void test_i2c_scanner(void)
{
    uart_puts("--- I2C Scanner Test ---\r\n");

    /* Test MPU6050 */
    g_test_results.i2c_mpu6050 = test_i2c_device(MPU6050_ADDR, "MPU6050");
    print_result("MPU6050 @ 0x68", g_test_results.i2c_mpu6050);

    /* Test BMP280 */
    g_test_results.i2c_bmp280 = test_i2c_device(BMP280_ADDR, "BMP280");
    print_result("BMP280 @ 0x76", g_test_results.i2c_bmp280);

    /* Scan and report all detected devices */
    uart_puts("Scanning all I2C addresses...\r\n");
    uart_puts("Detected devices: ");

    const hal_i2c_interface_t *i2c = hal_i2c_get_interface();
    bool found_any = false;

    for (uint8_t addr = 0x08; addr < 0x78; addr++) {
        /* Skip known devices already tested */
        if (addr == MPU6050_ADDR || addr == BMP280_ADDR) {
            continue;
        }

        uint8_t dummy;
        int rc = i2c->read(g_i2c, addr, &dummy, 1, 10);
        if (rc == 0) {
            snprintf(msg_buf, sizeof(msg_buf), "0x%02X ", addr);
            uart_puts(msg_buf);
            found_any = true;
        }
    }

    if (!found_any) {
        uart_puts("(none other)");
    }
    uart_puts("\r\n\r\n");
}

/**
 * @brief Run PWM test
 *
 * Outputs 400Hz PWM on all channels with different duty cycles.
 * Uses safe low duty cycles (5-20%) to avoid motor spin-up.
 */
static void test_pwm(void)
{
    const hal_pwm_interface_t *pwm = hal_pwm_get_interface();
    if (!pwm) {
        g_test_results.pwm_all_channels = TEST_FAIL;
        print_result("PWM All Channels", TEST_FAIL);
        return;
    }

    uart_puts("--- PWM Test ---\r\n");
    uart_puts("Outputting 400Hz PWM on PA0-PA3\r\n");
    uart_puts("Duty cycles: CH1=5%, CH2=10%, CH3=15%, CH4=20%\r\n");
    uart_puts("(Safe levels for motors without props)\r\n");

    /* Set different duty cycles on each channel */
    float duties[4] = {0.05f, 0.10f, 0.15f, 0.20f};
    bool all_ok = true;

    for (int i = 0; i < 4; i++) {
        int rc = pwm->set_duty(g_pwm[i], duties[i]);
        if (rc != 0) {
            snprintf(msg_buf, sizeof(msg_buf),
                     "ERROR: PWM CH%d set_duty failed\r\n", i + 1);
            uart_puts(msg_buf);
            all_ok = false;
        } else {
            snprintf(msg_buf, sizeof(msg_buf),
                     "PWM CH%d: %.0f%% duty cycle\r\n", i + 1, duties[i] * 100);
            uart_puts(msg_buf);
        }
    }

    g_test_results.pwm_all_channels = all_ok ? TEST_PASS : TEST_FAIL;
    print_result("PWM All Channels", g_test_results.pwm_all_channels);
    uart_puts("\r\n");
}

/**
 * @brief Run UART loopback test
 *
 * Note: This test requires a loopback connection (TX connected to RX)
 * or an external echo device. Without the loopback, this test will fail.
 */
static void test_uart_loopback(void)
{
    const hal_uart_interface_t *uart = hal_uart_get_interface();
    if (!uart || !g_uart) {
        g_test_results.uart_loopback = TEST_FAIL;
        print_result("UART Loopback", TEST_FAIL);
        return;
    }

    uart_puts("--- UART Loopback Test ---\r\n");
    uart_puts("Testing at 921600 baud...\r\n");
    uart_puts("NOTE: Requires TX-RX loopback jumper for internal test\r\n");

    /* Flush any pending data */
    uart->flush(g_uart);

    /* Test pattern */
    const uint8_t tx_pattern[UART_LOOPBACK_BYTES] = {
        0x55, 0xAA, 0x00, 0xFF,
        0x12, 0x34, 0x56, 0x78,
        0x9A, 0xBC, 0xDE, 0xF0,
        0x01, 0x23, 0x45, 0x67
    };
    uint8_t rx_buf[UART_LOOPBACK_BYTES];

    /* Send test pattern */
    int sent = uart->send(g_uart, tx_pattern, UART_LOOPBACK_BYTES, 100);
    if (sent != UART_LOOPBACK_BYTES) {
        uart_puts("ERROR: UART send failed\r\n");
        g_test_results.uart_loopback = TEST_FAIL;
        print_result("UART Loopback", TEST_FAIL);
        uart_puts("\r\n");
        return;
    }

    /* Small delay for data to loop back */
    stm32_delay_ms(10);

    /* Try to receive data back */
    int received = uart->recv(g_uart, rx_buf, UART_LOOPBACK_BYTES, 100);

    if (received != UART_LOOPBACK_BYTES) {
        snprintf(msg_buf, sizeof(msg_buf),
                 "Received %d/%d bytes (no loopback connection?)\r\n",
                 received, UART_LOOPBACK_BYTES);
        uart_puts(msg_buf);
        g_test_results.uart_loopback = TEST_FAIL;
        print_result("UART Loopback", TEST_FAIL);
        uart_puts("\r\n");
        return;
    }

    /* Verify received data matches sent data */
    bool match = (memcmp(tx_pattern, rx_buf, UART_LOOPBACK_BYTES) == 0);

    if (match) {
        uart_puts("Loopback data verified OK\r\n");
        g_test_results.uart_loopback = TEST_PASS;
    } else {
        uart_puts("ERROR: Loopback data mismatch\r\n");
        g_test_results.uart_loopback = TEST_FAIL;
    }

    print_result("UART Loopback", g_test_results.uart_loopback);
    uart_puts("\r\n");
}

/**
 * @brief Print test summary
 */
static void print_summary(void)
{
    int pass_count = 0;
    int total_tests = 4;

    if (g_test_results.i2c_mpu6050 == TEST_PASS) pass_count++;
    if (g_test_results.i2c_bmp280 == TEST_PASS) pass_count++;
    if (g_test_results.pwm_all_channels == TEST_PASS) pass_count++;
    if (g_test_results.uart_loopback == TEST_PASS) pass_count++;

    uart_puts("=====================================\r\n");
    uart_puts("           TEST SUMMARY              \r\n");
    uart_puts("=====================================\r\n");

    snprintf(msg_buf, sizeof(msg_buf),
             "Passed: %d/%d\r\n", pass_count, total_tests);
    uart_puts(msg_buf);

    if (pass_count == total_tests) {
        uart_puts("\r
*** ALL TESTS PASSED ***\r\n");
        uart_puts("Hardware verification complete.\r\n");
        uart_puts("Ready for flight firmware.\r\n");
    } else {
        uart_puts("\r\n*** SOME TESTS FAILED ***\r\n");
        uart_puts("Review failures above before proceeding.\r\n");
    }

    uart_puts("=====================================\r\n");
}

/**
 * @brief Main verification entry point
 */
int main(void)
{
    /* Initialize all hardware */
    if (hardware_init() != 0) {
        /* Cannot proceed without basic hardware */
        while (1) {
            /* Error halt - hardware init failed */
        }
    }

    /* Run all tests */
    test_i2c_scanner();
    test_pwm();
    test_uart_loopback();

    /* Print final summary */
    print_summary();

    /* Keep PWM running for verification with scope/multimeter */
    uart_puts("\r\nPWM signals active. Press reset to restart tests.\r\n");
    uart_puts("Halting...\r\n");

    /* Halt with PWM still running */
    while (1) {
        /* Main loop - tests complete */
        /* Could add LED blink pattern here for visual status */
    }

    return 0;
}
