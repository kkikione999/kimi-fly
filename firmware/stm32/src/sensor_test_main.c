/**
 * @file sensor_test_main.c
 * @brief STM32F411CEU6 传感器测试程序
 *
 * 注意: 本程序使用 USART1 (PA9=TX, PA10=RX) 作为调试输出
 *       ST-Link VCP 通过 PA9/PA10 连接到调试串口
 *
 * 测试三传感器:
 *   - ICM-42688-P (IMU, I2C1, 0x68)
 *   - LPS22HBTR (气压计, SPI3)
 *   - QMC5883P (磁力计, I2C1, 0x2C)
 */

#include "stm32f4xx_hal.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

/* ============================================================================
 * 硬件配置
 * ============================================================================ */

#define UART_BAUDRATE           460800U
#define I2C_CLOCK_SPEED         400000U

/* I2C 设备地址 */
/* ICM-42688-P: AD0 pin pulled HIGH on this hardware -> addr 0x69 */
#define ICM42688_ADDR           0x69U
#define QMC5883P_ADDR           0x2CU

/* ICM-42688-P 寄存器 */
#define ICM42688_REG_WHO_AM_I       0x75U
#define ICM42688_REG_PWR_MGMT0      0x4EU
#define ICM42688_REG_GYRO_CONFIG0   0x4FU
#define ICM42688_REG_ACCEL_CONFIG0  0x50U
#define ICM42688_REG_TEMP_DATA1     0x1DU
#define ICM42688_WHO_AM_I_VALUE     0x47U

/* LPS22HBTR 寄存器 */
#define LPS22HB_REG_WHO_AM_I        0x0FU
#define LPS22HB_REG_CTRL_REG1       0x10U
#define LPS22HB_REG_CTRL_REG2       0x11U
#define LPS22HB_REG_PRESS_OUT_XL    0x28U
#define LPS22HB_REG_TEMP_OUT_L      0x2BU
#define LPS22HB_WHO_AM_I_VALUE      0xB1U
#define LPS22HB_SPI_READ_BIT        0x80U

/* QMC5883P 寄存器 */
#define QMC5883P_REG_XOUT_L         0x00U
#define QMC5883P_REG_CTRL1          0x0BU
#define QMC5883P_REG_CTRL2          0x0CU

/* ============================================================================
 * 全局变量
 * ============================================================================ */

UART_HandleTypeDef huart1;
I2C_HandleTypeDef hi2c1;
SPI_HandleTypeDef hspi3;

static char g_tx_buffer[256];

/* ============================================================================
 * 函数声明
 * ============================================================================ */

static void SystemClock_Config(void);
static void GPIO_Init(void);
static void UART1_Init(void);
static void I2C1_Init(void);
static void SPI3_Init(void);

static void sensor_printf(const char *format, ...);

static HAL_StatusTypeDef I2C_Scan(uint8_t *found_addrs, uint8_t *found_count);
static HAL_StatusTypeDef I2C_ReadReg(uint16_t dev_addr, uint8_t reg, uint8_t *data);
static HAL_StatusTypeDef I2C_WriteReg(uint16_t dev_addr, uint8_t reg, uint8_t data);
static HAL_StatusTypeDef I2C_ReadRegs(uint16_t dev_addr, uint8_t reg, uint8_t *data, uint16_t len);
static HAL_StatusTypeDef SPI_ReadReg(uint8_t reg, uint8_t *data);
static HAL_StatusTypeDef SPI_WriteReg(uint8_t reg, uint8_t data);
static HAL_StatusTypeDef SPI_ReadRegs(uint8_t reg, uint8_t *data, uint16_t len);

static HAL_StatusTypeDef ICM42688_Init(void);
static HAL_StatusTypeDef ICM42688_ReadData(float *ax, float *ay, float *az,
                                            float *gx, float *gy, float *gz,
                                            float *temp);
static HAL_StatusTypeDef LPS22HB_Init(void);
static HAL_StatusTypeDef LPS22HB_ReadData(float *pressure, float *temp);
static HAL_StatusTypeDef QMC5883P_Init(void);
static HAL_StatusTypeDef QMC5883P_ReadData(float *mx, float *my, float *mz);

