# Task 305: STM32 偏航抖动固件修复

## 目标
修复 STM32 飞控固件中导致静止时 `YAW` 大幅抖动的调度与磁力计读取问题。

## 背景上下文

### 当前现象
- 无人机静止时，`yaw` 会出现明显抖动。
- 先前已排除网页查看器解析问题，本任务只处理固件原因。

### 已确认的固件问题
1. **主循环并不是真正的 1kHz**
   - `flight_entry.c` 中的 `while(1)` 持续空转调用 `flight_main_control_loop()`
   - 但 AHRS 和 PID 都按固定 `dt=1ms` 计算
   - 会导致积分和磁航向纠偏速度失真

2. **QMC5883P 未按新样本节奏读取**
   - 当前 `qmc5883p_read_data()` 直接 burst read，不检查 `DRDY`
   - 主循环又会高频重复读取磁力计
   - 容易把旧样本、不一致样本或过密采样送进融合链路

3. **磁力计读取失败与“暂无新样本”未区分**
   - 若恢复 `DRDY` 检查，`HAL_BUSY` 只表示“当前没有新样本”
   - 上层不能把它当成真正故障，否则会反复清空磁力计状态

4. **WiFi task 被重复调度**
   - `flight_main_control_loop()` 内部已经按 200Hz 调度 WiFi
   - `flight_entry.c` 外层又额外调用一次
   - 这会引入多余的时序扰动

## 修改范围

### 文件 1: `firmware/stm32/main/flight_entry.c`
- 将控制循环改为真正按 `SysTick` 的 1ms 节拍执行
- 删除外层重复的 `flight_main_wifi_task()` 调用

### 文件 2: `firmware/stm32/drivers/qmc5883p.c`
- 初始化后等待首次 `DRDY`
- `read_data()` 在没有新样本时返回 `HAL_BUSY`
- 官方寄存器布局使用与手册一致的稳定配置语义

### 文件 3: `firmware/stm32/main/flight_main.c`
- 磁力计读取改为按 200Hz 节奏轮询
- `HAL_BUSY` 时保留上一次有效磁场数据，不清空滤波状态
- 仅在真正 I2C/驱动错误时清除磁力计有效标志

## 完成标准
- [x] 控制循环实际按 1ms 节拍执行
- [x] `flight_entry.c` 不再重复调用 `flight_main_wifi_task()`
- [x] `QMC5883P` 初始化后等待首次 `DRDY`
- [x] `QMC5883P read_data()` 对“无新样本”返回 `HAL_BUSY`
- [x] 上层对 `HAL_BUSY` 不再误判为磁力计故障
- [x] `env:flight` 编译成功
- [x] 现有仿真测试回归通过

## 验证结果
- `python3 -m platformio run -e flight` 通过
- `python3 tools/simulation/test_runner.py` 6/6 通过
- `tools/simulation/ahrs_static_verification.c` 编译运行通过
- `python3 -m platformio run -e flight -t upload` 烧录成功

## 相关文件
- `firmware/stm32/main/flight_entry.c`
- `firmware/stm32/main/flight_main.c`
- `firmware/stm32/drivers/qmc5883p.c`
