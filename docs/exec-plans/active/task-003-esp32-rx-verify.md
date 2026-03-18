# Task 003: ESP32 UART接收验证

## 目标
验证ESP32 UART1接收端配置正确，能够正常接收STM32发送的数据，实现双向通信。

## 背景上下文

### 问题描述
- ESP32发送正常 (TX: 259条消息)
- ESP32接收为 0 (RX: 0)
- 需要验证ESP32 UART1接收配置是否正确

### 相关代码
- 文件: `firmware/esp32/main/main.c` - ESP32主程序
  - UART初始化配置
  - 接收任务实现
- 文件: `firmware/platform/comm_protocol.h` - 通信协议定义

### 依赖关系
- 前置任务: Task 002 (STM32 UART修复完成，TX输出正常)
- 外部依赖: ESP-IDF UART驱动

### 硬件信息
- 涉及引脚: GPIO4 (RX), GPIO5 (TX)
- 外设: UART1
- 波特率: 115200
- 数据格式: 8N1 (8数据位，无校验，1停止位)

### 连接关系
```
ESP32-C3          STM32F411CEU6
--------          -------------
GPIO4 (RX)  <--   PA2 (TX)
GPIO5 (TX)  -->   PA3 (RX)
GND         <->   GND
```

## 具体修改要求

### 文件 1: `firmware/esp32/main/main.c`

1. **添加详细的UART配置检查日志** (在初始化函数中):
```c
static void log_uart_config(void)
{
    ESP_LOGI(TAG, "UART1 Configuration:");
    ESP_LOGI(TAG, "  Baud rate: %d", 115200);
    ESP_LOGI(TAG, "  Data bits: 8");
    ESP_LOGI(TAG, "  Parity: None");
    ESP_LOGI(TAG, "  Stop bits: 1");
    ESP_LOGI(TAG, "  Flow control: None");
    ESP_LOGI(TAG, "  TX Pin: GPIO%d", TX_PIN);
    ESP_LOGI(TAG, "  RX Pin: GPIO%d", RX_PIN);
    ESP_LOGI(TAG, "  RX buffer: %d bytes", RX_BUF_SIZE);
    ESP_LOGI(TAG, "  TX buffer: %d bytes", TX_BUF_SIZE);
}
```

2. **增强接收任务调试输出**:
在接收任务 `uart_rx_task` 中添加:
```c
static void uart_rx_task(void *pvParameters)
{
    uint8_t data[128];
    int length = 0;

    ESP_LOGI(TAG, "RX task started");

    while (1) {
        // 使用更短的超时以便快速响应
        length = uart_read_bytes(UART_NUM_1, data, sizeof(data), 10 / portTICK_PERIOD_MS);

        if (length > 0) {
            ESP_LOGI(TAG, "[RX] Received %d bytes", length);
            // 打印前几个字节用于调试
            if (length >= 4) {
                ESP_LOGI(TAG, "[RX] Data: %02X %02X %02X %02X ...",
                         data[0], data[1], data[2], data[3]);
            }
            rx_count++;
            last_rx_time = esp_timer_get_time();

            // 处理接收到的数据
            process_received_data(data, length);
        }

        vTaskDelay(1);  // 短暂延时让出CPU
    }
}
```

3. **添加连接状态诊断**:
添加一个诊断任务，每5秒输出一次详细的通信统计:
```c
static void diagnostic_task(void *pvParameters)
{
    uint32_t last_tx = 0;
    uint32_t last_rx = 0;
    int no_rx_count = 0;

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(5000));

        // 输出统计信息
        ESP_LOGI(TAG, "=== Communication Statistics ===");
        ESP_LOGI(TAG, "TX total: %lu (+%lu)", tx_count, tx_count - last_tx);
        ESP_LOGI(TAG, "RX total: %lu (+%lu)", rx_count, rx_count - last_rx);
        ESP_LOGI(TAG, "Connection: %s", is_connected ? "YES" : "NO");

        // 检查接收状态
        if (rx_count == last_rx) {
            no_rx_count++;
            if (no_rx_count >= 2) {
                ESP_LOGW(TAG, "WARNING: No data received for %d seconds", no_rx_count * 5);
                ESP_LOGW(TAG, "  Possible causes:");
                ESP_LOGW(TAG, "  1. STM32 not sending (check Task 002)");
                ESP_LOGW(TAG, "  2. Hardware connection issue (TX/RX cross, GND)");
                ESP_LOGW(TAG, "  3. Baud rate mismatch");
            }
        } else {
            no_rx_count = 0;
            ESP_LOGI(TAG, "STATUS: Receiving data normally");
        }

        last_tx = tx_count;
        last_rx = rx_count;
    }
}
```

