# kimi-fly 无人机项目

基于 STM32F411CEU6 + ESP8266-12E 的四轴飞行控制器。

> 本项目采用 [Harness Engineering](harness-engineering.md) 方法论构建。

---

## 硬件规格

| 组件 | 型号 | 规格 |
|------|------|------|
| 主控芯片 | STM32F411CEU6 | 100MHz, 512KB Flash, 128KB RAM |
| WiFi模块 | ESP8266-12E | 802.11 b/g/n, 4MB Flash |
| 电机驱动 | SI2302 MOS管 | 4路直驱 |
| IMU | MPU6050 | I2C接口 |
| 气压计 | BMP280 | I2C接口 |

详细规格: [docs/HARDWARE.md](docs/HARDWARE.md)
引脚分配: [docs/pinout.md](docs/pinout.md)

---

## 项目结构

```
kimi-fly/
├── AGENTS.md              # Agent入口指南
├── ARCHITECTURE.md        # 系统架构总览
├── CLAUDE.md              # 编码规范
├── harness-engineering.md # Harness Engineering方法论
├── docs/                  # 文档
│   ├── AGENTS-GUIDE.md    # Agent使用指南
│   ├── HARDWARE.md        # 硬件规格
│   ├── design-docs/       # 设计文档
│   │   ├── core-beliefs.md
│   │   └── coding-standards/
│   ├── exec-plans/        # 执行计划
│   └── references/        # 参考资料
└── tools/                 # 工具链
    ├── flash/             # 烧录工具
    │   ├── flash_stm32.sh
    │   └── flash_esp8266.sh
    └── setup/             # 环境设置
        ├── check_env.py
        └── setup_macos.sh
```

---

## 快速开始

### 1. 环境设置

```bash
# 检查环境
python tools/setup/check_env.py

# macOS环境配置
./tools/setup/setup_macos.sh
```

### 2. 固件烧录

```bash
# 烧录STM32固件
cd tools/flash
./flash_stm32.sh

# 烧录ESP8266固件
./flash_esp8266.sh
```

---

## 文档导航

| 需要 | 查看 |
|------|------|
| 系统架构 | [ARCHITECTURE.md](ARCHITECTURE.md) |
| 编码规范 | [CLAUDE.md](CLAUDE.md) |
| Agent使用 | [AGENTS.md](AGENTS.md) |
| 硬件规格 | [docs/HARDWARE.md](docs/HARDWARE.md) |
| 设计原则 | [docs/design-docs/core-beliefs.md](docs/design-docs/core-beliefs.md) |

---

## 许可证

MIT License
