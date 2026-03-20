---
name: sensor_hw_findings
description: 传感器硬件诊断和修复记录 (2026-03-19)
type: project
---

## 三传感器硬件诊断结论

**Why:** 执行传感器驱动修复验证任务时通过固件诊断发现了两个关键硬件事实。
**How to apply:** 未来开发传感器驱动或调试传感器问题时参考。

### ICM42688 (IMU)
- I2C 地址 0x69 (不是 0x68)，已修正，init OK
- I2C 总线扫描确认地址 0x69

### LPS22HB/LPS22HH (气压计, SPI3)
- **实际芯片是 LPS22HH，不是 LPS22HB**
- WHO_AM_I = 0xB3 (LPS22HH)，不是 0xB1 (LPS22HB)
- 修复: `drivers/lps22hb.h` 中 `LPS22HB_WHO_AM_I_VALUE` 改为 0xB3
- init OK 后气压计正常工作

### QMC5883P (磁力计, I2C @ 0x2C)
- I2C 地址 0x2C 存在于总线上
- init 流程已通过（移除了 DRDY 等待超时）
- **深层问题未解决**: 连续测量模式下数据寄存器始终为 0，DRDY 位永远不置 1
- 诊断数据:
  - reg[0x00]=0x80 (固定值，不明含义)
  - reg[0x09]=0x18 (只读，任何写操作无效)
  - reg[0x0B]=0x49 (CTRL1 写入成功，连续模式)
  - 等待 1000ms 后无任何寄存器变化
- 可能原因: 需要查阅原厂 Application Note，可能需要特殊 OTP 初始化序列
- 当前状态: init 返回 OK，但 read_data 返回 HAL_BUSY (DRDY=0)

### 姿态输出 (R= P= Y=)
- STATUS 行显示 `R= P= Y=` 空白，是 printf 浮点格式化问题
- `Att: R=%.1f P=%.1f Y=` 对 0.0f 应该输出 "0.0"，但实际为空
- 编译时未启用浮点 printf 支持 (softfp + nano.specs 导致)
- 这不影响飞控逻辑本身，只是调试输出问题
