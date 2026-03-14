# Task 001: HAL 层目录结构与基础接口定义

## 目标
建立硬件抽象层(HAL)的基础目录结构和通用接口定义，为 STM32 和 ESP32-C3 双处理器提供统一的抽象接口。

## 背景上下文

### 项目架构
```
firmware/
├── hal/              # 硬件抽象层 (本任务)
│   ├── stm32/        # STM32 HAL/LL 实现
│   └── esp32c3/      # ESP32-C3 IDF 实现
├── drivers/          # 设备驱动
├── services/         # 高层服务
└── platform/         # 板级配置
```

### 硬件信息
- **STM32**: 主处理器，负责飞行控制 (候选: F411, F405, H743)
- **ESP32-C3**: 协处理器，负责 WiFi 通信 (型号: ESP32-C3-WROOM-02)
- **通信**: 两处理器通过 UART 连接

### 编码规范
- C11 标准
- 4空格缩进，K&R 括号风格
- 命名: `hal_verb_noun`, `hal_noun_t`
- 返回 int: 0=成功, 负数=错误码

## 具体修改要求

### 1. 文件: `firmware/hal/hal_common.h`
创建 HAL 通用类型定义和错误码接口。

内容需包含:
- 标准类型别名 (uint8_t, int16_t 等)
- 错误码枚举 (HAL_OK, HAL_ERR_PARAM, HAL_ERR_TIMEOUT 等)
- 布尔类型定义
- 编译时断言宏
- 版本信息宏

### 2. 文件: `firmware/hal/hal_interface.h`
创建 HAL 层对外暴露的统一接口声明。

内容需包含:
- GPIO 接口声明 (初始化、设置、读取)
- UART 接口声明 (初始化、发送、接收)
- SPI 接口声明 (初始化、传输)
- I2C 接口声明 (初始化、读写)
- PWM/Timer 接口声明 (初始化、设置占空比)
- 系统滴答接口声明 (获取当前时间、延时)

所有接口使用函数指针结构体或弱符号声明，便于平台实现覆盖。

### 3. 文件: `firmware/platform/board_config.h`
创建板级配置文件，定义引脚分配和外设配置。

内容需包含:
- 处理器类型定义宏
- 电机 PWM 引脚定义 (MOTOR1_PIN - MOTOR4_PIN)
- 传感器接口引脚定义 (SPI1, I2C1)
- 调试串口引脚定义
- STM32-ESP32 通信串口引脚定义
- 频率配置 (PWM频率、控制循环频率)

## 完成标准

- [ ] `firmware/hal/hal_common.h` 创建完成，包含完整类型定义和错误码
- [ ] `firmware/hal/hal_interface.h` 创建完成，所有外设接口声明清晰
- [ ] `firmware/platform/board_config.h` 创建完成，引脚配置合理
- [ ] 所有文件通过 clang-format 格式检查
- [ ] 文件头包含版权声明和文件描述
- [ ] 代码注释说明每个接口的用途和参数

## 相关文件
- `/Users/ll/kimi-fly/CLAUDE.md` - 编码规范
- `/Users/ll/kimi-fly/docs/HARDWARE.md` - 硬件规格
- `/Users/ll/kimi-fly/plan.md` - 项目计划

## 注意事项
- 保持接口平台无关，不要在接口头文件中包含平台特定头文件
- 使用 `__attribute__((weak))` 或函数指针表实现平台抽象
- 考虑后续将有两个独立的实现：stm32/ 和 esp32c3/
- 板级配置中的引脚定义应与硬件原理图一致（如已确定）
