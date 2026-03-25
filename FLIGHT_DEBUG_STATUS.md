# Flight Debug Status

> 目的: 后续会话先读这个文件, 直接接着执行.
> 规则: 只写当前阶段、关键结论、下一步. 后续每推进一步就在末尾追加一条.

## Current

- 日期: 2026-03-25
- 阶段: `AUTO_TETHER_BALANCE_TEST` 的 `0.35` 峰值自动迭代仍在继续; 当前最优已推进到完整通过 `STAB_22`、首次在 `STAB_26` 才触发 `5°` 熔断, 但尚未达到“全程不触发 `5°` 熔断并稳定悬停到 `0.35`”目标.
- 最新固件:
  - `firmware/stm32` 的 `env:flight_tether_balance` 已重新编译并成功烧录到板子
  - 板子当前固件已回到本轮最优 `round18` 对应方案
  - 板子当前固件已包含: `round12` 高油门姿态外环定向调度 + 本轮新增的高油门单侧恢复助推
    - pitch: 高油门正向恢复助推保留
    - roll: 高油门正向内环输出助推保留
  - 未保留本轮验证失败的两条路线:
    - “继续上推 `0.26+` 静态 roll 前馈” (`round19`)
    - “插入 `STAB_24` 过渡段” (`round20`)
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
- `0.35` 主线在本轮新增 5 轮实机自动测试:
  - `artifacts/flight_logs/tether_balance_20260325_035_round16.log`
  - `artifacts/flight_logs/tether_balance_20260325_035_round17.log`
  - `artifacts/flight_logs/tether_balance_20260325_035_round18.log`
  - `artifacts/flight_logs/tether_balance_20260325_035_round19.log`
  - `artifacts/flight_logs/tether_balance_20260325_035_round20.log`
- 本轮各方案结论:
  - `round16`: 首次证明“高油门单侧 pitch 恢复助推”方向有效, `STAB_20` 俯仰明显改善, 但 roll 轴失稳提前集中到 `STAB_22`, `ABORT_TILT roll=-6.52 pitch=-0.49 t=17418`
  - `round17`: 在保留 pitch 恢复助推的同时改为 roll 内环输出助推, `STAB_22` 持续时间提升到 `909 ms`, `ABORT_TILT roll=-6.20 pitch=-0.19 t=17809`
  - `round18`: 本轮最优; 进一步扩大高油门 roll 输出助推后, 已完整通过 `STAB_22`, 熔断点首次后移到 `STAB_26`, `ABORT_TILT roll=-5.45 pitch=0.38 t=18369`
    - `STAB_20`: `roll_min=-0.38° / roll_max=2.63° / pitch_min=-1.98°`
    - `STAB_22`: `roll_min=-3.71° / roll_max=0.04° / pitch_min=-2.14°`
    - `STAB_26`: `269 ms` 内从 `roll=-3.50°` 继续掉到 `roll=-5.53°`
  - `round19`: 仅抬高 `0.26+` 静态 roll 前馈会恶化整体一致性, 熔断反而退回 `STAB_22`, `ABORT_TILT roll=-6.43 pitch=-0.04 t=17617`
  - `round20`: 插入 `STAB_24` 过渡段同样不如 `round18`, 熔断仍落回 `STAB_22`, `ABORT_TILT roll=-6.51 pitch=1.20 t=17759`
- 当前最强结论:
  - 本轮最优基线是 `round18`, 不是最新一次实验代码
  - 有效方向已经确认:
    - 高油门 pitch 单侧恢复助推
    - 高油门 roll 内环输出助推
  - 两条本轮已证伪路线:
    - 继续上推 `0.26+` 静态 roll 前馈
    - 在 `0.22 -> 0.26` 之间插 `STAB_24` 过渡段
  - 当前剩余唯一主问题是 `STAB_22 -> STAB_26` 交界后的持续负 roll 漂移; pitch 已不再是主导熔断轴

## Verified

- `~/Library/Python/3.9/bin/pio run -e flight_tether_balance`
- `~/Library/Python/3.9/bin/pio run -e flight_tether_balance -t upload`
- `artifacts/flight_logs/tether_balance_20260325_round1.log`
- `artifacts/flight_logs/tether_balance_20260325_round2.log`
- `artifacts/flight_logs/tether_balance_20260325_035_round16.log`
- `artifacts/flight_logs/tether_balance_20260325_035_round17.log`
- `artifacts/flight_logs/tether_balance_20260325_035_round18.log`
- `artifacts/flight_logs/tether_balance_20260325_035_round19.log`
- `artifacts/flight_logs/tether_balance_20260325_035_round20.log`
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

