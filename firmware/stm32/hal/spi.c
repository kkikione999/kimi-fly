#include "spi.h"

/* RCC register definitions */
#define RCC_BASE            0x40023800
#define RCC_APB1ENR         (*(volatile uint32_t *)(RCC_BASE + 0x40))
#define RCC_AHB1ENR         (*(volatile uint32_t *)(RCC_BASE + 0x30))

/* RCC bits */
#define RCC_APB1ENR_SPI3EN  (1U << 15)
#define RCC_AHB1ENR_GPIOAEN (1U << 0)
#define RCC_AHB1ENR_GPIOBEN (1U << 1)
#define RCC_AHB1ENR_GPIOCEN (1U << 2)

/* SPI3 base address */
#define SPI3_BASE           0x40003C00

/* SPI register bit definitions */
/* CR1 bits */
#define SPI_CR1_CPHA        (1U << 0)
#define SPI_CR1_CPOL        (1U << 1)
#define SPI_CR1_MSTR        (1U << 2)
#define SPI_CR1_BR_Pos      3
#define SPI_CR1_BR_Msk      (0x7U << SPI_CR1_BR_Pos)
#define SPI_CR1_SPE         (1U << 6)
#define SPI_CR1_LSBFIRST    (1U << 7)
#define SPI_CR1_SSI         (1U << 8)
#define SPI_CR1_SSM         (1U << 9)
#define SPI_CR1_RXONLY      (1U << 10)
#define SPI_CR1_DFF         (1U << 11)
#define SPI_CR1_CRCNEXT     (1U << 12)
#define SPI_CR1_CRCEN       (1U << 13)
#define SPI_CR1_BIDIOE      (1U << 14)
#define SPI_CR1_BIDIMODE    (1U << 15)

/* CR2 bits */
#define SPI_CR2_RXDMAEN     (1U << 0)
#define SPI_CR2_TXDMAEN     (1U << 1)
#define SPI_CR2_SSOE        (1U << 2)
#define SPI_CR2_FRF         (1U << 4)
#define SPI_CR2_ERRIE       (1U << 5)
#define SPI_CR2_RXNEIE      (1U << 6)
#define SPI_CR2_TXEIE       (1U << 7)

/* SR bits */
#define SPI_SR_RXNE         (1U << 0)
#define SPI_SR_TXE          (1U << 1)
#define SPI_SR_CHSIDE       (1U << 2)
#define SPI_SR_UDR          (1U << 3)
#define SPI_SR_CRCERR       (1U << 4)
#define SPI_SR_MODF         (1U << 5)
#define SPI_SR_OVR          (1U << 6)
#define SPI_SR_BSY          (1U << 7)
#define SPI_SR_FRE          (1U << 8)

/* Default timeout in ms */
#define SPI_DEFAULT_TIMEOUT 100

/* SPI3 GPIO pins (Alternate Function 6) */
/* PC10 - SPI3_SCK, PC11 - SPI3_MISO, PC12 - SPI3_MOSI */
#define SPI3_SCK_PORT       GPIO_PORT_C
#define SPI3_SCK_PIN        GPIO_PIN_10
#define SPI3_MISO_PORT      GPIO_PORT_C
#define SPI3_MISO_PIN       GPIO_PIN_11
#define SPI3_MOSI_PORT      GPIO_PORT_C
#define SPI3_MOSI_PIN       GPIO_PIN_12

/* NSS pin - software controlled (PA15) */
#define SPI3_NSS_PORT       GPIO_PORT_A
#define SPI3_NSS_PIN        GPIO_PIN_15

/* Private function prototypes */
static void spi_enable_clock(spi_periph_t periph);
static hal_status_t spi_configure_gpio(spi_periph_t periph);
static hal_status_t spi_wait_flag(spi_handle_t *hspi, uint32_t flag, uint8_t set, uint32_t timeout_ms);

/**
 * @brief Enable SPI peripheral clock
 */
static void spi_enable_clock(spi_periph_t periph)
{
    if (periph == SPI_PERIPH_3) {
        RCC_APB1ENR |= RCC_APB1ENR_SPI3EN;
    }
}