/* ============================================================================
 * 基础函数
 * ============================================================================ */

static void sensor_printf(const char *format, ...)
{
    va_list args;
    int len;
    va_start(args, format);
    len = vsnprintf(g_tx_buffer, sizeof(g_tx_buffer), format, args);
    va_end(args);
    if (len > 0 && len < (int)sizeof(g_tx_buffer)) {
        HAL_UART_Transmit(&huart1, (uint8_t *)g_tx_buffer, len, 200);
    }
}

/* ============================================================================
 * I2C 辅助函数
 * ============================================================================ */

static HAL_StatusTypeDef I2C_Scan(uint8_t *found_addrs, uint8_t *found_count)
{
    *found_count = 0;
    for (uint16_t addr = 0x08; addr <= 0x77; addr++) {
        if (HAL_I2C_IsDeviceReady(&hi2c1, addr << 1, 1, 100) == HAL_OK) {
            if (*found_count < 16) {
                found_addrs[*found_count] = (uint8_t)addr;
            }
            (*found_count)++;
        }
    }
    return HAL_OK;
}

static HAL_StatusTypeDef I2C_ReadReg(uint16_t dev_addr, uint8_t reg, uint8_t *data)
{
    return HAL_I2C_Mem_Read(&hi2c1, dev_addr << 1, reg, I2C_MEMADD_SIZE_8BIT, data, 1, 100);
}

static HAL_StatusTypeDef I2C_WriteReg(uint16_t dev_addr, uint8_t reg, uint8_t data)
{
    return HAL_I2C_Mem_Write(&hi2c1, dev_addr << 1, reg, I2C_MEMADD_SIZE_8BIT, &data, 1, 100);
}

static HAL_StatusTypeDef I2C_ReadRegs(uint16_t dev_addr, uint8_t reg, uint8_t *data, uint16_t len)
{
    return HAL_I2C_Mem_Read(&hi2c1, dev_addr << 1, reg, I2C_MEMADD_SIZE_8BIT, data, len, 100);
}

/* ============================================================================
 * SPI 辅助函数
 * ============================================================================ */

static HAL_StatusTypeDef SPI_ReadReg(uint8_t reg, uint8_t *data)
{
    uint8_t tx[2] = {reg | LPS22HB_SPI_READ_BIT, 0xFF};
    uint8_t rx[2];
    HAL_StatusTypeDef st;
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_RESET);
    st = HAL_SPI_TransmitReceive(&hspi3, tx, rx, 2, 100);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_SET);
    *data = rx[1];
    return st;
}

static HAL_StatusTypeDef SPI_WriteReg(uint8_t reg, uint8_t data)
{
    uint8_t tx[2] = {reg, data};
    HAL_StatusTypeDef st;
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_RESET);
    st = HAL_SPI_Transmit(&hspi3, tx, 2, 100);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_SET);
    return st;
}

static HAL_StatusTypeDef SPI_ReadRegs(uint8_t reg, uint8_t *data, uint16_t len)
{
    uint8_t cmd = reg | LPS22HB_SPI_READ_BIT;
    HAL_StatusTypeDef st;
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_RESET);
    st = HAL_SPI_Transmit(&hspi3, &cmd, 1, 100);
    if (st == HAL_OK) {
        st = HAL_SPI_Receive(&hspi3, data, len, 100);
    }
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_SET);
    return st;
}

/* ============================================================================
 * ICM-42688-P 驱动
 * ============================================================================ */