- 继续自动调试时, 必须从当前已烧回板子的 `round18` 最优基线继续, 不要从 `round19/20` 的失败路线接着改
- 下一轮只处理 `STAB_22 -> STAB_26` 交界后的 roll 漂移:
  - 优先尝试更细粒度的高油门 roll 控制力调度或 `STAB_26` 入口控制策略
  - 不要再重改已经稳定的 `STAB_20 / STAB_22`
- 目标仍是: 在 `0.35` 峰值范围内稳定悬停并全程不触发 `5°` 熔断
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
- 2026-03-25: 仓库内存在未写回状态文件的 `0.35` 自动系绳日志: `tether_balance_20260325_035_round1~7_final.log`; 最新 `round7_final` 在 `STAB_20` 约 `t=16549 ms` 触发 `ABORT_TILT roll=-5.82 pitch=-3.49`, 说明当前主阻塞已从“`0.18` 稳定”转为“`0.20+` 段持续向负 roll / 负 pitch 漂移”
- 2026-03-25: 已从 `round5/6/7` 统一复盘出同一趋势: `PRE18_UNLOAD -> STAB_20` 段末端持续出现 `pitch≈-2.1~-2.8°`, 且控制器已给出正 `pitch rate / pitch out` 但仍压不住; 下一轮改动先提高高油门 `pitch` 前馈, 并从 `0.20` 起加入小幅正 `roll` 前馈来对冲负向漂移
- 2026-03-25: 第一轮 `0.20+` 前馈加码后已重新编译、烧录并完成 `artifacts/flight_logs/tether_balance_20260325_035_round8.log`; 结果是熔断点已从 `STAB_20` 后移到 `STAB_22`, `ABORT_TILT roll=-6.16 pitch=-3.39 t=17664`
- 2026-03-25: `round8` 的关键改善是 `STAB_20` 已完整跑完且阶段统计仅 `roll_min=-1.33° / roll_max=2.65° / pitch_min=-2.46°`; 当前剩余问题集中在 `STAB_22` 段, 其前 `764 ms` 内持续掉向负 `roll` 并最终到 `roll_min=-6.04°`, 因此下一轮继续把 `0.20 -> 0.22` 拆细并抬高 `STAB_21/22+` 的正 `roll/pitch` 前馈
- 2026-03-25: `artifacts/flight_logs/tether_balance_20260325_035_round9.log` 证实“继续堆前馈 + 新增 `STAB_21`”并没有解决问题; 失稳点提前到 `STAB_21`, `ABORT_TILT roll=-6.02 pitch=-2.15 t=17947`
- 2026-03-25: `artifacts/flight_logs/tether_balance_20260325_035_round10.log` 证实“在回退阶段表的同时整体增强姿态/角速度环增益”也不是正确方向; `STAB_20` 已出现 `roll_max=3.07°`, 最终仍在 `STAB_22` 熔断, `ABORT_TILT roll=-6.19 pitch=-1.07 t=17695`
- 2026-03-25: 当前最优已验证基线仍是 `round8` 对应方案, 即“保留首轮高油门前馈加码, 但不要加入 `STAB_21`, 也不要整体上调 PID”; 后续若继续自动调试, 应基于 `round8` 继续做更小粒度的 `0.22+` 阶段拆分或定向高油门增益调度, 不要重复 `round9/round10` 这两条失败路线
- 2026-03-25: 已重新将 `round8` 基线烧回板子并抓取 `artifacts/flight_logs/tether_balance_20260325_035_round11.log`; 结果与 `round8` 同样在 `STAB_22` 熔断, `ABORT_TILT roll=-6.26 pitch=-2.50 t=17375`, 说明当前问题可稳定复现, 不是板端残留旧固件
- 2026-03-25: 已尝试“仅在 `AUTO_TETHER_BALANCE_TEST` 高油门区提升姿态外环回正权重”的定向调度, 重新编译/烧录并完成 `artifacts/flight_logs/tether_balance_20260325_035_round12.log`; 熔断点后移到 `t=17885`, `STAB_22` 持续时间从 `475 ms` 提升到 `985 ms`, `ABORT_TILT roll=-5.35 pitch=-3.38`
- 2026-03-25: `round12` 证明“高油门定向增益调度”方向有效, 但 `STAB_20` 末段仍会带着 `pitch_min=-4.93° / roll_min=-1.64°` 进入 `STAB_22`; 下一轮继续沿同一路线前移增益拉满阈值, 优先让 `0.20` 段就拿到完整高油门回正权重, 不回到加 `STAB_21` 或全局增益放大的旧路线
- 2026-03-25: `artifacts/flight_logs/tether_balance_20260325_035_round13.log` 证实“把高油门调度提前到 `0.20` 就完全拉满”不是更优解; 虽然 `STAB_20` 改善到 `roll_min=-1.02° / pitch_min=-3.10°`, 但 `STAB_22` 反而更早熔断, `ABORT_TILT roll=-6.45 pitch=-2.59 t=17475`
- 2026-03-25: `artifacts/flight_logs/tether_balance_20260325_035_round14.log` 证实“保持 `0.22` 拉满窗口, 但继续抬高高油门 roll 外环上限到 `1.70x`”同样不如 `round12`; `STAB_22` 仅维持 `641 ms`, `ABORT_TILT roll=-6.07 pitch=-2.12 t=17541`
- 2026-03-25: `artifacts/flight_logs/tether_balance_20260325_035_round15.log` 证实“在 `round12` 基础上继续把 `STAB_22` 正 roll 前馈从 `0.04` 加到 `0.05`”会明显恶化, 熔断提前到 `t=17229`, `STAB_22` 仅 `329 ms`; 这条路应视为失败路线
- 2026-03-25: 截至当前, `round12` 仍是全部 `0.35` 尝试中的最优自动调试结果; 工作区代码已回退到 `round12` 对应方案, 即“高油门姿态外环定向增益调度保留, 但不提前到 `0.20` 全开, 也不继续上推 roll 上限或 `STAB_22` 静态前馈”
- 2026-03-25: 已在 `round12` 基线上加入“高油门单侧恢复助推”; `artifacts/flight_logs/tether_balance_20260325_035_round16.log` 证实该方向显著压低了 pitch 失稳, 但新的主熔断轴变成纯 roll, `ABORT_TILT roll=-6.52 pitch=-0.49 t=17418`
- 2026-03-25: `artifacts/flight_logs/tether_balance_20260325_035_round17.log` 证实“保留 pitch 恢复助推, 把 roll 改成高油门内环输出助推”方向继续有效; `STAB_22` 持续时间提升到 `909 ms`, 但仍在末段以 `roll=-6.20` 熔断
- 2026-03-25: `artifacts/flight_logs/tether_balance_20260325_035_round18.log` 成为新的最优基线; 已完整通过 `STAB_22`, 首次把熔断点后移到 `STAB_26`, `ABORT_TILT roll=-5.45 pitch=0.38 t=18369`
- 2026-03-25: `artifacts/flight_logs/tether_balance_20260325_035_round19.log` 证实“继续上推 `0.26+` 静态 roll 前馈”不是正确方向; 熔断退回 `STAB_22`, `ABORT_TILT roll=-6.43 pitch=-0.04 t=17617`
- 2026-03-25: `artifacts/flight_logs/tether_balance_20260325_035_round20.log` 证实“在 `0.22 -> 0.26` 之间插 `STAB_24` 过渡段”同样不如 `round18`; 熔断仍落回 `STAB_22`, `ABORT_TILT roll=-6.51 pitch=1.20 t=17759`
- 2026-03-25: 工作区代码与板子固件均已重新烧回 `round18` 对应最优方案, 即“高油门 pitch 单侧恢复助推 + 高油门 roll 内环输出助推保留, 阶段表回退到无 `STAB_24` 的 `round18` 版本”
- 2026-03-25: 已新增无人协助调试工具链: `tools/tether_log_summary.py`（抗串口二进制噪声的日志评分）与 `tools/tether_round_runner.py`（编译/烧录/抓串口/自动评分一体化）; `README.md` 已补充命令入口
- 2026-03-25: 已用新工具链自主完成两轮基线复测:
  - `artifacts/flight_logs/auto_round_probe_20260325_132621.log`: `ABORT_TILT roll=-6.09 pitch=-3.33 t=15350`, 提前在 `STAB_18_PROBE` 熔断
  - `artifacts/flight_logs/auto_round_probe2_20260325_132734.log`: `ABORT_TILT roll=-5.54 pitch=-4.21 t=15802`, 在 `STAB_20` 初段熔断
  - 两轮均显著早于历史最优 `round18 (t=18369, STAB_26)`; 当前最强判断是“台架/约束状态漂移导致可比性下降”, 下一轮应先做最小化基线稳定化验证，再继续 `STAB_22->26` 定向 roll 调参
