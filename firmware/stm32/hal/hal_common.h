/**
 * @file hal_common.h
 * @brief STM32 HAL通用定义和错误码
 *
 * @note 本文件为Ralph-loop v2.0 HAL层基础文件
 */

#ifndef HAL_COMMON_H
#define HAL_COMMON_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* STM32Cube HAL支持 - 当USE_HAL_DRIVER定义时包含HAL头文件 */
#ifdef USE_HAL_DRIVER
    #include "stm32f4xx_hal.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * HAL 状态定义
 * ============================================================================ */

/* USE_HAL_DRIVER由PlatformIO在使用STM32Cube框架时定义
 * 裸机模式下没有定义此宏
 */
#ifdef USE_HAL_DRIVER
    /* 使用STM32Cube HAL的HAL_StatusTypeDef作为hal_status_t的别名
     * 注意：STM32Cube HAL的头文件需要在包含此文件之前被包含
     */
    typedef HAL_StatusTypeDef hal_status_t;
#else
    /* 自定义HAL状态定义（裸机模式） */
    typedef enum {
        HAL_OK       = 0x00U,    /**< 操作成功 */
        HAL_ERROR    = 0x01U,    /**< 操作错误 */
        HAL_BUSY     = 0x02U,    /**< 外设忙 */
        HAL_TIMEOUT  = 0x03U     /**< 操作超时 */
    } hal_status_t;
#endif

/* ============================================================================
 * HAL 设备状态定义 (用于外设句柄)
 * ============================================================================ */
typedef enum {
    HAL_STATE_RESET     = 0x00U,    /**< 外设未初始化或已被反初始化 */
    HAL_STATE_READY     = 0x01U,    /**< 外设已初始化并准备就绪 */
    HAL_STATE_BUSY      = 0x02U,    /**< 外设正忙（处理中） */
    HAL_STATE_ERROR     = 0x04U     /**< 外设错误状态 */
} hal_device_state_t;

/* ============================================================================
 * 布尔值定义 (兼容STM32标准库)
 * ============================================================================ */

/* USE_HAL_DRIVER由PlatformIO在使用STM32Cube框架时定义
 * 裸机模式下定义ENABLE/DISABLE，STM32Cube模式下使用stm32f4xx.h中的枚举定义
 */
#ifndef USE_HAL_DRIVER
    /* 裸机模式 - 定义ENABLE/DISABLE */
    #ifndef ENABLE
        #define ENABLE  1
    #endif
    #ifndef DISABLE
        #define DISABLE 0
    #endif
#endif

/* ============================================================================
 * 超时定义
 * ============================================================================ */

#define HAL_MAX_DELAY      0xFFFFFFFFU   /**< 无限等待 */
#define HAL_DEFAULT_TIMEOUT 100U         /**< 默认超时100ms */

/* ============================================================================
 * 位操作宏
 * ============================================================================ */

#define SET_BIT(REG, BIT)     ((REG) |= (BIT))
#define CLEAR_BIT(REG, BIT)   ((REG) &= ~(BIT))
#define READ_BIT(REG, BIT)    ((REG) & (BIT))
#define CLEAR_REG(REG)        ((REG) = (0x0))
#define WRITE_REG(REG, VAL)   ((REG) = (VAL))
#define READ_REG(REG)         ((REG))
#define MODIFY_REG(REG, CLEARMASK, SETMASK)  WRITE_REG((REG), (((READ_REG(REG)) & (~(CLEARMASK))) | (SETMASK)))

/* ============================================================================
 * 中断使能/禁用宏 (用于临界区保护)
 * ============================================================================ */

#define HAL_DISABLE_INTERRUPTS()  __disable_irq()
#define HAL_ENABLE_INTERRUPTS()   __enable_irq()

/* ============================================================================
 * 断言宏
 * ============================================================================ */

#ifdef HAL_USE_ASSERT
    #define HAL_ASSERT(expr)  do { if (!(expr)) { while(1); } } while(0)
#else
    #define HAL_ASSERT(expr)  ((void)0U)
#endif

/* ============================================================================
 * 工具宏
 * ============================================================================ */

#define HAL_ARRAY_SIZE(arr)  (sizeof(arr) / sizeof((arr)[0]))

#define HAL_MIN(a, b)  (((a) < (b)) ? (a) : (b))
#define HAL_MAX(a, b)  (((a) > (b)) ? (a) : (b))

/* ============================================================================
 * 工具宏
 * ============================================================================ */

#define UNUSED(x) ((void)(x))   /*!< 消除未使用变量警告 */

/* ============================================================================
 * 时钟频率定义 (STM32F411CEU6)
 * @note STM32F411 APB1 max = 42MHz, APB2 max = 100MHz
 *       Current config: SYSCLK=84MHz, APB1=42MHz, APB2=84MHz
 * ============================================================================ */

#define HAL_SYSCLK_FREQ     84000000U   /**< 系统时钟 84MHz */
#define HAL_APB1_CLK_FREQ   42000000U   /**< APB1时钟 42MHz (USART2/I2C1, max for F411) */
#define HAL_APB2_CLK_FREQ   84000000U   /**< APB2时钟 84MHz (USART1) */

#ifdef __cplusplus
}
#endif

#endif /* HAL_COMMON_H */
