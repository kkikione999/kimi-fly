---
name: nano_specs_float_printf
description: nano.specs requires -u _printf_float linker flag to enable float formatting in snprintf/printf
type: feedback
---

When using `--specs=nano.specs` (newlib-nano), float formatting in `printf`/`snprintf` is disabled by default to save code size. Adding `%f` format specifiers without the float support produces empty strings silently.

Fix: add `-u _printf_float` to linker flags.

**Why:** Discovered in Task 203 - magnetometer and barometer float values printed as empty strings without any error.

**How to apply:** Whenever using `snprintf` with `%f`, `%g`, `%e` in embedded STM32 projects using nano.specs. Adds ~10KB to firmware size.