/**
 * @brief Configure GPIO pins for SPI3
 * PC10 - SCK (AF6), PC11 - MISO (AF6), PC12 - MOSI (AF6)
 * PA15 - NSS (software controlled, output)
 */
static hal_status_t spi_configure_gpio(spi_periph_t periph)
{
    gpio_init_t gpio_init;
    hal_status_t status;

    if (periph != SPI_PERIPH_3) {
        return HAL_ERROR;
    }

    /* Enable GPIO clocks */
    RCC_AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC_AHB1ENR |= RCC_AHB1ENR_GPIOCEN;

    /* Configure SCK (PC10) - Alternate Function, Push-pull, High speed */
    gpio_init.port = SPI3_SCK_PORT;
    gpio_init.pin = SPI3_SCK_PIN;
    gpio_init.mode = GPIO_MODE_AF;
    gpio_init.otype = GPIO_OTYPE_PP;
    gpio_init.speed = GPIO_SPEED_HIGH;
    gpio_init.pupd = GPIO_PUPD_NONE;
    gpio_init.af = 6;  /* AF6 for SPI3 */
    status = gpio_init(&gpio_init);
    if (status != HAL_OK) {
        return status;
    }

    /* Configure MISO (PC11) - Alternate Function, Push-pull, High speed */
    gpio_init.port = SPI3_MISO_PORT;
    gpio_init.pin = SPI3_MISO_PIN;
    gpio_init.mode = GPIO_MODE_AF;
    gpio_init.otype = GPIO_OTYPE_PP;
    gpio_init.speed = GPIO_SPEED_HIGH;
    gpio_init.pupd = GPIO_PUPD_NONE;
    gpio_init.af = 6;  /* AF6 for SPI3 */
    status = gpio_init(&gpio_init);
    if (status != HAL_OK) {
        return status;
    }

    /* Configure MOSI (PC12) - Alternate Function, Push-pull, High speed */
    gpio_init.port = SPI3_MOSI_PORT;
    gpio_init.pin = SPI3_MOSI_PIN;
    gpio_init.mode = GPIO_MODE_AF;
    gpio_init.otype = GPIO_OTYPE_PP;
    gpio_init.speed = GPIO_SPEED_HIGH;
    gpio_init.pupd = GPIO_PUPD_NONE;
    gpio_init.af = 6;  /* AF6 for SPI3 */
    status = gpio_init(&gpio_init);
    if (status != HAL_OK) {
        return status;
    }

    /* Configure NSS (PA15) - Output, Push-pull, High speed (software controlled) */
    gpio_init.port = SPI3_NSS_PORT;
    gpio_init.pin = SPI3_NSS_PIN;
    gpio_init.mode = GPIO_MODE_OUTPUT;
    gpio_init.otype = GPIO_OTYPE_PP;
    gpio_init.speed = GPIO_SPEED_HIGH;
    gpio_init.pupd = GPIO_PUPD_UP;  /* Pull-up for NSS */
    gpio_init.af = 0;
    status = gpio_init(&gpio_init);
    if (status != HAL_OK) {
        return status;
    }

    /* Set NSS high (inactive) */
    gpio_write(SPI3_NSS_PORT, SPI3_NSS_PIN, 1);

    return HAL_OK;
}

/**
 * @brief Wait for a flag to be set or cleared
 */
static hal_status_t spi_wait_flag(spi_handle_t *hspi, uint32_t flag, uint8_t set, uint32_t timeout_ms)
{
    spi_reg_t *spi = (spi_reg_t *)hspi->reg_base;
    uint32_t count = 0;
    uint32_t max_count = timeout_ms * 1000;  /* Rough approximation */

    if (set) {
        while (!(spi->SR & flag)) {
            if (++count > max_count) {
                return HAL_TIMEOUT;
            }
        }
    } else {
        while (spi->SR & flag) {
            if (++count > max_count) {
                return HAL_TIMEOUT;
            }
        }
    }

    return HAL_OK;
}

/**
 * @brief Initialize SPI peripheral
 */
