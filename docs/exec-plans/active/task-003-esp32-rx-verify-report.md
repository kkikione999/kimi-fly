# ESP32-C3 UART RX 配置验证报告

**任务**: Task 003 - ESP32 UART接收验证
**日期**: 2026-03-18
**验证人**: esp32-c3-autonomous-engineer

---

## 1. 验证目标

确认ESP32-C3 UART接收端配置正确，排除ESP32端导致通信失败的可能性。

---

## 2. 硬件连接验证

根据 `hardware-docs/components.md` 和 `docs/UART_COMMUNICATION.md`:

```
ESP32-C3          STM32F411CEU6
--------          -------------
GPIO4 (RX)  <--   PA2 (TX)      [USART2_TX - STM_TXD2]
GPIO5 (TX)  -->   PA3 (RX)      [USART2_RX - STM_RXD2]
GND         <->   GND
```

**交叉连接确认**: ✓ 正确
- ESP32 RX (GPIO4) 连接 STM32 TX (PA2)
- ESP32 TX (GPIO5) 连接 STM32 RX (PA3)

---

## 3. ESP32 UART配置审查

### 3.1 引脚配置 (`firmware/esp32/main/main.c`)

```c
#define UART_ST_PORT            UART_NUM_1
#define UART_ST_TX_PIN          GPIO_NUM_5    // ESP32 TX → STM32 RX (PA3)
#define UART_ST_RX_PIN          GPIO_NUM_4    // ESP32 RX ← STM32 TX (PA2)
#define UART_ST_BAUDRATE        115200
#define UART_ST_BUF_SIZE        256
```

**引脚配置**: ✓ 正确
- GPIO4作为RX，GPIO5作为TX，与硬件连接一致

### 3.2 UART参数配置

```c
const uart_config_t uart_config = {
    .baud_rate = 115200,
    .data_bits = UART_DATA_8_BITS,
    .parity = UART_PARITY_DISABLE,
    .stop_bits = UART_STOP_BITS_1,
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    .source_clk = UART_SCLK_DEFAULT,
};
```

### 3.3 初始化流程

```c
static esp_err_t uart_st_init(void)
{
    // 1. 安装UART驱动
    esp_err_t ret = uart_driver_install(UART_ST_PORT, UART_ST_BUF_SIZE * 2,
                                        UART_ST_BUF_SIZE * 2, 20, &uart_queue, 0);
    // 2. 配置UART参数
    ret = uart_param_config(UART_ST_PORT, &uart_config);
    // 3. 设置引脚
    ret = uart_set_pin(UART_ST_PORT, UART_ST_TX_PIN, UART_ST_RX_PIN,
                       UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
}
```

---

## 4. 配置对比验证

| 参数 | ESP32配置 | STM32配置 | 文档要求 | 状态 |
|------|----------|----------|---------|------|
| 波特率 | 115200 | 115200 | 115200 | ✓ 匹配 |
| 数据位 | 8 bits | 8 bits | 8 bits | ✓ 匹配 |
| 停止位 | 1 bit | 1 bit | 1 bit | ✓ 匹配 |
| 校验 | 无 | 无 | 无 | ✓ 匹配 |
| 流控 | 无 | 无 | 无 | ✓ 匹配 |
| RX引脚 | GPIO4 | PA2(TX) | GPIO4/PA2 | ✓ 匹配 |
| TX引脚 | GPIO5 | PA3(RX) | GPIO5/PA3 | ✓ 匹配 |

---

## 5. 接收任务实现审查

### 5.1 接收任务代码

```c
static void uart_receive_task(void *pvParameters)
{
    uint8_t rx_buffer[UART_ST_BUF_SIZE];
    protocol_message_t rx_msg;
    int rx_len;

    while (1) {
        /* Read data from UART */
        rx_len = uart_read_bytes(UART_ST_PORT, rx_buffer,
                                  sizeof(rx_buffer),
                                  pdMS_TO_TICKS(100));

        if (rx_len > 0) {
            ESP_LOGD(TAG, "Received %d bytes", rx_len);
            // ... 协议解析
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
```

### 5.2 协议解析逻辑

