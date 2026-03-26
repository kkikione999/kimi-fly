# 无人机 WiFi 飞行控制器

该仓库用于 STM32 飞控核心、ESP32-C3 WiFi 桥接、硬件资料和开发流程文档的统一管理。

## 目录入口

- `firmware/`：STM32、ESP32 和共享 HAL 源码
- `tools/`：地面站与仿真工具
- `hardware-docs/`：硬件真源与器件资料
- `docs/architecture/`：架构和技术路径
- `docs/plans/`：当前执行计划与技术债务
- `docs/archive/`：归档交付物与历史会话记录
- `knowledge-hub/`：知识库、项目地图、技术债账本

## 常用入口

```bash
cat docs/plans/active/plan.md
cat docs/plans/tech-debt-tracker.md
cat hardware-docs/pinout.md
cat knowledge-hub/README.md
```

## 硬件知识库

```bash
# 构建默认硬件知识库 chunk
python3 scripts/build_hardware_knowledge.py --clean

# 搜索已经生成的 chunk
python3 scripts/search_hardware_knowledge.py "who am i pwr_mgmt0" --chip icm-42688-p

# 启动 AnythingLLM 本地界面
cd knowledge-hub/platforms/anythingllm
cp .env.example .env
docker compose up -d
```

## 当前真实可用调试命令

先说明两件事:

- 当前仓库实际提供的串口抓取工具是 `tools/serial_capture.py`
- 如果 `FLIGHT_DEBUG_STATUS.md` 或旧会话记录里提到别的 `tools/*.py`, 先用 `rg --files tools` 确认文件是否仍在仓库中

### STM32 构建 / 烧录

```bash
# 推荐写法: 直接走 PlatformIO Python 入口, 不依赖 PATH 里的 pio
python3 -m platformio run -d firmware/stm32 -e flight
python3 -m platformio run -d firmware/stm32 -e flight -t upload

# 其他常用环境
python3 -m platformio run -d firmware/stm32 -e flight_tether_balance
python3 -m platformio run -d firmware/stm32 -e flight_tether_balance -t upload
python3 -m platformio run -d firmware/stm32 -e imu_map
python3 -m platformio run -d firmware/stm32 -e imu_map -t upload
```

### ESP32 构建 / 烧录 / 监视

```bash
# 当前 ESP32 主线是 ESP-IDF 工程: firmware/esp32/CMakeLists.txt + sdkconfig + main/*.c
source /Users/ll/esp/esp-idf/export.sh >/dev/null 2>&1 && idf.py -C firmware/esp32 build
source /Users/ll/esp/esp-idf/export.sh >/dev/null 2>&1 && idf.py -C firmware/esp32 flash
source /Users/ll/esp/esp-idf/export.sh >/dev/null 2>&1 && idf.py -C firmware/esp32 monitor
```

### 串口抓日志

```bash
# 查串口
ls /dev/cu.usbmodem* /dev/cu.usbserial* /dev/tty.usbmodem* /dev/tty.usbserial* 2>/dev/null

# STM32 调试串口抓取（USART1 @ 460800）
python3 tools/serial_capture.py /dev/cu.usbmodem212403 \
  --preset stm32-debug \
  --duration 20 \
  --flush-input \
  --text-out artifacts/serial_logs/stm32.log \
  --raw-out artifacts/serial_logs/stm32.bin

# ESP32 控制台抓取（115200）
python3 tools/serial_capture.py /dev/cu.usbmodemXXXX \
  --preset esp32-console \
  --duration 20 \
  --text-out artifacts/serial_logs/esp32.log
```
