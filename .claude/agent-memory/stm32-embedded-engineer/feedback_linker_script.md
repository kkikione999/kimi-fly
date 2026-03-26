---
name: linker_script_init_array
description: STM32 custom linker scripts must include .init_array and .fini_array sections to prevent HardFault in __libc_init_array
type: feedback
---

Always include `.init_array`, `.fini_array`, and `.preinit_array` sections in STM32 linker scripts, plus KEEP `.init` and `.fini`. Without them, `__libc_init_array` (called by startup_stm32f4xx.s) iterates garbage function pointers and causes a HardFault (CFSR=UNDEFINSTR).

Required additions to .text section:
```
KEEP(*(.init))
KEEP(*(.fini))
__preinit_array_start = .;
KEEP(*(.preinit_array*))
__preinit_array_end = .;
__init_array_start = .;
KEEP(*(SORT(.init_array.*)))
KEEP(*(.init_array*))
__init_array_end = .;
__fini_array_start = .;
KEEP(*(SORT(.fini_array.*)))
KEEP(*(.fini_array*))
__fini_array_end = .;
```

**Why:** Discovered via OpenOCD fault analysis - CFSR=0x00010000 (UNDEFINSTR), stacked PC pointed beyond firmware end.

**How to apply:** Every time writing a custom linker script for STM32 with GCC startup files.
