/**
 * @file uart.c
 * @brief STM32 UART HAL实现 (STM32Cube HAL API)
 * @note USART2用于ESP32-C3 WiFi通信
 *       GPIO: PA2 (TX), PA3 (RX)
 *       时钟源: APB1 = 42MHz
 */

#include "uart.h"
#include "stm32f4xx_hal.h"

/* ============================================================================
 * 私有变量
 * ============================================================================ */

/* UART句柄 - 使用HAL库提供的UART_HandleTypeDef */
static UART_HandleTypeDef huart1;
static UART_HandleTypeDef huart2;

/* ============================================================================
 * 私有函数声明
 * ============================================================================ */

static void uart_gpio_init(uart_instance_t instance);
static void uart_gpio_deinit(uart_instance_t instance);

/* ============================================================================
 * 私有函数实现
 * ============================================================================ */

/**
 * @brief 配置UART GPIO
 * @param instance UART实例
 */
static void uart_gpio_init(uart_instance_t instance)
{
    GPIO_InitTypeDef gpio_init_struct;

    if (instance == UART_INSTANCE_1) {
        /* 使能GPIOA时钟 */
        __HAL_RCC_GPIOA_CLK_ENABLE();

        /* PA9 - USART1_TX: 复用推挽输出 */
        gpio_init_struct.Pin = GPIO_PIN_9;
        gpio_init_struct.Mode = GPIO_MODE_AF_PP;
        gpio_init_struct.Pull = GPIO_PULLUP;
        gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;
        gpio_init_struct.Alternate = GPIO_AF7_USART1;
        HAL_GPIO_Init(GPIOA, &gpio_init_struct);

        /* PA10 - USART1_RX: 复用输入 */
        gpio_init_struct.Pin = GPIO_PIN_10;
        gpio_init_struct.Mode = GPIO_MODE_AF_PP;
        gpio_init_struct.Pull = GPIO_PULLUP;
        gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;
        gpio_init_struct.Alternate = GPIO_AF7_USART1;
        HAL_GPIO_Init(GPIOA, &gpio_init_struct);
    } else if (instance == UART_INSTANCE_2) {
        /* 使能GPIOA时钟 */
        __HAL_RCC_GPIOA_CLK_ENABLE();

        /* PA2 - USART2_TX: 复用推挽输出 */
        gpio_init_struct.Pin = GPIO_PIN_2;
        gpio_init_struct.Mode = GPIO_MODE_AF_PP;
        gpio_init_struct.Pull = GPIO_PULLUP;
        gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;
        gpio_init_struct.Alternate = GPIO_AF7_USART2;
        HAL_GPIO_Init(GPIOA, &gpio_init_struct);

        /* PA3 - USART2_RX: 复用输入 */
        gpio_init_struct.Pin = GPIO_PIN_3;
        gpio_init_struct.Mode = GPIO_MODE_AF_PP;
        gpio_init_struct.Pull = GPIO_PULLUP;
        gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;
        gpio_init_struct.Alternate = GPIO_AF7_USART2;
        HAL_GPIO_Init(GPIOA, &gpio_init_struct);
    }
}

/**
 * @brief 反初始化UART GPIO
 * @param instance UART实例
 */
static void uart_gpio_deinit(uart_instance_t instance)
{
    if (instance == UART_INSTANCE_1) {
        /* PA9 - 恢复为输入模式 */
        HAL_GPIO_DeInit(GPIOA, GPIO_PIN_9);

        /* PA10 - 恢复为输入模式 */
        HAL_GPIO_DeInit(GPIOA, GPIO_PIN_10);
    } else if (instance == UART_INSTANCE_2) {
        /* PA2 - 恢复为输入模式 */
        HAL_GPIO_DeInit(GPIOA, GPIO_PIN_2);

        /* PA3 - 恢复为输入模式 */
        HAL_GPIO_DeInit(GPIOA, GPIO_PIN_3);
    }
}

/**
 * @brief 将自定义波特率配置转换为HAL波特率值
 * @param baudrate 波特率值
 * @return HAL库使用的波特率值
 */
