# UART通信调试计划

## 目标
解决STM32 USART2 TX不输出数据的问题，实现STM32与ESP32-C3的双向UART通信。

## 当前问题
- ESP32发送正常 (TX: 259条消息)
- ESP32接收为 0 (RX: 0)
- STM32 USART2 TX 未输出数据

## 调试阶段

### Phase 1: 诊断 (Task 001)
**负责人**: STM32 Embedded Engineer
**目标**: 诊断STM32 USART2配置问题
**检查项**:
- PA2 GPIO复用功能配置 (GPIO_AF7_USART2)
- USART2时钟使能 (APB1总线)
- 波特率计算 (115200 @ 50MHz APB1)
- USART2发送使能位

### Phase 2: 修复 (Task 002)
**负责人**: STM32 Embedded Engineer
**目标**: 根据诊断结果修复HAL层配置
**依赖**: Task 001完成

### Phase 3: 验证 (Task 003)
**负责人**: ESP32 Embedded Engineer
**目标**: 验证ESP32接收端配置
**依赖**: Task 002完成

## 完成标准
- [ ] STM32 USART2 TX输出正常信号
- [ ] ESP32能接收到STM32发送的消息
- [ ] 双向通信统计正常 (TX/RX计数都增加)

## 相关文档
- `/docs/UART_TEST_REPORT.md` - 测试报告
- `/docs/UART_COMMUNICATION.md` - 通信协议
- `/firmware/stm32/hal/uart.c` - STM32 UART HAL
- `/firmware/stm32/main/uart_comm_test.c` - 测试程序

## 风险评估
- **风险**: 硬件连接问题 (TX/RX交叉)
- **缓解**: Task 001包含硬件状态检查
