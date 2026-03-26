---
name: drone-debug
description: >
  本仓库无人机飞控开发/调试工作流技能。只要任务涉及无人机飞控、台架/系绳调试、编译烧录、串口抓日志、飞行日志复盘、STM32 <-> ESP32 联调、或判断当前该用什么工具时，就必须使用此技能。先确认仓库内真实存在的工具和代码路径，再继续分派到 `stm32-dev` / `esp32-c3-idf`，并使用 `python3 -m platformio`、`source /Users/ll/esp/esp-idf/export.sh && idf.py`、`tools/serial_capture.py`、`openocd`、`arm-none-eabi-gdb`、`minicom` 等本地工具。
compatibility: "bash; python3; PlatformIO via python3 -m platformio; ESP-IDF via /Users/ll/esp/esp-idf/export.sh; openocd; arm-none-eabi-gdb; minicom"
---

# 无人机开发/调试工作流技能

## 1. 先做工具分派, 再做代码改动

这个 skill 负责回答两个问题:

1. 当前任务到底应该走哪条工具链
2. 还需要继续加载哪些硬件 skill

先判断任务类型:

- 只改 `firmware/stm32/` 或硬件敏感飞控逻辑: 继续加载 `stm32-dev`
- 只改 `firmware/esp32/` 的 WiFi bridge / UART bridge / ESP-IDF 配置: 继续加载 `esp32-c3-idf`
- 跨 STM32 <-> ESP32 串口链路、协议边界、复位/启动路径: 两个 skill 都加载
- 只做编译、烧录、抓串口、日志落盘、端口发现、当前工具链判断: 保持在本 skill 即可

规则:

- 这个 skill 不替代硬件检索 skill
- 一旦任务触碰硬件事实、引脚、寄存器、时序、坐标系, 立刻切到对应硬件 skill 并按其检索顺序继续

## 2. 先确认仓库真实状态, 不信旧文档

先核对真实文件树和工具:

```bash
sed -n '1,220p' AGENTS.md
sed -n '1,220p' README.md
sed -n '1,220p' FLIGHT_DEBUG_STATUS.md
rg --files tools
find firmware/stm32 -maxdepth 2 \\( -name '*.c' -o -name '*.h' -o -name 'platformio.ini' \\) | sort
find firmware/esp32 -maxdepth 2 \\( -name '*.c' -o -name '*.h' -o -name '*.cpp' -o -name 'CMakeLists.txt' -o -name 'sdkconfig' -o -name 'platformio.ini' \\) | sort
```

如果文档和文件树冲突:

- 真实存在的文件优先
- `rg --files tools` 查不到的脚本视为 stale
- `firmware/esp32/CMakeLists.txt` + `firmware/esp32/sdkconfig` + `firmware/esp32/main/*.c` 视为当前 ESP-IDF 主线
- `firmware/esp32/platformio.ini` + `firmware/esp32/src/main.cpp` 默认视为旧 Arduino/PlatformIO 路线, 除非用户明确要求走它

## 3. 当前仓库优先工具链

### 3.1 STM32 构建 / 烧录

`pio` 不在 PATH 时, 优先用 `python3 -m platformio`:

```bash
python3 -m platformio --version
python3 -m platformio run -d firmware/stm32 -e flight
python3 -m platformio run -d firmware/stm32 -e flight -t upload
python3 -m platformio run -d firmware/stm32 -e flight_tether_balance
python3 -m platformio run -d firmware/stm32 -e flight_tether_balance -t upload
python3 -m platformio run -d firmware/stm32 -e imu_map
python3 -m platformio run -d firmware/stm32 -e imu_map -t upload
```

如果必须用裸 `pio`, 当前机器已知路径是:

```bash
/Users/ll/Library/Python/3.9/bin/pio
```

### 3.2 ESP32 构建 / 烧录 / 监视

`idf.py` 默认不在 PATH, 先 source:

```bash
source /Users/ll/esp/esp-idf/export.sh >/dev/null 2>&1 && idf.py --version
source /Users/ll/esp/esp-idf/export.sh >/dev/null 2>&1 && idf.py -C firmware/esp32 build
source /Users/ll/esp/esp-idf/export.sh >/dev/null 2>&1 && idf.py -C firmware/esp32 flash
source /Users/ll/esp/esp-idf/export.sh >/dev/null 2>&1 && idf.py -C firmware/esp32 monitor
```

注意:

- 当前仓库 `sdkconfig` 显示 `ESP-IDF v5.2`
- 不要无条件运行 `idf.py set-target esp32c3`, 它会重置 `sdkconfig` 和 `build/`

### 3.3 串口端口发现 / 抓日志

先找端口:

```bash
ls /dev/cu.usbmodem* /dev/cu.usbserial* /dev/tty.usbmodem* /dev/tty.usbserial* 2>/dev/null
```

仓库当前真实存在的抓串口工具是:

```bash
python3 tools/serial_capture.py --help
```

常用抓取方式:

```bash
python3 tools/serial_capture.py /dev/cu.usbmodem212403 --preset stm32-debug --duration 20 --flush-input --text-out artifacts/serial_logs/stm32.log --raw-out artifacts/serial_logs/stm32.bin
python3 tools/serial_capture.py /dev/cu.usbmodem212403 --preset stm32-debug --echo
python3 tools/serial_capture.py /dev/cu.usbmodemXXXX --preset esp32-console --duration 20 --text-out artifacts/serial_logs/esp32.log
```

已知串口预设:

- `stm32-debug = 460800`
- `esp32-console = 115200`

### 3.4 SWD / GDB / 交互串口

当前机器已知可用工具:

```bash
which openocd arm-none-eabi-gdb minicom
```

常用命令:

```bash
openocd -f interface/stlink.cfg -f target/stm32f4x.cfg
/Users/ll/embedded-tools/arm-gnu-toolchain-13.3.rel1-darwin-arm64-arm-none-eabi/bin/arm-none-eabi-gdb firmware/stm32/.pio/build/flight/firmware.elf
minicom -D /dev/cu.usbmodem212403 -b 460800
```

如果需要更完整的 SWD/GDB 流程, 再读:

- `./.agents/skills/stm32-dev/references/openocd-advanced.md`

## 4. 针对这个仓库的额外规则

- `README.md` 或 `FLIGHT_DEBUG_STATUS.md` 里如果提到 `tools/*.py`, 先用 `rg --files tools` 确认文件还在不在
- 不要把“历史最优轮次”或“旧 round runner”当成当前真实工具链
- 需要飞行状态、台架结论、当前固件状态时, 读 `FLIGHT_DEBUG_STATUS.md`; 需要决定用什么命令时, 先信文件树和本 skill
- 做 STM32 / 传感器 / 电机相关改动时, 仍然必须遵守 `stm32-dev` 的硬件检索门禁
- 做 ESP32 GPIO / UART / boot / reset / WiFi bridge 改动时, 仍然必须遵守 `esp32-c3-idf` 的硬件检索门禁