static HAL_StatusTypeDef ICM42688_Init(void)
{
    uint8_t id;
    HAL_StatusTypeDef st;

    st = I2C_ReadReg(ICM42688_ADDR, ICM42688_REG_WHO_AM_I, &id);
    if (st != HAL_OK) {
        sensor_printf("[ERROR] ICM42688 I2C failed st=%d\r\n", st);
        return HAL_ERROR;
    }
    if (id != ICM42688_WHO_AM_I_VALUE) {
        sensor_printf("[ERROR] ICM42688 WHO_AM_I=0x%02X (expect 0x47)\r\n", id);
        return HAL_ERROR;
    }

    st = I2C_WriteReg(ICM42688_ADDR, ICM42688_REG_PWR_MGMT0, 0x0F);
    if (st != HAL_OK) return st;
    HAL_Delay(10);

    st = I2C_WriteReg(ICM42688_ADDR, ICM42688_REG_GYRO_CONFIG0, 0x06);
    if (st != HAL_OK) return st;

    st = I2C_WriteReg(ICM42688_ADDR, ICM42688_REG_ACCEL_CONFIG0, 0x06);
    if (st != HAL_OK) return st;
    HAL_Delay(10);

    return HAL_OK;
}

static HAL_StatusTypeDef ICM42688_ReadData(float *ax, float *ay, float *az,
                                            float *gx, float *gy, float *gz,
                                            float *temp)
{
    uint8_t data[14];
    int16_t raw_t, raw_ax, raw_ay, raw_az, raw_gx, raw_gy, raw_gz;
    HAL_StatusTypeDef st;

    st = I2C_ReadRegs(ICM42688_ADDR, ICM42688_REG_TEMP_DATA1, data, 14);
    if (st != HAL_OK) return st;

    raw_t  = (int16_t)((data[0]  << 8) | data[1]);
    raw_ax = (int16_t)((data[2]  << 8) | data[3]);
    raw_ay = (int16_t)((data[4]  << 8) | data[5]);
    raw_az = (int16_t)((data[6]  << 8) | data[7]);
    raw_gx = (int16_t)((data[8]  << 8) | data[9]);
    raw_gy = (int16_t)((data[10] << 8) | data[11]);
    raw_gz = (int16_t)((data[12] << 8) | data[13]);

    *temp = (raw_t / 132.48f) + 25.0f;
    *ax = raw_ax * 0.00048828125f;
    *ay = raw_ay * 0.00048828125f;
    *az = raw_az * 0.00048828125f;
    *gx = raw_gx * 0.06103515625f;
    *gy = raw_gy * 0.06103515625f;
    *gz = raw_gz * 0.06103515625f;

    return HAL_OK;
}

/* ============================================================================
 * LPS22HBTR 驱动
 * ============================================================================ */

static HAL_StatusTypeDef LPS22HB_Init(void)
{
    uint8_t id;
    HAL_StatusTypeDef st;

    st = SPI_ReadReg(LPS22HB_REG_WHO_AM_I, &id);
    if (st != HAL_OK) {
        sensor_printf("[ERROR] LPS22HB SPI failed st=%d\r\n", st);
        return HAL_ERROR;
    }
    /* Accept 0xB1 (LPS22HB) and 0xB3 (LPS22HH - compatible variant) */
    if (id != LPS22HB_WHO_AM_I_VALUE && id != 0xB3U) {
        sensor_printf("[ERROR] LPS22HB WHO_AM_I=0x%02X (expect 0xB1 or 0xB3)\r\n", id);
        return HAL_ERROR;
    }
    sensor_printf("[INFO] Baro WHO_AM_I=0x%02X (%s)\r\n", id,
                  id == 0xB3U ? "LPS22HH" : "LPS22HB");

    st = SPI_WriteReg(LPS22HB_REG_CTRL_REG2, 0x04);  /* sw reset */
    if (st != HAL_OK) return st;
    HAL_Delay(10);

    st = SPI_WriteReg(LPS22HB_REG_CTRL_REG1, 0x34);  /* 25Hz ODR, BDU */
    if (st != HAL_OK) return st;
    HAL_Delay(50);

    return HAL_OK;
}

