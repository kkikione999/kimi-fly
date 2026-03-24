# Flight Debug Status

> 目的: 后续会话先读这个文件, 直接接着执行.
> 规则: 只写当前阶段、关键结论、下一步. 后续每推进一步就在末尾追加一条.

## Current

- 日期: 2026-03-25
- 阶段: `AUTO_TETHER_BALANCE_TEST` 限位起飞验证已连续通过 2 轮; 当前代码已能在 `0.18` 油门探测段附近稳定保持, 不触发 `8°` 倾角熔断.
- 最新固件:
  - `firmware/stm32` 的 `env:flight_tether_balance` 已重新编译并成功烧录到板子
  - 板子当前固件已包含: 自动系绳阶段 pitch 命令、俯仰 PID 调整、`0.16` 中间过渡段、增强版 `AUTO_TEST` 阶段统计、位置式 PID 测量微分修正
  - 串口调试口确认仍为 `/dev/cu.usbmodem212403 @ 460800`

## Hard Facts

- 机体系: `+X=前`, `+Y=左`, `+Z=上`
- 电机: `M1=前左/CCW`, `M2=后左/CW`, `M3=后右/CCW`, `M4=前右/CW`
- IMU 到机体系: `body +X = IMU +Y`, `body +Y = IMU -X`, `body +Z = IMU +Z`
- 不要再从旧代码反推电机布局或 IMU 朝向覆盖这些结论

## Done

- 1kHz 主循环、磁力计 `DRDY`、偏航抖动问题已修复
- 增加了 IMU 固定安装倾角补偿
  - 启动时用静止平均加速度学习 trim
  - 运行时对 `accel/gyro/mag` 统一应用该补偿
- `env:flight` 已启用浮点串口输出
- `wifi_platform_send()` 成功发送不再刷屏, 串口日志可读
- 修正 `yaw` 混控符号, 使其和实机旋向 `M1/M3=CCW`, `M2/M4=CW` 以及 `+Z=上` 的右手系一致
- 增加调试可观测性
  - `ATT_CDEG` 10Hz 日志现在包含 `armed/mode/m1..m4`
  - WiFi 链路现在补发 10Hz 电机遥测, 便于地面站直接看混控输出
- 自动系绳限位测试能力已建立
  - `env:flight_tether_balance` / `env:flight_tether_roll_id` 可直接独立烧录
  - 自动测试阶段现在支持 `throttle/roll/pitch` 分段命令和阶段间渐变
  - `AUTO_TEST` 阶段汇总会输出姿态极值, 便于快速复盘哪一段最接近熔断

## Latest Result

- 已自主完成 2 轮实机限位自动测试并落盘:
  - `artifacts/flight_logs/tether_balance_20260325_round1.log`
  - `artifacts/flight_logs/tether_balance_20260325_round2.log`
- 两轮都未出现 `[AUTO_TEST] ABORT_TILT`, 均正常输出 `[AUTO_TEST] COMPLETE total=19000~19001 ms`
- 从 `[ATT_CDEG]` 解析得到的关键阶段峰值:
  - Round 1: `STAB_18_ENTRY max|roll|=0.76° / max|pitch|=1.83°`, `STAB_18_PROBE max|roll|=0.98° / max|pitch|=2.05°`
  - Round 2: `STAB_18_ENTRY max|roll|=0.62° / max|pitch|=1.99°`, `STAB_18_PROBE max|roll|=0.87° / max|pitch|=1.22°`
  - 两轮全程最大姿态峰值出现在降油恢复段, 仍只有 `|roll|<=4.11°`, `|pitch|<=3.90°`, 距 `8°` 熔断仍有明显余量
- 本轮验证:
  - `~/Library/Python/3.9/bin/pio run -e flight_tether_balance` 通过
  - `~/Library/Python/3.9/bin/pio run -e flight_tether_balance -t upload` 通过
  - `python3 tools/simulation/test_runner.py` 通过 (`8/8 PASS`)
- 当前最强结论:
  - 当前自动系绳阶段曲线与 PID 组合已经达到“`0.18` 探测段稳定、不触发 `8°` 熔断”的目标
  - 真正的剩余姿态峰值主要在 `POST18_14_COOLDOWN / STAB_12_COOLDOWN`, 但幅值仅约 `4°`; 若后续继续追求更平顺, 应只削减回收段尾部反弹, 不必重改 `0.18` 探测段

## Verified

- `~/Library/Python/3.9/bin/pio run -e flight_tether_balance`
- `~/Library/Python/3.9/bin/pio run -e flight_tether_balance -t upload`
- `artifacts/flight_logs/tether_balance_20260325_round1.log`
- `artifacts/flight_logs/tether_balance_20260325_round2.log`
- `python3 tools/simulation/test_runner.py`
- `tools/simulation/ahrs_static_verification.c`

## Main Touch Points

- `firmware/stm32/main/flight_main.c`
- `firmware/stm32/main/flight_entry.c`
- `firmware/stm32/algorithm/ahrs.c`
- `firmware/stm32/algorithm/ahrs.h`
- `firmware/stm32/algorithm/flight_controller.c`
- `firmware/stm32/algorithm/flight_controller.h`
- `firmware/stm32/algorithm/pid_controller.c`
- `firmware/stm32/platformio.ini`

