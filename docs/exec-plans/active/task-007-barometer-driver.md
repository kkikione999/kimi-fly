# Task 007: 气压计驱动 (LPS22HBTR)

## 目标
实现LPS22HBTR气压计传感器的驱动程序，提供气压和温度数据读取功能。

## 背景上下文

### 相关代码
- 文件: `/Users/ll/kimi-fly/firmware/stm32/hal/i2c.c` - I2C1 HAL实现
- 文件: `/Users/ll/kimi-fly/firmware/stm32/hal/i2c.h` - I2C接口定义，包含LPS22HBTR地址定义
- 参考: 现有HAL代码使用寄存器直接操作风格

### 依赖关系
- 前置任务: Task 004 (I2C HAL) - 已完成
- 外部依赖: I2C1 HAL接口
- 硬件接口: I2C1

### 硬件信息
- **芯片型号**: LPS22HBTR
- **接口**: I2C1
- **引脚连接**:
  - SCL: PB6 (STMI2C_SCL1)
  - SDA: PB7 (STMI2C_SDA1)
  - INT_DRDY: BARO_INT (中断/数据就绪，可选使用)
- **I2C地址**: 0x5C (SA0=GND, 电路中R52将SA0接地)
- **WHO_AM_I寄存器**: 0x0F，预期值 0xB1

### LPS22HBTR 关键寄存器
| 寄存器 | 地址 | 说明 |
|--------|------|------|
| WHO_AM_I | 0x0F | 设备ID (0xB1) |
| CTRL_REG1 | 0x10 | 控制寄存器1 (ODR, BDU, EN_LPFP) |
| CTRL_REG2 | 0x11 | 控制寄存器2 (ONE_SHOT, SWRESET) |
| CTRL_REG3 | 0x12 | 控制寄存器3 (中断配置) |
| STATUS | 0x27 | 状态寄存器 |
| PRESS_OUT_XL | 0x28 | 气压数据低字节 |
| PRESS_OUT_L | 0x29 | 气压数据中字节 |
| PRESS_OUT_H | 0x2A | 气压数据高字节 |
| TEMP_OUT_L | 0x2B | 温度数据低字节 |
| TEMP_OUT_H | 0x2C | 温度数据高字节 |

### 输出数据率 (ODR) 配置
| ODR[2:0] | 气压计ODR | 温度ODR |
|----------|-----------|---------|
| 000 | One-shot | One-shot |
| 001 | 1 Hz | 1 Hz |
| 010 | 10 Hz | 10 Hz |
| 011 | 25 Hz | 25 Hz |
| 100 | 50 Hz | 50 Hz |
| 101 | 75 Hz | 75 Hz |

### 代码规范
- 遵循 STM32 HAL 风格
- 寄存器访问使用 volatile
- 使用 I2C HAL 接口进行通信 (i2c_mem_read/i2c_mem_write)
- 提供初始化、读取、配置等标准接口

## 具体修改要求

### 文件 1: `/Users/ll/kimi-fly/firmware/stm32/drivers/lps22hb.h`
1. 定义 LPS22HBTR 寄存器地址常量
2. 定义 ODR (输出数据率) 枚举
3. 定义 LPS22HBTR 句柄结构体 (包含i2c_handle_t指针和设备地址)
4. 定义数据读取结构体 (气压、温度)
5. 声明API函数:
   - `lps22hb_init()` - 初始化传感器
   - `lps22hb_deinit()` - 反初始化
   - `lps22hb_read_id()` - 读取WHO_AM_I
   - `lps22hb_read_data()` - 读取气压和温度数据
   - `lps22hb_set_odr()` - 设置输出数据率
   - `lps22hb_one_shot()` - 单次测量模式触发
   - `lps22hb_data_ready()` - 检查数据是否就绪

### 文件 2: `/Users/ll/kimi-fly/firmware/stm32/drivers/lps22hb.c`
1. 包含 `lps22hb.h` 和 `i2c.h`
2. 实现寄存器读写函数 (内部使用，基于i2c_mem_read/i2c_mem_write)
3. 实现 `lps22hb_init()`:
   - 验证 WHO_AM_I (应为0xB1)
   - 配置默认ODR (推荐10Hz或25Hz)
   - 启用BDU (Block Data Update) 确保数据一致性
4. 实现 `lps22hb_read_data()`:
   - 读取气压数据 (24位，需要3字节)
   - 读取温度数据 (16位，需要2字节)
   - 数据格式: 气压为24位有符号数，温度为16位有符号数
5. 实现ODR配置函数
6. 实现数据转换函数:
   - 气压原始值转hPa (除以4096)
   - 温度原始值转摄氏度 (除以100)

## 完成标准 (必须可验证)

- [ ] 代码编译通过，无警告
- [ ] `lps22hb_read_id()` 返回 0xB1
- [ ] 提供数据读取测试代码片段 (可在main中调用验证)
- [ ] 气压数据转换为合理范围 (约950-1050 hPa，取决于海拔)
- [ ] 温度数据转换为合理范围 (室温范围)
- [ ] 代码遵循项目HAL风格规范

## 相关文件 (Hook范围检查用)
- `/Users/ll/kimi-fly/firmware/stm32/drivers/lps22hb.h`
- `/Users/ll/kimi-fly/firmware/stm32/drivers/lps22hb.c`
- `/Users/ll/kimi-fly/firmware/stm32/hal/i2c.h` (只读引用)

## 注意事项
- LPS22HBTR 气压数据是24位有符号数，需要特殊处理符号扩展
- 使用BDU (Block Data Update) 位确保读取时数据不会更新
- 单次测量模式适合低功耗应用，连续模式适合实时飞行控制
- 推荐ODR: 25Hz (平衡响应速度和功耗)
- 气压计数据需要温度补偿，芯片内部已处理，直接读取即可

## 验收清单 (Reviewer使用)
- [ ] 修改在任务范围内
- [ ] 无新引入的未声明依赖
- [ ] 寄存器地址定义正确
- [ ] WHO_AM_I验证逻辑正确
- [ ] 24位气压数据读取和符号扩展正确
- [ ] 数据转换公式正确
- [ ] 无新的技术债务