4. **添加消息解析调试**:
在消息解析函数中添加调试输出:
```c
static void process_received_data(const uint8_t *data, int length)
{
    // 添加到解析缓冲区
    // ...

    // 尝试解析消息
    protocol_message_t msg;
    int consumed = comm_parse_message(rx_buffer, rx_index, &msg);

    if (consumed > 0) {
        ESP_LOGI(TAG, "[RX] Parsed message: type=0x%02X, len=%d",
                 msg.header.type, msg.header.length);

        switch (msg.header.type) {
            case MSG_TYPE_HEARTBEAT:
                ESP_LOGI(TAG, "[RX] Heartbeat from STM32");
                break;
            case MSG_TYPE_STATUS:
                ESP_LOGI(TAG, "[RX] Status: %s", msg.payload);
                break;
            // ... 其他消息类型
        }
    }
}
```

## 完成标准 (必须可验证)

- [ ] 标准 1: 编译通过，无警告
```bash
cd firmware/esp32
idf.py build
```

- [ ] 标准 2: 烧写后ESP32启动时输出详细的UART配置信息
```bash
idf.py flash monitor
```
预期输出:
```
I (1234) ESP32_UART: UART1 Configuration:
I (1234) ESP32_UART:   Baud rate: 115200
I (1234) ESP32_UART:   TX Pin: GPIO5
I (1234) ESP32_UART:   RX Pin: GPIO4
```

- [ ] 标准 3: 能够观察到接收任务的调试输出
  - 显示 `[RX] Received X bytes`
  - 显示接收到的数据内容

- [ ] 标准 4: ESP32统计输出显示RX计数增加
  - 每5秒输出的统计中RX计数持续增加
  - 显示 `STATUS: Receiving data normally`

- [ ] 标准 5: 双向通信正常
  - TX计数增加 (ESP32发送正常)
  - RX计数增加 (ESP32接收正常)
  - Connection状态显示为 `YES`

- [ ] 标准 6: 代码审查通过

## 测试验证步骤

1. **确保STM32已烧写Task 002修复后的固件**

2. **编译并烧写ESP32固件**:
```bash
cd firmware/esp32
idf.py build
idf.py flash monitor
```

3. **观察ESP32输出，验证**:

   a. **初始化成功**:
   ```
   I (1234) ESP32_UART: UART1 Configuration:
   I (1234) ESP32_UART:   Baud rate: 115200
   I (1234) ESP32_UART:   Data bits: 8
   I (2345) ESP32_UART: RX task started
   ```

   b. **接收数据正常**:
   ```
   I (3456) ESP32_UART: [RX] Received 8 bytes
   I (3456) ESP32_UART: [RX] Data: AA 55 01 04 ...
   I (3456) ESP32_UART: [RX] Parsed message: type=0x01, len=4
   I (3456) ESP32_UART: [RX] Heartbeat from STM32
   ```

   c. **统计信息正常**:
   ```
   I (5000) ESP32_UART: === Communication Statistics ===
   I (5000) ESP32_UART: TX total: 1 (+1)
   I (5000) ESP32_UART: RX total: 1 (+1)
   I (5000) ESP32_UART: Connection: YES
   I (5000) ESP32_UART: STATUS: Receiving data normally
   ```

4. **如果RX仍为0**:
   - 检查硬件连接:
     - ESP32 GPIO4 是否连接到 STM32 PA2
     - ESP32 GPIO5 是否连接到 STM32 PA3
     - GND是否共地
   - 检查STM32 TX是否正常 (Task 002验证)
   - 使用示波器检查PA2引脚信号

## 相关文件 (Hook范围检查用)
- `firmware/esp32/main/main.c`
- `firmware/esp32/CMakeLists.txt`
- `firmware/platform/comm_protocol.h`

## 注意事项
- 如果RX仍为0，需检查:
  1. STM32 TX是否正常 (Task 002验证)
  2. 硬件连接是否正确 (TX/RX交叉)
  3. ESP32 UART配置是否正确
- 调试日志在验证完成后可以降级或移除
- 记录任何发现的硬件问题到技术债务

## 验收清单 (Reviewer使用)
- [ ] 修改在任务范围内
- [ ] 无新引入的未声明依赖
- [ ] ESP32能正确接收STM32数据
- [ ] 双向通信统计正常
- [ ] 无新的技术债务
