#include "spi.h"
#include "stm32f4xx_hal.h"

/* Default timeout in ms */
#define SPI_DEFAULT_TIMEOUT 100

/* SPI3 GPIO pins (Alternate Function 6) - matching reference design */
/* PB3 - SPI3_SCK, PB4 - SPI3_MISO, PB5 - SPI3_MOSI */
#define SPI3_SCK_PORT       GPIO_PORT_B
#define SPI3_SCK_PIN        GPIO_PIN_3
#define SPI3_MISO_PORT      GPIO_PORT_B
#define SPI3_MISO_PIN       GPIO_PIN_4
#define SPI3_MOSI_PORT      GPIO_PORT_B
#define SPI3_MOSI_PIN       GPIO_PIN_5

/* NSS pin - software controlled (PA4) - Gyroscope_SPI_Software_NSS */
#define SPI3_NSS_PORT       GPIO_PORT_A
#define SPI3_NSS_PIN        GPIO_PIN_4

/* HAL SPI handle for SPI3 */
static SPI_HandleTypeDef hspi3;

/* Private function prototypes */
static void spi_enable_clock(spi_periph_t periph);
static hal_status_t spi_configure_gpio(spi_periph_t periph);

/**
 * @brief Enable SPI peripheral clock
 */
static void spi_enable_clock(spi_periph_t periph)
{
    if (periph == SPI_PERIPH_3) {
        __HAL_RCC_SPI3_CLK_ENABLE();
    }
}

/**
 * @brief Configure GPIO pins for SPI3
 * PB3 - SCK (AF6), PB4 - MISO (AF6), PB5 - MOSI (AF6)
 * PA4 - NSS (software controlled, output)
 */
static hal_status_t spi_configure_gpio(spi_periph_t periph)
{
    gpio_handle_t gpio_handle;
    gpio_config_t gpio_config;
    hal_status_t status;

    if (periph != SPI_PERIPH_3) {
        return HAL_ERROR;
    }

    /* Enable GPIO clocks */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* Configure SCK (PB3) - Alternate Function, Push-pull, High speed */
    gpio_handle.port = SPI3_SCK_PORT;
    gpio_handle.pin_mask = SPI3_SCK_PIN;
    gpio_config.mode = KF_GPIO_MODE_AF;
    gpio_config.otype = GPIO_OTYPE_PP;
    gpio_config.speed = GPIO_SPEED_HIGH;
    gpio_config.pupd = GPIO_PUPD_NONE;
    gpio_config.af = GPIO_AF_6;  /* AF6 for SPI3 */
    status = gpio_init(&gpio_handle, &gpio_config);
    if (status != HAL_OK) {
        return status;
    }

    /* Configure MISO (PB4) - Alternate Function, Push-pull, High speed */
    gpio_handle.port = SPI3_MISO_PORT;
    gpio_handle.pin_mask = SPI3_MISO_PIN;
    gpio_config.mode = KF_GPIO_MODE_AF;
    gpio_config.otype = GPIO_OTYPE_PP;
    gpio_config.speed = GPIO_SPEED_HIGH;
    gpio_config.pupd = GPIO_PUPD_NONE;
    gpio_config.af = GPIO_AF_6;  /* AF6 for SPI3 */
    status = gpio_init(&gpio_handle, &gpio_config);
    if (status != HAL_OK) {
        return status;
    }

    /* Configure MOSI (PB5) - Alternate Function, Push-pull, High speed */
    gpio_handle.port = SPI3_MOSI_PORT;
    gpio_handle.pin_mask = SPI3_MOSI_PIN;
    gpio_config.mode = KF_GPIO_MODE_AF;
    gpio_config.otype = GPIO_OTYPE_PP;
    gpio_config.speed = GPIO_SPEED_HIGH;
    gpio_config.pupd = GPIO_PUPD_NONE;
    gpio_config.af = GPIO_AF_6;  /* AF6 for SPI3 */
    status = gpio_init(&gpio_handle, &gpio_config);
    if (status != HAL_OK) {
        return status;
    }

    /* Configure NSS (PA4) - Output, Push-pull, High speed (software controlled) */
    gpio_handle.port = SPI3_NSS_PORT;
    gpio_handle.pin_mask = SPI3_NSS_PIN;
    gpio_config.mode = KF_GPIO_MODE_OUTPUT;
    gpio_config.otype = GPIO_OTYPE_PP;
    gpio_config.speed = GPIO_SPEED_HIGH;
    gpio_config.pupd = GPIO_PUPD_UP;  /* Pull-up for NSS */
    gpio_config.af = GPIO_AF_0;
    status = gpio_init(&gpio_handle, &gpio_config);
    if (status != HAL_OK) {
        return status;
    }

    /* Set NSS high (inactive) */
    gpio_handle.port = SPI3_NSS_PORT;
    gpio_handle.pin_mask = SPI3_NSS_PIN;
    gpio_write(&gpio_handle, 1);

    return HAL_OK;
}

