# ESP32-C3 UART Communication Firmware

This is the ESP32-C3 firmware for WiFi communication with STM32F411 flight controller.

## Hardware Connection

```
ESP32-C3          STM32F411
--------          ---------
GPIO4 (RX)  <--   PA2 (TX)
GPIO5 (TX)  -->   PA3 (RX)
GND         <->   GND
```

**Note**: Cross-connect TX/RX pins

## Build Instructions

### Prerequisites

1. Install ESP-IDF v5.0 or later:
```bash
git clone -b v5.0.2 --recursive https://github.com/espressif/esp-idf.git ~/esp/esp-idf
~/esp/esp-idf/install.sh esp32c3
```

2. Set up environment:
```bash
. ~/esp/esp-idf/export.sh
```

### Build and Flash

```bash
# Navigate to ESP32 firmware directory
cd firmware/esp32

# Set target (only needed once)
idf.py set-target esp32c3

# Build
idf.py build

# Flash and monitor (auto-detects port)
idf.py flash monitor

# Or specify port explicitly
idf.py -p /dev/ttyUSB0 flash monitor
```

### Monitor Only (without flashing)

```bash
idf.py monitor
```

## Protocol

The firmware implements a simple binary protocol for STM32 communication:

- **Header**: 0xAA55 (2 bytes)
- **Type**: 1 byte (message type)
- **Length**: 1 byte (payload length)
- **Payload**: 0-64 bytes
- **Checksum**: XOR of all previous bytes

### Message Types

| Type | Value | Description |
|------|-------|-------------|
| HEARTBEAT | 0x01 | Keep-alive message |
| STATUS | 0x02 | Status information |
| CONTROL | 0x03 | Control commands |
| SENSOR | 0x04 | Sensor data |
| DEBUG | 0x05 | Debug output |
| ACK | 0x06 | Acknowledgment |
| ERROR | 0x07 | Error report |

## Configuration

Edit the following in `main/main.c`:

```c
#define UART_ST_PORT            UART_NUM_1      // UART port
#define UART_ST_TX_PIN          GPIO_NUM_5      // TX pin
#define UART_ST_RX_PIN          GPIO_NUM_4      // RX pin
#define UART_ST_BAUDRATE        115200          // Baudrate
```

## Troubleshooting

### No communication with STM32

1. Check wiring: TX/RX are crossed (ESP32 TX -> STM32 RX, ESP32 RX -> STM32 TX)
2. Verify both devices share common GND
3. Check baudrate matches on both sides (default 115200)
4. Use logic analyzer to verify signal levels

### Build errors

1. Ensure ESP-IDF environment is set up: `. ~/esp/esp-idf/export.sh`
2. Clean build: `idf.py fullclean && idf.py build`
3. Check ESP-IDF version: `idf.py --version` (should be 5.0+)

### Monitor shows garbage

1. ESP32-C3 uses USB-Serial built-in, baudrate doesn't matter for USB
2. Check if the correct USB port is selected
3. Try resetting ESP32 after opening monitor