static HAL_StatusTypeDef LPS22HB_ReadData(float *pressure, float *temp)
{
    uint8_t data[5];
    uint8_t status_reg;
    int32_t raw_p;
    int16_t raw_t;
    HAL_StatusTypeDef st;
    uint8_t retries = 0;

    /* Wait for data ready: STATUS reg bit 0=P_DA, bit 1=T_DA */
    do {
        st = SPI_ReadReg(0x27, &status_reg);  /* STATUS register */
        if (st != HAL_OK) return st;
        if (++retries > 100) break;
        HAL_Delay(5);
    } while ((status_reg & 0x01) == 0);

    st = SPI_ReadRegs(LPS22HB_REG_PRESS_OUT_XL, data, 5);
    if (st != HAL_OK) return st;

    raw_p = ((int32_t)data[2] << 16) | ((int32_t)data[1] << 8) | data[0];
    if (raw_p & 0x00800000) raw_p |= (int32_t)0xFF000000;
    raw_t = (int16_t)((data[4] << 8) | data[3]);

    *pressure = raw_p / 4096.0f;
    *temp = raw_t / 100.0f;

    return HAL_OK;
}

/* ============================================================================
 * QMC5883P 驱动
 * ============================================================================ */

static HAL_StatusTypeDef QMC5883P_Init(void)
{
    HAL_StatusTypeDef st;

    st = I2C_WriteReg(QMC5883P_ADDR, QMC5883P_REG_CTRL2, 0x80);  /* soft reset */
    if (st != HAL_OK) return st;
    HAL_Delay(10);

    st = I2C_WriteReg(QMC5883P_ADDR, QMC5883P_REG_CTRL1, 0x79);  /* continuous, 100Hz, 2G, OSR512 */
    if (st != HAL_OK) return st;
    HAL_Delay(20);

    return HAL_OK;
}

static HAL_StatusTypeDef QMC5883P_ReadData(float *mx, float *my, float *mz)
{
    uint8_t data[6];
    uint8_t status;
    int16_t raw_x, raw_y, raw_z;
    HAL_StatusTypeDef st;
    uint8_t retries = 0;

    /* Wait for DRDY: STATUS[0]=DRDY */
    do {
        st = I2C_ReadReg(QMC5883P_ADDR, 0x06, &status);  /* STATUS reg */
        if (st != HAL_OK) return st;
        if (++retries > 50) break;
        HAL_Delay(5);
    } while ((status & 0x01) == 0);

    st = I2C_ReadRegs(QMC5883P_ADDR, QMC5883P_REG_XOUT_L, data, 6);
    if (st != HAL_OK) return st;

    raw_x = (int16_t)((data[1] << 8) | data[0]);
    raw_y = (int16_t)((data[3] << 8) | data[2]);
    raw_z = (int16_t)((data[5] << 8) | data[4]);

    /* CTRL1=0x79 sets RNG=11 (8G), sensitivity=3000 LSB/G */
    *mx = raw_x / 3000.0f;
    *my = raw_y / 3000.0f;
    *mz = raw_z / 3000.0f;

    return HAL_OK;
}

/* ============================================================================
 * 外设初始化
 * ============================================================================ */

static void GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* PA15 - SPI3 CS */
    GPIO_InitStruct.Pin = GPIO_PIN_15;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_SET);

    /* PB14 - LED */
    GPIO_InitStruct.Pin = GPIO_PIN_14;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_RESET);
}

static void UART1_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_USART1_CLK_ENABLE();

    /* PA9 (TX), PA10 (RX) */
    GPIO_InitStruct.Pin = GPIO_PIN_9 | GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    huart1.Instance = USART1;
    huart1.Init.BaudRate = UART_BAUDRATE;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart1);
}

static void I2C1_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_I2C1_CLK_ENABLE();

    GPIO_InitStruct.Pin = GPIO_PIN_6 | GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    hi2c1.Instance = I2C1;
    hi2c1.Init.ClockSpeed = I2C_CLOCK_SPEED;
    hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
    hi2c1.Init.OwnAddress1 = 0;
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.OwnAddress2 = 0;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    HAL_I2C_Init(&hi2c1);
}

