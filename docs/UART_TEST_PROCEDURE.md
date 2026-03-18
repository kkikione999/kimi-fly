# ESP32-STM32 UART 通信测试步骤

## 测试前准备

### 硬件检查清单
- [ ] ESP32-C3 开发板
- [ ] STM32F411CEU6 开发板
- [ ] USB 数据线 x2
- [ ] 杜邦线 x3 (TX, RX, GND)
- [ ] ST-Link 调试器

### 接线图
```
ESP32-C3          STM32F411CEU6         电脑
--------          -------------         ----
GPIO4 (RX)  <--   PA2 (TX)
GPIO5 (TX)  -->   PA3 (RX)
GND         <-->  GND
USB
                ST-Link --> USB (调试)
                USB     --> USB (供电)
```

**重要**: TX/RX 要交叉连接！

---

## 第一步：编译固件

### 1.1 编译 STM32 固件

```bash
cd /Users/ll/kimi-fly/firmware/stm32

# 清理并编译
make clean
make TARGET=uart_comm_test

# 确认编译成功
ls -la build/uart_comm_test.bin
```

预期输出：
```
=== Firmware Size ===
   text    data     bss     dec     hex filename
   9192     100    2452   11744    2de0 build/uart_comm_test.elf
```

### 1.2 编译 ESP32 固件

```bash
cd /Users/ll/kimi-fly/firmware/esp32

# 设置环境（如果还没设置）
. ~/esp/esp-idf/export.sh

# 设置目标（只需一次）
idf.py set-target esp32c3

# 编译
idf.py build

# 确认编译成功
ls -la build/kimi_fly_esp32.bin
```

---

## 第二步：烧写固件

### 2.1 烧写 STM32（终端 1）

```bash
cd /Users/ll/kimi-fly/firmware/stm32
make flash
```

或使用 st-flash：
```bash
st-flash write build/uart_comm_test.bin 0x08000000
```

### 2.2 烧写 ESP32（终端 2）

```bash
cd /Users/ll/kimi-fly/firmware/esp32
idf.py flash
```

---

## 第三步：测试通信

### 3.1 打开调试输出监控（STM32）

STM32 调试串口输出到 USART1 (PA9/PA10)，波特率 921600。

```bash
# 查看串口设备
ls /dev/tty.* | grep -i usb

# 连接调试串口（替换为你的设备名）
minicom -b 921600 -D /dev/tty.usbserial-XXXX -8

# 或者使用 screen
screen /dev/tty.usbserial-XXXX 921600
```

### 3.2 打开 ESP32 监控

```bash
cd /Users/ll/kimi-fly/firmware/esp32
idf.py monitor
```

### 3.3 预期输出

**STM32 调试输出：**
```
================================
STM32F411 UART Test
Target: ESP32-C3 Communication
================================

[INIT] Ready for communication

[TX] Heartbeat sent
[RX] Heartbeat from ESP32
[STAT] TX:1 RX:1 ERR:0 CONN:YES
[TX] Heartbeat sent
[RX] Heartbeat from ESP32
[STAT] TX:2 RX:2 ERR:0 CONN:YES
```

**ESP32 输出：**
```
I (1234) ESP32_UART: =================================
I (1234) ESP32_UART: Kimi-Fly ESP32-C3 UART Bridge
I (1234) ESP32_UART: Target: STM32F411CEU6
I (2345) ESP32_UART: [RX] Heartbeat from STM32
I (4356) ESP32_UART: Stats - TX: 1, RX: 1, Active: YES
```

---

## 第四步：故障排查

### 问题 1：没有输出

**检查清单：**
```bash
# 1. 检查接线
# - 用万用表测量连通性
# - 确认 TX/RX 交叉连接

# 2. 检查供电
# - STM32 电源 LED 是否亮
# - ESP32 电源 LED 是否亮

# 3. 检查串口设备
ls /dev/tty.*

# 4. 检查波特率
# STM32: 调试 921600, 通信 115200
# ESP32: 通信 115200
```

### 问题 2：单向通信（只有发送没有接收）

**测试方法：**
1. 短路 ESP32 的 TX 和 RX（GPIO4 和 GPIO5）
2. 发送数据，看是否能收到回显
3. 同样测试 STM32（PA2 和 PA3 短接）

**代码修改测试** - 在 STM32 添加回环测试：
```c
// 在 main() 中添加测试代码
uint8_t test_data[] = {0x55, 0xAA, 0x01, 0x02};
while (1) {
    uart_send(&wifi_uart, test_data, 4, 100);
    HAL_Delay(1000);
}
```

### 问题 3：数据错误/乱码

**检查时钟配置：**
```c
// STM32 时钟必须是 100MHz
// 检查 system_clock_config() 输出
```

**检查 GPIO 复用：**
```bash
# 确认 GPIO 配置正确
# PA2 = AF7 (USART2_TX)
# PA3 = AF7 (USART2_RX)
```

### 问题 4：协议解析失败

**添加调试打印：**
```c
// 在 process_message() 中添加
printf("RX: %02X %02X %02X %02X\n",
       buffer[0], buffer[1], buffer[2], buffer[3]);
```

---

## 第五步：高级测试

### 5.1 压力测试

修改代码发送更多数据：

```c
// STM32 - 每秒发送 10 次心跳
if (now - last_heartbeat >= 100) {  // 100ms
    esp32_send_heartbeat();
}
```

### 5.2 大数据包测试

```c
// 发送 64 字节 payload
uint8_t large_payload[64];
for (int i = 0; i < 64; i++) {
    large_payload[i] = i;
}
protocol_message_t msg;
int len = comm_build_message(&msg, MSG_TYPE_DEBUG, large_payload, 64);
```

### 5.3 使用逻辑分析仪

如果有逻辑分析仪，捕获 UART 信号：
- 通道 1: STM32 PA2 (TX)
- 通道 2: ESP32 GPIO4 (RX)
- 采样率: 1MHz 以上
- 触发: 下降沿

---

## 测试记录表

| 测试项 | 预期结果 | 实际结果 | 状态 |
|--------|----------|----------|------|
| STM32 编译 | 无错误 | | |
| ESP32 编译 | 无错误 | | |
| STM32 烧写 | 成功 | | |
| ESP32 烧写 | 成功 | | |
| STM32 调试输出 | 有输出 | | |
| ESP32 日志输出 | 有输出 | | |
| 心跳包发送 | TX 计数增加 | | |
| 心跳包接收 | RX 计数增加 | | |
| 双向通信 | TX=RX | | |
| 5分钟稳定性 | 无断开 | | |

---

## 下一步

测试通过后，可以：
1. 添加 WiFi 功能到 ESP32
2. 添加传感器数据上报
3. 添加控制命令处理