hal_status_t spi_init(spi_handle_t *hspi, const spi_config_t *config)
{
    spi_reg_t *spi;
    uint32_t cr1 = 0;
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
    hspi->reg_base = (volatile uint32_t *)SPI3_BASE;
    hspi->nss_port = SPI3_NSS_PORT;
    hspi->nss_pin = SPI3_NSS_PIN;

    /* Enable peripheral clock */
    spi_enable_clock(hspi->periph);

    /* Configure GPIO */
    status = spi_configure_gpio(hspi->periph);
    if (status != HAL_OK) {
        return status;
    }

    spi = (spi_reg_t *)hspi->reg_base;

    /* Disable SPI before configuration */
    spi->CR1 &= ~SPI_CR1_SPE;

    /* Configure CR1 */
    /* Clock phase */
    if (config->cpha == SPI_CPHA_2EDGE) {
        cr1 |= SPI_CR1_CPHA;
    }

    /* Clock polarity */
    if (config->cpol == SPI_CPOL_HIGH) {
        cr1 |= SPI_CR1_CPOL;
    }

    /* Master mode */
    cr1 |= SPI_CR1_MSTR;

    /* Baud rate prescaler */
    cr1 |= ((uint32_t)config->baudrate_prescaler << SPI_CR1_BR_Pos) & SPI_CR1_BR_Msk;

    /* Data size */
    if (config->data_size == SPI_DATASIZE_16BIT) {
        cr1 |= SPI_CR1_DFF;
    }

    /* NSS management */
    if (config->nss_mode == SPI_NSS_SOFT) {
        cr1 |= SPI_CR1_SSM | SPI_CR1_SSI;  /* Software NSS, internal slave select high */
    }

    /* Bit order */
    if (!config->msb_first) {
        cr1 |= SPI_CR1_LSBFIRST;  /* LSB first */
    }

    /* Full duplex, 2-line unidirectional */
    /* BIDIMODE = 0 (default), RXONLY = 0 (default) */

    /* Write CR1 */
    spi->CR1 = cr1;

    /* Configure CR2 - disable interrupts and DMA */
    spi->CR2 = 0;

    /* Enable SPI */
    spi->CR1 |= SPI_CR1_SPE;

    hspi->initialized = 1;

    return HAL_OK;
}

/**
 * @brief Deinitialize SPI peripheral
 */
hal_status_t spi_deinit(spi_handle_t *hspi)
{
    spi_reg_t *spi;

    if (hspi == NULL || !hspi->initialized) {
        return HAL_ERROR;
    }

    spi = (spi_reg_t *)hspi->reg_base;

    /* Disable SPI */
    spi->CR1 &= ~SPI_CR1_SPE;

    hspi->initialized = 0;

    return HAL_OK;
}

/**
 * @brief Transmit data in blocking mode
 */
hal_status_t spi_transmit(spi_handle_t *hspi, const uint8_t *p_data, uint16_t size, uint32_t timeout_ms)
{
    spi_reg_t *spi;
    hal_status_t status;
    uint16_t i;

    if (hspi == NULL || p_data == NULL || size == 0 || !hspi->initialized) {
        return HAL_ERROR;
    }

    spi = (spi_reg_t *)hspi->reg_base;

    /* Wait until not busy */
    status = spi_wait_flag(hspi, SPI_SR_BSY, 0, timeout_ms);
    if (status != HAL_OK) {
        return status;
    }

    /* Transmit data */
    for (i = 0; i < size; i++) {
        /* Wait for TXE (transmit buffer empty) */
        status = spi_wait_flag(hspi, SPI_SR_TXE, 1, timeout_ms);
        if (status != HAL_OK) {
            return status;
        }

        /* Write data */
        *((volatile uint8_t *)&spi->DR) = p_data[i];

        /* Wait for RXNE (receive buffer not empty) - needed to complete transfer */
        status = spi_wait_flag(hspi, SPI_SR_RXNE, 1, timeout_ms);
        if (status != HAL_OK) {
            return status;
        }

        /* Read received data (discard) */
        (void)*((volatile uint8_t *)&spi->DR);
    }

    /* Wait until not busy */
    status = spi_wait_flag(hspi, SPI_SR_BSY, 0, timeout_ms);
    if (status != HAL_OK) {
        return status;
    }

    return HAL_OK;
}

