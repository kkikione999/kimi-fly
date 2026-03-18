# Task 001: STM32 USART2 UART诊断

## 目标
诊断STM32 USART2 TX不输出数据的根本原因，输出详细的寄存器状态报告，定位配置问题。

## 背景上下文

### 问题描述
- ESP32发送正常 (TX: 259条消息)
- ESP32接收为 0 (RX: 0)
- STM32 USART2 TX 未输出数据

### 相关代码
- 文件: `firmware/stm32/hal/uart.c` - UART HAL实现
  - `uart_gpio_init()`: GPIO初始化，PA2使用GPIO_AF7_USART2
  - `uart_init()`: USART2时钟使能在APB1总线
- 文件: `firmware/stm32/main/uart_comm_test.c` - 测试程序
  - 系统时钟配置: APB1 = 50MHz
  - USART2波特率: 115200

### 硬件信息
- 涉及引脚: PA2 (USART2_TX), PA3 (USART2_RX)
- 外设: USART2
- 时钟: APB1 = 50MHz
- 波特率: 115200 (BRR = 50MHz / 115200 ≈ 434)

### 代码规范
- 遵循 STM32 HAL 风格
- 寄存器访问使用 volatile
- 调试输出使用USART1 (PA9/PA10 @ 921600 baud)

## 具体修改要求

### 文件 1: `firmware/stm32/main/uart_comm_test.c`

1. **添加寄存器诊断函数** (在 `wifi_uart_init()` 函数后):
```c
/**
 * @brief 诊断USART2配置状态
 */
static void diagnose_usart2_config(void)
{
    debug_puts("\r
[DIAG] USART2 Configuration Report\r\n");
    debug_puts("====================================\r\n");

    // 1. 检查APB1时钟使能
    uint32_t apb1enr = RCC->APB1ENR;
    debug_printf("[DIAG] RCC->APB1ENR = 0x%08X\r\n", apb1enr);
    debug_printf("[DIAG] USART2EN bit (bit 17) = %d\r\n", (apb1enr >> 17) & 0x01);

    // 2. 检查GPIOA时钟使能
    uint32_t ahb1enr = RCC->AHB1ENR;
    debug_printf("[DIAG] RCC->AHB1ENR = 0x%08X\r\n", ahb1enr);
    debug_printf("[DIAG] GPIOAEN bit (bit 0) = %d\r\n", ahb1enr & 0x01);

    // 3. 检查GPIOA模式寄存器
    uint32_t moder = GPIOA->MODER;
    debug_printf("[DIAG] GPIOA->MODER = 0x%08X\r\n", moder);
    uint32_t pa2_mode = (moder >> 4) & 0x03;  // PA2对应bits[5:4]
    debug_printf("[DIAG] PA2 Mode (bits[5:4]) = %d (0=Input, 1=Output, 2=AF, 3=Analog)\r\n", pa2_mode);

    // 4. 检查GPIOA复用功能寄存器
    uint32_t afrl = GPIOA->AFR[0];
    debug_printf("[DIAG] GPIOA->AFR[0] = 0x%08X\r\n", afrl);
    uint32_t pa2_af = (afrl >> 8) & 0x0F;  // PA2对应bits[11:8]
    debug_printf("[DIAG] PA2 AF (bits[11:8]) = %d (7=USART2)\r\n", pa2_af);

    // 5. 检查USART2状态寄存器
    uint32_t sr = USART2->SR;
    debug_printf("[DIAG] USART2->SR = 0x%04X\r\n", sr);
    debug_printf("[DIAG] TXE (bit 7) = %d, TC (bit 6) = %d\r\n", (sr >> 7) & 1, (sr >> 6) & 1);

    // 6. 检查USART2控制寄存器1
    uint32_t cr1 = USART2->CR1;
    debug_printf("[DIAG] USART2->CR1 = 0x%04X\r\n", cr1);
    debug_printf("[DIAG] UE (bit 13) = %d, TE (bit 3) = %d, RE (bit 2) = %d\r\n",
                 (cr1 >> 13) & 1, (cr1 >> 3) & 1, (cr1 >> 2) & 1);

    // 7. 检查USART2波特率寄存器
    uint32_t brr = USART2->BRR;
    debug_printf("[DIAG] USART2->BRR = 0x%04X (expected: ~0x01B2 for 115200@50MHz)\r\n", brr);

    // 8. 检查系统时钟
    uint32_t cfgr = RCC->CFGR;
    debug_printf("[DIAG] RCC->CFGR = 0x%08X\r\n", cfgr);
    uint32_t ppre1 = (cfgr >> 10) & 0x07;
    debug_printf("[DIAG] PPRE1 (APB1 prescaler) = %d (0=/1, 4=/2, 5=/4, 6=/8, 7=/16)\r\n", ppre1);

    debug_puts("====================================\r\n");
}
```

