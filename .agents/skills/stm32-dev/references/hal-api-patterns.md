# HAL 外设配置代码片段参考

> 这些是常用模板。实际使用前请通过 context7 MCP 确认函数签名与参数细节。

---

## UART

### 阻塞发送 / 接收
```c
// 发送字符串（阻塞，超时 100ms）
HAL_UART_Transmit(&huart2, (uint8_t*)"Hello\r\n", 7, 100);

// 接收固定长度（阻塞）
uint8_t buf[10];
HAL_UART_Receive(&huart2, buf, sizeof(buf), 1000);
```

### DMA 接收（空闲中断 + DMA，推荐用于不定长帧）
```c
// 在 main.c 初始化后启动
HAL_UARTEx_ReceiveToIdle_DMA(&huart2, rx_buf, RX_BUF_SIZE);
__HAL_DMA_DISABLE_IT(&hdma_usart2_rx, DMA_IT_HT); // 关闭半完成中断（可选）

// 回调（在 stm32xxx_it.c 调用 HAL_UART_IRQHandler 后触发）
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size) {
    if (huart->Instance == USART2) {
        // 处理 rx_buf[0..Size-1]
        HAL_UARTEx_ReceiveToIdle_DMA(&huart2, rx_buf, RX_BUF_SIZE); // 重启
    }
}
```

---

## SPI

```c
// 全双工（阻塞）
uint8_t tx[4] = {0x01, 0x02, 0x03, 0x04};
uint8_t rx[4];
HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, GPIO_PIN_RESET); // CS low
HAL_SPI_TransmitReceive(&hspi1, tx, rx, 4, 100);
HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, GPIO_PIN_SET);   // CS high
```

---

## I2C

```c
// 主机发送（写寄存器）
uint8_t data[2] = {REG_ADDR, value};
HAL_I2C_Master_Transmit(&hi2c1, DEVICE_ADDR << 1, data, 2, 100);

// 主机接收（读寄存器，先写地址再读）
HAL_I2C_Master_Transmit(&hi2c1, DEVICE_ADDR << 1, &REG_ADDR, 1, 100);
HAL_I2C_Master_Receive(&hi2c1,  DEVICE_ADDR << 1, rx_buf, len, 100);

// 或使用 Mem API（更简洁）
HAL_I2C_Mem_Write(&hi2c1, DEVICE_ADDR << 1, REG_ADDR, I2C_MEMADD_SIZE_8BIT, data, len, 100);
HAL_I2C_Mem_Read (&hi2c1, DEVICE_ADDR << 1, REG_ADDR, I2C_MEMADD_SIZE_8BIT, rx_buf, len, 100);
```

---

## TIM PWM

```c
// 启动 PWM（在 HAL_TIM_PWM_MspInit 之后调用）
HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);

// 动态修改占空比（0 ~ ARR）
__HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, duty); // duty: 0..ARR

// ARR（周期值）在 CubeMX 设置，也可动态修改
__HAL_TIM_SET_AUTORELOAD(&htim3, period - 1);
```

---

## ADC（DMA 多通道）

```c
// 启动 DMA 转换（连续模式）
uint32_t adc_buf[NUM_CHANNELS];
HAL_ADC_Start_DMA(&hadc1, adc_buf, NUM_CHANNELS);

// 转换完成回调
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc) {
    if (hadc->Instance == ADC1) {
        // adc_buf 已更新
    }
}
```

---

## GPIO

```c
// 写引脚
HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);

// 翻转引脚
HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);

// 读引脚
GPIO_PinState state = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_0);
```

---

## FreeRTOS 常用 API

```c
// 创建任务
xTaskCreate(
    myTaskFunction,   // 函数指针
    "MyTask",         // 名称（调试用）
    128,              // 栈大小（word 为单位）
    NULL,             // 参数
    1,                // 优先级
    &myTaskHandle     // 任务句柄（可为 NULL）
);

// 延时（相对）
vTaskDelay(pdMS_TO_TICKS(100));  // 100ms

// 延时（绝对，防止漂移）
TickType_t last = xTaskGetTickCount();
vTaskDelayUntil(&last, pdMS_TO_TICKS(10));

// 队列
QueueHandle_t q = xQueueCreate(10, sizeof(uint32_t));
xQueueSend(q, &value, portMAX_DELAY);         // 发送（阻塞）
xQueueReceive(q, &received, portMAX_DELAY);   // 接收（阻塞）

// 信号量
SemaphoreHandle_t sem = xSemaphoreCreateBinary();
xSemaphoreGive(sem);          // 释放（或在中断中用 FromISR 版本）
xSemaphoreTake(sem, portMAX_DELAY); // 等待
```