static uint32_t uart_get_hal_baudrate(uint32_t baudrate)
{
    return baudrate;
}

/**
 * @brief 将自定义数据位配置转换为HAL WordLength值
 * @param databits 数据位枚举
 * @return HAL库使用的WordLength值
 */
static uint32_t uart_get_hal_wordlength(uart_databits_t databits)
{
    switch (databits) {
        case UART_DATABITS_9:
            return UART_WORDLENGTH_9B;
        case UART_DATABITS_8:
        default:
            return UART_WORDLENGTH_8B;
    }
}

/**
 * @brief 将自定义停止位配置转换为HAL StopBits值
 * @param stopbits 停止位枚举
 * @return HAL库使用的StopBits值
 */
static uint32_t uart_get_hal_stopbits(uart_stopbits_t stopbits)
{
    switch (stopbits) {
        case UART_STOPBITS_0_5_VAL:
            return UART_STOPBITS_0_5;
        case UART_STOPBITS_2_VAL:
            return UART_STOPBITS_2;
        case UART_STOPBITS_1_5_VAL:
            return UART_STOPBITS_1_5;
        case UART_STOPBITS_1_VAL:
        default:
            return UART_STOPBITS_1;
    }
}

/**
 * @brief 将自定义校验位配置转换为HAL Parity值
 * @param parity 校验位枚举
 * @return HAL库使用的Parity值
 */
static uint32_t uart_get_hal_parity(uart_parity_t parity)
{
    switch (parity) {
        case UART_PARITY_EVEN_VAL:
            return UART_PARITY_EVEN;
        case UART_PARITY_ODD_VAL:
            return UART_PARITY_ODD;
        case UART_PARITY_NONE_VAL:
        default:
            return UART_PARITY_NONE;
    }
}

/**
 * @brief 将自定义模式配置转换为HAL Mode值
 * @param mode 模式枚举
 * @return HAL库使用的Mode值
 */
static uint32_t uart_get_hal_mode(uart_mode_t mode)
{
    switch (mode) {
        case UART_MODE_RX_VAL:
            return UART_MODE_RX;
        case UART_MODE_TX_VAL:
            return UART_MODE_TX;
        case UART_MODE_TX_RX_VAL:
        default:
            return UART_MODE_TX_RX;
    }
}

/**
 * @brief 将自定义硬件流控配置转换为HAL HwFlowCtl值
 * @param hwcontrol 硬件流控枚举
 * @return HAL库使用的HwFlowCtl值
 */
static uint32_t uart_get_hal_hwflowctl(uart_hwcontrol_t hwcontrol)
{
    switch (hwcontrol) {
        case UART_HWCONTROL_RTS_VAL:
            return UART_HWCONTROL_RTS;
        case UART_HWCONTROL_CTS_VAL:
            return UART_HWCONTROL_CTS;
        case UART_HWCONTROL_RTS_CTS_VAL:
            return UART_HWCONTROL_RTS_CTS;
        case UART_HWCONTROL_NONE_VAL:
        default:
            return UART_HWCONTROL_NONE;
    }
}

/**
 * @brief 将HAL错误码转换为自定义错误码
 * @param hal_error HAL错误码
 * @return 自定义错误码
 */
static uint32_t uart_convert_hal_error(uint32_t hal_error)
{
    uint32_t error_code = UART_ERROR_NONE;

    if (hal_error & HAL_UART_ERROR_PE) {
        error_code |= UART_ERROR_PE;
    }
    if (hal_error & HAL_UART_ERROR_NE) {
        error_code |= UART_ERROR_NE;
    }
    if (hal_error & HAL_UART_ERROR_FE) {
        error_code |= UART_ERROR_FE;
    }
    if (hal_error & HAL_UART_ERROR_ORE) {
        error_code |= UART_ERROR_ORE;
    }
    if (hal_error & HAL_UART_ERROR_DMA) {
        error_code |= UART_ERROR_DMA;
    }

    return error_code;
}

/* ============================================================================
 * 公共API实现
 * ============================================================================ */

