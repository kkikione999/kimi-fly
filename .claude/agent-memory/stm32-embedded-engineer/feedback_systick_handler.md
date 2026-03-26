---
name: SysTick_Handler required in user code for PlatformIO stm32cube
description: PlatformIO + stm32cube framework does NOT provide SysTick_Handler — omitting it causes MCU lockup after HAL_Init()
type: feedback
---

When using PlatformIO with the stm32cube framework, you MUST provide `SysTick_Handler` in your own source files.

The startup file (`startup_stm32f411xe.s`) defines `SysTick_Handler` as a weak alias to `Default_Handler` (an infinite loop). If `HAL_Init()` is called (which configures and enables SysTick for 1ms interrupts), the MCU will immediately enter `SysTick_Handler` which loops forever — preventing any application code from running.

**Why:** This caused the STM32 to be completely silent on the debug UART despite firmware being successfully flashed. Discovered via OpenOCD register dump showing MCU stuck at `Default_Handler` address in SysTick mode.

**How to apply:** Always include this in `main.c` or `stm32f4xx_it.c`:
```c
void SysTick_Handler(void)
{
    HAL_IncTick();
}
```

This is NOT needed when CubeMX generates code (it creates `stm32f4xx_it.c` with this handler). But for hand-written firmware using PlatformIO + stm32cube, it must be explicit.
