# 无人机 WiFi 飞行控制器

该仓库用于 STM32 飞控核心、ESP32-C3 WiFi 桥接、硬件资料和开发流程文档的统一管理。

## 目录入口

- `firmware/`：STM32、ESP32 和共享 HAL 源码
- `tools/`：地面站与仿真工具
- `hardware-docs/`：硬件真源与器件资料
- `docs/architecture/`：架构和技术路径
- `docs/plans/`：当前执行计划与技术债务
- `docs/archive/`：归档交付物与历史会话记录
- `knowledge-hub/`：知识库、项目地图、技术债账本

## 常用入口

```bash
cat docs/plans/active/plan.md
cat docs/plans/tech-debt-tracker.md
cat hardware-docs/pinout.md
cat knowledge-hub/README.md
```

## 硬件知识库

```bash
# 构建默认硬件知识库 chunk
python3 scripts/build_hardware_knowledge.py --clean

# 搜索已经生成的 chunk
python3 scripts/search_hardware_knowledge.py "who am i pwr_mgmt0" --chip icm-42688-p

# 启动 AnythingLLM 本地界面
cd knowledge-hub/platforms/anythingllm
cp .env.example .env
docker compose up -d
```

## 自动系绳调试命令

```bash
# 历史日志自动评分（抗串口乱码）
python3 tools/tether_log_summary.py "artifacts/flight_logs/tether_balance_20260325_035_round*.log"

# 单轮无人协助执行：编译 + 烧录 + 抓串口 + 自动评分
python3 tools/tether_round_runner.py --env flight_tether_balance --duration 35
```