/**
 * @brief Map internal baudrate prescaler to HAL baudrate prescaler
 */
static uint32_t spi_map_baudrate_prescaler(spi_baudrate_prescaler_t prescaler)
{
    switch (prescaler) {
        case SPI_BAUDRATEPRESCALER_2:
            return SPI_BAUDRATEPRESCALER_2;
        case SPI_BAUDRATEPRESCALER_4:
            return SPI_BAUDRATEPRESCALER_4;
        case SPI_BAUDRATEPRESCALER_8:
            return SPI_BAUDRATEPRESCALER_8;
        case SPI_BAUDRATEPRESCALER_16:
            return SPI_BAUDRATEPRESCALER_16;
        case SPI_BAUDRATEPRESCALER_32:
            return SPI_BAUDRATEPRESCALER_32;
        case SPI_BAUDRATEPRESCALER_64:
            return SPI_BAUDRATEPRESCALER_64;
        case SPI_BAUDRATEPRESCALER_128:
            return SPI_BAUDRATEPRESCALER_128;
        case SPI_BAUDRATEPRESCALER_256:
            return SPI_BAUDRATEPRESCALER_256;
        default:
            return SPI_BAUDRATEPRESCALER_8;
    }
}

/**
 * @brief Initialize SPI peripheral
 */
hal_status_t spi_init(spi_handle_t *hspi, const spi_config_t *config)
{
    HAL_StatusTypeDef hal_status;
    hal_status_t status;

    if (hspi == NULL || config == NULL) {
        return HAL_ERROR;
    }

    /* Only SPI3 is supported for IMU */
    if (hspi->periph != SPI_PERIPH_3) {
        return HAL_ERROR;
    }

    /* Store configuration */
    hspi->config = *config;
    hspi->nss_port = SPI3_NSS_PORT;
    hspi->nss_pin = SPI3_NSS_PIN;

    /* Enable peripheral clock */
    spi_enable_clock(hspi->periph);

    /* Configure GPIO */
    status = spi_configure_gpio(hspi->periph);
    if (status != HAL_OK) {
        return status;
    }

    /* Configure HAL SPI handle */
    hspi3.Instance = SPI3;
    hspi3.Init.Mode = SPI_MODE_MASTER;
    hspi3.Init.Direction = SPI_DIRECTION_2LINES;
    hspi3.Init.DataSize = (config->data_size == SPI_DATASIZE_16BIT) ? SPI_DATASIZE_16BIT : SPI_DATASIZE_8BIT;
    hspi3.Init.CLKPolarity = (config->cpol == KF_SPI_CPOL_HIGH) ? SPI_POLARITY_HIGH : SPI_POLARITY_LOW;
    hspi3.Init.CLKPhase = (config->cpha == KF_SPI_CPHA_2EDGE) ? SPI_PHASE_2EDGE : SPI_PHASE_1EDGE;
    hspi3.Init.NSS = (config->nss_mode == KF_SPI_NSS_SOFT) ? SPI_NSS_SOFT : SPI_NSS_HARD_OUTPUT;
    hspi3.Init.BaudRatePrescaler = spi_map_baudrate_prescaler(config->baudrate_prescaler);
    hspi3.Init.FirstBit = (config->msb_first) ? SPI_FIRSTBIT_MSB : SPI_FIRSTBIT_LSB;
    hspi3.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi3.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hspi3.Init.CRCPolynomial = 10;

    /* Initialize SPI */
    hal_status = HAL_SPI_Init(&hspi3);
    if (hal_status != HAL_OK) {
        return HAL_ERROR;
    }

    hspi->initialized = 1;

    return HAL_OK;
}

/**
 * @brief Deinitialize SPI peripheral
 */
