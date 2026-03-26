# ESP32-C3 Register Lookup Guide

## High-Value Areas For This Project

- UART peripheral behavior for STM32 bridge traffic
- WiFi startup and coexistence constraints
- GPIO and reset behavior
- Interrupt and task interaction points when debugging bridge latency

## Search Examples

```bash
python3 scripts/search_hardware_knowledge.py "esp32-c3 uart fifo interrupt" --chip esp32-c3
python3 scripts/search_hardware_knowledge.py "esp32-c3 gpio0 gpio1 strapping" --chip esp32-c3
```
