# ESP32-C3 BLE Reference (NimBLE)

ESP-IDF ships two BLE stacks: **NimBLE** (recommended, smaller) and **Bluedroid** (legacy, larger).
This guide uses **NimBLE** which is the preferred stack for ESP32-C3.

## sdkconfig for NimBLE
```
CONFIG_BT_ENABLED=y
CONFIG_BT_NIMBLE_ENABLED=y
CONFIG_BT_CONTROLLER_ENABLED=y
# Disable Bluedroid to save RAM:
# CONFIG_BT_BLUEDROID_ENABLED is not set
```

Enable via: `menuconfig → Component config → Bluetooth → NimBLE`

## Required CMakeLists dependency
```cmake
idf_component_register(SRCS "main.c"
                        INCLUDE_DIRS "."
                        REQUIRES bt nvs_flash)
```

## BLE Advertising Skeleton (NimBLE)

```c
#include "esp_nimble_hci.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

static const char *TAG = "ble_adv";
static uint8_t own_addr_type;

static void ble_app_advertise(void);

static int ble_gap_event_cb(struct ble_gap_event *event, void *arg)
{
    switch (event->type) {
    case BLE_GAP_EVENT_CONNECT:
        ESP_LOGI(TAG, "Connection %s; handle=%d",
                 event->connect.status == 0 ? "established" : "failed",
                 event->connect.conn_handle);
        if (event->connect.status != 0) {
            ble_app_advertise(); // restart advertising
        }
        break;
    case BLE_GAP_EVENT_DISCONNECT:
        ESP_LOGI(TAG, "Disconnected; reason=%d", event->disconnect.reason);
        ble_app_advertise();
        break;
    default:
        break;
    }
    return 0;
}

static void ble_app_advertise(void)
{
    struct ble_hs_adv_fields fields = {0};
    fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
    const char *name = ble_svc_gap_device_name();
    fields.name = (uint8_t *)name;
    fields.name_len = strlen(name);
    fields.name_is_complete = 1;
    ble_gap_adv_set_fields(&fields);

    struct ble_gap_adv_params adv_params = {0};
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;
    ble_gap_adv_start(own_addr_type, NULL, BLE_HS_FOREVER, &adv_params,
                      ble_gap_event_cb, NULL);
}

static void ble_app_on_sync(void)
{
    ble_hs_id_infer_auto(0, &own_addr_type);
    ble_app_advertise();
}

static void nimble_host_task(void *param)
{
    nimble_port_run(); // blocks until nimble_port_stop()
    nimble_port_freertos_deinit();
}

void ble_init(void)
{
    nimble_port_init();
    ble_svc_gap_device_name_set("ESP32-C3");
    ble_svc_gap_init();
    ble_svc_gatt_init();
    ble_hs_cfg.sync_cb = ble_app_on_sync;
    nimble_port_freertos_init(nimble_host_task);
}
```

## GATT Service Definition Pattern

```c
// Characteristic read callback
static int my_char_access_cb(uint16_t conn_handle, uint16_t attr_handle,
                               struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
        uint8_t val = 42;
        os_mbuf_append(ctxt->om, &val, sizeof(val));
    }
    return 0;
}

static const struct ble_gatt_svc_def gatt_svcs[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = BLE_UUID16_DECLARE(0x180F), // Battery Service example
        .characteristics = (struct ble_gatt_chr_def[]){
            {
                .uuid = BLE_UUID16_DECLARE(0x2A19),
                .access_cb = my_char_access_cb,
                .flags = BLE_GATT_CHR_F_READ,
            },
            { 0 } // terminator
        },
    },
    { 0 } // terminator
};

// Register in ble_app_on_sync (before advertising):
ble_gatts_count_cfg(gatt_svcs);
ble_gatts_add_svcs(gatt_svcs);
```

## Gotchas
- NVS must be initialized before BLE init.
- BLE and Wi-Fi can coexist on ESP32-C3; enable `CONFIG_ESP_COEX_SW_COEXIST_ENABLE=y`.
- NimBLE runs in its own FreeRTOS task (`nimble_host_task`).
- Use Context7 MCP to look up specific `ble_gap_*` / `ble_gatt_*` function signatures.
