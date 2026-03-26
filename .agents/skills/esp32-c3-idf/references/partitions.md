# ESP32-C3 Custom Partition Tables

## Enable custom partitions
In `menuconfig → Partition Table → Custom partition table CSV`:
```
CONFIG_PARTITION_TABLE_CUSTOM=y
CONFIG_PARTITION_TABLE_CUSTOM_FILENAME="partitions.csv"
```

## Default layout (4 MB flash) — OTA-capable
```csv
# Name,   Type, SubType, Offset,   Size,   Flags
nvs,      data, nvs,     0x9000,   0x6000,
phy_init, data, phy,     0xf000,   0x1000,
otadata,  data, ota,     0x10000,  0x2000,
ota_0,    app,  ota_0,   0x20000,  0x1C0000,
ota_1,    app,  ota_1,   0x1E0000, 0x1C0000,
spiffs,   data, spiffs,  0x3A0000, 0x60000,
```

## Minimal single-app layout (4 MB flash)
```csv
# Name,   Type, SubType, Offset,  Size,   Flags
nvs,      data, nvs,     0x9000,  0x6000,
phy_init, data, phy,     0xf000,  0x1000,
factory,  app,  factory, 0x10000, 0x300000,
spiffs,   data, spiffs,  0x310000,0xF0000,
```

## Rules
- Offsets must be 4 KB aligned (0x1000).
- `app` partitions must be 64 KB aligned (0x10000).
- First app partition offset is typically `0x10000`.
- Total of all partition sizes + offsets must not exceed flash size.
- `nvs` must exist if Wi-Fi, BLE, or `nvs_flash_init()` is used.
- `phy_init` stores RF calibration data — always include it.

## SPIFFS usage
```c
#include "esp_spiffs.h"

esp_vfs_spiffs_conf_t conf = {
    .base_path = "/spiffs",
    .partition_label = NULL,   // uses first spiffs partition
    .max_files = 5,
    .format_if_mount_failed = true,
};
esp_vfs_spiffs_register(&conf);

// Use standard POSIX file I/O:
FILE *f = fopen("/spiffs/test.txt", "w");
fprintf(f, "Hello SPIFFS!\n");
fclose(f);
```

## Checking free space
```c
size_t total = 0, used = 0;
esp_spiffs_info(NULL, &total, &used);
ESP_LOGI(TAG, "SPIFFS: %d / %d bytes used", used, total);
```
