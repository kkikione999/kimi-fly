# L1 — Tools (Ground Station & Simulation)

> Path: `tools/` | Language: Python | Status: ✅ Reference implementations

---

## Ground Station (`tools/ground_station/`)

| File | Role |
|------|------|
| `ground_station.py` | TCP client connecting to ESP32-C3:8888, sends RC commands, displays telemetry |
| `protocol.py` | Python implementation of the binary protocol (matches `firmware/stm32/comm/protocol.h`) |
| `README.md` | Usage instructions |

Usage pattern:
```python
# protocol.py
pack_frame(cmd, payload) → bytes
unpack_frame(data) → (cmd, payload)
```

Connect: `TCP → ESP32_IP:8888`

---

## Simulation (`tools/simulation/`)

| File | Role |
|------|------|
| `drone_dynamics.py` | 6-DOF rigid body dynamics model (quadrotor) |
| `sil_wrapper.py` | Software-in-the-loop wrapper — runs flight controller logic in Python against dynamics model |
| `test_runner.py` | Automated SIL test suite |
| `README.md` | Usage instructions |

Purpose: validate PID tuning and flight controller logic without hardware. Not a hardware-in-the-loop setup.
