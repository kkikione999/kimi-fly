# Task 303: QMC5883P 磁力计 Y/Z 轴为 0 修复

## 目标
修复 QMC5883P 磁力计 Y 轴和 Z 轴读数持续为 0 的问题。

## 背景上下文

### 相关代码
- 文件: `firmware/stm32/drivers/qmc5883p.c` - 磁力计驱动实现
- 文件: `firmware/stm32/drivers/qmc5883p.h` - 寄存器定义

### 问题说明
**当前现象**: QMC5883P X 轴有读数，Y/Z 轴为 0。

**可能原因分析** (按概率排序):

1. **数据就绪检查不足**: 调用 `qmc5883p_read_data()` 前未等待 `DRDY` 置位，读到了上一次转换的部分数据（Y/Z 尚未更新）。当前 `data_ready` 检查后立即读取，可能竞争条件。

2. **QMC5883P 连续模式启动延迟**: 初始化后进入连续测量模式需要约 1 个 ODR 周期（100Hz → 10ms）才有有效数据，未等待。

3. **SET/RESET 操作缺失**: QMC5883P 手册要求上电后执行一次 SET 操作（设置 CTRL2 的 SET 位），否则可能数据异常。

### 依赖关系
- 前置任务: 无
- 外部依赖: 无

### 硬件信息
- 涉及引脚: PB6 (I2C1_SCL), PB7 (I2C1_SDA)
- I2C 地址: 0x2C (已确认)
- 参考: `hardware-docs/pinout.md` 第 3.3 节

## 具体修改要求

### 文件 1: `firmware/stm32/drivers/qmc5883p.h`
1. 在 CTRL2 寄存器位定义添加 SET 操作位:
   ```c
   #define QMC5883P_CTRL2_SET          (1U << 7)   /**< 执行SET操作 (激活磁力计) */
   #define QMC5883P_CTRL2_RESET        (1U << 6)   /**< 执行RESET操作 */
   ```

### 文件 2: `firmware/stm32/drivers/qmc5883p.c`
1. 在 `qmc5883p_init()` 中，配置完 CTRL1 后添加:
   - 执行 SET 操作: 写 CTRL2 寄存器置位 `QMC5883P_CTRL2_SET`
   - 等待 10ms（SET 操作完成时间）

2. 在 `qmc5883p_init()` 完成后添加等待首次数据就绪:
   - 循环检查 `qmc5883p_data_ready()`，最多等待 50ms
   - 超时返回 `HAL_TIMEOUT`

3. 修改 `qmc5883p_read_data()` 中读取前的数据就绪检查:
   - 当前代码直接读 7 字节，未检查 DRDY
   - 添加: 若 STATUS.DRDY=0，返回 `HAL_BUSY` 让调用者重试

## 完成标准 (必须可验证)

- [ ] 标准 1: `qmc5883p_init()` 执行 SET 操作后等待 10ms
- [ ] 标准 2: 初始化后等待首次 DRDY 就绪再返回
- [ ] 标准 3: `qmc5883p_read_data()` 在读取前检查 DRDY
- [ ] 标准 4: 固件编译成功
- [ ] 标准 5: 磁力计三轴数据均非零
  - 验证方法: 调试串口输出 X/Y/Z 三轴均有合理读数（±几百到几千 counts）
  - 验证方法: 旋转传感器时各轴读数变化
- [ ] 代码审查通过

## 相关文件 (Hook范围检查用)
- `firmware/stm32/drivers/qmc5883p.h`
- `firmware/stm32/drivers/qmc5883p.c`

## 注意事项
- SET/RESET 操作在 QMC5883P 数据手册中为可选，但强烈建议
- 如修复后仍然 Y/Z 为 0，考虑检查 I2C 读取的字节数（是否完整读取 6 字节）
- CTRL2 的 SET 位写入后会自动清零，不需要主动清除

## 验收清单 (Reviewer使用)
- [ ] SET 操作在 CTRL1 配置之后执行
- [ ] 有首次数据就绪等待
- [ ] read_data 有 DRDY 检查
- [ ] 无新引入的未声明依赖
