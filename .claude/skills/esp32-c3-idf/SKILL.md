---
name: esp32-c3-idf
description: >
  Expert assistant for developing firmware on the ESP32-C3 microcontroller using the ESP-IDF framework.
  Trigger this skill whenever the user mentions ESP32-C3, ESP-IDF, idf.py, sdkconfig, FreeRTOS on Espressif
  hardware, Wi-Fi/BLE on ESP32-C3, RISC-V embedded development, or asks to write, configure, debug, flash,
  or review any ESP-IDF project. Also trigger when the user asks about CMakeLists.txt for ESP-IDF, Kconfig
  options, partitions, NVS, SPIFFS, esp_log, gpio, LEDC, SPI, I2C, UART, or any other ESP-IDF peripheral
  driver. When in doubt, trigger — this skill is essential for correct ESP32-C3 code and configuration.
compatibility: "Requires bash tool. Works best with context7 MCP connected for live ESP-IDF API lookups."
---

# ESP32-C3 + ESP-IDF Development Skill

## Overview

The **ESP32-C3** is a single-core 32-bit **RISC-V** SoC made by Espressif Systems. It is *not* an Xtensa
core — never use Xtensa-specific compiler flags or assembly. Key hardware specs:

| Feature | Detail |
|---|---|
| CPU | RISC-V (RV32IMC), up to 160 MHz |
| RAM | 400 KB SRAM (only ~327 KB usable) |
| Flash | External SPI flash (typically 4 MB) |
| Wi-Fi | 802.11 b/g/n, 2.4 GHz |
| BLE | BLE 5.0 (coexist with Wi-Fi) |
| GPIO | 22 GPIO pins (0–21) |
| USB | Built-in USB Serial/JTAG (GPIO18/19) |
| ADC | 2× SAR ADC, 12-bit |
| No DAC | ⚠️ ESP32-C3 has NO DAC peripheral |
| No Ethernet | ⚠️ No built-in Ethernet MAC |

---

## 1. ESP-IDF Version Pinning

Always confirm or set the ESP-IDF version. ESP32-C3 is **fully supported from IDF v4.3+**. Prefer **v5.x** for new projects.

```bash
# Check current version
idf.py --version

# Set target (MUST be done before first build)
idf.py set-target esp32c3
```

> ⚠️ After `set-target`, the `sdkconfig` and `build/` directory are reset. Warn the user before doing this.

---

## 2. Project Structure

```
my_project/
├── CMakeLists.txt          # Top-level (boilerplate, rarely changed)
├── sdkconfig               # Generated config — commit to VCS
├── partitions.csv          # Custom partition table (optional)
├── main/
│   ├── CMakeLists.txt      # Lists source files for 'main' component
│   └── main.c / app_main.c
└── components/             # Optional: custom components
    └── my_component/
        ├── CMakeLists.txt
        ├── include/
        │   └── my_component.h
        └── my_component.c
```

### Minimal `main/CMakeLists.txt`
```cmake
idf_component_register(SRCS "main.c"
                        INCLUDE_DIRS ".")
```

### Minimal top-level `CMakeLists.txt`
```cmake
cmake_minimum_required(VERSION 3.16)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(my_project)
```

---

## 3. Entry Point

The entry point is always `app_main()`, **not** `main()`:

```c
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "main";

void app_main(void)
{
    ESP_LOGI(TAG, "Hello from ESP32-C3!");
    // Do NOT return from app_main — start tasks instead
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
```

> ⚠️ `app_main` runs in its default task with a limited stack (~3.5 KB by default). Spawn FreeRTOS tasks for heavier work.

---

## 4. Logging

Use `esp_log.h` — never `printf` for production code:

```c
#include "esp_log.h"
static const char *TAG = "my_module";

ESP_LOGE(TAG, "Error: %d", err);    // Error (always shown)
ESP_LOGW(TAG, "Warning: %s", msg);  // Warning
ESP_LOGI(TAG, "Info: %d", value);   // Info (default visible)
ESP_LOGD(TAG, "Debug value: %f", f);// Debug (enabled via menuconfig)
ESP_LOGV(TAG, "Verbose");           // Verbose
```

