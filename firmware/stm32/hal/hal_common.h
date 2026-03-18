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

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * HAL 状态定义
 * ============================================================================ */

typedef enum {
    HAL_OK       = 0x00U,    /**< 操作成功 */
    HAL_ERROR    = 0x01U,    /**< 操作错误 */
    HAL_BUSY     = 0x02U,    /**< 外设忙 */
    HAL_TIMEOUT  = 0x03U     /**< 操作超时 */
} hal_status_t;

/* ============================================================================
 * 布尔值定义 (兼容STM32标准库)
 * ============================================================================ */

#ifndef ENABLE
#define ENABLE  1
#endif

#ifndef DISABLE
#define DISABLE 0
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
 * 时钟频率定义 (STM32F411CEU6)
 * ============================================================================ */

#define HAL_SYSCLK_FREQ     96000000U   /**< 系统时钟 96MHz */
#define HAL_APB1_CLK_FREQ   42000000U   /**< APB1时钟 42MHz (I2C1时钟源) */
#define HAL_APB2_CLK_FREQ   96000000U   /**< APB2时钟 96MHz */

#ifdef __cplusplus
}
#endif

#endif /* HAL_COMMON_H */