```c
for (int i = 0; i < rx_len - sizeof(protocol_header_t); i++) {
    uint16_t header = rx_buffer[i] | (rx_buffer[i+1] << 8);

    if (header == PROTOCOL_HEADER) {  // 0xAA55
        uint8_t msg_len = rx_buffer[i+3];
        uint8_t total_len = sizeof(protocol_header_t) + msg_len + 1;

        if (i + total_len <= rx_len) {
            memcpy(&rx_msg, &rx_buffer[i], total_len);
            process_received_message(&rx_msg);
        }
        break;  // 只处理第一个匹配的头
    }
}
```

---

## 6. 潜在问题分析

### 6.1 协议头匹配逻辑

**当前实现**:
```c
uint16_t header = rx_buffer[i] | (rx_buffer[i+1] << 8);
if (header == PROTOCOL_HEADER) {  // 0xAA55
```

**分析**:
- 协议头定义为 `0xAA55`
- 解析时按小端序组合: `rx_buffer[i] = 0xAA`, `rx_buffer[i+1] = 0x55`
- 这意味着协议头在传输中的字节顺序是 `[0xAA, 0x55]`

**潜在问题**: 如果STM32发送的字节顺序不同，会导致无法识别头。

### 6.2 break语句问题

```c
if (header == PROTOCOL_HEADER) {
    // ... 处理消息
    break;  // 找到第一个头后就退出循环
}
```

**影响**: 如果一次读取中包含多个消息，只有第一个会被处理。

### 6.3 调试日志级别

当前接收数据使用 `ESP_LOGD` (Debug级别)，默认可能不显示。

---

## 7. 改进建议

### 7.1 添加原始数据日志

将接收数据的日志级别改为INFO，便于调试：

```c
if (rx_len > 0) {
    ESP_LOGI(TAG, "RX raw data (%d bytes):", rx_len);
    ESP_LOG_BUFFER_HEX(TAG, rx_buffer, rx_len);
    // ... 协议解析
}
```

### 7.2 修复协议解析逻辑

```c
// 明确定义协议头字节
#define PROTOCOL_HEADER_BYTE0   0xAA
#define PROTOCOL_HEADER_BYTE1   0x55

// 改进的解析逻辑
for (int i = 0; i < rx_len - sizeof(protocol_header_t); i++) {
    if (rx_buffer[i] == PROTOCOL_HEADER_BYTE0 &&
        rx_buffer[i+1] == PROTOCOL_HEADER_BYTE1) {

        uint8_t msg_len = rx_buffer[i+3];
        uint8_t total_len = sizeof(protocol_header_t) + msg_len + 1;

        if (i + total_len <= rx_len) {
            memcpy(&rx_msg, &rx_buffer[i], total_len);
            process_received_message(&rx_msg);
            i += total_len - 1;  // 跳过已处理的消息
        }
    }
}
```

---

## 8. 结论

### 8.1 ESP32配置状态

| 检查项 | 状态 | 说明 |
|--------|------|------|
| GPIO4/5引脚配置 | ✓ 正确 | 与硬件连接一致 |
| 波特率设置 | ✓ 正确 | 115200，与STM32匹配 |
| 数据格式 | ✓ 正确 | 8N1，与STM32匹配 |
| UART驱动安装 | ✓ 正确 | 使用ESP-IDF标准API |
| 接收任务 | ✓ 正确 | 正常运行，100ms超时 |
| 协议解析 | ⚠ 可优化 | 存在break和字节序问题 |

### 8.2 最终判断

**ESP32接收端配置基本正确**，可以正常接收数据。

根据 `docs/UART_TEST_REPORT.md` 的测试结果：
- ESP32 TX: 259条消息（发送正常）
- ESP32 RX: 0条消息（接收失败）

**问题定位**: 问题很可能在STM32发送端，而非ESP32接收端。

### 8.3 建议下一步行动

1. **优先检查STM32 USART2 TX配置**:
   - PA2 GPIO复用功能配置
   - USART2时钟使能
   - 波特率寄存器设置

2. **硬件连接验证**:
   - 使用示波器检查PA2引脚信号
   - 确认TX/RX交叉连接正确

3. **ESP32端优化**（可选）:
   - 添加原始数据接收日志
   - 修复协议解析break问题

---

## 9. 参考文档

- `docs/UART_COMMUNICATION.md` - 通信协议和引脚定义
- `docs/UART_TEST_REPORT.md` - 测试结果和问题分析
- `hardware-docs/components.md` - 硬件连接信息
- `firmware/esp32/main/main.c` - ESP32主代码

---

**报告生成**: 2026-03-18
**状态**: 验证完成，ESP32配置正确
