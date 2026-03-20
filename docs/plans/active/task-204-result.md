# Task 204 Result: STM32-ESP32 UART Communication Verification

**Date**: 2026-03-19
**Status**: PARTIAL - Firmware verified running, hardware UART link not established
**Agent**: stm32-embedded-engineer

---

## Executive Summary

Both STM32 and ESP32 firmware are confirmed running and outputting debug data. The UART communication protocol is correctly implemented on both sides. However, bidirectional data transfer between STM32 USART2 and ESP32 Serial1 is NOT working — RX=0 on both sides — indicating a hardware connection issue.

---

## Hardware Connection (Verified via Schematic)

| Signal | STM32 Pin | ESP32 Pin | Direction |
|--------|-----------|-----------|-----------|
| UART TX | PA2 (USART2_TX) | GPIO4 (RX) | STM32 → ESP32 |
| UART RX | PA3 (USART2_RX) | GPIO5 (TX) | ESP32 → STM32 |
| GND | GND | GND | Common |
| Baudrate | 115200 | 115200 | Both |

Source: `hardware-docs/components.md` and `hardware-docs/pinout.md`

---

## Firmware Status

### STM32 (stm32f411 env, `src/main.c`)

**Status**: RUNNING - confirmed via debug UART (USART1 PA9/PA10 @ 460800)

Sample output:
```
[STAT] TX:50 RX:0 ERR:0 CONN:NO
[STAT] TX:51 RX:0 ERR:0 CONN:NO
[STAT] TX:55 RX:0 ERR:0 CONN:NO
```

- Heartbeats sent every 2 seconds via USART2 (PA2/PA3 @ 115200)
- Protocol: `[0x55 0xAA][TYPE:1][LEN:1][PAYLOAD:N][XOR_CHECKSUM:1]`
- TX counter incrementing (confirmed sending)
- RX counter = 0 (not receiving from ESP32)

**Key bug fixed during this task**: Missing `SysTick_Handler` in user code caused MCU to lock up in `Default_Handler` on first SysTick interrupt after `HAL_Init()`. Added `SysTick_Handler` calling `HAL_IncTick()`.

**Clock config fixed**: Original firmware had `HAL_RCC_OscConfig` blocking on HSE timeout with no PLL fallback. Updated to HSI-based PLL (16MHz HSI → 84MHz SYSCLK) which is independent of external crystal.

### ESP32 (Arduino framework, `src/main.cpp`)

**Status**: RUNNING - confirmed via USB serial @ 115200

Sample output:
```
I (789954) ESP32_UART: TX total: 534 (+4)
I (789954) ESP32_UART: RX total: 0 (+0)
I (789954) ESP32_UART: UART Connection: NO
I (789954) ESP32_UART: WiFi Status: CONNECTED
I (789964) ESP32_UART: WiFi IP: 192.168.50.132
```

- Heartbeats sent every ~2 seconds via Serial1 (GPIO5=TX, GPIO4=RX @ 115200)
- TX counter = 534+ (confirmed sending)
- RX counter = 0 (not receiving from STM32)

---

## Test Results

| Test Item | Expected | Actual | Status |
|-----------|----------|--------|--------|
| STM32 firmware boots | Boot message on USART1 | STAT messages seen | PASS |
| ESP32 firmware boots | Boot + WiFi connected | WiFi connected, stats seen | PASS |
| STM32 → ESP32 heartbeat | ESP32 RX > 0 | ESP32 RX = 0 | FAIL |
| ESP32 → STM32 heartbeat | STM32 RX > 0 | STM32 RX = 0 | FAIL |
| Protocol validation | ACK exchanged | No exchange | FAIL |
| Bidirectional verified | Both CONN:YES | Both CONN:NO | FAIL |

---

## Root Cause Analysis

Both firmware sides are correctly implemented and running. The UART connection failure in BOTH directions (STM32 TX→ESP32 RX and ESP32 TX→STM32 RX) points to a **hardware layer problem**.

Most likely causes (in priority order):

1. **TX/RX cross-wiring wrong**: Physical wires may be connected incorrectly (TX→TX, RX→RX instead of TX→RX cross)
2. **Physical disconnect**: Wire not firmly seated or broken connection
3. **Voltage level mismatch**: STM32 and ESP32 both operate at 3.3V so this should not be an issue
4. **GND reference not shared**: Missing common ground between STM32 and ESP32 boards

The ESP32 firmware itself warns:
```
W ESP32_UART: Hardware connection issue (TX/RX cross, GND)
```

---

## Bugs Fixed During This Task

### Bug 1: Missing SysTick_Handler (CRITICAL)
- **Symptom**: STM32 debug UART completely silent, MCU stuck in `Default_Handler`
- **Root cause**: PlatformIO + stm32cube framework does NOT automatically provide `SysTick_Handler`. The startup file defines it as a weak alias to `Default_Handler` (infinite loop). HAL_Init() enables SysTick, which fires immediately and MCU locks up.
- **Fix**: Added `void SysTick_Handler(void) { HAL_IncTick(); }` to `src/main.c`
- **File**: `firmware/stm32/src/main.c`

### Bug 2: HSE-dependent clock config with no timeout handling
- **Symptom**: `HAL_RCC_OscConfig()` would return error if 8MHz external crystal not ready, leaving MCU on default HSI without proper UART baud rate setup
- **Fix**: Switched to HSI-based PLL (16MHz → 84MHz SYSCLK) with error fallback. Added required `__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1)` call before PLL configuration.
- **File**: `firmware/stm32/src/main.c`

---

## Files Modified

| File | Change |
|------|--------|
| `firmware/stm32/src/main.c` | Complete UART comm test firmware: SysTick handler added, clock config fixed (HSI PLL), protocol implementation |
| `firmware/stm32/platformio.ini` | Added `build_src_filter` to prevent multiple `main()` compile errors |

---

## Next Steps Required

1. **Physical hardware inspection**: Verify TX/RX wire connections between STM32 PA2/PA3 and ESP32 GPIO5/GPIO4
   - STM32 PA2 (TX) should connect to ESP32 GPIO4 (RX)
   - STM32 PA3 (RX) should connect to ESP32 GPIO5 (TX)
   - Common GND between boards must be connected

2. **Re-run communication test** after hardware connection is confirmed

3. If wiring is confirmed correct, try loopback test on STM32 (connect PA2 to PA3) to verify USART2 is functional

---

## Serial Ports

| Device | Port | Baudrate | Notes |
|--------|------|----------|-------|
| STM32 Debug | `/dev/cu.usbmodem212403` | 460800 | USART1 via ST-Link VCP |
| ESP32 Debug | `/dev/cu.usbmodem212301` | 115200 | USB serial |
