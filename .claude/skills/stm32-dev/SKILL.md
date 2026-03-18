---
name: stm32-dev
description: >
  STM32嵌入式开发全流程辅助技能。当用户涉及以下任何场景时必须使用此技能：编写或调试STM32 C/C++代码、配置STM32CubeMX/HAL/LL驱动、编写Makefile或CMake构建脚本、使用gcc-arm-none-eabi工具链编译、使用OpenOCD + ST-Link烧写固件、使用minicom串口调试、查询HAL/CMSIS/FreeRTOS函数用法（调用context7 MCP）、排查编译报错或硬件连接问题。只要用户提到STM32、HAL_、CMSIS、OpenOCD、ST-Link、minicom、arm-none-eabi中的任何一个关键词，立刻加载此技能。
compatibility: "bash; gcc-arm-none-eabi; openocd; stlink-tools; minicom; context7 MCP (optional)"
---

# STM32 嵌入式开发技能

> **首要原则**：每次涉及不熟悉的 HAL / LL / CMSIS / FreeRTOS 函数时，**先调用 context7 MCP 查询文档**，再编写代码。不要凭记忆猜测 API 签名。

---

## 1. 工具链快速参考

### 1.1 gcc-arm-none-eabi 编译

```bash
# 编译单个文件（调试模式）
arm-none-eabi-gcc \
  -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard \
  -DSTM32F407xx \
  -DUSE_HAL_DRIVER \
  -ICore/Inc -IDrivers/STM32F4xx_HAL_Driver/Inc \
  -IDrivers/CMSIS/Device/ST/STM32F4xx/Include \
  -IDrivers/CMSIS/Include \
  -Wall -Wextra -Og -g3 -gdwarf-2 \
  -ffunction-sections -fdata-sections \
  -c Core/Src/main.c -o build/main.o

# 链接（需要链接脚本）
arm-none-eabi-gcc \
  -T STM32F407VGTx_FLASH.ld \
  -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard \
  --specs=nano.specs -lc -lm -lnosys \
  -Wl,--gc-sections -Wl,-Map=build/output.map \
  build/*.o -o build/firmware.elf

# 生成 .bin / .hex
arm-none-eabi-objcopy -O ihex   build/firmware.elf build/firmware.hex
arm-none-eabi-objcopy -O binary build/firmware.elf build/firmware.bin

# 查看固件大小
arm-none-eabi-size build/firmware.elf
```

**常用 -mcpu 值**（按芯片系列选择）：

| 系列 | -mcpu | 浮点选项 |
|------|-------|----------|
| STM32F0/G0 | cortex-m0 / cortex-m0plus | 无 |
| STM32F1 | cortex-m3 | 无 |
| STM32F3/L4 | cortex-m4 | -mfpu=fpv4-sp-d16 -mfloat-abi=hard |
| STM32F4/F7(单精度) | cortex-m4/m7 | -mfpu=fpv4-sp-d16 -mfloat-abi=hard |
| STM32F7/H7(双精度) | cortex-m7 | -mfpu=fpv5-d16 -mfloat-abi=hard |
| STM32H7/U5 | cortex-m33/m55 | -mfpu=fpv5-d16 -mfloat-abi=hard |

---

### 1.2 OpenOCD + ST-Link 烧写与调试

```bash
# ① 列出已连接的 ST-Link 设备
st-info --probe

# ② 用 OpenOCD 烧写 .bin（通用方式，适合所有 STM32）
openocd \
  -f interface/stlink.cfg \
  -f target/stm32f4x.cfg \
  -c "program build/firmware.bin verify reset exit 0x08000000"

# 烧写 .elf（自动解析地址，推荐）
openocd \
  -f interface/stlink.cfg \
  -f target/stm32f4x.cfg \
  -c "program build/firmware.elf verify reset exit"

# ③ 仅复位芯片（不烧写）
openocd \
  -f interface/stlink.cfg \
  -f target/stm32f4x.cfg \
  -c "init; reset run; exit"

# ④ 启动 GDB 服务器（用于调试）
openocd \
  -f interface/stlink.cfg \
  -f target/stm32f4x.cfg
# 另开终端：
arm-none-eabi-gdb build/firmware.elf \
  -ex "target extended-remote :3333" \
  -ex "monitor reset halt" \
  -ex "load" \
  -ex "continue"
```

**常用 target 配置文件**（`-f target/xxx.cfg`）：

