# Tech Debt Tracker - 技术债务跟踪

> **项目**: 无人机WiFi飞行控制器
> **创建日期**: 2026-03-17

---

## 待处理 (TODO)

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