2. **在 `main()` 函数中调用诊断函数**:
   - 在 `wifi_uart_init()` 调用成功后，添加 `diagnose_usart2_config();`

### 文件 2: `firmware/stm32/hal/uart.c`

1. **在 `uart_init()` 函数中添加调试输出** (在关键步骤后):
```c
    /* 根据实例选择HAL UART句柄 */
    if (instance == UART_INSTANCE_1) {
        // ... 原有代码
    } else {
        hal_huart = &huart2;
        hal_huart->Instance = USART2;
        /* 使能USART2时钟 (APB1) */
        __HAL_RCC_USART2_CLK_ENABLE();
        // 添加调试输出
        // #ifdef UART_DEBUG
        // printf("[UART] USART2 clock enabled\r\n");
        // #endif
    }
```

2. **在 `uart_gpio_init()` 函数中添加调试输出**:
```c
    // 在HAL_GPIO_Init调用后添加
    // #ifdef UART_DEBUG
    // printf("[UART] GPIOA configured for USART%d\r\n", instance == UART_INSTANCE_1 ? 1 : 2);
    // #endif
```

## 完成标准 (必须可验证)

- [ ] 标准 1: 编译通过，无警告
```bash
cd firmware/stm32
make clean
make TARGET=uart_comm_test
```

- [ ] 标准 2: 烧写后调试串口输出诊断报告
```bash
make flash
# 使用串口工具连接PA9/PA10 @ 921600 baud查看输出
```

- [ ] 标准 3: 诊断报告包含以下关键信息:
  - [ ] RCC->APB1ENR 的 USART2EN 位是否为1
  - [ ] RCC->AHB1ENR 的 GPIOAEN 位是否为1
  - [ ] GPIOA->MODER 的 PA2 模式是否为复用功能 (2)
  - [ ] GPIOA->AFR[0] 的 PA2 复用功能是否为 AF7 (7)
  - [ ] USART2->CR1 的 UE 位是否为1 (USART使能)
  - [ ] USART2->CR1 的 TE 位是否为1 (发送使能)
  - [ ] USART2->BRR 的值是否为 0x1B2 (434) 或相近值
  - [ ] RCC->CFGR 的 PPRE1 值 (确认APB1分频)

- [ ] 标准 4: 根据诊断结果输出问题定位结论
  - 明确指出哪个配置项异常
  - 提供修复建议

- [ ] 标准 5: 代码审查通过

## 相关文件 (Hook范围检查用)
- `firmware/stm32/main/uart_comm_test.c`
- `firmware/stm32/hal/uart.c`
- `firmware/stm32/hal/uart.h`

## 注意事项
- 诊断代码仅在调试阶段使用，使用条件编译 `#ifdef UART_DEBUG` 包裹
- 不要修改原有的初始化逻辑，只添加诊断输出
- 调试输出使用USART1 (PA9/PA10 @ 921600 baud)
- 诊断报告格式要清晰，便于快速定位问题

## 验收清单 (Reviewer使用)
- [ ] 修改在任务范围内
- [ ] 无新引入的未声明依赖
- [ ] 诊断代码可正常编译和运行
- [ ] 诊断报告包含所有要求的寄存器信息
- [ ] 无新的技术债务
