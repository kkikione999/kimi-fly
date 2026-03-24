# Flight Debug Status

> 目的: 后续会话先读这个文件, 直接接着执行.
> 规则: 只写当前阶段、关键结论、下一步. 后续每推进一步就在末尾追加一条.

## Current

- 日期: 2026-03-24
- 阶段: STM32 飞控已完成当前轮姿态零点与混控方向修正, 可继续做限位低风险实机验证.
- 最新固件: `firmware/stm32` 的 `env:flight` 已编译、烧录到板子, 包含 `yaw` 混控修正和新的 `ATT_CDEG/motor telemetry` 调试输出.

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
- STM32 烧录成功
- 新固件特性已写入:
  - `yaw` 混控已按 `M1/M3=CCW`, `M2/M4=CW` 修正
  - `ATT_CDEG` 已增加 `arm/md/m1..m4`
  - WiFi 电机遥测已启用 (10Hz)
- 当前静止串口状态已接近零姿态:
  - `Att: R=0.0 P=-0.1 Y=0.1`
  - `ATT_CDEG ... r=0 p=-6 y=13 rr=0 pr=0 yr=0`
- 修复前静止大致为:
  - `R≈1.4°`
  - `P≈4.8°`

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

- 每次上电后先静止水平放置 2-3 秒
- 先确认静止 `roll/pitch` 仍在约 `±1°` 内
- 然后做限位状态下 `STABILIZE + ARM + 低油门` 的补偿方向验证
  - 重点看 `ATT_CDEG` 里的 `md=2` 是否成立, 避免误在 `ARMED` 怠速模式下测试
  - 重点看 `m1..m4` 是否符合:
    - 左高右低时, 右侧电机应相对增大
    - 头高尾低时, 前侧电机应相对增大
- 若补偿方向正确, 再做限位小离地测试

## Log

- 2026-03-24: 加入 IMU 固定倾角补偿并重新烧录; 静止姿态从 `R≈1.4° / P≈4.8°` 收敛到接近 `0°`
- 2026-03-24: 发现 `yaw` 混控符号与当前实机旋向不一致; 已修正, 并补充 `mode/motor outputs` 调试输出用于下一轮限位验证
- 2026-03-24: 新调试固件已重新编译并烧录; ST-Link 烧录与校验通过, 可直接进入下一轮限位验证