| 系列 | cfg 文件 |
|------|---------|
| STM32F0 | stm32f0x.cfg |
| STM32F1 | stm32f1x.cfg |
| STM32F3 | stm32f3x.cfg |
| STM32F4 | stm32f4x.cfg |
| STM32F7 | stm32f7x.cfg |
| STM32G0 | stm32g0x.cfg |
| STM32G4 | stm32g4x.cfg |
| STM32H7 | stm32h7x.cfg |
| STM32L4 | stm32l4x.cfg |
| STM32U5 | stm32u5x.cfg |
| STM32WB | stm32wbx.cfg |

---

### 1.3 st-flash（stlink-tools，轻量烧写）

```bash
# 烧写 .bin（指定起始地址）
st-flash write build/firmware.bin 0x08000000

# 擦除全片
st-flash erase

# 复位
st-flash reset

# 读取固件（导出为 .bin）
st-flash read backup.bin 0x08000000 <size_bytes>
```

---

### 1.4 minicom 串口调试

```bash
# 查看可用串口设备
ls /dev/tty* | grep -E "USB|ACM|AMA"

# 启动 minicom（115200 8N1，关闭硬件流控）
minicom -b 115200 -D /dev/ttyUSB0 -8

# 或临时配置（不保存）
minicom -s   # 进入设置菜单

# 常用 minicom 快捷键（Ctrl+A 前缀）
#   Ctrl+A Z  → 帮助菜单
#   Ctrl+A X  → 退出
#   Ctrl+A W  → 切换换行
#   Ctrl+A L  → 保存日志到文件
#   Ctrl+A E  → 本地回显开关
```

**minicom 配置文件** (`~/.minirc.dfl`)：
```
pu port             /dev/ttyUSB0
pu baudrate         115200
pu bits             8
pu parity           N
pu stopbits         1
pu rtscts           No
pu xonxoff          No
```

---

## 2. 使用 context7 MCP 查询函数文档

> 每当需要使用 HAL / LL / CMSIS / FreeRTOS / LWIP 等库的函数时，**必须**先通过 context7 MCP 查询，而不是依赖训练记忆。

### 查询流程

```
1. 调用 context7 的 resolve-library-id 工具，找到对应库的 ID
   示例：query = "STM32 HAL Driver" 或 "FreeRTOS"

2. 调用 context7 的 get-library-docs 工具，按 topic 查询
   示例：topic = "HAL_UART_Transmit" 或 "xTaskCreate"

3. 根据返回的文档内容编写代码
```

### 常见查询示例

| 需要了解的函数 | context7 query 建议 |
|---------------|-------------------|
| HAL_UART_Transmit / Receive | "STM32 HAL UART transmit receive DMA" |
| HAL_TIM_PWM_Start | "STM32 HAL Timer PWM configuration" |
| HAL_I2C_Master_Transmit | "STM32 HAL I2C master transmit" |
| HAL_SPI_TransmitReceive | "STM32 HAL SPI full duplex" |
| HAL_ADC_Start_DMA | "STM32 HAL ADC DMA conversion" |
| HAL_GPIO_WritePin | "STM32 HAL GPIO write pin" |
| xTaskCreate / vTaskDelay | "FreeRTOS task creation delay" |
| HAL_RCC_ClockConfig | "STM32 HAL RCC clock configuration" |

---

## 3. 典型项目结构（STM32CubeMX 生成）

```
project/
├── Core/
│   ├── Inc/          ← main.h, stm32f4xx_hal_conf.h, ...
│   └── Src/          ← main.c, stm32f4xx_hal_msp.c, stm32f4xx_it.c, ...
├── Drivers/
│   ├── CMSIS/
│   └── STM32F4xx_HAL_Driver/
├── Middlewares/      ← FreeRTOS / LWIP / USB（如有）
├── build/            ← 编译输出（.elf .hex .bin .map）
├── STM32F407VGTx_FLASH.ld   ← 链接脚本
├── Makefile          ← 或 CMakeLists.txt
└── openocd.cfg       ← 可选，固化烧写配置
```

---

## 4. Makefile 最简模板

