# Tech Debt Tracker - 技术债务跟踪

> **项目**: 无人机WiFi飞行控制器
> **创建日期**: 2026-03-17

---

## 待处理 (TODO)

- [x] 2026-03-19 - ICM42688 I2C地址硬编码错误 (0x68, 实际为0x69) - 来源: Round 2 硬件验证 - 解决: 2026-03-19
  - 原因: 硬件 AD0 引脚被拉高至 VCC，实际地址 0x69
  - 方案: icm42688.h 中 ICM42688_I2C_ADDR 改为 0x69U
  - 修复: Task 301

- [x] 2026-03-19 - LPS22HB 气压计读数异常 (283hPa) - 来源: Round 2 硬件验证 - 解决: 2026-03-19
  - 原因: SPI 多字节读未启用地址自动递增 (bit6=1)
  - 方案: lps22hb.h 添加 LPS22HB_SPI_AUTO_INC_BIT=0x40，lps22hb_read_regs() 使用 0xC0|reg
  - 修复: Task 302

- [x] 2026-03-19 - QMC5883P 磁力计 Y/Z 轴为 0 - 来源: Round 2 硬件验证 - 解决: 2026-03-19
  - 原因: 未执行 SET 操作、未等待首次数据就绪
  - 方案: qmc5883p_init() 添加 CTRL2_SET 操作 + 10ms 等待 + 50ms DRDY 轮询
  - 修复: Task 303

- [ ] 2026-03-19 - STM32-ESP32 UART 物理连接待确认 - 来源: Round 2 硬件验证 - 优先级: P1
  - 原因: 双向 RX=0，疑似 TX/RX 未交叉连接
  - 修复: Task 304 (软件诊断工具) + 人工检查硬件连接

- [x] 2026-03-19 - STM32 HAL层头文件常量命名冲突 - 来源: Ralph-loop Round 1 - 解决: 2026-03-19
  - 方案: 将gpio.h中GPIO_MODE_*/GPIO_OTYPE_*/GPIO_SPEED_*/GPIO_PUPD_*重命名为HAL_GPIO_*前缀
  - 修改文件: `firmware/stm32/hal/gpio.h`, `firmware/stm32/hal/gpio.c`

- [x] 2026-03-19 - uart.h中类型错误hal_state_t - 来源: Ralph-loop Round 1 - 解决: 2026-03-19
  - 方案: 将hal_state_t改为hal_status_t
  - 修改文件: `firmware/stm32/hal/uart.h`

## 已解决 (Resolved)

- [x] 2026-03-18 - STM32 HAL层基础兼容性问题 - 来源: Ralph-loop Iteration 9 - 解决: 2026-03-18
  - 方案: 修改hal_common.h使用USE_HAL_DRIVER条件编译，包含stm32f4xx_hal.h
  - 修改文件: `firmware/stm32/hal/hal_common.h`

- [x] 2026-03-18 - STM32 HAL头文件enum冲突(spi.h/uart.h/pwm.h) - 来源: Ralph-loop Iteration 9 - 解决: 2026-03-18
  - 方案: 使用#ifndef USE_HAL_DRIVER包裹自定义enum，STM32Cube模式下使用HAL类型定义
  - 修改文件: `firmware/stm32/hal/spi.h`, `uart.h`, `pwm.h`

---

## 已解决 (Resolved)

*暂无已解决记录*

---

## 记录模板

### 添加新债务
```markdown
- [ ] YYYY-MM-DD - {简要描述问题} - 来源: {任务/轮次} - 优先级: {P0/P1/P2/P3}
```

### 解决债务
```markdown
- [x] YYYY-MM-DD - {简要描述问题} - 来源: {任务/轮次} - 解决: YYYY-MM-DD - 方案: {如何解决}
```

---

## 优先级定义

| 优先级 | 含义 | 处理时限 |
|--------|------|----------|
| P0 - Blocker | 阻塞后续开发 | 立即 |
| P1 - Warning | 可能引发问题 | 本轮或下轮 |
| P2 - Tech Debt | 代码债务 | 后续迭代 |
| P3 - Nice to have | 优化建议 | 有时间再处理 |