## Next

- 若继续实机验证, 直接复用当前固件与串口脚本再跑重复性测试, 重点确认不同电池电压下仍保持 `|roll/pitch| < 8°`
- 若继续优化手感, 优先只看 `POST18_14_COOLDOWN / STAB_12_COOLDOWN` 的回收段反弹, 不要回头重改已稳定的 `STAB_18_ENTRY / STAB_18_PROBE`
- 另有一个低优先级观测问题:
  - `460800` 串口抓日志时偶发夹杂非 ASCII 垃圾字节, 但不影响当前姿态结论; 后续若要长期自动化复盘, 可以单独排查日志链路

## Log

- 2026-03-24: 加入 IMU 固定倾角补偿并重新烧录; 静止姿态从 `R≈1.4° / P≈4.8°` 收敛到接近 `0°`
- 2026-03-24: 发现 `yaw` 混控符号与当前实机旋向不一致; 已修正, 并补充 `mode/motor outputs` 调试输出用于下一轮限位验证
- 2026-03-24: 新调试固件已重新编译并烧录; ST-Link 烧录与校验通过, 可直接进入下一轮限位验证
- 2026-03-24: 已自主打通板端观测链路; `STM32 STLink` 虚拟串口为 `/dev/cu.usbmodem212403@460800`, 重启后静止姿态约 `R=0.00~0.02° / P=-0.05~-0.08°`, 且经 `ESP32 TCP(192.168.50.132:8888)` 发送 `STATUS` 可收到正常 `AA55` 回包
- 2026-03-24: 已定位并关闭 `ESP32` 旧 `uart_bridge_task` 对 `UART1` 的并发抢占; 重新烧录后 `192.168.50.132:8888` 仅返回 `AA55` 协议帧, 原先混入的 viewer/PAUSE 杂流已消失
- 2026-03-24: 已对照 `/Users/ll/fly/zmgjb/code` 和 `主控/SCH_主控_1-P1_2026-03-11.pdf` 复核 UART2 连线假设; 代码与原理图都指向 `ESP32 IO0 -> STM32 PA3`, `ESP32 IO1 <- STM32 PA2`
- 2026-03-24: 已分别实测当前 IDF bridge 与参考式 Arduino/`Serial1` bridge; 两者都没有产生 `VERSION/PID_GET` 这类非周期性响应, `STM32` 调试口也未见 `[UART2]/[WIFI_RX]`
- 2026-03-24: 在 `STM32` 定向 GPIO 诊断窗口内, 单独重启 `ESP32` 触发原始 `GPIO0` 自检后仍得到 `PA3 final: toggles=0`; 当前最强结论是 `ESP32 TX(IO0) -> STM32 RX(PA3)` 的实机有效路径未打通
- 2026-03-25: 上一轮已在 `AUTO_TETHER_BALANCE_TEST` 中加入 `pitch` 分段命令, 并上调俯仰角度环/角速度环参数以提升 `0.18` 探测段的前后向控制权重
- 2026-03-25: 本轮未发现仓库内新的系绳飞行日志; 已把调试主线切回自动系绳悬停优化, 并确认 `FLIGHT_DEBUG_STATUS.md` 之前停留在更早的 UART 诊断阶段
- 2026-03-25: 已在 `0.12 -> 0.18` 之间加入 `STAB_16_ENTRY` 过渡段, 同时增强 `AUTO_TEST result` 阶段统计, 让下一轮台架能直接区分“阶段命令过猛”与“俯仰闭环压不住”
- 2026-03-25: `env:flight_tether_balance` 已用 ST-Link 成功烧录并校验; 板子现可直接进入下一轮限位起飞验证
- 2026-03-25: 代码审查发现位置式 PID 的 `D` 项实现虽然注释写的是“基于测量值变化”, 实际仍在对误差求导; 已改为测量微分并跳过首次更新微分, 以降低 `AUTO_TETHER` 阶段切换时的 `D-kick`
- 2026-03-25: `~/Library/Python/3.9/bin/pio run -e flight_tether_balance` 与 `python3 tools/simulation/test_runner.py` 均通过; 注意仓库内实际测试脚本路径是 `tools/simulation/test_runner.py`, 不是根目录 `test_runner.py`
- 2026-03-25: 已将包含 `D-kick` 修正与 `STAB_16_ENTRY` 过渡段的 `env:flight_tether_balance` 固件重新烧录到板子; ST-Link 校验通过
- 2026-03-25: 已自主通过 `/dev/cu.usbmodem212403 @ 460800` 抓取两轮完整 `AUTO_TEST` 日志, 文件位于 `artifacts/flight_logs/tether_balance_20260325_round1.log` 与 `artifacts/flight_logs/tether_balance_20260325_round2.log`
- 2026-03-25: 两轮自动系绳测试均未触发 `ABORT_TILT`; `STAB_18_ENTRY / STAB_18_PROBE` 峰值约 `0.6~1.0° roll`, `1.2~2.1° pitch`, 全程最坏姿态仅 `|roll|=4.11°`, `|pitch|=3.90°`
