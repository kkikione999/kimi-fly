# PR #7 审查报告 - STM32 Task 001 USART2诊断

**审查时间**: 2026-03-18
**审查人**: plan-reviewer
**PR**: #7 - Task 001: STM32 USART2配置诊断

---

## 审查结论

**状态**: 🟡 PENDING (等待查看实际代码)

PR描述清晰，诊断计划完整，但尚未在仓库中看到实际代码变更。

---

## PR描述审查

### 诊断功能规划

**1. `usart2_dump_registers()` 函数**

计划检查的寄存器：

| 寄存器 | 位/字段 | 预期值 | 说明 |
|--------|---------|--------|------|
| RCC->APB1ENR | USART2EN (bit 17) | 1 | USART2时钟使能 |
| RCC->AHB1ENR | GPIOAEN (bit 0) | 1 | GPIOA时钟使能 |
| GPIOA->MODER | PA2 (bits 4:5) | 0b10 | PA2复用功能模式 |
| GPIOA->AFR[0] | PA2 (bits 8:11) | 0b0111 | AF7 (USART2_TX) |
| USART2->CR1 | UE (bit 13) | 1 | USART使能 |
| USART2->CR1 | TE (bit 3) | 1 | 发送使能 |
| USART2->CR1 | RE (bit 2) | 1 | 接收使能 |
| USART2->BRR | 全16位 | 0x1B2 (434) | 115200@50MHz |

**2. `test_pa2_gpio()` 函数**

目的：测试PA2物理连接
- 将PA2配置为GPIO输出
- 翻转电平并用示波器/万用表检测
- 验证硬件连接正常

---

## 预期输出审查

```
USART2EN = 1
PA2 mode = 0x2 (Alternate Function)
PA2 AF = 0x7 (AF7 - USART2_TX)
TE = 1, UE = 1
BRR = 0x1B2 (434)
```

**注意**: BRR=0x1B2 (434) 对应 50MHz APB1 时钟：
- BRR = 50MHz / 115200 = 434.027 ≈ 434 (0x1B2)

这与之前发现的 APB1 时钟问题一致。

---

## 建议

### 代码结构建议

1. **在 `main()` 中添加诊断调用**
   ```c
   int main(void) {
       // ... 初始化 ...

       // 诊断：打印USART2寄存器状态
       usart2_dump_registers();

       // 诊断：测试PA2物理连接
       test_pa2_gpio();

       // ... 主循环 ...
   }
   ```

2. **诊断代码使用条件编译**
   ```c
   #ifdef USART2_DIAGNOSTIC
   usart2_dump_registers();
   test_pa2_gpio();
   #endif
   ```

3. **添加BRR计算验证**
   ```c
   uint32_t apb1_freq = HAL_RCC_GetPCLK1Freq();
   uint16_t brr = USART2->BRR;
   debug_printf("APB1 freq: %lu, BRR: 0x%04X, calc baud: %lu\r\n",
                apb1_freq, brr, apb1_freq / brr);
   ```

---

## 阻塞问题

**无法完成审查**：尚未在仓库中看到 PR #7 的实际代码变更。

**需要**:
1. 确认代码已提交到 PR 分支
2. 或提供代码片段供审查

---

## 下一步

1. **STM32工程师**: 确认代码已提交到 PR #7
2. **Reviewer**: 重新审查实际代码
3. **合并后**: 烧录固件并查看调试输出
4. **根据诊断结果**: 决定是否激活 Task 002 修复
