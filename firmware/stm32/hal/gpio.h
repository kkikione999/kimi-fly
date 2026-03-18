/**
 * @file gpio.h
 * @brief STM32 GPIO HAL接口定义
 *
 * @note 本文件为Ralph-loop v2.0 HAL层基础文件
 *       当USE_HAL_DRIVER定义时，直接映射到STM32Cube HAL
 */

#ifndef GPIO_H
#define GPIO_H

#include "hal_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * GPIO 端口定义
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
 * GPIO 引脚定义 - 条件定义避免冲突
 * ============================================================================ */

#ifndef GPIO_PIN_0
#define GPIO_PIN_0                 ((uint16_t)0x0001)
#define GPIO_PIN_1                 ((uint16_t)0x0002)
#define GPIO_PIN_2                 ((uint16_t)0x0004)
#define GPIO_PIN_3                 ((uint16_t)0x0008)
#define GPIO_PIN_4                 ((uint16_t)0x0010)
#define GPIO_PIN_5                 ((uint16_t)0x0020)
#define GPIO_PIN_6                 ((uint16_t)0x0040)
#define GPIO_PIN_7                 ((uint16_t)0x0080)
#define GPIO_PIN_8                 ((uint16_t)0x0100)
#define GPIO_PIN_9                 ((uint16_t)0x0200)
#define GPIO_PIN_10                ((uint16_t)0x0400)
#define GPIO_PIN_11                ((uint16_t)0x0800)
#define GPIO_PIN_12                ((uint16_t)0x1000)
#define GPIO_PIN_13                ((uint16_t)0x2000)
#define GPIO_PIN_14                ((uint16_t)0x4000)
#define GPIO_PIN_15                ((uint16_t)0x8000)
#define GPIO_PIN_ALL               ((uint16_t)0xFFFF)
#endif

/* ============================================================================
 * GPIO 模式定义 - 条件定义避免冲突
 * ============================================================================ */

#ifndef GPIO_MODE_INPUT
#define GPIO_MODE_INPUT                 (0x00000000U)
#define GPIO_MODE_OUTPUT_PP             (0x00000001U)
#define GPIO_MODE_OUTPUT_OD             (0x00000011U)
#define GPIO_MODE_AF_PP                 (0x00000002U)
#define GPIO_MODE_AF_OD                 (0x00000012U)
#define GPIO_MODE_ANALOG                (0x00000003U)
#endif

typedef enum {
    KF_GPIO_MODE_INPUT = 0,
    KF_GPIO_MODE_OUTPUT = 1,
    KF_GPIO_MODE_AF = 2,
    KF_GPIO_MODE_ANALOG = 3
} gpio_mode_t;

/* ============================================================================
 * GPIO 速度定义 - 条件定义避免冲突
 * ============================================================================ */

#ifndef GPIO_SPEED_FREQ_LOW
#define GPIO_SPEED_FREQ_LOW         (0x00000000U)
#define GPIO_SPEED_FREQ_MEDIUM      (0x00000001U)
#define GPIO_SPEED_FREQ_HIGH        (0x00000002U)
#define GPIO_SPEED_FREQ_VERY_HIGH   (0x00000003U)
#endif

typedef enum {
    KF_GPIO_SPEED_LOW = 0,
    KF_GPIO_SPEED_MEDIUM = 1,
    KF_GPIO_SPEED_FAST = 2,
    KF_GPIO_SPEED_HIGH = 3
} gpio_speed_t;

#define GPIO_SPEED_LOW      GPIO_SPEED_FREQ_LOW
#define GPIO_SPEED_MEDIUM   GPIO_SPEED_FREQ_MEDIUM
#define GPIO_SPEED_FAST     GPIO_SPEED_FREQ_HIGH
#define GPIO_SPEED_HIGH     GPIO_SPEED_FREQ_VERY_HIGH

/* ============================================================================
 * GPIO 上下拉定义 - 条件定义避免冲突
 * ============================================================================ */

#ifndef GPIO_NOPULL
#define GPIO_NOPULL                 (0x00000000U)
#define GPIO_PULLUP                 (0x00000001U)
#define GPIO_PULLDOWN               (0x00000002U)
#endif

typedef enum {
    KF_GPIO_PUPD_NONE = 0,
    KF_GPIO_PUPD_UP = 1,
    KF_GPIO_PUPD_DOWN = 2
} gpio_pupd_t;

#define GPIO_PUPD_NONE  KF_GPIO_PUPD_NONE
#define GPIO_PUPD_UP    KF_GPIO_PUPD_UP
#define GPIO_PUPD_DOWN  KF_GPIO_PUPD_DOWN