Control log level via `menuconfig → Component config → Log output` or at runtime:
```c
esp_log_level_set("my_module", ESP_LOG_DEBUG);
esp_log_level_set("*", ESP_LOG_WARN); // silence all others
```

---

## 5. GPIO

```c
#include "driver/gpio.h"

// Configure output
gpio_config_t io_conf = {
    .pin_bit_mask = (1ULL << GPIO_NUM_5),
    .mode = GPIO_MODE_OUTPUT,
    .pull_up_en = GPIO_PULLUP_DISABLE,
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .intr_type = GPIO_INTR_DISABLE,
};
gpio_config(&io_conf);
gpio_set_level(GPIO_NUM_5, 1);

// Configure input with interrupt
io_conf.pin_bit_mask = (1ULL << GPIO_NUM_2);
io_conf.mode = GPIO_MODE_INPUT;
io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
io_conf.intr_type = GPIO_INTR_NEGEDGE;
gpio_config(&io_conf);
gpio_install_isr_service(0);
gpio_isr_handler_add(GPIO_NUM_2, my_isr_handler, NULL);
```

> ⚠️ GPIO18/19 are the USB D+/D- pins. Do not use them as regular GPIO when USB CDC is enabled.  
> ⚠️ GPIO11 is connected to SPI flash — avoid it on most boards.

---

## 6. FreeRTOS Tasks

```c
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void my_task(void *pvParameters)
{
    while (1) {
        // work...
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// Spawn from app_main:
xTaskCreate(my_task, "my_task", 4096, NULL, 5, NULL);
// args:        func    name    stackB  param  prio  handle
```

**Stack sizing rule of thumb:** start with 4096 bytes; use `uxTaskGetHighWaterMark()` to tune down.

---

## 7. NVS (Non-Volatile Storage)

```c
#include "nvs_flash.h"
#include "nvs.h"

// Always initialize NVS at startup
esp_err_t ret = nvs_flash_init();
if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    nvs_flash_erase();
    nvs_flash_init();
}

// Read/write
nvs_handle_t handle;
nvs_open("storage", NVS_READWRITE, &handle);
nvs_set_i32(handle, "counter", 42);
nvs_commit(handle);
nvs_close(handle);
```

---

## 8. Error Handling

Always check `esp_err_t` return values. Use the canonical macros:

```c
#include "esp_check.h"

// Logs error and returns err if check fails
ESP_RETURN_ON_ERROR(gpio_config(&io_conf), TAG, "GPIO config failed");

// Logs error and jumps to label
ESP_GOTO_ON_ERROR(i2c_master_start(cmd), TAG, err, "I2C start failed");

// Manual pattern
esp_err_t err = some_function();
if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed: %s", esp_err_to_name(err));
    return err;
}
```

---

## 9. Wi-Fi (Station Mode Template)

See `references/wifi.md` for full Wi-Fi station + event loop boilerplate.

---

## 10. BLE

See `references/ble.md` for NimBLE / Bluedroid guidance.

---

## 11. Build, Flash & Monitor

```bash
# Full build
idf.py build

# Flash (auto-detect port)
idf.py flash

# Flash + monitor
idf.py flash monitor

# Specify port
idf.py -p /dev/ttyUSB0 flash monitor

# Menu-based configuration
idf.py menuconfig

# Clean build
idf.py fullclean
```

**Baud rate for ESP32-C3 with USB-JTAG:** the monitor baud rate doesn't matter — it uses USB CDC. Set it to `115200` in `menuconfig → Component config → ESP System Settings → UART console baud rate` only when using a UART adapter.

---

## 12. sdkconfig Key Options for ESP32-C3