static void SPI3_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_SPI3_CLK_ENABLE();

    GPIO_InitStruct.Pin = GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF6_SPI3;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    hspi3.Instance = SPI3;
    hspi3.Init.Mode = SPI_MODE_MASTER;
    hspi3.Init.Direction = SPI_DIRECTION_2LINES;
    hspi3.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi3.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi3.Init.CLKPhase = SPI_PHASE_1EDGE;
    hspi3.Init.NSS = SPI_NSS_SOFT;
    hspi3.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
    hspi3.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi3.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi3.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hspi3.Init.CRCPolynomial = 10;
    HAL_SPI_Init(&hspi3);
}

static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    /* Enable PWR clock and set voltage scaling to Scale1 for PLL */
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    /* Use HSI (16MHz internal oscillator) - no external crystal dependency
     * 16MHz HSI / PLLM(16) * PLLN(200) / PLLP(2) = 100MHz SYSCLK */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
    RCC_OscInitStruct.PLL.PLLM = 16;   /* 16MHz / 16 = 1MHz VCO input */
    RCC_OscInitStruct.PLL.PLLN = 200;  /* 1MHz * 200 = 200MHz VCO output */
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;  /* 200MHz / 2 = 100MHz SYSCLK */
    RCC_OscInitStruct.PLL.PLLQ = 4;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        /* PLL failed - stay on HSI at 16MHz, peripherals will still work */
        return;
    }

    /* SYSCLK = 100MHz, HCLK = 100MHz, APB1 = 50MHz, APB2 = 100MHz
     * FLASH_LATENCY_3 required for HCLK > 90MHz @ 2.7-3.6V */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                  RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK) {
        /* Clock switch failed - remain on HSI */
        return;
    }
}

/* ============================================================================
 * 主函数
 * ============================================================================ */