/* ============================================================================
 * GPIO 复用功能定义 - 条件定义避免冲突
 * ============================================================================ */

#ifndef GPIO_AF0
#define GPIO_AF0                     ((uint8_t)0x00)
#define GPIO_AF1                     ((uint8_t)0x01)
#define GPIO_AF2                     ((uint8_t)0x02)
#define GPIO_AF3                     ((uint8_t)0x03)
#define GPIO_AF4                     ((uint8_t)0x04)
#define GPIO_AF5                     ((uint8_t)0x05)
#define GPIO_AF6                     ((uint8_t)0x06)
#define GPIO_AF7                     ((uint8_t)0x07)
#define GPIO_AF8                     ((uint8_t)0x08)
#define GPIO_AF9                     ((uint8_t)0x09)
#define GPIO_AF10                    ((uint8_t)0x0A)
#define GPIO_AF11                    ((uint8_t)0x0B)
#define GPIO_AF12                    ((uint8_t)0x0C)
#define GPIO_AF13                    ((uint8_t)0x0D)
#define GPIO_AF14                    ((uint8_t)0x0E)
#define GPIO_AF15                    ((uint8_t)0x0F)
#endif

typedef enum {
    KF_GPIO_AF_0 = 0,
    KF_GPIO_AF_1 = 1,
    KF_GPIO_AF_2 = 2,
    KF_GPIO_AF_3 = 3,
    KF_GPIO_AF_4 = 4,
    KF_GPIO_AF_5 = 5,
    KF_GPIO_AF_6 = 6,
    KF_GPIO_AF_7 = 7,
    KF_GPIO_AF_8 = 8,
    KF_GPIO_AF_9 = 9,
    KF_GPIO_AF_10 = 10,
    KF_GPIO_AF_11 = 11,
    KF_GPIO_AF_12 = 12,
    KF_GPIO_AF_13 = 13,
    KF_GPIO_AF_14 = 14,
    KF_GPIO_AF_15 = 15
} gpio_af_t;

#define GPIO_AF_0   KF_GPIO_AF_0
#define GPIO_AF_1   KF_GPIO_AF_1
#define GPIO_AF_2   KF_GPIO_AF_2
#define GPIO_AF_3   KF_GPIO_AF_3
#define GPIO_AF_4   KF_GPIO_AF_4
#define GPIO_AF_5   KF_GPIO_AF_5
#define GPIO_AF_6   KF_GPIO_AF_6
#define GPIO_AF_7   KF_GPIO_AF_7
#define GPIO_AF_8   KF_GPIO_AF_8
#define GPIO_AF_9   KF_GPIO_AF_9
#define GPIO_AF_10  KF_GPIO_AF_10
#define GPIO_AF_11  KF_GPIO_AF_11
#define GPIO_AF_12  KF_GPIO_AF_12
#define GPIO_AF_13  KF_GPIO_AF_13
#define GPIO_AF_14  KF_GPIO_AF_14
#define GPIO_AF_15  KF_GPIO_AF_15

/* ============================================================================
 * GPIO 输出类型定义
 * ============================================================================ */

typedef enum {
    KF_GPIO_OTYPE_PP = 0,
    KF_GPIO_OTYPE_OD = 1
} gpio_otype_t;

#define GPIO_OTYPE_PP   KF_GPIO_OTYPE_PP
#define GPIO_OTYPE_OD   KF_GPIO_OTYPE_OD

/* ============================================================================
 * GPIO 配置结构体
 * ============================================================================ */

typedef struct {
    gpio_mode_t   mode;
    gpio_otype_t  otype;
    gpio_speed_t  speed;
    gpio_pupd_t   pupd;
    gpio_af_t     af;
} gpio_config_t;

/* ============================================================================
 * GPIO 句柄结构体
 * ============================================================================ */

typedef struct {
    gpio_port_t   port;
    uint16_t      pin_mask;
} gpio_handle_t;

/* ============================================================================
 * GPIO API 声明
 * ============================================================================ */

hal_status_t gpio_init(gpio_handle_t *gpio, const gpio_config_t *config);
hal_status_t gpio_deinit(gpio_handle_t *gpio);
hal_status_t gpio_write(gpio_handle_t *gpio, uint8_t value);
hal_status_t gpio_read(gpio_handle_t *gpio, uint8_t *value);
hal_status_t gpio_toggle(gpio_handle_t *gpio);
hal_status_t gpio_clk_enable(gpio_port_t port);
hal_status_t gpio_clk_disable(gpio_port_t port);

#ifdef __cplusplus
}
#endif

#endif /* GPIO_H */
