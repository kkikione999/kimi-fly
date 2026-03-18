#ifndef SPI_H
#define SPI_H

#include "hal_common.h"
#include "gpio.h"

/* SPI peripheral identifiers */
typedef enum {
    SPI_PERIPH_1 = 0,
    SPI_PERIPH_2 = 1,
    SPI_PERIPH_3 = 2,
    SPI_PERIPH_MAX
} spi_periph_t;

/* SPI clock polarity */
typedef enum {
    SPI_CPOL_LOW = 0,   /* Clock idle low */
    SPI_CPOL_HIGH = 1   /* Clock idle high */
} spi_cpol_t;

/* SPI clock phase */
typedef enum {
    SPI_CPHA_1EDGE = 0, /* Data captured on first edge */
    SPI_CPHA_2EDGE = 1  /* Data captured on second edge */
} spi_cpha_t;

/* SPI data size */
typedef enum {
    SPI_DATASIZE_8BIT = 0,
    SPI_DATASIZE_16BIT = 1
} spi_datasize_t;

/* SPI NSS management */
typedef enum {
    SPI_NSS_SOFT = 0,   /* Software NSS management */
    SPI_NSS_HARD = 1    /* Hardware NSS management */
} spi_nss_mode_t;

/* SPI baud rate prescaler (based on APB1 clock 42MHz) */
typedef enum {
    SPI_BAUDRATEPRESCALER_2 = 0,    /* 21 MHz */
    SPI_BAUDRATEPRESCALER_4 = 1,    /* 10.5 MHz */
    SPI_BAUDRATEPRESCALER_8 = 2,    /* 5.25 MHz */
    SPI_BAUDRATEPRESCALER_16 = 3,   /* 2.625 MHz */
    SPI_BAUDRATEPRESCALER_32 = 4,   /* 1.3125 MHz */
    SPI_BAUDRATEPRESCALER_64 = 5,   /* 656.25 kHz */
    SPI_BAUDRATEPRESCALER_128 = 6,  /* 328.125 kHz */
    SPI_BAUDRATEPRESCALER_256 = 7   /* 164.0625 kHz */
} spi_baudrate_prescaler_t;

/* SPI configuration structure */
typedef struct {
    spi_cpol_t cpol;                        /* Clock polarity */
    spi_cpha_t cpha;                        /* Clock phase */
    spi_datasize_t data_size;               /* Data size (8/16 bit) */
    spi_nss_mode_t nss_mode;                /* NSS management mode */
    spi_baudrate_prescaler_t baudrate_prescaler; /* Baud rate prescaler */
    uint8_t msb_first;                      /* 1 = MSB first, 0 = LSB first */
} spi_config_t;

/* SPI handle structure */
typedef struct {
    spi_periph_t periph;                    /* SPI peripheral */
    spi_config_t config;                    /* Configuration */
    volatile uint32_t *reg_base;            /* Register base address */
    gpio_port_t nss_port;                   /* NSS GPIO port (for software NSS) */
    gpio_pin_t nss_pin;                     /* NSS GPIO pin (for software NSS) */
    uint8_t initialized;                    /* Initialization flag */
} spi_handle_t;

/* SPI register structure */
typedef struct {
    volatile uint32_t CR1;      /* 0x00 - Control register 1 */
    volatile uint32_t CR2;      /* 0x04 - Control register 2 */
    volatile uint32_t SR;       /* 0x08 - Status register */
    volatile uint32_t DR;       /* 0x0C - Data register */
    volatile uint32_t CRCPR;    /* 0x10 - CRC polynomial register */
    volatile uint32_t RXCRCR;   /* 0x14 - RX CRC register */
    volatile uint32_t TXCRCR;   /* 0x18 - TX CRC register */
    volatile uint32_t I2SCFGR;  /* 0x1C - I2S configuration register */
    volatile uint32_t I2SPR;    /* 0x20 - I2S prescaler register */
} spi_reg_t;

/* Function declarations */
hal_status_t spi_init(spi_handle_t *hspi, const spi_config_t *config);
hal_status_t spi_deinit(spi_handle_t *hspi);
hal_status_t spi_transmit(spi_handle_t *hspi, const uint8_t *p_data, uint16_t size, uint32_t timeout_ms);
hal_status_t spi_receive(spi_handle_t *hspi, uint8_t *p_data, uint16_t size, uint32_t timeout_ms);
hal_status_t spi_transmit_receive(spi_handle_t *hspi, const uint8_t *p_tx_data, uint8_t *p_rx_data, uint16_t size, uint32_t timeout_ms);
void spi_set_nss(spi_handle_t *hspi, uint8_t level);

/* Convenience function for SPI3 IMU initialization */
hal_status_t spi3_init_for_imu(spi_handle_t *hspi);

#endif /* SPI_H */
