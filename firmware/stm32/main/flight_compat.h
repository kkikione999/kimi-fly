/**
 * @file flight_compat.h
 * @brief Compatibility shim for [env:flight] build
 *
 * Force-included (-include) before every translation unit so that:
 *  1. CMSIS device header is pulled in once (include guards prevent re-entry).
 *  2. TIM1-TIM4 pointer macros are undefined AFTER CMSIS defines them, allowing
 *     the custom tim_t enum in pwm.h to declare integer values TIM1=0..TIM4=3.
 *  3. TIM1_BASE-TIM4_BASE are undefined so pwm.c can redefine them as integers.
 *  4. AFRL/AFRH aliases are provided for gpio.c which still uses those names.
 *
 * @note This file must be force-included via -include in build_flags.
 *       Do NOT include it from regular source files.
 */

#ifndef FLIGHT_COMPAT_H
#define FLIGHT_COMPAT_H

/* Pull in the CMSIS device header through the STM32Cube HAL chain.
 * The include guard in stm32f411xe.h ensures this runs at most once per
 * translation unit, so subsequent #include "stm32f4xx_hal.h" in user
 * sources are no-ops and cannot re-introduce the conflicting macros. */
#include "stm32f4xx_hal.h"

/* ------------------------------------------------------------------
 * Fix 1: TIM1-TIM4 pointer-cast macros clash with the custom tim_t
 * enum in hal/pwm.h which needs integer constants (0-3) for switch/case.
 * Undefine them here; the enum in pwm.h will re-introduce them as ints.
 * ------------------------------------------------------------------ */
#ifdef TIM1
#undef TIM1
#endif
#ifdef TIM2
#undef TIM2
#endif
#ifdef TIM3
#undef TIM3
#endif
#ifdef TIM4
#undef TIM4
#endif

/* Also undefine the BASE address macros that pwm.c redefines locally. */
#ifdef TIM1_BASE
#undef TIM1_BASE
#endif
#ifdef TIM2_BASE
#undef TIM2_BASE
#endif
#ifdef TIM3_BASE
#undef TIM3_BASE
#endif
#ifdef TIM4_BASE
#undef TIM4_BASE
#endif

/* ------------------------------------------------------------------
 * Fix 2: CMSIS defines GPIO_TypeDef with AFR[2] (an array).
 * gpio.c defines the same struct with separate AFRL/AFRH members.
 * Set a guard so gpio.c can skip its own typedef and instead add
 * AFRL/AFRH field-name aliases for the CMSIS AFR[] array.
 * gpio.c checks FLIGHT_COMPAT_HAVE_GPIO_TYPEDEF and adds the aliases.
 * ------------------------------------------------------------------ */
#define FLIGHT_COMPAT_HAVE_GPIO_TYPEDEF   1

/* ------------------------------------------------------------------
 * Fix 3: I2C_OAR1_ADD7_1_Pos / _Msk are used in i2c.c but defined
 * only in the bare-metal #else block (excluded when USE_HAL_DRIVER
 * is set). Add them here using the correct bit positions.
 * ------------------------------------------------------------------ */
#ifndef I2C_OAR1_ADD7_1_Pos
#define I2C_OAR1_ADD7_1_Pos   1U
#define I2C_OAR1_ADD7_1_Msk   (0x7FU << I2C_OAR1_ADD7_1_Pos)
#endif

/* ------------------------------------------------------------------
 * Fix 4: i2c.c bare-metal section defines GPIOB_* and RCC_A*ENR
 * macros that CMSIS exposes differently (as struct pointer members).
 * Re-expose them as direct register accessors that i2c.c expects.
 * These are defined in i2c.c's #else block but are referenced OUTSIDE
 * that block in static helper functions (i2c_gpio_init / i2c_clk_*).
 * We provide them here so they are visible regardless of USE_HAL_DRIVER.
 * ------------------------------------------------------------------ */
#ifndef GPIOB_BASE
#define GPIOB_BASE   0x40020400U
#endif

#ifndef GPIOB_MODER
#define GPIOB_MODER    (*(volatile uint32_t *)(GPIOB_BASE + 0x00U))
#define GPIOB_OTYPER   (*(volatile uint32_t *)(GPIOB_BASE + 0x04U))
#define GPIOB_OSPEEDR  (*(volatile uint32_t *)(GPIOB_BASE + 0x08U))
#define GPIOB_PUPDR    (*(volatile uint32_t *)(GPIOB_BASE + 0x0CU))
#define GPIOB_AFRL     (*(volatile uint32_t *)(GPIOB_BASE + 0x20U))
#endif

#ifndef RCC_BASE
#define RCC_BASE   0x40023800U
#endif

#ifndef RCC_AHB1ENR
#define RCC_AHB1ENR   (*(volatile uint32_t *)(RCC_BASE + 0x30U))
#endif
#ifndef RCC_APB1ENR
#define RCC_APB1ENR   (*(volatile uint32_t *)(RCC_BASE + 0x40U))
#endif
#ifndef RCC_APB2ENR
#define RCC_APB2ENR   (*(volatile uint32_t *)(RCC_BASE + 0x44U))
#endif

#ifndef RCC_AHB1ENR_GPIOBEN
#define RCC_AHB1ENR_GPIOBEN  (1U << 1)
#endif
#ifndef RCC_APB1ENR_I2C1EN
#define RCC_APB1ENR_I2C1EN   (1U << 21)
#endif

#endif /* FLIGHT_COMPAT_H */
