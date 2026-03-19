# Task: Flight Build Result

## Status: SUCCESS

`pio run -e flight` compiles without errors.

## Build Output

```
RAM:   [          ]   0.9% (used 1212 bytes from 131072 bytes)
Flash: [          ]   4.2% (used 22060 bytes from 524288 bytes)
========================= [SUCCESS] Took 5.38 seconds =========================
```

## Root Cause Analysis

The `stm32cube` framework always injects `-DUSE_HAL_DRIVER` into every
translation unit. This caused the custom HAL layer to include
`stm32f4xx_hal.h` → `stm32f411xe.h`, which defined:

- `TIM1`-`TIM4` as pointer-cast macros, clashing with the `tim_t` enum
  in `hal/pwm.h` (which needs integer values for `switch/case`)
- `GPIO_TypeDef` as a struct with `AFR[2]`, clashing with `gpio.c`'s
  local typedef using separate `AFRL`/`AFRH` fields

## Solution

Added `-UUSE_HAL_DRIVER` to `build_flags` in `[env:flight]`. GCC
processes `-D`/`-U` flags left-to-right; placing `-UUSE_HAL_DRIVER`
after the framework's implicit `-DUSE_HAL_DRIVER` effectively undefines
it for all custom source files.

With `USE_HAL_DRIVER` unset, `hal_common.h` skips `#include
"stm32f4xx_hal.h"`, so the CMSIS device header (and its conflicting
macros) is never seen by the custom HAL layer.

## Modified Files

| File | Change |
|------|--------|
| `firmware/stm32/platformio.ini` | Added `[env:flight]` environment with `framework = stm32cube`, `-UUSE_HAL_DRIVER`, `-Ihal -Idrivers -Ialgorithm -Icomm -Imain`, `build_src_filter` pointing to flight sources via `+<../hal/*.c>` etc. |
| `firmware/stm32/main/flight_entry.c` | **Created** — `main()` entry point; defines global HAL handles (`huart2`, `hi2c1`, `hspi3`); implements `platform_*` weak-reference interface (SysTick, UART debug print, delay) |
| `firmware/stm32/main/flight_compat.h` | **Created** — Not used in final solution (superseded by `-UUSE_HAL_DRIVER`) but kept as documentation artefact |

## Existing Environment Unaffected

`pio run -e stm32f411` continues to compile and link successfully.
Flash: 1.7%, RAM: 0.5%.

## Warnings (non-fatal)

- `flight_main.c`: `init_sensor_drivers` defined but not used — this is
  an existing issue in the source, not introduced by this task.
