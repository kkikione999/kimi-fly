---
name: PWR voltage scaling required before PLL configuration on STM32F4
description: HAL_RCC_OscConfig with PLL requires PWR clock enable and voltage scaling to Scale1 first
type: feedback
---

Before calling `HAL_RCC_OscConfig()` to configure PLL on STM32F4xx, you MUST:
1. Enable PWR clock: `__HAL_RCC_PWR_CLK_ENABLE()`
2. Set voltage scaling: `__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1)`

Without these, `HAL_RCC_ClockConfig()` to switch to PLL clock may fail silently or produce incorrect behavior.

**Why:** Discovered when comparing working reference firmware (`/Users/ll/fly/zmgjb/code/411/`) to hand-written firmware. The reference always calls these two lines before oscillator configuration.

**How to apply:** Use this template for system_clock_config():
```c
static void system_clock_config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    // ... oscillator and PLL config ...
}
```
