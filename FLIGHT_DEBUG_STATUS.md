# Flight Debug Status

> 目的: 后续会话先读这个文件, 直接接着执行.
> 规则: 只写当前阶段、关键结论、下一步. 后续每推进一步就在末尾追加一条.

## Current

- 日期: 2026-03-24
- 阶段: 已从飞控姿态/混控问题切到 `ESP32 -> STM32` 下行链路定位; 当前结论是该方向故障在协议层以下.
- 最新固件:
  - `firmware/stm32` 的 `env:flight` 已重新烧录回板子
  - `firmware/esp32/main` 的常规 IDF TCP-UART bridge (`GPIO0 TX / GPIO1 RX`) 已重新烧录回板子

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

## Latest Result

- 板子在线:
  - ST-Link 可识别
  - ESP32-C3 USB 可识别
- 常规固件恢复完成:
  - `python3 -m platformio run -e flight -t upload` 成功
  - `source ~/esp/esp-idf/export.sh && idf.py -p /dev/cu.usbmodem212301 build flash` 成功
- 参考代码与原理图结论已再次交叉确认:
  - 参考机 `C3` 代码仍是 `Serial1.begin(..., 1, 0)` => `ESP32 IO0 = TX`, `IO1 = RX`
  - `主控` 原理图文本坐标显示 `STM_RXD2` 对应 `U10 pin12`, `STM_TXD2` 对应 `U10 pin13`
  - 结合 `U10` 引脚表可得: `STM_RXD2 <- IO0`, `STM_TXD2 -> IO1`
- 实机诊断结论:
  - 用当前常规 IDF bridge 测 `VERSION/PID_GET/STATUS`, 只有周期性 `STATUS/HEARTBEAT/TELEMETRY`, 没有非周期性 `0x03/0x30`
  - 切到仓库里的 Arduino/`Serial1` 参考式 bridge 后复测, 结果相同
  - 最关键: 在 `STM32` 诊断固件的 `PA3` 定向采样窗口内, 我单独重启 `ESP32` 触发原始 `GPIO0` 自检, 仍得到:
    - `PA3 final: samples=942301 toggles=0`
    - `[FAIL] No signal detected on PA3 from GPIO0.`
- 当前最强结论:
  - `ESP32 TX(IO0) -> STM32 RX(PA3)` 这条实机有效路径没有被观测到, 问题在协议/解析层以下
  - 仅靠继续改 `IDF`/`Arduino` 桥接逻辑已经不能解释现象

## Verified

- `python3 -m platformio run -e flight`
- `python3 -m platformio run -e flight -t upload`
- `python3 test_runner.py`
- `tools/simulation/ahrs_static_verification.c`

## Main Touch Points

- `firmware/stm32/main/flight_main.c`
- `firmware/stm32/main/flight_entry.c`
- `firmware/stm32/algorithm/ahrs.c`
- `firmware/stm32/algorithm/ahrs.h`
- `firmware/stm32/algorithm/flight_controller.c`
- `firmware/stm32/algorithm/flight_controller.h`
- `firmware/stm32/platformio.ini`

## Next

- 优先排查板级物理路径, 不要再先改飞控算法:
  - 以 `主控` 原理图的 `STM_RXD2/STM_TXD2` 网名为基准, 核对装配板上 `U10 pin12(IO0)` 到 `PA3` 的真实连通
  - 查是否存在 0Ω 电阻、焊接、ESD/隔离/门控、或板版次差异导致 `IO0 -> PA3` 实际未接通
  - 若硬件确认无误, 下一步应直接做板上连续性/示波器观测, 而不是继续猜协议
- 飞控姿态/混控修正本轮先冻结:
  - `yaw` 混控修正和 `ATT_CDEG/motor telemetry` 已在 `env:flight`
  - 在下行链路恢复前, 不做新的限位起飞验证

## Log

- 2026-03-24: 加入 IMU 固定倾角补偿并重新烧录; 静止姿态从 `R≈1.4° / P≈4.8°` 收敛到接近 `0°`
- 2026-03-24: 发现 `yaw` 混控符号与当前实机旋向不一致; 已修正, 并补充 `mode/motor outputs` 调试输出用于下一轮限位验证
- 2026-03-24: 新调试固件已重新编译并烧录; ST-Link 烧录与校验通过, 可直接进入下一轮限位验证
- 2026-03-24: 已自主打通板端观测链路; `STM32 STLink` 虚拟串口为 `/dev/cu.usbmodem212403@460800`, 重启后静止姿态约 `R=0.00~0.02° / P=-0.05~-0.08°`, 且经 `ESP32 TCP(192.168.50.132:8888)` 发送 `STATUS` 可收到正常 `AA55` 回包
- 2026-03-24: 已定位并关闭 `ESP32` 旧 `uart_bridge_task` 对 `UART1` 的并发抢占; 重新烧录后 `192.168.50.132:8888` 仅返回 `AA55` 协议帧, 原先混入的 viewer/PAUSE 杂流已消失
- 2026-03-24: 已对照 `/Users/ll/fly/zmgjb/code` 和 `主控/SCH_主控_1-P1_2026-03-11.pdf` 复核 UART2 连线假设; 代码与原理图都指向 `ESP32 IO0 -> STM32 PA3`, `ESP32 IO1 <- STM32 PA2`
- 2026-03-24: 已分别实测当前 IDF bridge 与参考式 Arduino/`Serial1` bridge; 两者都没有产生 `VERSION/PID_GET` 这类非周期性响应, `STM32` 调试口也未见 `[UART2]/[WIFI_RX]`
- 2026-03-24: 在 `STM32` 定向 GPIO 诊断窗口内, 单独重启 `ESP32` 触发原始 `GPIO0` 自检后仍得到 `PA3 final: toggles=0`; 当前最强结论是 `ESP32 TX(IO0) -> STM32 RX(PA3)` 的实机有效路径未打通