```makefile
TARGET   = firmware
BUILD_DIR = build

# 工具链前缀
PREFIX   = arm-none-eabi-
CC       = $(PREFIX)gcc
OBJCOPY  = $(PREFIX)objcopy
SIZE     = $(PREFIX)size

# 芯片参数（按实际修改）
CPU      = -mcpu=cortex-m4
FPU      = -mfpu=fpv4-sp-d16
FLOAT    = -mfloat-abi=hard
MCU      = $(CPU) -mthumb $(FPU) $(FLOAT)

# 宏定义
DEFS     = -DSTM32F407xx -DUSE_HAL_DRIVER

# 头文件路径
INCS     = -ICore/Inc \
           -IDrivers/STM32F4xx_HAL_Driver/Inc \
           -IDrivers/CMSIS/Device/ST/STM32F4xx/Include \
           -IDrivers/CMSIS/Include

# 源文件（按实际修改）
C_SRCS   = $(wildcard Core/Src/*.c) \
           $(wildcard Drivers/STM32F4xx_HAL_Driver/Src/*.c)

OBJS     = $(patsubst %.c,$(BUILD_DIR)/%.o,$(C_SRCS))

# 编译标志
CFLAGS   = $(MCU) $(DEFS) $(INCS) -Wall -Og -g3 -gdwarf-2 \
           -ffunction-sections -fdata-sections

# 链接标志
LDSCRIPT = STM32F407VGTx_FLASH.ld
LDFLAGS  = $(MCU) -T$(LDSCRIPT) --specs=nano.specs \
           -lc -lm -lnosys -Wl,--gc-sections \
           -Wl,-Map=$(BUILD_DIR)/$(TARGET).map

all: $(BUILD_DIR)/$(TARGET).elf
	$(SIZE) $<

$(BUILD_DIR)/$(TARGET).elf: $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) -o $@
	$(OBJCOPY) -O ihex   $@ $(BUILD_DIR)/$(TARGET).hex
	$(OBJCOPY) -O binary $@ $(BUILD_DIR)/$(TARGET).bin

$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $@

flash: all
	openocd -f interface/stlink.cfg -f target/stm32f4x.cfg \
	  -c "program $(BUILD_DIR)/$(TARGET).elf verify reset exit"

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all flash clean
```

---

## 5. 常见错误与解决方案

### 编译错误

| 错误信息 | 原因 | 解决 |
|---------|------|------|
| `undefined reference to HAL_xxx` | 未编译对应 HAL 驱动文件 | 将 `stm32xxx_hal_yyy.c` 加入 C_SRCS |
| `multiple definition of _exit` | --specs 冲突 | 改用 `--specs=nosys.specs` 或删除自定义 `_exit` |
| `cannot open linker script` | 链接脚本路径错误 | 检查 -T 参数或当前工作目录 |
| `region FLASH overflow` | 固件太大 | 启用 -Os 优化，或检查是否有大数组放在 Flash |

### 烧写错误

| 错误信息 | 原因 | 解决 |
|---------|------|------|
| `Error: open failed` (OpenOCD) | ST-Link 未识别 | `lsusb` 检查，重插，检查 udev 规则 |
| `Error: jtag status contains invalid mode` | 目标芯片无响应 | 确认供电，检查 SWD 接线（SWDIO/SWDCLK/GND） |
| `SRAM run failed` | 对 SRAM 地址调用 program | 确认烧写地址为 `0x08000000` |
| `stlink_open: Couldn't find ST-Link` | stlink-tools 版本过旧 | 升级：`sudo apt upgrade stlink-tools` |

### minicom 问题

| 现象 | 原因 | 解决 |
|------|------|------|
| 没有输出 | 波特率不匹配 | 确认代码中 `huart.Init.BaudRate` |
| 乱码 | 数据位/停止位不对 | 确认 8N1 |
| 无法输入 | 硬件流控开启 | 在 minicom 设置中关闭 RTS/CTS |
| Permission denied /dev/ttyUSB0 | 当前用户不在 dialout 组 | `sudo usermod -aG dialout $USER`，重新登录 |

---

## 6. 代码编写规范

1. **中断回调**：HAL 回调（`HAL_UART_RxCpltCallback` 等）必须在 `stm32xxx_it.c` 调用 HAL 的 IT 处理函数之后才能触发。不要在回调内做阻塞操作。

2. **DMA 使用**：启动 DMA 传输前，确认缓冲区在整个传输期间有效（不是栈上的临时变量）。

3. **时钟配置**：修改时钟树时先调用 context7 查询 `HAL_RCC_ClockConfig` 和 `SystemClock_Config`，避免配置错误导致芯片死锁。

4. **Volatile**：所有在中断和主循环之间共享的变量必须声明为 `volatile`。

5. **FreeRTOS 与 HAL 结合**：使用 FreeRTOS 时，`HAL_Delay` 依赖 `SysTick`；需要将 FreeRTOS tick 配置为使用其他定时器（如 TIM6）以避免冲突。调用 context7 查询 `configUSE_TICKLESS_IDLE` 相关文档。

---

## 7. 参考文档（按需加载）

- `references/hal-api-patterns.md` — 常用 HAL 外设配置代码片段（UART/SPI/I2C/TIM/ADC）
- `references/openocd-advanced.md` — OpenOCD 高级调试：断点、内存查看、RTT

如需上述文件的内容，直接 `view` 对应路径即可（首次使用时由用户或 AI 创建）。
