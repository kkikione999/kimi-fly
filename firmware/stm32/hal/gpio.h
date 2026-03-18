/**
 * @file gpio.h
 * @brief STM32 GPIO HAL接口定义
 *
 * @note 本文件为Ralph-loop v2.0 HAL层基础文件
 */

#ifndef GPIO_H
#define GPIO_H

#include "hal_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * GPIO 端口定义 (STM32F411CEU6)
 * ============================================================================ */

typedef enum {
    GPIO_PORT_A = 0,
    GPIO_PORT_B = 1,
    GPIO_PORT_C = 2,
    GPIO_PORT_D = 3,
    GPIO_PORT_E = 4,
    GPIO_PORT_H = 5
} gpio_port_t;

/* ============================================================================
 * GPIO 引脚定义
 * ============================================================================ */

typedef enum {
    GPIO_PIN_0  = 0x0001U,
    GPIO_PIN_1  = 0x0002U,
    GPIO_PIN_2  = 0x0004U,
    GPIO_PIN_3  = 0x0008U,
    GPIO_PIN_4  = 0x0010U,
    GPIO_PIN_5  = 0x0020U,
    GPIO_PIN_6  = 0x0040U,
    GPIO_PIN_7  = 0x0080U,
    GPIO_PIN_8  = 0x0100U,
    GPIO_PIN_9  = 0x0200U,
    GPIO_PIN_10 = 0x0400U,
    GPIO_PIN_11 = 0x0800U,
    GPIO_PIN_12 = 0x1000U,
    GPIO_PIN_13 = 0x2000U,
    GPIO_PIN_14 = 0x4000U,
    GPIO_PIN_15 = 0x8000U,
    GPIO_PIN_ALL = 0xFFFFU
} gpio_pin_t;

/* ============================================================================
 * GPIO 模式定义
 * ============================================================================ */

typedef enum {
    GPIO_MODE_INPUT     = 0x00U,  /**< 输入模式 */
    GPIO_MODE_OUTPUT    = 0x01U,  /**< 输出模式 */
    GPIO_MODE_AF        = 0x02U,  /**< 复用功能模式 */
    GPIO_MODE_ANALOG    = 0x03U   /**< 模拟模式 */
} gpio_mode_t;

/* ============================================================================
 * GPIO 输出类型定义
 * ============================================================================ */

typedef enum {
    GPIO_OTYPE_PP = 0x00U,  /**< 推挽输出 */
    GPIO_OTYPE_OD = 0x01U   /**< 开漏输出 */
} gpio_otype_t;

/* ============================================================================
 * GPIO 速度定义
 * ============================================================================ */

typedef enum {
    GPIO_SPEED_LOW    = 0x00U,  /**< 低速 2MHz */
    GPIO_SPEED_MEDIUM = 0x01U,  /**< 中速 25MHz */
    GPIO_SPEED_FAST   = 0x02U,  /**< 快速 50MHz */
    GPIO_SPEED_HIGH   = 0x03U   /**< 高速 100MHz */
} gpio_speed_t;

/* ============================================================================
 * GPIO 上下拉定义
 * ============================================================================ */

typedef enum {
    GPIO_PUPD_NONE = 0x00U,  /**< 无上下拉 */
    GPIO_PUPD_UP   = 0x01U,  /**< 上拉 */
    GPIO_PUPD_DOWN = 0x02U   /**< 下拉 */
} gpio_pupd_t;

/* ============================================================================
 * GPIO 复用功能定义 (AF0-AF15)
 * ============================================================================ */

typedef enum {
    GPIO_AF_0  = 0x00U,   /**< AF0: SYS_AF */
    GPIO_AF_1  = 0x01U,   /**< AF1: TIM1/TIM2 */
    GPIO_AF_2  = 0x02U,   /**< AF2: TIM3/TIM4/TIM5 */
    GPIO_AF_3  = 0x03U,   /**< AF3: TIM9/TIM10/TIM11 */
    GPIO_AF_4  = 0x04U,   /**< AF4: I2C1/I2C2/I2C3 */
    GPIO_AF_5  = 0x05U,   /**< AF5: SPI1/SPI2/SPI3/SPI4/SPI5 */
    GPIO_AF_6  = 0x06U,   /**< AF6: SPI3 */
    GPIO_AF_7  = 0x07U,   /**< AF7: USART1/USART2 */
    GPIO_AF_8  = 0x08U,   /**< AF8: USART6 */
    GPIO_AF_9  = 0x09U,   /**< AF9: I2C2/I2C3 */
    GPIO_AF_10 = 0x0AU,   /**< AF10: OTG_FS */
    GPIO_AF_11 = 0x0BU,   /**< AF11: Reserved */
    GPIO_AF_12 = 0x0CU,   /**< AF12: SDIO */
    GPIO_AF_13 = 0x0DU,   /**< AF13: Reserved */
    GPIO_AF_14 = 0x0EU,   /**< AF14: Reserved */
    GPIO_AF_15 = 0x0FU    /**< AF15: EVENTOUT */
} gpio_af_t;

/* ============================================================================
 * GPIO 配置结构体
 * ============================================================================ */

typedef struct {
    gpio_mode_t   mode;      /**< GPIO模式 */
    gpio_otype_t  otype;     /**< 输出类型 */
    gpio_speed_t  speed;     /**< 速度 */
    gpio_pupd_t   pupd;      /**< 上下拉配置 */
    gpio_af_t     af;        /**< 复用功能 (仅在AF模式下有效) */
} gpio_config_t;

/* ============================================================================
 * GPIO 句柄结构体
 * ============================================================================ */

typedef struct {
    gpio_port_t   port;      /**< GPIO端口 */
    uint16_t      pin_mask;  /**< 引脚掩码 (可多位) */
} gpio_handle_t;

/* ============================================================================
 * GPIO API 声明
 * ============================================================================ */

/**
 * @brief 初始化GPIO
 * @param gpio GPIO句柄
 * @param config GPIO配置
 * @return HAL状态
 */
hal_status_t gpio_init(gpio_handle_t *gpio, const gpio_config_t *config);

/**
 * @brief 反初始化GPIO
 * @param gpio GPIO句柄
 * @return HAL状态
 */
hal_status_t gpio_deinit(gpio_handle_t *gpio);

/**
 * @brief 设置GPIO输出电平
 * @param gpio GPIO句柄
 * @param value 电平值 (0=低, 1=高)
 * @return HAL状态
 */
hal_status_t gpio_write(gpio_handle_t *gpio, uint8_t value);

/**
 * @brief 读取GPIO输入电平
 * @param gpio GPIO句柄
 * @param value 输出电平值
 * @return HAL状态
 */
hal_status_t gpio_read(gpio_handle_t *gpio, uint8_t *value);

/**
 * @brief 切换GPIO输出电平
 * @param gpio GPIO句柄
 * @return HAL状态
 */
hal_status_t gpio_toggle(gpio_handle_t *gpio);

/**
 * @brief 使能GPIO端口时钟
 * @param port GPIO端口
 * @return HAL状态
 */
hal_status_t gpio_clk_enable(gpio_port_t port);

/**
 * @brief 禁用GPIO端口时钟
 * @param port GPIO端口
 * @return HAL状态
 */
hal_status_t gpio_clk_disable(gpio_port_t port);

#ifdef __cplusplus
}
#endif

#endif /* GPIO_H */
