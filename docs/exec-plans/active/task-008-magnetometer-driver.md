# Task 008: 磁力计驱动 (QMC5883P)

## 目标
实现QMC5883P磁力计传感器的驱动程序，提供三轴磁场数据读取功能。

## 背景上下文

### 相关代码
- 文件: `/Users/ll/kimi-fly/firmware/stm32/hal/i2c.c` - I2C1 HAL实现
- 文件: `/Users/ll/kimi-fly/firmware/stm32/hal/i2c.h` - I2C接口定义，包含QMC5883P地址定义
- 参考: 现有HAL代码使用寄存器直接操作风格

### 依赖关系
- 前置任务: Task 004 (I2C HAL) - 已完成
- 外部依赖: I2C1 HAL接口
- 硬件接口: I2C1 (与气压计共享)

### 硬件信息
- **芯片型号**: QMC5883P
- **接口**: I2C1
- **引脚连接**:
  - SCL: PB6 (STMI2C_SCL1, 与气压计共享)
  - SDA: PB7 (STMI2C_SDA1, 与气压计共享)
- **I2C地址**: 0x0D (固定地址)
- **CHIP_ID寄存器**: 0x0D，预期值需要确认 (通常为0xFF或其他特定值)

### QMC5883P 关键寄存器
| 寄存器 | 地址 | 说明 |
|--------|------|------|
| CHIP_ID | 0x0D | 芯片ID |
| DATA_OUT_X_L | 0x00 | X轴数据低字节 |
| DATA_OUT_X_H | 0x01 | X轴数据高字节 |
| DATA_OUT_Y_L | 0x02 | Y轴数据低字节 |
| DATA_OUT_Y_H | 0x03 | Y轴数据高字节 |
| DATA_OUT_Z_L | 0x04 | Z轴数据低字节 |
| DATA_OUT_Z_H | 0x05 | Z轴数据高字节 |
| STATUS | 0x06 | 状态寄存器 |
| TEMP_OUT_L | 0x07 | 温度数据低字节 |
| TEMP_OUT_H | 0x08 | 温度数据高字节 |
| CTRL_REG1 | 0x09 | 控制寄存器1 (模式、ODR、量程) |
| CTRL_REG2 | 0x0A | 控制寄存器2 (软复位、中断) |
| SET_PERIOD | 0x0B | 设置/复位周期 |

### 控制寄存器1 (CTRL_REG1) 配置
| 位 | 名称 | 说明 |
|----|------|------|
| 7:6 | OSR | 过采样率 (00=512, 01=256, 10=128, 11=64) |
| 5:4 | RNG | 量程 (00=2G, 01=8G) |
| 3:2 | ODR | 输出数据率 (00=10Hz, 01=50Hz, 10=100Hz, 11=200Hz) |
| 1:0 | MODE | 模式 (00=待机, 01=连续, 10=单次) |

### 推荐配置
- **模式**: 连续测量模式 (MODE=01)
- **ODR**: 100Hz (ODR=10) 或 50Hz (ODR=01)
- **量程**: 8G (RNG=01) - 适合无人机应用
- **过采样率**: 256 (OSR=01) - 平衡精度和速度

### 代码规范
- 遵循 STM32 HAL 风格
- 寄存器访问使用 volatile
- 使用 I2C HAL 接口进行通信 (i2c_mem_read/i2c_mem_write)
- 提供初始化、读取、配置等标准接口

## 具体修改要求

### 文件 1: `/Users/ll/kimi-fly/firmware/stm32/drivers/qmc5883p.h`
1. 定义 QMC5883P 寄存器地址常量
2. 定义量程 (RNG) 枚举: 2G, 8G
3. 定义 ODR 枚举: 10Hz, 50Hz, 100Hz, 200Hz
4. 定义模式枚举: 待机、连续、单次
5. 定义过采样率枚举: 512, 256, 128, 64
6. 定义 QMC5883P 句柄结构体 (包含i2c_handle_t指针和设备地址)
7. 定义数据读取结构体 (X/Y/Z三轴磁场、温度)
8. 声明API函数:
   - `qmc5883p_init()` - 初始化传感器
   - `qmc5883p_deinit()` - 反初始化
   - `qmc5883p_read_id()` - 读取CHIP_ID
   - `qmc5883p_read_data()` - 读取三轴磁场数据
   - `qmc5883p_set_mode()` - 设置工作模式
   - `qmc5883p_set_config()` - 设置ODR/量程/过采样率
   - `qmc5883p_data_ready()` - 检查数据是否就绪
   - `qmc5883p_soft_reset()` - 软件复位

### 文件 2: `/Users/ll/kimi-fly/firmware/stm32/drivers/qmc5883p.c`
1. 包含 `qmc5883p.h` 和 `i2c.h`
2. 实现寄存器读写函数 (内部使用，基于i2c_mem_read/i2c_mem_write)
3. 实现 `qmc5883p_init()`:
   - 读取 CHIP_ID 验证设备存在
   - 软件复位 (可选)
   - 配置默认参数 (连续模式, 100Hz ODR, 8G量程)
4. 实现 `qmc5883p_read_data()`:
   - 从寄存器0x00开始连续读取6字节 (X,Y,Z各2字节)
   - 数据格式: 16位有符号数，小端序
   - 可选读取温度数据 (2字节)
5. 实现配置函数 (set_mode, set_config)
6. 实现数据转换函数 (原始值转高斯单位)
   - 2G量程: 1 LSB = 0.000125 Gauss (1/8000 Gauss)
   - 8G量程: 1 LSB = 0.0005 Gauss (1/2000 Gauss)

## 完成标准 (必须可验证)

- [ ] 代码编译通过，无警告
- [ ] `qmc5883p_read_id()` 能正确读取CHIP_ID
- [ ] 提供数据读取测试代码片段 (可在main中调用验证)
- [ ] 磁场数据在合理范围 (地磁场约0.3-0.6 Gauss)
- [ ] 三轴数据随传感器方向变化而变化
- [ ] 代码遵循项目HAL风格规范

## 相关文件 (Hook范围检查用)
- `/Users/ll/kimi-fly/firmware/stm32/drivers/qmc5883p.h`
- `/Users/ll/kimi-fly/firmware/stm32/drivers/qmc5883p.c`
- `/Users/ll/kimi-fly/firmware/stm32/hal/i2c.h` (只读引用)

## 注意事项
- QMC5883P 与气压计共享I2C1总线，注意总线仲裁
- 数据为小端序 (Little Endian)，与IMU的大端序不同
- 磁力计容易受周围磁场干扰，需要远离电机和电源线
- 无人机应用中通常需要进行硬铁/软铁校准
- 地磁场在不同地理位置强度不同，约0.3-0.6 Gauss
- 8G量程适合无人机应用，2G量程精度更高但容易饱和
- 温度数据可用于温度补偿 (如果需要)

## 验收清单 (Reviewer使用)
- [ ] 修改在任务范围内
- [ ] 无新引入的未声明依赖
- [ ] 寄存器地址定义正确
- [ ] CHIP_ID读取逻辑正确
- [ ] 数据读取使用正确的小端序
- [ ] 数据转换公式正确
- [ ] 无新的技术债务
