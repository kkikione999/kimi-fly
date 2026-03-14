# Task 003: ESP32-C3 HAL 层实现

## 目标
基于 HAL 接口定义，实现 ESP32-C3 平台的 HAL 层。

## 背景上下文

### 前置任务
- Task-001 已完成 HAL 接口定义
- Task-002 正在进行 STM32 HAL 实现（可参考其模式）

### 硬件平台
- **目标芯片**: ESP32-C3-WROOM-02
- **主频**: 160MHz
- **架构**: RISC-V RV32IMC
- **功能**: WiFi/BLE 通信、与 STM32 通过 UART 通信

### ESP32-C3 特有的外设
- **LEDC**: 用于 PWM (代替 STM32 的 TIM)
- **GPIO**: 简化 GPIO 控制
- **UART**: 2 个 UART 控制器
- **SPI**: 用于外设通信
- **I2C**: 用于传感器通信
- **WiFi**: 802.11 b/g/n

## 具体修改要求

### 1. 文件: `firmware/hal/esp32c3/esp32c3_hal.h`
ESP32-C3 HAL 头文件。

内容需包含:
- ESP-IDF 头文件引用
- ESP32-C3 特有的配置宏
- 函数声明

### 2. 文件: `firmware/hal/esp32c3/esp32c3_hal.c`
ESP32-C3 系统初始化。

内容需包含:
- `esp32c3_system_init()`: 初始化 NVS、网络栈等
- `esp32c3_get_tick_ms()`: 使用 `esp_timer_get_time()`
- `esp32c3_delay_ms()`: 使用 `vTaskDelay`
- `esp32c3_delay_us()`: 使用 `esp_rom_delay_us`
- 中断控制实现

### 3. 文件: `firmware/hal/esp32c3/esp32c3_gpio.c`
GPIO 接口实现。

内容需包含:
- 使用 ESP-IDF GPIO API
- 实现 init, set, get, toggle, deinit
- 注册到 HAL 接口表

### 4. 文件: `firmware/hal/esp32c3/esp32c3_uart.c`
UART 接口实现。

内容需包含:
- UART0: 调试（与 USB 复用）
- UART1: 与 STM32 通信
- 使用 ESP-IDF UART 驱动
- 实现 init, send, recv, flush, deinit
- 注册到 HAL 接口表

### 5. 文件: `firmware/hal/esp32c3/esp32c3_wifi.c`
WiFi 接口（HAL 扩展）。

内容需包含:
- WiFi 初始化
- AP 模式配置（创建热点供手机连接）
- STA 模式配置（连接到路由器）
- UDP/TCP 服务器初始化
- 简单的数据包收发接口

## 编码规范
- 使用 ESP-IDF 风格
- 使用 `ESP_LOG*` 宏进行日志输出
- 使用 `ESP_ERROR_CHECK` 检查返回值
- 最小化栈使用，优先使用静态分配

## 完成标准

- [ ] `esp32c3_hal.h` - 头文件
- [ ] `esp32c3_hal.c` - 系统初始化和时间管理
- [ ] `esp32c3_gpio.c` - GPIO 实现
- [ ] `esp32c3_uart.c` - UART 实现
- [ ] `esp32c3_wifi.c` - WiFi 实现
- [ ] 代码风格符合 ESP-IDF 规范
- [ ] 每个函数有完整文档注释

## 依赖关系

```
Depends on: task-001-hal-structure
Parallel to: task-002-stm32-hal
Blocks: task-004-communication-protocol
```

## 相关文件
- `firmware/hal/hal_interface.h` - 接口定义
- `firmware/platform/board_config.h` - 引脚配置

## 注意事项
- ESP32-C3 和 STM32 的引脚定义不同，注意条件编译
- WiFi 功能需要包含 `esp_wifi.h`
- 注意 ESP-IDF 的 FreeRTOS 集成
