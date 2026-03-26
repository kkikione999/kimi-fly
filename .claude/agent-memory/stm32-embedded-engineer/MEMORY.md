# Agent Memory Index

## Feedback

- [SysTick_Handler required in PlatformIO stm32cube](feedback_systick_handler.md) - Must add `SysTick_Handler { HAL_IncTick(); }` in user code or MCU locks up after HAL_Init()
- [PWR voltage scaling before PLL config](feedback_pwr_voltage_scaling.md) - Must call `__HAL_RCC_PWR_CLK_ENABLE()` and `__HAL_PWR_VOLTAGESCALING_CONFIG(Scale1)` before oscillator setup
- [Linker script must include init_array sections](feedback_linker_script.md) - Missing .init_array causes HardFault in __libc_init_array at startup
- [nano.specs float printf](feedback_float_printf.md) - Add -u _printf_float linker flag for %f support with nano.specs

## Project

- [Task 203 hardware findings](project_hardware_findings.md) - ICM-42688-P at 0x69 (not 0x68), LPS22HH variant (WHO_AM_I=0xB3), serial port mapping
- [Task 204 UART communication status](project_task204_uart.md) - STM32-ESP32 UART test: firmware verified running as of 2026-03-19, hardware link physical inspection pending
