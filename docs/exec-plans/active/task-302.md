# Task 302: LPS22HB SPI 多字节读取修复 (气压计 283hPa 异常)

## 目标
修复 LPS22HB 气压计 SPI 多字节连续读取问题，使气压读数恢复正常 (~1000hPa)。

## 背景上下文

### 相关代码
- 文件: `firmware/stm32/drivers/lps22hb.c` - SPI 读取实现
- 文件: `firmware/stm32/drivers/lps22hb.h` - 寄存器定义和 SPI 位掩码

### 问题说明
**当前现象**: 气压读数为 283hPa，正常值应为约 1000-1020hPa。

**根本原因**: LPS22HBTR 的 SPI 协议规定：
- bit7=1: 读操作
- bit6=1: 地址自动递增 (multi-byte read)
- bit6=0: 单字节读，地址不递增

当前代码 `LPS22HB_SPI_READ_BIT = 0x80`，只设了 bit7，未设 bit6。
对于多字节读取（读 5 字节气压+温度），未启用地址自动递增，导致重复读同一地址数据，气压数据解析错误。

**参考 LPS22HBTR 数据手册**:
```
SPI 地址字节格式:
  bit7  = RW   (1=读, 0=写)
  bit6  = MS   (1=自动递增, 0=不递增)
  bit5-0 = 寄存器地址
```

### 依赖关系
- 前置任务: 无
- 外部依赖: 无

### 硬件信息
- 涉及引脚: PA15 (SPI3_CS), PB3 (SCK), PB4 (MISO), PB5 (MOSI)
- SPI 模式: Mode 0 (CPOL=0, CPHA=0), 8-bit, MSB first
- 参考: `hardware-docs/pinout.md` 第 3.2 节

## 具体修改要求

### 文件 1: `firmware/stm32/drivers/lps22hb.h`
1. 新增多字节读取位宏定义:
   ```c
   #define LPS22HB_SPI_AUTO_INC_BIT    0x40U   /**< 地址自动递增位 (bit6=1, multi-byte) */
   ```
2. 新增组合宏:
   ```c
   #define LPS22HB_SPI_READ_MULTI      (LPS22HB_SPI_READ_BIT | LPS22HB_SPI_AUTO_INC_BIT)   /**< 多字节读: bit7=1, bit6=1 */
   ```

### 文件 2: `firmware/stm32/drivers/lps22hb.c`
1. 修改 `lps22hb_read_regs()` 函数中的地址字节，使用 `LPS22HB_SPI_READ_MULTI` 替代 `LPS22HB_SPI_READ_BIT`：
   ```c
   // 修改前:
   tx_byte = reg | LPS22HB_SPI_READ_BIT;
   // 修改后:
   tx_byte = reg | LPS22HB_SPI_READ_MULTI;
   ```
2. 单字节读 `lps22hb_read_reg()` 保持使用 `LPS22HB_SPI_READ_BIT`（不需要自动递增）

## 完成标准 (必须可验证)

- [ ] 标准 1: `lps22hb_read_regs()` 使用 0xC0|reg 作为地址字节 (bit7=1 读, bit6=1 递增)
- [ ] 标准 2: `lps22hb_read_reg()` 保持使用 0x80|reg (单字节读不变)
- [ ] 标准 3: 固件编译成功
- [ ] 标准 4: 气压读数恢复正常范围
  - 验证方法: 调试串口输出气压值在 950-1050 hPa 范围内
  - 验证方法: 温度值在合理范围 15-35°C
- [ ] 代码审查通过

## 相关文件 (Hook范围检查用)
- `firmware/stm32/drivers/lps22hb.h`
- `firmware/stm32/drivers/lps22hb.c`

## 注意事项
- WHO_AM_I 读取是单字节，不需要自动递增（保持不变）
- 只有 `lps22hb_read_regs()` 需要改，`lps22hb_read_reg()` 不变
- 改完后如果气压值仍不正常，检查 SPI Mode (CPOL/CPHA) 是否正确

## 验收清单 (Reviewer使用)
- [ ] 仅修改了多字节读函数的地址字节组合
- [ ] 单字节读函数未被修改
- [ ] 宏定义清晰，注释说明原因
- [ ] 无新引入的未声明依赖
