#ifndef SPI_H
#define SPI_H

#include "hal_common.h"
#include "gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * SPI 定义 (兼容STM32Cube HAL)
 * ============================================================================ */

typedef enum {
    SPI_PERIPH_1 = 0,
    SPI_PERIPH_2 = 1,
    SPI_PERIPH_3 = 2,
    SPI_PERIPH_MAX
} spi_periph_t;

#ifndef USE_HAL_DRIVER
/* 裸机模式 - 自定义定义 */

typedef enum {
    KF_SPI_CPOL_LOW = 0,
    KF_SPI_CPOL_HIGH = 1
} spi_cpol_t;

typedef enum {
    KF_SPI_CPHA_1EDGE = 0,
    KF_SPI_CPHA_2EDGE = 1
} spi_cpha_t;

typedef enum {
    KF_SPI_DATASIZE_8BIT = 0,
    KF_SPI_DATASIZE_16BIT = 1
} spi_datasize_t;

typedef enum {
    KF_SPI_NSS_SOFT = 0,
    KF_SPI_NSS_HARD = 1
} spi_nss_mode_t;

typedef enum {
    KF_SPI_BAUDRATEPRESCALER_2 = 0,
    KF_SPI_BAUDRATEPRESCALER_4 = 1,
    KF_SPI_BAUDRATEPRESCALER_8 = 2,
    KF_SPI_BAUDRATEPRESCALER_16 = 3,
    KF_SPI_BAUDRATEPRESCALER_32 = 4,
    KF_SPI_BAUDRATEPRESCALER_64 = 5,
    KF_SPI_BAUDRATEPRESCALER_128 = 6,
    KF_SPI_BAUDRATEPRESCALER_256 = 7
} spi_baudrate_prescaler_t;

#else
/* STM32Cube HAL模式 - 使用HAL定义 */

typedef uint32_t spi_cpol_t;
#define KF_SPI_CPOL_LOW     SPI_POLARITY_LOW
#define KF_SPI_CPOL_HIGH    SPI_POLARITY_HIGH

typedef uint32_t spi_cpha_t;
#define KF_SPI_CPHA_1EDGE   SPI_PHASE_1EDGE
#define KF_SPI_CPHA_2EDGE   SPI_PHASE_2EDGE

typedef uint32_t spi_datasize_t;
#define KF_SPI_DATASIZE_8BIT    SPI_DATASIZE_8BIT
#define KF_SPI_DATASIZE_16BIT   SPI_DATASIZE_16BIT

typedef uint32_t spi_nss_mode_t;
#define KF_SPI_NSS_SOFT     SPI_NSS_SOFT
#define KF_SPI_NSS_HARD     SPI_NSS_HARD_OUTPUT

typedef uint32_t spi_baudrate_prescaler_t;
#ifndef SPI_BAUDRATEPRESCALER_2
#define SPI_BAUDRATEPRESCALER_2         ((uint32_t)0x00000000U)
#define SPI_BAUDRATEPRESCALER_4         ((uint32_t)0x00000001U)
#define SPI_BAUDRATEPRESCALER_8         ((uint32_t)0x00000002U)
#define SPI_BAUDRATEPRESCALER_16        ((uint32_t)0x00000003U)
#define SPI_BAUDRATEPRESCALER_32        ((uint32_t)0x00000004U)
#define SPI_BAUDRATEPRESCALER_64        ((uint32_t)0x00000005U)
#define SPI_BAUDRATEPRESCALER_128       ((uint32_t)0x00000006U)
#define SPI_BAUDRATEPRESCALER_256       ((uint32_t)0x00000007U)
#endif

#endif /* USE_HAL_DRIVER */

/* ============================================================================
 * SPI 配置结构体
 * ============================================================================ */

typedef struct {
    spi_cpol_t cpol;
    spi_cpha_t cpha;
    spi_datasize_t data_size;
    spi_nss_mode_t nss_mode;
    spi_baudrate_prescaler_t baudrate_prescaler;
    uint8_t msb_first;
} spi_config_t;

/* ============================================================================
 * SPI 句柄结构体
 * ============================================================================ */

typedef struct {
    spi_periph_t periph;
    spi_config_t config;
    gpio_port_t nss_port;
    uint16_t nss_pin;
    uint8_t initialized;
} spi_handle_t;

/* ============================================================================
 * API函数声明
 * ============================================================================ */

hal_status_t spi_init(spi_handle_t *hspi, const spi_config_t *config);
hal_status_t spi_deinit(spi_handle_t *hspi);
hal_status_t spi_transmit(spi_handle_t *hspi, const uint8_t *p_data, uint16_t size, uint32_t timeout_ms);
hal_status_t spi_receive(spi_handle_t *hspi, uint8_t *p_data, uint16_t size, uint32_t timeout_ms);
hal_status_t spi_transmit_receive(spi_handle_t *hspi, const uint8_t *p_tx_data, uint8_t *p_rx_data, uint16_t size, uint32_t timeout_ms);
void spi_set_nss(spi_handle_t *hspi, uint8_t level);
hal_status_t spi3_init_for_imu(spi_handle_t *hspi);

/* 兼容旧代码的宏定义 - 仅在裸机模式下定义 */
#ifndef USE_HAL_DRIVER
#define SPI_CPOL_LOW    KF_SPI_CPOL_LOW
#define SPI_CPOL_HIGH   KF_SPI_CPOL_HIGH
#define SPI_CPHA_1EDGE  KF_SPI_CPHA_1EDGE
#define SPI_CPHA_2EDGE  KF_SPI_CPHA_2EDGE
#define SPI_DATASIZE_8BIT   KF_SPI_DATASIZE_8BIT
#define SPI_DATASIZE_16BIT  KF_SPI_DATASIZE_16BIT
#define SPI_NSS_SOFT    KF_SPI_NSS_SOFT
#define SPI_NSS_HARD    KF_SPI_NSS_HARD
#define SPI_BAUDRATEPRESCALER_2     KF_SPI_BAUDRATEPRESCALER_2
#define SPI_BAUDRATEPRESCALER_4     KF_SPI_BAUDRATEPRESCALER_4
#define SPI_BAUDRATEPRESCALER_8     KF_SPI_BAUDRATEPRESCALER_8
#define SPI_BAUDRATEPRESCALER_16    KF_SPI_BAUDRATEPRESCALER_16
#define SPI_BAUDRATEPRESCALER_32    KF_SPI_BAUDRATEPRESCALER_32
#define SPI_BAUDRATEPRESCALER_64    KF_SPI_BAUDRATEPRESCALER_64
#define SPI_BAUDRATEPRESCALER_128   KF_SPI_BAUDRATEPRESCALER_128
#define SPI_BAUDRATEPRESCALER_256   KF_SPI_BAUDRATEPRESCALER_256
#endif

#ifdef __cplusplus
}
#endif

#endif /* SPI_H */