int main(void)
{
    uint8_t found_addrs[16];
    uint8_t found_count = 0;
    HAL_StatusTypeDef status;
    uint8_t imu_ok = 0, baro_ok = 0, mag_ok = 0;

    HAL_Init();
    SystemClock_Config();
    GPIO_Init();
    UART1_Init();
    I2C1_Init();
    SPI3_Init();

    /* 等待 2 秒让串口监视器就绪 */
    HAL_Delay(2000);

    sensor_printf("\r\n");
    sensor_printf("========================================\r\n");
    sensor_printf("  SENSOR TEST v1.0 (STM32F411)\r\n");
    sensor_printf("========================================\r\n\r\n");

    /* --- Test 1: I2C Bus Scan --- */
    sensor_printf("--- Test 1: I2C Bus Scan ---\r\n");
    I2C_Scan(found_addrs, &found_count);
    sensor_printf("Found %d I2C device(s):\r\n", found_count);
    for (uint8_t i = 0; i < found_count && i < 16; i++) {
        sensor_printf("  [FOUND] 0x%02X", found_addrs[i]);
        if (found_addrs[i] == ICM42688_ADDR) sensor_printf(" (ICM-42688-P)");
        if (found_addrs[i] == QMC5883P_ADDR) sensor_printf(" (QMC5883P)");
        sensor_printf("\r\n");
    }

    uint8_t icm_found = 0, qmc_found = 0;
    for (uint8_t i = 0; i < found_count; i++) {
        if (found_addrs[i] == ICM42688_ADDR) icm_found = 1;
        if (found_addrs[i] == QMC5883P_ADDR) qmc_found = 1;
    }
    sensor_printf("I2C: ICM=%s, QMC=%s\r\n\r\n",
                  icm_found ? "OK" : "MISS",
                  qmc_found ? "OK" : "MISS");

    /* --- Test 2: ICM-42688-P IMU --- */
    sensor_printf("--- Test 2: ICM-42688-P IMU ---\r\n");
    status = ICM42688_Init();
    if (status == HAL_OK) {
        sensor_printf("[PASS] WHO_AM_I=0x47\r\n");
        sensor_printf("#    AccX(g)   AccY(g)   AccZ(g)   GyrX      GyrY      GyrZ      T(C)\r\n");

        float ax, ay, az, gx, gy, gz, temp;
        for (uint8_t i = 0; i < 10; i++) {
            status = ICM42688_ReadData(&ax, &ay, &az, &gx, &gy, &gz, &temp);
            if (status == HAL_OK) {
                char line[128];
                snprintf(line, sizeof(line), "%-4d %9.3f %9.3f %9.3f %9.2f %9.2f %9.2f %5.1f\r\n",
                         i + 1, ax, ay, az, gx, gy, gz, temp);
                HAL_UART_Transmit(&huart1, (uint8_t *)line, strlen(line), 200);
            } else {
                sensor_printf("[ERR] Read failed\r\n");
            }
            HAL_Delay(100);
        }
        sensor_printf("[PASS] IMU done\r\n\r\n");
        imu_ok = 1;
    } else {
        sensor_printf("[FAIL] IMU init failed\r\n\r\n");
    }

    /* --- Test 3: LPS22HBTR Barometer --- */
    sensor_printf("--- Test 3: LPS22HBTR Barometer ---\r\n");
    status = LPS22HB_Init();
    if (status == HAL_OK) {
        sensor_printf("[PASS] WHO_AM_I=0xB1\r\n");
        sensor_printf("#    Pressure(hPa)  Temp(C)\r\n");

        float pressure, temp;
        for (uint8_t i = 0; i < 10; i++) {
            status = LPS22HB_ReadData(&pressure, &temp);
            if (status == HAL_OK) {
                char line[64];
                snprintf(line, sizeof(line), "%-4d %13.3f %8.2f\r\n", i + 1, pressure, temp);
                HAL_UART_Transmit(&huart1, (uint8_t *)line, strlen(line), 200);
            } else {
                sensor_printf("[ERR] Read failed\r\n");
            }
            HAL_Delay(50);
        }
        sensor_printf("[PASS] Barometer done\r\n\r\n");
        baro_ok = 1;
    } else {
        sensor_printf("[FAIL] Barometer init failed\r\n\r\n");
    }

    /* --- Test 4: QMC5883P Magnetometer --- */
    sensor_printf("--- Test 4: QMC5883P Magnetometer ---\r\n");
    status = QMC5883P_Init();
    if (status == HAL_OK) {
        sensor_printf("[PASS] Magnetometer initialized\r\n");
        sensor_printf("#    MagX(G)   MagY(G)   MagZ(G)\r\n");

        float mx, my, mz;
        for (uint8_t i = 0; i < 10; i++) {
            status = QMC5883P_ReadData(&mx, &my, &mz);
            if (status == HAL_OK) {
                char line[64];
                snprintf(line, sizeof(line), "%-4d %9.3f %9.3f %9.3f\r\n", i + 1, mx, my, mz);
                HAL_UART_Transmit(&huart1, (uint8_t *)line, strlen(line), 200);
            } else {
                sensor_printf("[ERR] Read failed\r\n");
            }
            HAL_Delay(100);
        }
        sensor_printf("[PASS] Magnetometer done\r\n\r\n");
        mag_ok = 1;
    } else {
        sensor_printf("[FAIL] Magnetometer init failed\r\n\r\n");
    }

    /* --- Summary --- */
    sensor_printf("========================================\r\n");
    sensor_printf("  TEST REPORT\r\n");
    sensor_printf("========================================\r\n");
    sensor_printf("I2C Scan:    %d devices\r\n", found_count);
    sensor_printf("ICM-42688-P: %s\r\n", imu_ok ? "PASS" : "FAIL");
    sensor_printf("LPS22HBTR:   %s\r\n", baro_ok ? "PASS" : "FAIL");
    sensor_printf("QMC5883P:    %s\r\n", mag_ok ? "PASS" : "FAIL");
    sensor_printf("ALL DONE.\r\n");

    while (1) {
        HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_14);
        HAL_Delay(500);
    }
}

void SysTick_Handler(void)
{
    HAL_IncTick();
}

void Error_Handler(void)
{
    sensor_printf("[ERROR] Error_Handler\r\n");
    while (1) {
        HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_14);
        HAL_Delay(100);
    }
}
