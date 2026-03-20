# L1 — STM32 Communication Layer

> Path: `firmware/stm32/comm/` | Language: C | Status: ⚠️ Protocol ready, UART wiring unverified

---

## Files

| File | Role |
|------|------|
| `protocol.h/.c` | Binary frame codec (pack/unpack), CRC16 |
| `wifi_command.h/.c` | Decode incoming WiFi commands, encode telemetry, drive flight controller |
| `comm_protocol.h` | Legacy stub / shared types (⚠️ intent unclear — may be redundant with protocol.h) |

---

## Binary Protocol (protocol.h)

Frame format:
```
[0xAA][0x55][LEN:1][CMD:1][PAYLOAD:0-64][CRC16:2]
```

| Field | Size | Notes |
|-------|------|-------|
| Header | 2 bytes | 0xAA 0x55, big-endian |
| Length | 1 byte | payload length only |
| CMD | 1 byte | command enum |
| Payload | 0–64 bytes | `PROTOCOL_MAX_PAYLOAD_SIZE=64` |
| CRC16 | 2 bytes | over CMD+PAYLOAD |

Command set:
```c
// System (0x00–0x0F)
CMD_HEARTBEAT=0x00, CMD_ACK=0x01, CMD_NACK=0x02, CMD_VERSION=0x03, CMD_STATUS=0x04

// Flight control (0x10–0x1F)
CMD_ARM=0x10, CMD_DISARM=0x11, CMD_MODE=0x12, CMD_RC_INPUT=0x13

// Telemetry (0x20–0x2F)
CMD_TELEMETRY_ATT=0x20, CMD_TELEMETRY_IMU=0x21,
CMD_TELEMETRY_MOTOR=0x22, CMD_TELEMETRY_BATTERY=0x23

// Config (0x30–0x3F)
// ... (PID params, calibration)
```

Public API:
```c
hal_status_t protocol_pack(uint8_t cmd, const uint8_t *payload, uint8_t len, uint8_t *frame, uint8_t *frame_len);
hal_status_t protocol_unpack(const uint8_t *frame, uint8_t frame_len, uint8_t *cmd, uint8_t *payload, uint8_t *payload_len);
uint16_t     protocol_crc16(const uint8_t *data, uint16_t len);
```

---

## WiFi Command Handler (wifi_command.h)

Ties UART RX → protocol decode → flight controller API → UART TX telemetry.

Config:
```c
WIFI_CMD_BUFFER_SIZE        128     // RX buffer
WIFI_TX_BUFFER_SIZE         128     // TX buffer
WIFI_CMD_TIMEOUT_MS         500     // command timeout
WIFI_HEARTBEAT_INTERVAL_MS  100     // heartbeat period
WIFI_TELEMETRY_INTERVAL_MS  20      // telemetry TX (50Hz)
```

States: `WIFI_DISCONNECTED → WIFI_CONNECTED`

Public API:
```c
hal_status_t wifi_command_init(void);
void         wifi_command_process(void);      // call from main loop
void         wifi_telemetry_send(void);       // called @ 50Hz
wifi_state_t wifi_command_get_state(void);
```

---

## Known Issue — UART Physical Wiring (TD-001)

Current PDF-verified hardware mapping for the bench-test firmware:
- STM32: PA2=TX, PA3=RX
- ESP32-C3: GPIO1=RX (U10 pin13 / IO1), GPIO0=TX (U10 pin12 / IO0)
- Expected cross: STM32_TX(PA2) → ESP32_RX(GPIO1), ESP32_TX(GPIO0) → STM32_RX(PA3)

Task 304 adds a software diagnostic tool. Physical wiring must be manually inspected.
