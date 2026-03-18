# Task 002: STM32 UART HAL层修复

## 目标
根据Task 001诊断结果，修复STM32 USART2配置问题，使TX正常输出数据。

## 背景上下文

### 问题描述
- ESP32发送正常 (TX: 259条消息)
- ESP32接收为 0 (RX: 0)
- STM32 USART2 TX 未输出数据

### 相关代码
- 文件: `firmware/stm32/hal/uart.c` - UART HAL实现
  - `uart_init()`: USART2初始化
  - `uart_gpio_init()`: GPIO初始化
- 文件: `firmware/stm32/main/uart_comm_test.c` - 测试程序

### 依赖关系
- 前置任务: Task 001 (诊断完成，确定具体问题)
- 外部依赖: STM32Cube HAL库

### 硬件信息
- 涉及引脚: PA2 (USART2_TX), PA3 (USART2_RX)
- 外设: USART2
- 时钟: APB1 = 50MHz
- 波特率: 115200 (BRR = 50MHz / 115200 ≈ 434 = 0x1B2)

### 可能的修复点
根据Task 001诊断结果，可能需要修复:
1. **GPIO复用功能错误**: PA2未正确配置为AF7
2. **时钟使能顺序问题**: GPIO时钟未在配置前使能
3. **USART未使能**: UE位未设置
4. **发送未使能**: TE位未设置
5. **波特率计算错误**: BRR值不正确

## 具体修改要求

### 文件 1: `firmware/stm32/hal/uart.c`

根据Task 001诊断结果，选择性地修复以下问题:

#### 修复1: GPIO复用功能配置 (如PA2_AF不等于7)
```c
static void uart_gpio_init(uart_instance_t instance)
{
    GPIO_InitTypeDef gpio_init_struct;

    if (instance == UART_INSTANCE_1) {
        // ... 原有代码
    } else if (instance == UART_INSTANCE_2) {
        /* 使能GPIOA时钟 - 确保在GPIO配置前使能 */
        __HAL_RCC_GPIOA_CLK_ENABLE();

        /* PA2 - USART2_TX: 复用推挽输出 */
        gpio_init_struct.Pin = GPIO_PIN_2;
        gpio_init_struct.Mode = GPIO_MODE_AF_PP;
        gpio_init_struct.Pull = GPIO_PULLUP;
        gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;
        gpio_init_struct.Alternate = GPIO_AF7_USART2;  // 确认使用AF7
        HAL_GPIO_Init(GPIOA, &gpio_init_struct);

        /* PA3 - USART2_RX: 复用输入 */
        gpio_init_struct.Pin = GPIO_PIN_3;
        gpio_init_struct.Mode = GPIO_MODE_AF_PP;
        gpio_init_struct.Pull = GPIO_PULLUP;
        gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;
        gpio_init_struct.Alternate = GPIO_AF7_USART2;  // 确认使用AF7
        HAL_GPIO_Init(GPIOA, &gpio_init_struct);
    }
}
```

#### 修复2: USART使能 (如UE位未设置)
在 `uart_init()` 函数中，在 `HAL_UART_Init()` 后添加:
```c
    /* 初始化UART */
    hal_status = HAL_UART_Init(hal_huart);

    if (hal_status != HAL_OK) {
        huart->state = UART_STATE_ERROR;
        huart->error_code = uart_convert_hal_error(hal_huart->ErrorCode);
        return HAL_ERROR;
    }

    /* 确保USART使能 - 某些HAL版本需要手动使能 */
    if (instance == UART_INSTANCE_2) {
        SET_BIT(USART2->CR1, USART_CR1_UE);
    }

    /* 确保发送使能 */
    if (instance == UART_INSTANCE_2) {
        SET_BIT(USART2->CR1, USART_CR1_TE);
    }
```

