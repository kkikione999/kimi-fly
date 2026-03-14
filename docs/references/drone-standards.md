# Drone/UAV Standards and References

无人机/飞行器开发相关标准和参考资料。

## 安全标准

### 中国
- **AC-92-01**: 民用无人驾驶航空器系统适航审定管理程序
- **CCAR-92**: 民用无人驾驶航空器运行管理规定
- **GB/T 38058-2019**: 民用多旋翼无人机系统通用要求

### 国际
- **ASTM F2910**: Standard Specification for Design and Construction of Multi-Rotor Unmanned Aircraft Systems
- **ASTM F3005**: Standard Specification for Batteries for Use in Small Unmanned Aircraft Systems
- **ISO 21384-3**: Unmanned aircraft systems — Part 3: Operational procedures

## 通信协议

### 遥控协议
- **CRSF**: Crossfire (TBS)
- **SBUS**: Futaba serial bus
- **IBUS**: Flysky serial protocol
- **DSM/DSM2/DSMX**: Spektrum protocol

### 遥测协议
- **MAVLink**: Micro Air Vehicle communication protocol
- **MSP**: MultiWii Serial Protocol (Betaflight)
- **LTM**: Light Telemetry

### 电机控制
- **PWM**: Standard servo pulse (1000-2000us)
- **OneShot125**: 125us pulse width
- **OneShot42**: 42us pulse width
- **DShot**: Digital shot (DShot150/300/600/1200)

## 飞控算法参考

### 姿态估计
- **Madgwick Filter**: Gradient descent algorithm
- **Mahony Filter**: Complementary filter
- **EKF**: Extended Kalman Filter

### 控制理论
- **PID**: Proportional-Integral-Derivative control
- **Cascade PID**: Rate + Angle control
- **LQR**: Linear Quadratic Regulator

## 开源项目参考

### 飞控固件
- **Betaflight**: Popular FPV racing firmware
- **INAV**: Navigation-focused firmware
- **ArduPilot**: Full-featured autopilot
- **PX4**: Professional autopilot

### 软件框架
- **libopencm3**: Open source ARM Cortex-M library
- **ChibiOS**: RTOS for embedded systems
- **FreeRTOS**: Widely used RTOS

## 电机/电调

### 电机参数
- KV rating: RPM per volt
- Stator size: e.g., 2207 (22mm diameter, 7mm height)
- Max current: Amps

### 电调固件
- **BLHeli_S**: 8-bit ESC firmware
- **BLHeli_32**: 32-bit ESC firmware
- **AM32**: Open source 32-bit ESC firmware

## 传感器规格

### IMU
- **Accelerometer**: ±2g/±4g/±8g/±16g range
- **Gyroscope**: ±250/±500/±1000/±2000 dps range
- **ODR**: Output data rate (typically 1-8kHz)

### 气压计
- **Resolution**: ~10cm altitude
- **Update rate**: Typically 20-100Hz

### GPS
- **Update rate**: 1-10Hz typical
- **Accuracy**: 2-3m (standard GPS), <1m (RTK)

## 电池安全

### LiPo Guidelines
- Never discharge below 3.0V per cell
- Charge at 1C max (unless specified)
- Store at 3.8V per cell (storage charge)
- Use fireproof bag for charging/storage

### 计算公式
```
Flight time (min) = (Battery capacity mAh / Average current mA) * 60 * 0.8
                    ^ safety margin
```

## 工具链

### 编译器
- **ARM GCC**: arm-none-eabi-gcc
- **RISC-V GCC**: riscv32-esp-elf-gcc

### 调试
- **OpenOCD**: Open On-Chip Debugger
- **GDB**: GNU Debugger
- **SEGGER J-Link**: Commercial debug probe

### 仿真
- **MATLAB/Simulink**: Control system design
- **Gazebo**: Physics simulation (ROS)

---

*This is a living document. Add new references as the project evolves.*
