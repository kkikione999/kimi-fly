# User Intent - 无人机自主开发

> **创建日期**: 2026-03-17
> **最后更新**: 2026-03-17

---

## 终极目标

**通过WiFi远程控制无人机飞行。**

---

## 硬件配置

### 主控制器
- **MCU**: STM32F411CEU6 (ARM Cortex-M4, 100MHz)
- **IMU**: ICM-42688-P (6轴陀螺仪/加速度计, SPI3)
- **气压计**: LPS22HBTR (I2C1)
- **磁力计**: QMC5883P (I2C1)

### 通信模块
- **WiFi**: ESP32-C3
- **连接**: UART2 (STM32_TXD2/RXD2)
- **WiFi配置**: SSID=`whc`, 密码=`12345678`

### 动力系统
- **电机**: 空心杯电机 × 4 (MOSFET驱动)
- **电源**: 当前USB供电 (安全模式，动力不足无法起飞)
- **电池**: 用户将在飞控调试阶段按需插入辅助

---

## 关键约束

1. **安全优先**: 目前无电池，USB供电仅用于开发测试
2. **自主开发**: AI独立完成所有开发，无需人工介入测试
3. **渐进迭代**: 小步快跑，每个阶段可验证
4. **WiFi控制**: 最终通过WiFi实现遥控飞行

---

## 成功标准

- [ ] 电机可通过命令控制转速
- [ ] 传感器数据正确读取
- [ ] 姿态解算输出稳定
- [ ] WiFi连接建立
- [ ] 远程控制命令响应
- [ ] 基础飞行控制实现 (需要电池时用户辅助)

---

## 参考文档

- 硬件文档: `hardware-docs/`
  - 引脚定义: `hardware-docs/pinout.md`
  - 电路图: `hardware-docs/schematics.md`
  - 元器件规格: `hardware-docs/components.md`
  - 电路图PDF: `hardware-docs/SCH_*.pdf`
- 技术债务: `docs/exec-plans/tech-debt-tracker.md`