/**
 * @brief 获取HAL UART句柄
 * @param instance UART实例
 * @return HAL UART句柄指针
 */
static UART_HandleTypeDef* uart_get_hal_handle(uart_instance_t instance)
{
    if (instance == UART_INSTANCE_1) {
        return &huart1;
    } else if (instance == UART_INSTANCE_2) {
        return &huart2;
    }
    return NULL;
}

/**
 * @brief 初始化UART外设
 * @param huart UART句柄指针
 * @param instance UART实例
 * @param config UART配置指针
 * @return HAL状态
 */
hal_status_t uart_init(uart_handle_t *huart, uart_instance_t instance, const uart_config_t *config)
{
    HAL_StatusTypeDef hal_status;
    UART_HandleTypeDef *hal_huart = NULL;

    /* 参数检查 */
    if (huart == NULL || config == NULL) {
        return HAL_ERROR;
    }

    if (instance != UART_INSTANCE_1 && instance != UART_INSTANCE_2) {
        return HAL_ERROR;   /* 目前只支持USART1和USART2 */
    }

    /* 初始化句柄 */
    huart->instance = instance;
    /* 设置忙状态 */
    huart->state = UART_STATE_BUSY;
    huart->error_code = UART_ERROR_NONE;

    /* 保存配置 */
    huart->config = *config;

    /* 根据实例选择HAL UART句柄 */
    if (instance == UART_INSTANCE_1) {
        hal_huart = &huart1;
        hal_huart->Instance = USART1;
        /* 使能USART1时钟 (APB2) */
        __HAL_RCC_USART1_CLK_ENABLE();
    } else {
        hal_huart = &huart2;
        hal_huart->Instance = USART2;
        /* 使能USART2时钟 (APB1) */
        __HAL_RCC_USART2_CLK_ENABLE();
    }

    /* 配置HAL UART参数 */
    hal_huart->Init.BaudRate = uart_get_hal_baudrate(config->baudrate);
    hal_huart->Init.WordLength = uart_get_hal_wordlength(config->databits);
    hal_huart->Init.StopBits = uart_get_hal_stopbits(config->stopbits);
    hal_huart->Init.Parity = uart_get_hal_parity(config->parity);
    hal_huart->Init.Mode = uart_get_hal_mode(config->mode);
    hal_huart->Init.HwFlowCtl = uart_get_hal_hwflowctl(config->hwcontrol);
    hal_huart->Init.OverSampling = UART_OVERSAMPLING_16;

    /* 配置GPIO */
    uart_gpio_init(instance);

    /* 初始化UART */
    hal_status = HAL_UART_Init(hal_huart);

    if (hal_status != HAL_OK) {
        huart->state = UART_STATE_ERROR;
        huart->error_code = uart_convert_hal_error(hal_huart->ErrorCode);
        return HAL_ERROR;
    }

    /* 设置状态为就绪 */
    huart->state = UART_STATE_READY;

    return HAL_OK;
}

/**
 * @brief 反初始化UART外设
 * @param huart UART句柄指针
 * @return HAL状态
 */
hal_status_t uart_deinit(uart_handle_t *huart)
{
    HAL_StatusTypeDef hal_status;
    UART_HandleTypeDef *hal_huart = NULL;

    if (huart == NULL) {
        return HAL_ERROR;
    }

    if (huart->instance != UART_INSTANCE_1 && huart->instance != UART_INSTANCE_2) {
        return HAL_ERROR;
    }

    hal_huart = uart_get_hal_handle(huart->instance);
    if (hal_huart == NULL) {
        return HAL_ERROR;
    }

    huart->state = UART_STATE_BUSY;

    /* 反初始化HAL UART */
    hal_status = HAL_UART_DeInit(hal_huart);

    if (hal_status != HAL_OK) {
        huart->state = UART_STATE_ERROR;
        return HAL_ERROR;
    }

    /* 反初始化GPIO */
    uart_gpio_deinit(huart->instance);

    /* 禁用USART时钟 */
    if (huart->instance == UART_INSTANCE_1) {
        __HAL_RCC_USART1_CLK_DISABLE();
    } else {
        __HAL_RCC_USART2_CLK_DISABLE();
    }

    /* 重置句柄 */
    huart->state = UART_STATE_RESET;
    huart->error_code = UART_ERROR_NONE;

    return HAL_OK;
}

