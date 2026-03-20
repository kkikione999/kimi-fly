# L1 — ESP32-C3 WiFi Bridge

> Path: `firmware/esp32/main/` | Framework: ESP-IDF | Status: ✅ TCP bridge ready

---

## Architecture

```
Ground Station (TCP:8888)
    ↕ TCP socket (WiFi, STA mode)
ESP32-C3 TCP Server (tcp_server.c)
    ↕ Binary frames forwarded as-is
UART1 (GPIO0=TX, GPIO1=RX, 115200 baud)
    ↕
STM32 USART2 (PA2=TX, PA3=RX)
```

The ESP32 is a **transparent bridge** — it does not interpret protocol frames, just forwards them bidirectionally.

---

## Files

| File | Role |
|------|------|
| `main.c` | App entry, UART1 init, task spawning, NVS init |
| `wifi_sta.c/.h` | WiFi STA mode connect, reconnect logic |
| `tcp_server.c/.h` | TCP server on port 8888, client management, bidirectional relay |

---

## WiFi STA (wifi_sta.h)

```c
#define WIFI_SSID       "whc"
#define WIFI_PASSWORD   "12345678"
#define WIFI_MAX_RETRY  5

esp_err_t wifi_sta_init(void);
wifi_connection_status_t wifi_sta_get_status(void);
// States: WIFI_DISCONNECTED → WIFI_CONNECTING → WIFI_CONNECTED
```

---

## TCP Server (tcp_server.h)

```c
#define TCP_SERVER_PORT         8888
#define TCP_SERVER_MAX_CONN     3
#define TCP_BUF_SIZE            256
#define TCP_CLIENT_TIMEOUT_SEC  30

esp_err_t tcp_server_init(void);   // call after WiFi connected
bool      tcp_server_is_running(void);
```

---

## UART Bridge (main.c)

```c
#define UART_ST_PORT        UART_NUM_1
#define UART_ST_TX_PIN      GPIO_NUM_0   // → STM32 PA3 (RX)
#define UART_ST_RX_PIN      GPIO_NUM_1   // ← STM32 PA2 (TX)
#define UART_ST_BAUDRATE    115200
#define UART_ST_BUF_SIZE    256
```

Two FreeRTOS tasks:
- `uart_rx_task` — reads UART1, forwards to TCP clients
- TCP → UART write handled in TCP server task callback

---

## Build

```ini
# firmware/esp32/platformio.ini
platform = espressif32
board = esp32-c3-devkitm-1
framework = arduino          # Arduino wrapper over ESP-IDF
upload_protocol = esptool
monitor_speed = 115200
build_flags = -DWIFI_STA_SSID=\"whc\" -DWIFI_STA_PASS=\"12345678\"
```

ESP-IDF native build also available via `firmware/esp32/CMakeLists.txt` (project: `esp32_wifi_bridge`).

---

## Known Issue

UART physical wiring to STM32 unverified (see L1-stm32-comm.md TD-001). If wiring is correct, the bridge should work as-is since baud rates match.
