# ESP32-C3 与 STM32 UART 通信测试指南

## 硬件连接

```
ESP32-C3          STM32F411CEU6
--------          -------------
GPIO4 (RX)  <--   PA2 (TX)
GPIO5 (TX)  -->   PA3 (RX)
GND         <->   GND

注意：TX/RX 需要交叉连接
```

## 项目文件结构

```
firmware/
├── esp32/              # ESP32-C3 固件
│   ├── main/
│   │   └── main.c      # ESP32 UART通信主程序
│   ├── CMakeLists.txt
│   └── README.md       # ESP32编译说明
│
├── stm32/              # STM32 固件
│   ├── main/
│   │   ├── verify_hardware.c   # 硬件测试程序
│   │   └── uart_comm_test.c    # UART通信测试程序
│   ├── hal/
│   │   ├── uart.c      # UART HAL驱动
│   │   └── uart.h
│   └── Makefile
│
└── platform/
    ├── board_config.h  # 硬件配置
    └── comm_protocol.h # 通信协议定义（共享）
```

## 通信协议

### 数据帧格式

```
+--------+--------+--------+---------+----------+-----------+
| Header |  Type  | Length | Payload | Checksum |
| 2 bytes| 1 byte | 1 byte | N bytes |  1 byte  |
+--------+--------+--------+---------+----------+-----------+
```

- **Header**: 0xAA55 (固定值)
- **Type**: 消息类型 (见下表)
- **Length**: Payload长度 (0-64字节)
- **Payload**: 数据内容
- **Checksum**: 前面所有字节的XOR校验

### 消息类型

| 类型 | 值 | 说明 |
|------|-----|------|
| HEARTBEAT | 0x01 | 心跳包，保持连接 |
| STATUS | 0x02 | 状态信息 |
| CONTROL | 0x03 | 控制命令 |
| SENSOR | 0x04 | 传感器数据 |
| DEBUG | 0x05 | 调试输出 |
| ACK | 0x06 | 确认应答 |
| ERROR | 0x07 | 错误报告 |

## STM32 编译和烧写

### 编译 UART 通信测试固件

```bash
cd firmware/stm32

# 编译 UART 通信测试程序
make uart-test

# 或者
make clean
make TARGET=uart_comm_test
```

### 烧写到 STM32

```bash
# 使用 OpenOCD + ST-Link
make flash

# 或使用 st-flash
make flash-stlink
```

### 查看调试输出

STM32 调试 UART (USART1 - PA9/PA10) 输出日志：
```bash
# Linux/Mac
minicom -b 921600 -D /dev/ttyUSB0

# 或 screen
screen /dev/ttyUSB0 921600
```

## ESP32 编译和烧写

### 环境准备

```bash
# 设置 ESP-IDF 环境（如果尚未设置）
. ~/esp/esp-idf/export.sh
```

### 编译和烧写

```bash
cd firmware/esp32

# 设置目标芯片（只需执行一次）
idf.py set-target esp32c3

# 编译
idf.py build

# 烧写并监控
idf.py flash monitor

# 指定端口
idf.py -p /dev/ttyUSB1 flash monitor
```

### 仅监控输出

```bash
idf.py monitor
```

## 测试步骤

### 1. 硬件准备

1. 按上图连接 ESP32 和 STM32 的 UART 引脚
2. 确保共地连接
3. STM32 连接 ST-Link 调试器
4. ESP32 通过 USB 连接电脑

### 2. 烧写固件

**终端 1 - STM32:**
```bash
cd firmware/stm32
make uart-test
make flash
```

**终端 2 - ESP32:**
```bash
cd firmware/esp32
idf.py flash monitor
```

### 3. 验证通信

正常工作时，你应该看到：

**ESP32 输出:**
```
I (1234) ESP32_UART: =================================
I (1234) ESP32_UART: Kimi-Fly ESP32-C3 UART Bridge
I (1234) ESP32_UART: Target: STM32F411CEU6
I (1234) ESP32_UART: UART: TX=GPIO5, RX=GPIO4, Baud=115200
I (1234) ESP32_UART: =================================
I (2345) ESP32_UART: Initialization complete
I (2345) ESP32_UART: [RX] Heartbeat from STM32
I (4356) ESP32_UART: Stats - TX: 1, RX: 1, Active: YES
```

**STM32 调试输出 (921600 baud):**
```
================================
STM32F411 UART Test
Target: ESP32-C3 Communication
UART: USART2 (PA2/PA3) @ 115200
Debug: USART1 (PA9/PA10) @ 921600
================================

[INIT] Ready for communication

[TX] Heartbeat sent
[RX] Heartbeat from ESP32
[STAT] TX:1 RX:1 ERR:0 CONN:YES
```

## 故障排查

### 没有数据收发

1. **检查接线**
   - TX/RX 是否交叉连接（ESP32 TX → STM32 RX）
   - GND 是否连接

2. **检查波特率**
   - 双方波特率应都为 115200

3. **检查 GPIO 配置**
   - ESP32: GPIO4=RX, GPIO5=TX
   - STM32: PA2=TX, PA3=RX

4. **使用逻辑分析仪**
   - 检查信号电平（应为 3.3V）
   - 确认数据格式（8N1）

### 乱码或数据错误

1. 检查时钟配置是否正确
2. 确认校验位、停止位设置一致
3. 检查是否有干扰（缩短线缆，使用屏蔽线）

### 编译错误

**STM32:**
- 确保 PlatformIO 的 STM32Cube 包已安装
- 路径 `~/.platformio/packages/framework-stm32cubef4` 存在

**ESP32:**
- 确保 ESP-IDF 环境已设置: `. ~/esp/esp-idf/export.sh`
- 检查 ESP-IDF 版本: `idf.py --version` (需要 5.0+)

## 扩展功能

### 添加自定义消息类型

1. 在 `comm_protocol.h` 中添加新的消息类型：
```c
typedef enum {
    // ... existing types
    MSG_TYPE_CUSTOM = 0x10,  // 自定义消息
} msg_type_t;
```

2. 在收发双方添加处理代码

### 提高通信速率

可尝试提高波特率到 230400 或 460800：

```c
// 双方同时修改
#define UART_ST_BAUDRATE    230400
```

注意：波特率越高，对时钟精度和线缆质量要求越高。

## API 参考

### STM32 HAL UART

```c
// 初始化 UART
uart_config_t config = {
    .baudrate = 115200,
    .databits = UART_DATABITS_8,
    .stopbits = UART_STOPBITS_1_VAL,
    .parity = UART_PARITY_NONE_VAL,
    .mode = UART_MODE_TX_RX_VAL
};
uart_init(&wifi_uart, UART_INSTANCE_2, &config);

// 发送数据
uart_send(&wifi_uart, data, size, timeout_ms);

// 接收数据
uart_receive(&wifi_uart, buffer, size, timeout_ms);
```

### ESP32 UART

```c
// 配置 UART
const uart_config_t uart_config = {
    .baud_rate = 115200,
    .data_bits = UART_DATA_8_BITS,
    .parity = UART_PARITY_DISABLE,
    .stop_bits = UART_STOP_BITS_1,
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
};

// 安装驱动
uart_driver_install(UART_NUM_1, RX_BUF_SIZE, TX_BUF_SIZE, 0, NULL, 0);
uart_param_config(UART_NUM_1, &uart_config);
uart_set_pin(UART_NUM_1, TX_PIN, RX_PIN, -1, -1);

// 发送数据
uart_write_bytes(UART_NUM_1, data, length);

// 接收数据
uart_read_bytes(UART_NUM_1, buffer, length, timeout_ticks);
```