/**
 * @brief 发送数据 (阻塞模式)
 * @param huart UART句柄指针
 * @param data 数据缓冲区指针
 * @param size 数据大小
 * @param timeout 超时时间(毫秒)
 * @return HAL状态
 */
hal_status_t uart_send(uart_handle_t *huart, const uint8_t *data, uint16_t size, uint32_t timeout)
{
    HAL_StatusTypeDef hal_status;
    UART_HandleTypeDef *hal_huart = NULL;

    if (huart == NULL || data == NULL || size == 0) {
        return HAL_ERROR;
    }

    if (huart->state != UART_STATE_READY && huart->state != UART_STATE_BUSY) {
        return HAL_BUSY;
    }

    if (huart->instance != UART_INSTANCE_1 && huart->instance != UART_INSTANCE_2) {
        return HAL_ERROR;
    }

    hal_huart = uart_get_hal_handle(huart->instance);
    if (hal_huart == NULL) {
        return HAL_ERROR;
    }

    /* 设置忙状态 */
    huart->state = UART_STATE_BUSY;
    huart->tx_buffer = (uint8_t *)data;
    huart->tx_size = size;
    huart->tx_count = 0;

    /* 使用HAL_UART_Transmit发送数据 */
    hal_status = HAL_UART_Transmit(hal_huart, (uint8_t *)data, size, timeout);

    if (hal_status == HAL_OK) {
        huart->tx_count = size;
        huart->state = UART_STATE_READY;
        return HAL_OK;
    } else if (hal_status == HAL_TIMEOUT) {
        huart->error_code |= UART_ERROR_TIMEOUT;
        huart->state = UART_STATE_ERROR;
        return HAL_TIMEOUT;
    } else {
        huart->error_code = uart_convert_hal_error(hal_huart->ErrorCode);
        huart->state = UART_STATE_ERROR;
        return HAL_ERROR;
    }
}

/**
 * @brief 接收数据 (阻塞模式)
 * @param huart UART句柄指针
 * @param data 数据缓冲区指针
 * @param size 数据大小
 * @param timeout 超时时间(毫秒)
 * @return HAL状态
 */
hal_status_t uart_receive(uart_handle_t *huart, uint8_t *data, uint16_t size, uint32_t timeout)
{
    HAL_StatusTypeDef hal_status;
    UART_HandleTypeDef *hal_huart = NULL;

    if (huart == NULL || data == NULL || size == 0) {
        return HAL_ERROR;
    }

    if (huart->state != UART_STATE_READY && huart->state != UART_STATE_BUSY) {
        return HAL_BUSY;
    }

    if (huart->instance != UART_INSTANCE_1 && huart->instance != UART_INSTANCE_2) {
        return HAL_ERROR;
    }

    hal_huart = uart_get_hal_handle(huart->instance);
    if (hal_huart == NULL) {
        return HAL_ERROR;
    }

    /* 设置忙状态 */
    huart->state = UART_STATE_BUSY;
    huart->rx_buffer = data;
    huart->rx_size = size;
    huart->rx_count = 0;

    /* 使用HAL_UART_Receive接收数据 */
    hal_status = HAL_UART_Receive(hal_huart, data, size, timeout);

    if (hal_status == HAL_OK) {
        huart->rx_count = size;
        huart->state = UART_STATE_READY;
        return HAL_OK;
    } else if (hal_status == HAL_TIMEOUT) {
        huart->error_code |= UART_ERROR_TIMEOUT;
        huart->state = UART_STATE_ERROR;
        return HAL_TIMEOUT;
    } else {
        huart->error_code = uart_convert_hal_error(hal_huart->ErrorCode);
        huart->state = UART_STATE_ERROR;
        return HAL_ERROR;
    }
}

