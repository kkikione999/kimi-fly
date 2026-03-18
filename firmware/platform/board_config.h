/**
 * @file board_config.h
 * @brief Board configuration for kimi-fly drone
 * @note Hardware: STM32F411CEU6 + ESP32-C3 WiFi module
 */

#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * MCU Configuration
 * ============================================================================ */

#define MCU_TYPE                STM32F411CEU6
#define MCU_CORE                Cortex_M4
#define MCU_FREQ_HZ             100000000U  /* 100 MHz */
#define MCU_FLASH_SIZE          512000U     /* 512 KB */
#define MCU_RAM_SIZE            128000U     /* 128 KB */

/* ============================================================================
 * Clock Configuration
 * ============================================================================ */

#define HSE_FREQ_HZ             8000000U    /* 8 MHz external crystal */
#define HSI_FREQ_HZ             16000000U   /* 16 MHz internal RC */
#define SYSCLK_FREQ_HZ          100000000U  /* 100 MHz system clock */
#define APB1_FREQ_HZ            42000000U   /* 42 MHz APB1 clock */
#define APB2_FREQ_HZ            84000000U   /* 84 MHz APB2 clock */
#define AHB_FREQ_HZ             100000000U  /* 100 MHz AHB clock */

/* ============================================================================
 * UART Configuration (ESP32-C3 WiFi)
 * ============================================================================ */

#define WIFI_UART_INSTANCE      UART_INSTANCE_2
#define WIFI_UART_BAUDRATE      115200U
#define WIFI_UART_GPIO_TX       GPIO_PIN_PA2
#define WIFI_UART_GPIO_RX       GPIO_PIN_PA3
#define WIFI_UART_AF            GPIO_AF7_USART2

#define DEBUG_UART_INSTANCE     UART_INSTANCE_1
#define DEBUG_UART_BAUDRATE     921600U

/* WiFi Network Configuration */
#define WIFI_SSID               "whc"
#define WIFI_PASSWORD           "12345678"
#define WIFI_MODE_STA           1

/* ============================================================================
 * I2C Configuration (Sensors)
 * ============================================================================ */

#define SENSOR_I2C_BUS          I2C_BUS_1
#define SENSOR_I2C_SPEED        I2C_SPEED_FAST  /* 400 kHz */
#define SENSOR_I2C_GPIO_SCL     GPIO_PIN_PB6
#define SENSOR_I2C_GPIO_SDA     GPIO_PIN_PB7
#define SENSOR_I2C_AF           GPIO_AF4_I2C1

/* I2C Device Addresses */
#define LPS22HBTR_ADDR          0x5C    /* Barometer (SA0=0) */
#define LPS22HBTR_ADDR_ALT      0x5D    /* Barometer (SA0=1) */
#define QMC5883P_ADDR           0x0D    /* Magnetometer */

/* ============================================================================
 * SPI Configuration (IMU)
 * ============================================================================ */

#define IMU_SPI_BUS             SPI_BUS_3
#define IMU_SPI_MODE            SPI_MODE_0      /* CPOL=0, CPHA=0 */
#define IMU_SPI_FREQ_HZ         5250000U        /* 5.25 MHz (8MHz max for IMU) */
#define IMU_SPI_GPIO_SCK        GPIO_PIN_PC10
#define IMU_SPI_GPIO_MISO       GPIO_PIN_PC11
#define IMU_SPI_GPIO_MOSI       GPIO_PIN_PC12
#define IMU_SPI_GPIO_NSS        GPIO_PIN_PA15
#define IMU_SPI_AF              GPIO_AF6_SPI3

/* IMU Configuration */
#define IMU_TYPE                ICM42688P
#define IMU_SPI_MODE_REQ        0               /* Mode 0 required by ICM-42688-P */
#define IMU_ACCEL_RANGE         16              /* +/- 16g */
#define IMU_GYRO_RANGE          2000            /* +/- 2000 dps */

/* ============================================================================
 * PWM Configuration (Motors)
 * ============================================================================ */

#define MOTOR_PWM_FREQ_HZ       400U            /* 400 Hz motor PWM */
#define MOTOR_PWM_MIN_DUTY      0.05f           /* 5% min duty */
#define MOTOR_PWM_MAX_DUTY      0.95f           /* 95% max duty */
#define MOTOR_PWM_RESOLUTION    1000U           /* 0.1% resolution */

#define MOTOR1_PWM_CHANNEL      PWM_CHANNEL_1
#define MOTOR1_GPIO             GPIO_PIN_PA0
#define MOTOR2_PWM_CHANNEL      PWM_CHANNEL_2
#define MOTOR2_GPIO             GPIO_PIN_PA1
#define MOTOR3_PWM_CHANNEL      PWM_CHANNEL_3
#define MOTOR3_GPIO             GPIO_PIN_PA2
#define MOTOR4_PWM_CHANNEL      PWM_CHANNEL_4
#define MOTOR4_GPIO             GPIO_PIN_PA3

/* Note: PA2/PA3 shared with UART - need to reassign if using both */
#define MOTOR_PWM_TIM           TIM2

/* Alternative motor pins if PA2/PA3 used for UART */
#define MOTOR1_GPIO_ALT         GPIO_PIN_PB0
#define MOTOR2_GPIO_ALT         GPIO_PIN_PB1
#define MOTOR3_GPIO_ALT         GPIO_PIN_PB6
#define MOTOR4_GPIO_ALT         GPIO_PIN_PB7

/* ============================================================================
 * GPIO Configuration (LEDs & Misc)
 * ============================================================================ */

#define LED_STATUS_GPIO         GPIO_PIN_PC13   /* Status LED */
#define LED_ERROR_GPIO          GPIO_PIN_PC14   /* Error LED */

#define BUTTON_USER_GPIO        GPIO_PIN_PA0    /* User button */

/* ============================================================================
 * ADC Configuration (Battery Monitoring)
 * ============================================================================ */

#define BATTERY_ADC_CHANNEL     ADC_CHANNEL_0
#define BATTERY_ADC_GPIO        GPIO_PIN_PA0
#define BATTERY_VOLTAGE_DIV     11.0f           /* 1:11 voltage divider */
#define BATTERY_ADC_VREF        3.3f
#define BATTERY_ADC_RESOLUTION  4096            /* 12-bit ADC */

/* ============================================================================
 * System Configuration
 * ============================================================================ */

#define TICK_RATE_HZ            1000U           /* 1ms tick */
#define SYSTICK_FREQ_HZ         1000U

#define MAIN_LOOP_FREQ_HZ       1000U           /* 1kHz main loop */
#define IMU_SAMPLE_RATE_HZ      1000U           /* 1kHz IMU sampling */
#define CONTROL_LOOP_RATE_HZ    400U            /* 400Hz control loop */

/* ============================================================================
 * Debug Configuration
 * ============================================================================ */

#define DEBUG_ENABLED           1
#define DEBUG_UART_BAUDRATE     921600U
#define DEBUG_BUFFER_SIZE       256U

/* ============================================================================
 * Safety Limits
 * ============================================================================ */

#define MAX_THROTTLE_PERCENT    95.0f
#define MAX_ANGLE_DEGREES       45.0f
#define MAX_ANGULAR_RATE_DPS    360.0f

#ifdef __cplusplus
}
#endif

#endif /* BOARD_CONFIG_H */