#### 修复3: 时钟使能顺序 (如诊断显示时钟未使能)
确保时钟使能顺序正确:
```c
hal_status_t uart_init(uart_handle_t *huart, uart_instance_t instance, const uart_config_t *config)
{
    // ... 参数检查 ...

    /* 先使能GPIO时钟 */
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /* 根据实例选择HAL UART句柄 */
    if (instance == UART_INSTANCE_1) {
        // ...
        __HAL_RCC_USART1_CLK_ENABLE();
    } else {
        hal_huart = &huart2;
        hal_huart->Instance = USART2;
        /* 使能USART2时钟 (APB1) */
        __HAL_RCC_USART2_CLK_ENABLE();
    }

    /* 配置GPIO */
    uart_gpio_init(instance);

    // ... 后续配置 ...
}
```

### 文件 2: `firmware/stm32/main/uart_comm_test.c`

1. **移除或条件编译诊断代码**:
```c
// 在诊断完成后，将诊断代码移到条件编译块
#ifdef UART_DIAGNOSE
static void diagnose_usart2_config(void) { /* ... */ }
#endif
```

2. **添加发送测试验证**:
在 `main()` 函数中，在初始化后添加简单的发送测试:
```c
    /* Send initial status */
    esp32_send_status("STM32 initialized and ready");
    debug_puts("[INIT] Ready for communication\r\n\r\n");

    /* 简单的发送测试 - 验证TX是否工作 */
    debug_puts("[TEST] Sending test message to ESP32...\r\n");
    hal_status_t test_status = uart_send(&wifi_uart, (uint8_t*)"TEST\r\n", 6, 100);
    debug_printf("[TEST] Send result: %d (0=OK)\r\n", test_status);
```

## 完成标准 (必须可验证)

- [ ] 标准 1: 编译通过，无警告
```bash
cd firmware/stm32
make clean
make TARGET=uart_comm_test
```

- [ ] 标准 2: 烧写后STM32调试串口输出正常
```bash
make flash
```

- [ ] 标准 3: STM32 TX计数增加
  - 观察调试输出中的 `[STAT] TX:X RX:Y ERR:Z CONN:xxx`
  - TX计数应该持续增加

- [ ] 标准 4: ESP32能接收到数据 (关键验证)
  - 使用示波器或逻辑分析仪检查PA2引脚是否有信号输出
  - 或观察ESP32串口输出，确认RX计数增加

- [ ] 标准 5: 修复代码符合HAL规范
  - 不引入硬编码的寄存器操作 (除非必要)
  - 优先使用HAL库提供的API

- [ ] 标准 6: 代码审查通过

## 测试验证步骤

1. **编译并烧写修复后的固件**:
```bash
cd firmware/stm32
make clean
make TARGET=uart_comm_test
make flash
```

2. **监控STM32调试输出** (USART1 PA9/PA10 @ 921600):
预期输出:
```
================================
STM32F411 UART Test
Target: ESP32-C3 Communication
UART: USART2 (PA2/PA3) @ 115200
Debug: USART1 (PA9/PA10) @ 921600
================================

[UART] WiFi UART initialized (USART2, 115200)
[INIT] Ready for communication

[TEST] Sending test message to ESP32...
[TEST] Send result: 0 (0=OK)
[TX] Heartbeat OK (total:1 err:0)
[STAT] TX:1 RX:0 ERR:0 CONN:NO
```

3. **监控ESP32输出**:
```bash
cd firmware/esp32
idf.py monitor
```

4. **预期ESP32输出**:
```
I (1234) ESP32_UART: [RX] Received data from STM32
I (2345) ESP32_UART: Stats - TX: 1, RX: 1, Active: YES
```

5. **如果RX仍为0**:
   - 返回Task 001进一步诊断
   - 检查硬件连接 (TX/RX交叉、GND连接)

## 相关文件 (Hook范围检查用)
- `firmware/stm32/hal/uart.c`
- `firmware/stm32/hal/uart.h`
- `firmware/stm32/main/uart_comm_test.c`

## 注意事项
- 修复必须基于Task 001的诊断结果
- 如果诊断发现硬件连接问题，需记录到技术债务
- 保留调试输出直到双向通信验证通过
- 不要引入回归问题，确保原有功能正常

## 验收清单 (Reviewer使用)
- [ ] 修改在任务范围内
- [ ] 无新引入的未声明依赖
- [ ] 修复基于Task 001的诊断结果
- [ ] STM32 TX输出正常
- [ ] 无新的技术债务