/**
 * @brief 动态修改波特率
 * @param huart UART句柄指针
 * @param baudrate 新波特率
 * @return HAL状态
 */
hal_status_t uart_set_baudrate(uart_handle_t *huart, uint32_t baudrate)
{
    HAL_StatusTypeDef hal_status;
    UART_HandleTypeDef *hal_huart = NULL;

    if (huart == NULL) {
        return HAL_ERROR;
    }

    if (huart->instance != UART_INSTANCE_1 && huart->instance != UART_INSTANCE_2) {
        return HAL_ERROR;
    }

    hal_huart = uart_get_hal_handle(huart->instance);
    if (hal_huart == NULL) {
        return HAL_ERROR;
    }

    /* 临时禁用UART以修改配置 */
    HAL_UART_Abort(hal_huart);

    /* 更新波特率配置 */
    hal_huart->Init.BaudRate = uart_get_hal_baudrate(baudrate);

    /* 重新初始化UART */
    hal_status = HAL_UART_Init(hal_huart);

    if (hal_status == HAL_OK) {
        huart->config.baudrate = baudrate;
        return HAL_OK;
    } else {
        huart->error_code = uart_convert_hal_error(hal_huart->ErrorCode);
        return HAL_ERROR;
    }
}

/**
 * @brief 获取当前波特率
 * @param huart UART句柄指针
 * @return 当前波特率
 */
uint32_t uart_get_baudrate(const uart_handle_t *huart)
{
    if (huart == NULL) {
        return 0;
    }

    return huart->config.baudrate;
}

/**
 * @brief 检查UART是否忙
 * @param huart UART句柄指针
 * @return true=忙, false=空闲
 */
bool uart_is_busy(const uart_handle_t *huart)
{
    HAL_UART_StateTypeDef hal_state;
    UART_HandleTypeDef *hal_huart = NULL;

    if (huart == NULL) {
        return false;
    }

    hal_huart = uart_get_hal_handle(huart->instance);
    if (hal_huart == NULL) {
        return false;
    }

    /* 获取HAL UART状态 */
    hal_state = HAL_UART_GetState(hal_huart);

    return (hal_state == HAL_UART_STATE_BUSY ||
            hal_state == HAL_UART_STATE_BUSY_TX ||
            hal_state == HAL_UART_STATE_BUSY_RX ||
            hal_state == HAL_UART_STATE_BUSY_TX_RX);
}

/**
 * @brief 清除错误标志
 * @param huart UART句柄指针
 */
void uart_clear_error(uart_handle_t *huart)
{
    UART_HandleTypeDef *hal_huart = NULL;

    if (huart == NULL) {
        return;
    }

    if (huart->instance != UART_INSTANCE_1 && huart->instance != UART_INSTANCE_2) {
        return;
    }

    hal_huart = uart_get_hal_handle(huart->instance);
    if (hal_huart == NULL) {
        return;
    }

    /* 清除HAL UART错误标志 */
    __HAL_UART_CLEAR_PEFLAG(hal_huart);
    __HAL_UART_CLEAR_FEFLAG(hal_huart);
    __HAL_UART_CLEAR_NEFLAG(hal_huart);
    __HAL_UART_CLEAR_OREFLAG(hal_huart);

    /* 清除句柄中的错误码 */
    huart->error_code = UART_ERROR_NONE;

    /* 如果之前是错误状态,恢复为就绪 */
    if (huart->state == UART_STATE_ERROR) {
        huart->state = UART_STATE_READY;
    }
}

/**
 * @brief 获取错误码
 * @param huart UART句柄指针
 * @return 错误码
 */
uint32_t uart_get_error(const uart_handle_t *huart)
{
    UART_HandleTypeDef *hal_huart = NULL;

    if (huart == NULL) {
        return UART_ERROR_NONE;
    }

    /* 同步HAL UART的错误码 */
    hal_huart = uart_get_hal_handle(huart->instance);
    if (hal_huart != NULL) {
        return uart_convert_hal_error(hal_huart->ErrorCode) | huart->error_code;
    }

    return huart->error_code;
}