/**
 * @brief Receive data in blocking mode
 */
hal_status_t spi_receive(spi_handle_t *hspi, uint8_t *p_data, uint16_t size, uint32_t timeout_ms)
{
    spi_reg_t *spi;
    hal_status_t status;
    uint16_t i;

    if (hspi == NULL || p_data == NULL || size == 0 || !hspi->initialized) {
        return HAL_ERROR;
    }

    spi = (spi_reg_t *)hspi->reg_base;

    /* Wait until not busy */
    status = spi_wait_flag(hspi, SPI_SR_BSY, 0, timeout_ms);
    if (status != HAL_OK) {
        return status;
    }

    /* Receive data */
    for (i = 0; i < size; i++) {
        /* Wait for TXE */
        status = spi_wait_flag(hspi, SPI_SR_TXE, 1, timeout_ms);
        if (status != HAL_OK) {
            return status;
        }

        /* Send dummy byte to generate clock */
        *((volatile uint8_t *)&spi->DR) = 0xFF;

        /* Wait for RXNE */
        status = spi_wait_flag(hspi, SPI_SR_RXNE, 1, timeout_ms);
        if (status != HAL_OK) {
            return status;
        }

        /* Read received data */
        p_data[i] = *((volatile uint8_t *)&spi->DR);
    }

    /* Wait until not busy */
    status = spi_wait_flag(hspi, SPI_SR_BSY, 0, timeout_ms);
    if (status != HAL_OK) {
        return status;
    }

    return HAL_OK;
}

/**
 * @brief Transmit and receive data simultaneously in blocking mode (full duplex)
 */
hal_status_t spi_transmit_receive(spi_handle_t *hspi, const uint8_t *p_tx_data, uint8_t *p_rx_data, uint16_t size, uint32_t timeout_ms)
{
    spi_reg_t *spi;
    hal_status_t status;
    uint16_t i;

    if (hspi == NULL || p_tx_data == NULL || p_rx_data == NULL || size == 0 || !hspi->initialized) {
        return HAL_ERROR;
    }

    spi = (spi_reg_t *)hspi->reg_base;

    /* Wait until not busy */
    status = spi_wait_flag(hspi, SPI_SR_BSY, 0, timeout_ms);
    if (status != HAL_OK) {
        return status;
    }

    /* Transmit and receive data */
    for (i = 0; i < size; i++) {
        /* Wait for TXE */
        status = spi_wait_flag(hspi, SPI_SR_TXE, 1, timeout_ms);
        if (status != HAL_OK) {
            return status;
        }

        /* Write data */
        *((volatile uint8_t *)&spi->DR) = p_tx_data[i];

        /* Wait for RXNE */
        status = spi_wait_flag(hspi, SPI_SR_RXNE, 1, timeout_ms);
        if (status != HAL_OK) {
            return status;
        }

        /* Read received data */
        p_rx_data[i] = *((volatile uint8_t *)&spi->DR);
    }

    /* Wait until not busy */
    status = spi_wait_flag(hspi, SPI_SR_BSY, 0, timeout_ms);
    if (status != HAL_OK) {
        return status;
    }

    return HAL_OK;
}

/**
 * @brief Set NSS pin level (software control)
 * @param level: 0 = active (low), 1 = inactive (high)
 */
void spi_set_nss(spi_handle_t *hspi, uint8_t level)
{
    if (hspi == NULL || !hspi->initialized) {
        return;
    }

    gpio_write(hspi->nss_port, hspi->nss_pin, level);
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
    config.cpol = SPI_CPOL_LOW;
    config.cpha = SPI_CPHA_1EDGE;
    config.data_size = SPI_DATASIZE_8BIT;
    config.nss_mode = SPI_NSS_SOFT;
    config.baudrate_prescaler = SPI_BAUDRATEPRESCALER_8;  /* 5.25 MHz */
    config.msb_first = 1;

    return spi_init(hspi, &config);
}
