# 技术路径规范

> 原则：上下文可管理、确定性行为、可验证

---

## 1. 核心约束

| 项目 | 规则 | 理由 |
|------|------|------|
| 代码生成 | 手写，禁止CubeMX | AI可操作，透明 |
| 内存分配 | FreeRTOS静态分配，禁止malloc | 可追踪，无碎片 |
| 文件长度 | ≤200行 | AI上下文可全读 |
| 函数长度 | ≤50行 | 单屏可读 |
| 错误处理 | 必须检查返回值 | 不静默失败 |

---

## 2. 编码模式

### STM32初始化
```c
// 直接寄存器配置，一行完成一个设置
TIM2->PSC = 84 - 1;     // 1MHz timer clock
TIM2->ARR = 2500 - 1;   // 400Hz PWM
TIM2->CCMR1 = TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1PE;
```

### 静态任务
```c
static StaticTask_t xTaskBuffer;
static StackType_t xStack[256];
TaskHandle_t g_taskHandle = xTaskCreateStatic(TaskFunc, "name", 256, NULL, 2, xStack, &xTaskBuffer);
```

### 错误码
```c
#define ERR_LAYER_HAL     0x1000
#define ERR_LAYER_DRIVER  0x2000
#define ERR_HAL_I2C_TIMEOUT  (ERR_LAYER_HAL | 0x0201)
```

---

## 3. 目录结构

```
firmware/
├── stm32/
│   ├── hal/          # 寄存器配置，MX_*_Init()
│   ├── drivers/      # 传感器驱动
│   ├── control/      # PID, AHRS
│   └── tasks/        # FreeRTOS任务
├── esp32c3/
│   ├── main.c
│   ├── wifi_sta.c    # WiFi连接
│   ├── tcp_server.c  # TCP服务
│   └── protocol.c    # 协议解析
└── shared/
    └── protocol.h    # 双端共用
```

---

## 4. ESP32-C3 约束

- **任务上限**: 最多4个任务
  - `WiFiManager` (优先级3) - 连接管理、自动重连
  - `TCPServer` (优先级2) - 监听、客户端管理
  - `UARTBridge` (优先级2) - TCP↔UART数据转发
  - `Protocol` (优先级1) - 协议帧解析、心跳管理
- **配置固化**: 代码中常量，无menuconfig
- **版本**: ESP-IDF 5.1.2

---

## 5. 版本锁定

| 组件 | 版本 |
|------|------|
| STM32CubeF4 HAL | 1.28.0 |
| FreeRTOS | 10.5.1 |
| ESP-IDF | 5.1.2 |
| GCC ARM | 12.2.Rel1 |

---

## 6. 检查清单

编码前确认：
- [ ] 任务已拆分到单文件≤200行
- [ ] 错误处理策略明确
- [ ] 调试输出接口已定义