| Config Key | Notes |
|---|---|
| `CONFIG_IDF_TARGET="esp32c3"` | Set automatically by `set-target` |
| `CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y` | Use built-in USB for console (no UART adapter needed) |
| `CONFIG_ESP_MAIN_TASK_STACK_SIZE` | Default 3584; increase if `app_main` does heavy work |
| `CONFIG_FREERTOS_HZ` | Default 100; increase to 1000 for finer delays |
| `CONFIG_PARTITION_TABLE_CUSTOM=y` | Enable custom `partitions.csv` |
| `CONFIG_ESPTOOLPY_FLASHSIZE_4MB=y` | Match your board's flash size |

---

## 13. Common Pitfalls

| Mistake | Correct Approach |
|---|---|
| Using `main()` instead of `app_main()` | Always use `app_main()` |
| Blocking in ISR | Use `xQueueSendFromISR()` / `xSemaphoreGiveFromISR()` |
| Using `delay()` (Arduino) | Use `vTaskDelay(pdMS_TO_TICKS(ms))` |
| `printf` without newline | `printf` output may not flush; prefer `ESP_LOGI` |
| Using GPIO18/19 with USB CDC enabled | Reserve them for USB |
| Missing `nvs_flash_init()` | Wi-Fi and BLE both require NVS initialized first |
| Wrong flash voltage | ESP32-C3 uses 3.3 V flash — never configure 1.8 V |
| Forgetting `idf_component_register` SRCS | New `.c` files must be listed in `main/CMakeLists.txt` |
| Heap allocation without null check | Always check `malloc`/`heap_caps_malloc` return value |

---

## 14. Using Context7 MCP for API Lookups

When you need the exact signature, parameters, or return values of an ESP-IDF function, **use the Context7 MCP tool** to fetch live documentation:

### How to call Context7

1. **Resolve the library ID first:**
```
mcp: context7 → resolve_library_id
query: "esp-idf"
```

2. **Fetch docs for a specific topic:**
```
mcp: context7 → get_library_docs
libraryId: <id from step 1>
topic: "gpio_config" | "i2c_master" | "esp_wifi_init" | ...
tokens: 5000
```

### When to use Context7

- Exact function signatures with all parameters
- Struct field names (e.g., `wifi_config_t`, `i2c_config_t`)
- Return value meanings (`esp_err_t` codes)
- Kconfig symbol names for `sdkconfig`
- Peripheral driver APIs (SPI, I2C, UART, LEDC, ADC, RMT, etc.)
- FreeRTOS API nuances on ESP-IDF

### Example — looking up I2C master API

```
context7.resolve_library_id("esp-idf")
→ libraryId: "/espressif/esp-idf"

context7.get_library_docs(
  libraryId: "/espressif/esp-idf",
  topic: "i2c driver master",
  tokens: 6000
)
```

Always prefer Context7 over guessing function signatures — ESP-IDF API changed significantly between v4.x and v5.x (e.g., the legacy I2C driver vs. the new `i2c_master` driver in v5.1+).

---

## 15. Peripheral Quick Reference

| Peripheral | Header | Notes |
|---|---|---|
| GPIO | `driver/gpio.h` | 22 pins; see §5 |
| ADC | `esp_adc/adc_oneshot.h` (v5) | 12-bit, CH0–CH4 on ADC1 |
| LEDC (PWM) | `driver/ledc.h` | 6 channels, use for PWM/LED |
| I2C | `driver/i2c_master.h` (v5.1+) | New master API; use Context7 |
| SPI | `driver/spi_master.h` | SPI2 is user-accessible |
| UART | `driver/uart.h` | UART0=console, UART1 free |
| RMT | `driver/rmt_tx.h` / `rmt_rx.h` | Good for WS2812 LEDs |
| Timer | `driver/gptimer.h` | General purpose timers |
| WDT | `esp_task_wdt.h` | Feed watchdog in long loops |

For any peripheral not listed or for parameter details, **invoke Context7**.

---

## Reference Files

- `references/wifi.md` — Wi-Fi station boilerplate + event loop
- `references/ble.md` — BLE advertising + GATT server skeleton
- `references/partitions.md` — Custom partition table examples
