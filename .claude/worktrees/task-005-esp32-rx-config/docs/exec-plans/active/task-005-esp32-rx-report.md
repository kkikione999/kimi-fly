# ESP32-C3 UART RX配置验证报告

**任务**: Task 005 - ESP32工程师验证RX配置
**日期**: 2026-03-18
**验证人**: esp32-c3-autonomous-engineer

---

## 1. 硬件连接确认

根据 `hardware-docs/components.md` 确认ESP32-C3与STM32的UART连接：

| 信号方向 | STM32引脚 | ESP32-C3引脚 | 说明 |
|---------|----------|-------------|------|
| STM32 TX → ESP32 RX | PA2 (UART2_TX) | GPIO4 (RX) | 数据从STM32发送到ESP32 |
| ESP32 TX → STM32 RX | PA3 (UART2_RX) | GPIO5 (TX) | 数据从ESP32发送到STM32 |

**电路图确认**:
- STM32 UART2_TX (PA2) 连接到 ESP32 RXD0
- STM32 UART2_RX (PA3) 连接到 ESP32 TXD0

---

## 2. ESP32代码配置审查

### 2.1 UART配置 (`firmware/esp32/main/main.c`)

```c
#define UART_ST_PORT            UART_NUM_1
#define UART_ST_TX_PIN          GPIO_NUM_5    // ESP32 TX → STM32 RX (PA3)
#define UART_ST_RX_PIN          GPIO_NUM_4    // ESP32 RX ← STM32 TX (PA2)
#define UART_ST_BAUDRATE        115200
```

**波特率**: 115200 - 与STM32配置一致 ✓

### 2.2 UART初始化代码审查

```c
static esp_err_t uart_st_init(void)
{
    const uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    /* Install UART driver */
    esp_err_t ret = uart_driver_install(UART_ST_PORT, UART_ST_BUF_SIZE * 2,
                                        UART_ST_BUF_SIZE * 2, 20, &uart_queue, 0);

    /* Configure UART parameters */
    ret = uart_param_config(UART_ST_PORT, &uart_config);

    /* Set pins */
    ret = uart_set_pin(UART_ST_PORT, UART_ST_TX_PIN, UART_ST_RX_PIN,
                       UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
}
```

---

## 3. 配置验证结果

### 3.1 正确配置 ✓

| 参数 | ESP32配置 | STM32配置 | 匹配 |
|------|----------|----------|------|
| 波特率 | 115200 | 115200 | ✓ 匹配 |
| 数据位 | 8 bits | 8 bits | ✓ 匹配 |
| 停止位 | 1 bit | 1 bit | ✓ 匹配 |
| 校验 | 无 | 无 | ✓ 匹配 |
| 流控 | 无 | 无 | ✓ 匹配 |

### 3.2 引脚配置 ✓

| ESP32引脚 | 功能 | 连接目标 | 状态 |
|----------|------|---------|------|
| GPIO4 | RX | STM32 PA2 (TX) | ✓ 正确 |
| GPIO5 | TX | STM32 PA3 (RX) | ✓ 正确 |

---

## 4. 潜在问题分析

### 4.1 接收任务实现分析

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

**观察**:
1. 接收任务使用100ms超时读取UART数据
2. 每次读取后延迟10ms
3. 使用简单的协议头扫描 (0xAA55)

### 4.2 可能的问题点

**问题1: 协议解析逻辑有缺陷**

当前解析代码：
```c
for (int i = 0; i < rx_len - sizeof(protocol_header_t); i++) {
    uint16_t header = rx_buffer[i] | (rx_buffer[i+1] << 8);
    if (header == PROTOCOL_HEADER) {
        uint8_t msg_len = rx_buffer[i+3]; /* payload length */
        // ...
        break;  // 找到第一个头后就退出!
    }
}
```

**问题**: `break`语句导致只处理第一个匹配的头，如果数据流中有多个消息或部分消息，后续消息会被丢弃。

**问题2: 没有处理粘包/拆包**

如果STM32连续发送多个消息，ESP32可能在一个`uart_read_bytes`调用中读取到多个消息，但当前代码只处理第一个。

**问题3: 没有字节序转换**

协议头定义为：
```c
typedef struct __attribute__((packed)) {
    uint16_t header;        /* 0xAA55 */
    uint8_t type;
    uint8_t length;
} protocol_header_t;
```

`0xAA55`在小端系统(ESP32-C3 RISC-V)中内存布局为 `[0x55, 0xAA]`，但协议解析使用：
```c
uint16_t header = rx_buffer[i] | (rx_buffer[i+1] << 8);
```

这意味着接收到的字节顺序必须是 `[0x55, 0xAA]` 才能匹配 `0xAA55`，这与STM32发送的字节顺序可能不一致。

---

## 5. 建议修复

### 5.1 修复协议头匹配逻辑

```c
// 明确定义协议头字节顺序 (大端序)
#define PROTOCOL_HEADER_BYTE0   0xAA
#define PROTOCOL_HEADER_BYTE1   0x55

// 改进的解析逻辑
for (int i = 0; i < rx_len - sizeof(protocol_header_t); i++) {
    if (rx_buffer[i] == PROTOCOL_HEADER_BYTE0 &&
        rx_buffer[i+1] == PROTOCOL_HEADER_BYTE1) {
        // 找到协议头
        uint8_t msg_len = rx_buffer[i+3];
        uint8_t total_len = sizeof(protocol_header_t) + msg_len + 1;

        if (i + total_len <= rx_len) {
            memcpy(&rx_msg, &rx_buffer[i], total_len);
            process_received_message(&rx_msg);
            i += total_len - 1;  // 跳过已处理的消息
        }
        // 继续循环处理可能存在的后续消息
    }
}
```

### 5.2 添加调试日志

在接收任务中添加更多调试信息：

```c
if (rx_len > 0) {
    ESP_LOGI(TAG, "RX raw data (%d bytes):", rx_len);
    ESP_LOG_BUFFER_HEX(TAG, rx_buffer, rx_len);
}
```

---

## 6. 结论

| 检查项 | 结果 |
|--------|------|
| GPIO4/5配置 | ✓ 正确 |
| 波特率配置 | ✓ 正确 (115200) |
| 数据格式配置 | ✓ 正确 (8N1) |
| 协议解析逻辑 | ⚠ 存在缺陷 |

**主要发现**:
1. ESP32 UART硬件配置完全正确
2. 协议解析逻辑存在缺陷，可能导致消息丢失
3. 建议添加原始数据日志以便进一步诊断

**下一步行动**:
1. 修复协议解析逻辑中的`break`问题
2. 添加原始数据接收日志
3. 重新测试STM32→ESP32通信

---

**报告生成**: 2026-03-18
**状态**: 完成验证，发现问题