hal_status_t spi_deinit(spi_handle_t *hspi)
{
    HAL_StatusTypeDef hal_status;

    if (hspi == NULL || !hspi->initialized) {
        return HAL_ERROR;
    }

    /* Deinitialize SPI */
    hal_status = HAL_SPI_DeInit(&hspi3);
    if (hal_status != HAL_OK) {
        return HAL_ERROR;
    }

    hspi->initialized = 0;

    return HAL_OK;
}

/**
 * @brief Transmit data in blocking mode
 */
hal_status_t spi_transmit(spi_handle_t *hspi, const uint8_t *p_data, uint16_t size, uint32_t timeout_ms)
{
    HAL_StatusTypeDef hal_status;

    if (hspi == NULL || p_data == NULL || size == 0 || !hspi->initialized) {
        return HAL_ERROR;
    }

    hal_status = HAL_SPI_Transmit(&hspi3, (uint8_t *)p_data, size, timeout_ms);

    if (hal_status == HAL_OK) {
        return HAL_OK;
    } else if (hal_status == HAL_TIMEOUT) {
        return HAL_TIMEOUT;
    } else {
        return HAL_ERROR;
    }
}

/**
 * @brief Receive data in blocking mode
 */
hal_status_t spi_receive(spi_handle_t *hspi, uint8_t *p_data, uint16_t size, uint32_t timeout_ms)
{
    HAL_StatusTypeDef hal_status;

    if (hspi == NULL || p_data == NULL || size == 0 || !hspi->initialized) {
        return HAL_ERROR;
    }

    hal_status = HAL_SPI_Receive(&hspi3, p_data, size, timeout_ms);

    if (hal_status == HAL_OK) {
        return HAL_OK;
    } else if (hal_status == HAL_TIMEOUT) {
        return HAL_TIMEOUT;
    } else {
        return HAL_ERROR;
    }
}

/**
 * @brief Transmit and receive data simultaneously in blocking mode (full duplex)
 */
hal_status_t spi_transmit_receive(spi_handle_t *hspi, const uint8_t *p_tx_data, uint8_t *p_rx_data, uint16_t size, uint32_t timeout_ms)
{
    HAL_StatusTypeDef hal_status;

    if (hspi == NULL || p_tx_data == NULL || p_rx_data == NULL || size == 0 || !hspi->initialized) {
        return HAL_ERROR;
    }

    hal_status = HAL_SPI_TransmitReceive(&hspi3, (uint8_t *)p_tx_data, p_rx_data, size, timeout_ms);

    if (hal_status == HAL_OK) {
        return HAL_OK;
    } else if (hal_status == HAL_TIMEOUT) {
        return HAL_TIMEOUT;
    } else {
        return HAL_ERROR;
    }
}

/**
 * @brief Set NSS pin level (software control)
 * @param level: 0 = active (low), 1 = inactive (high)
 */
void spi_set_nss(spi_handle_t *hspi, uint8_t level)
{
    gpio_handle_t gpio_handle;

    if (hspi == NULL || !hspi->initialized) {
        return;
    }

    gpio_handle.port = hspi->nss_port;
    gpio_handle.pin_mask = hspi->nss_pin;
    gpio_write(&gpio_handle, level);
}

/**
 * @brief Initialize SPI3 for IMU (ICM-42688-P)
 * Mode 0 (CPOL=0, CPHA=0), 8-bit data, software NSS, 8MHz max clock
 */
hal_status_t spi3_init_for_imu(spi_handle_t *hspi)
{
    spi_config_t config;

    if (hspi == NULL) {
        return HAL_ERROR;
    }

    hspi->periph = SPI_PERIPH_3;

    /* Configure for IMU:
     * - Mode 0: CPOL=0, CPHA=0
     * - 8-bit data
     * - Software NSS
     * - 8MHz max (APB1=42MHz, DIV8=5.25MHz, DIV4=10.5MHz too fast)
     *   Use DIV8 for safety (5.25MHz)
     * - MSB first
     */
    config.cpol = KF_SPI_CPOL_LOW;
    config.cpha = KF_SPI_CPHA_1EDGE;
    config.data_size = KF_SPI_DATASIZE_8BIT;
    config.nss_mode = KF_SPI_NSS_SOFT;
    config.baudrate_prescaler = SPI_BAUDRATEPRESCALER_8;  /* 5.25 MHz */
    config.msb_first = 1;

    return spi_init(hspi, &config);
}
