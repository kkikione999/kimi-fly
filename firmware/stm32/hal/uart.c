/**
 * @file uart.c
 * @brief STM32 UART HAL实现
 * @note USART2用于ESP32-C3 WiFi通信
 *       GPIO: PA2 (TX), PA3 (RX)
 *       时钟源: APB1 = 42MHz
 */

#include "uart.h"

/* ============================================================================
 * 寄存器定义
 * ============================================================================ */

/* USART2 基地址 */
#define USART2_BASE_ADDR    0x40004400UL

/* RCC 寄存器 */
#define RCC_BASE_ADDR       0x40023800UL
#define RCC_APB1ENR_OFFSET  0x40UL
#define RCC_APB1RSTR_OFFSET 0x20UL

#define RCC_APB1ENR     (*(volatile uint32_t *)(RCC_BASE_ADDR + RCC_APB1ENR_OFFSET))
#define RCC_APB1RSTR    (*(volatile uint32_t *)(RCC_BASE_ADDR + RCC_APB1RSTR_OFFSET))

/* USART2时钟使能位 */
#define RCC_APB1ENR_USART2EN    (1UL << 17)
#define RCC_APB1RSTR_USART2RST  (1UL << 17)

/* USART寄存器结构 */
typedef struct {
    volatile uint32_t SR;       /* 0x00 - 状态寄存器 */
    volatile uint32_t DR;       /* 0x04 - 数据寄存器 */
    volatile uint32_t BRR;      /* 0x08 - 波特率寄存器 */
    volatile uint32_t CR1;      /* 0x0C - 控制寄存器1 */
    volatile uint32_t CR2;      /* 0x10 - 控制寄存器2 */
    volatile uint32_t CR3;      /* 0x14 - 控制寄存器3 */
    volatile uint32_t GTPR;     /* 0x18 - 保护时间和预分频寄存器 */
} usart_reg_t;

/* USART2寄存器指针 */
#define USART2  ((usart_reg_t *)USART2_BASE_ADDR)

/* ============================================================================
 * 寄存器位定义
 * ============================================================================ */

/* SR - 状态寄存器 */
#define USART_SR_PE     (1UL << 0)   /* 校验错误 */
#define USART_SR_FE     (1UL << 1)   /* 帧错误 */
#define USART_SR_NE     (1UL << 2)   /* 噪声错误 */
#define USART_SR_ORE    (1UL << 3)   /* 溢出错误 */
#define USART_SR_IDLE   (1UL << 4)   /* 空闲线路检测 */
#define USART_SR_RXNE   (1UL << 5)   /* 接收数据寄存器非空 */
#define USART_SR_TC     (1UL << 6)   /* 发送完成 */
#define USART_SR_TXE    (1UL << 7)   /* 发送数据寄存器空 */
#define USART_SR_LBD    (1UL << 8)   /* LIN断开检测 */
#define USART_SR_CTS    (1UL << 9)   /* CTS标志 */

/* CR1 - 控制寄存器1 */
#define USART_CR1_SBK       (1UL << 0)   /* 发送断开 */
#define USART_CR1_RWU       (1UL << 1)   /* 接收唤醒 */
#define USART_CR1_RE        (1UL << 2)   /* 接收使能 */
#define USART_CR1_TE        (1UL << 3)   /* 发送使能 */
#define USART_CR1_IDLEIE    (1UL << 4)   /* 空闲中断使能 */
#define USART_CR1_RXNEIE    (1UL << 5)   /* RXNE中断使能 */
#define USART_CR1_TCIE      (1UL << 6)   /* 发送完成中断使能 */
#define USART_CR1_TXEIE     (1UL << 7)   /* TXE中断使能 */
#define USART_CR1_PEIE      (1UL << 8)   /* 校验错误中断使能 */
#define USART_CR1_PS        (1UL << 9)   /* 校验选择 */
#define USART_CR1_PCE       (1UL << 10)  /* 校验控制使能 */
#define USART_CR1_WAKE      (1UL << 11)  /* 唤醒方法 */
#define USART_CR1_M         (1UL << 12)  /* 字长 */
#define USART_CR1_UE        (1UL << 13)  /* USART使能 */
#define USART_CR1_OVER8     (1UL << 15)  /* 过采样8模式 */

/* CR2 - 控制寄存器2 */
#define USART_CR2_ADD_MASK  (0x0FUL << 0)    /* 地址掩码 */
#define USART_CR2_LBDL      (1UL << 5)       /* LIN断开长度 */
#define USART_CR2_LBDIE     (1UL << 6)       /* LIN断开中断使能 */
#define USART_CR2_LBCL      (1UL << 8)       /* 最后位时钟脉冲 */
#define USART_CR2_CPHA      (1UL << 9)       /* 时钟相位 */
#define USART_CR2_CPOL      (1UL << 10)      /* 时钟极性 */
#define USART_CR2_CLKEN     (1UL << 11)      /* 时钟使能 */
#define USART_CR2_STOP_MASK (0x03UL << 12)   /* 停止位掩码 */
#define USART_CR2_STOP_1    (0x00UL << 12)   /* 1个停止位 */
#define USART_CR2_STOP_0_5  (0x01UL << 12)   /* 0.5个停止位 */
#define USART_CR2_STOP_2    (0x02UL << 12)   /* 2个停止位 */
#define USART_CR2_STOP_1_5  (0x03UL << 12)   /* 1.5个停止位 */
#define USART_CR2_LINEN     (1UL << 14)      /* LIN模式使能 */

/* CR3 - 控制寄存器3 */
#define USART_CR3_EIE       (1UL << 0)   /* 错误中断使能 */
#define USART_CR3_IREN      (1UL << 1)   /* 红外模式使能 */
#define USART_CR3_IRLP      (1UL << 2)   /* 红外低功耗 */
#define USART_CR3_HDSEL     (1UL << 3)   /* 半双工选择 */
#define USART_CR3_NACK      (1UL << 4)   /* 智能卡NACK使能 */
#define USART_CR3_SCEN      (1UL << 5)   /* 智能卡模式使能 */
#define USART_CR3_DMAR      (1UL << 6)   /* DMA接收使能 */
#define USART_CR3_DMAT      (1UL << 7)   /* DMA发送使能 */
#define USART_CR3_RTSE      (1UL << 8)   /* RTS使能 */
#define USART_CR3_CTSE      (1UL << 9)   /* CTS使能 */
#define USART_CR3_CTSIE     (1UL << 10)  /* CTS中断使能 */
#define USART_CR3_ONEBIT    (1UL << 11)  /* 一位采样方法 */

/* ============================================================================
 * 私有函数声明
 * ============================================================================ */

static void uart_gpio_init(uart_instance_t instance);
static void uart_gpio_deinit(uart_instance_t instance);
static hal_status_t uart_set_brr(usart_reg_t *usart, uint32_t baudrate);
static void uart_wait_timeout(uint32_t ms);

/* ============================================================================
 * 私有函数实现
 * ============================================================================ */

/**
 * @brief 简单的忙等待延时函数
 * @param ms 毫秒数
 * @note 基于APB1=42MHz, 粗略估计
 */
static void uart_wait_timeout(uint32_t ms)
{
    volatile uint32_t count;
    /* 粗略延时: 42MHz时钟, 约42000个周期1ms */
    for (count = 0; count < (ms * 42000); count++) {
        __asm__ volatile("nop");
    }
}

/**
 * @brief 配置UART GPIO
 * @param instance UART实例
 */
static void uart_gpio_init(uart_instance_t instance)
{
    gpio_handle_t gpio_handle;
    gpio_config_t gpio_cfg;

    if (instance != UART_INSTANCE_2) {
        return;
    }

    /* PA2 - USART2_TX: 复用推挽输出 */
    gpio_handle.port = GPIO_PORT_A;
    gpio_handle.pin_mask = GPIO_PIN_2;
    gpio_cfg.mode = HAL_GPIO_MODE_AF;
    gpio_cfg.otype = HAL_GPIO_OTYPE_PP;
    gpio_cfg.speed = HAL_GPIO_SPEED_HIGH;
    gpio_cfg.pupd = HAL_GPIO_PUPD_UP;
    gpio_cfg.af = GPIO_AF_7;   /* AF7 = USART2 */
    gpio_init(&gpio_handle, &gpio_cfg);

    /* PA3 - USART2_RX: 复用输入 */
    gpio_handle.pin_mask = GPIO_PIN_3;
    gpio_init(&gpio_handle, &gpio_cfg);
}

/**
 * @brief 反初始化UART GPIO
 * @param instance UART实例
 */
static void uart_gpio_deinit(uart_instance_t instance)
{
    gpio_handle_t gpio_handle;
    gpio_config_t gpio_cfg;

    if (instance != UART_INSTANCE_2) {
        return;
    }

    /* PA2 - 恢复为输入模式 */
    gpio_handle.port = GPIO_PORT_A;
    gpio_handle.pin_mask = GPIO_PIN_2;
    gpio_cfg.mode = HAL_GPIO_MODE_INPUT;
    gpio_cfg.otype = HAL_GPIO_OTYPE_PP;
    gpio_cfg.speed = HAL_GPIO_SPEED_LOW;
    gpio_cfg.pupd = HAL_GPIO_PUPD_NONE;
    gpio_cfg.af = GPIO_AF_0;
    gpio_init(&gpio_handle, &gpio_cfg);

    /* PA3 - 恢复为输入模式 */
    gpio_handle.pin_mask = GPIO_PIN_3;
    gpio_init(&gpio_handle, &gpio_cfg);
}

/**
 * @brief 设置波特率寄存器
 * @param usart USART寄存器指针
 * @param baudrate 波特率
 * @return HAL状态
 * @note APB1时钟 = 42MHz
 *       使用16倍过采样 (OVER8=0)
 *       BRR = APB1_FREQ / baudrate
 */
static hal_status_t uart_set_brr(usart_reg_t *usart, uint32_t baudrate)
{
    uint32_t brr;

    if (baudrate == 0) {
        return HAL_ERROR;
    }

    /* 计算波特率寄存器值: BRR = APB1_FREQ / baudrate */
    /* 使用16倍过采样, 整数部分直接除法 */
    brr = HAL_APB1_CLK_FREQ / baudrate;

    /* 检查波特率是否在有效范围内 */
    if (brr == 0 || brr > 0xFFFF) {
        return HAL_ERROR;
    }

    usart->BRR = brr;

    return HAL_OK;
}

/* ============================================================================
 * 公共API实现
 * ============================================================================ */

/**
 * @brief 初始化UART外设
 * @param huart UART句柄指针
 * @param instance UART实例
 * @param config UART配置指针
 * @return HAL状态
 */
hal_status_t uart_init(uart_handle_t *huart, uart_instance_t instance, const uart_config_t *config)
{
    usart_reg_t *usart;
    uint32_t cr1_val = 0;
    uint32_t cr2_val = 0;
    uint32_t cr3_val = 0;

    /* 参数检查 */
    if (huart == NULL || config == NULL) {
        return HAL_ERROR;
    }

    if (instance != UART_INSTANCE_2) {
        return HAL_ERROR;   /* 目前只支持USART2 */
    }

    /* 初始化句柄 */
    huart->instance = instance;
    huart->state = HAL_STATE_BUSY;
    huart->error_code = UART_ERROR_NONE;

    /* 保存配置 */
    huart->config = *config;

    /* 获取USART寄存器指针 */
    usart = USART2;

    /* 1. 使能USART2时钟 */
    RCC_APB1ENR |= RCC_APB1ENR_USART2EN;

    /* 2. 复位USART2 */
    RCC_APB1RSTR |= RCC_APB1RSTR_USART2RST;
    for (volatile int i = 0; i < 10; i++);  /* 短暂延时 */
    RCC_APB1RSTR &= ~RCC_APB1RSTR_USART2RST;

    /* 3. 配置GPIO */
    uart_gpio_init(instance);

    /* 4. 配置波特率 */
    if (uart_set_brr(usart, config->baudrate) != HAL_OK) {
        huart->state = HAL_STATE_ERROR;
        return HAL_ERROR;
    }

    /* 5. 配置CR2: 停止位 */
    cr2_val &= ~USART_CR2_STOP_MASK;
    switch (config->stopbits) {
        case UART_STOPBITS_1:
            cr2_val |= USART_CR2_STOP_1;
            break;
        case UART_STOPBITS_0_5:
            cr2_val |= USART_CR2_STOP_0_5;
            break;
        case UART_STOPBITS_2:
            cr2_val |= USART_CR2_STOP_2;
            break;
        case UART_STOPBITS_1_5:
            cr2_val |= USART_CR2_STOP_1_5;
            break;
        default:
            cr2_val |= USART_CR2_STOP_1;
            break;
    }
    usart->CR2 = cr2_val;

    /* 6. 配置CR3: 硬件流控 */
    cr3_val = 0;
    if (config->hwcontrol & 0x01) {
        cr3_val |= USART_CR3_RTSE;
    }
    if (config->hwcontrol & 0x02) {
        cr3_val |= USART_CR3_CTSE;
    }
    usart->CR3 = cr3_val;

    /* 7. 配置CR1: 数据位、校验、模式、使能USART */
    cr1_val = 0;

    /* 数据位 */
    if (config->databits == UART_DATABITS_9) {
        cr1_val |= USART_CR1_M;
    }

    /* 校验 */
    if (config->parity != UART_PARITY_NONE) {
        cr1_val |= USART_CR1_PCE;
        if (config->parity == UART_PARITY_ODD) {
            cr1_val |= USART_CR1_PS;
        }
    }

    /* 模式 */
    if (config->mode & UART_MODE_TX) {
        cr1_val |= USART_CR1_TE;
    }
    if (config->mode & UART_MODE_RX) {
        cr1_val |= USART_CR1_RE;
    }

    /* 使能USART */
    cr1_val |= USART_CR1_UE;

    usart->CR1 = cr1_val;

    /* 8. 设置状态为就绪 */
    huart->state = HAL_STATE_READY;

    return HAL_OK;
}

/**
 * @brief 反初始化UART外设
 * @param huart UART句柄指针
 * @return HAL状态
 */
hal_status_t uart_deinit(uart_handle_t *huart)
{
    usart_reg_t *usart;

    if (huart == NULL) {
        return HAL_ERROR;
    }

    if (huart->instance != UART_INSTANCE_2) {
        return HAL_ERROR;
    }

    huart->state = HAL_STATE_BUSY;

    usart = USART2;

    /* 1. 禁用USART */
    usart->CR1 = 0;
    usart->CR2 = 0;
    usart->CR3 = 0;

    /* 2. 反初始化GPIO */
    uart_gpio_deinit(huart->instance);

    /* 3. 禁用USART时钟 */
    RCC_APB1ENR &= ~RCC_APB1ENR_USART2EN;

    /* 4. 重置句柄 */
    huart->state = HAL_STATE_RESET;
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
    usart_reg_t *usart;
    uint16_t i;
    uint32_t tickstart;

    if (huart == NULL || data == NULL || size == 0) {
        return HAL_ERROR;
    }

    if (huart->state != HAL_STATE_READY && huart->state != HAL_STATE_BUSY) {
        return HAL_BUSY;
    }

    if (huart->instance != UART_INSTANCE_2) {
        return HAL_ERROR;
    }

    usart = USART2;

    /* 设置忙状态 */
    huart->state = HAL_STATE_BUSY;
    huart->tx_buffer = (uint8_t *)data;
    huart->tx_size = size;
    huart->tx_count = 0;

    tickstart = 0;  /* 简化处理,使用轮询计数 */

    /* 逐字节发送 */
    for (i = 0; i < size; i++) {
        /* 等待发送数据寄存器空 (TXE=1) */
        tickstart = 0;
        while (!(usart->SR & USART_SR_TXE)) {
            if (++tickstart > timeout * 1000) {
                huart->error_code |= UART_ERROR_TIMEOUT;
                huart->state = HAL_STATE_ERROR;
                return HAL_TIMEOUT;
            }
        }

        /* 写入数据寄存器 */
        usart->DR = data[i];
        huart->tx_count++;
    }

    /* 等待发送完成 (TC=1) */
    tickstart = 0;
    while (!(usart->SR & USART_SR_TC)) {
        if (++tickstart > timeout * 1000) {
            huart->error_code |= UART_ERROR_TIMEOUT;
            huart->state = HAL_STATE_ERROR;
            return HAL_TIMEOUT;
        }
    }

    /* 清除发送完成标志 */
    usart->SR &= ~USART_SR_TC;

    huart->state = HAL_STATE_READY;

    return HAL_OK;
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
    usart_reg_t *usart;
    uint16_t i;
    uint32_t tickstart;
    uint32_t sr;

    if (huart == NULL || data == NULL || size == 0) {
        return HAL_ERROR;
    }

    if (huart->state != HAL_STATE_READY && huart->state != HAL_STATE_BUSY) {
        return HAL_BUSY;
    }

    if (huart->instance != UART_INSTANCE_2) {
        return HAL_ERROR;
    }

    usart = USART2;

    /* 设置忙状态 */
    huart->state = HAL_STATE_BUSY;
    huart->rx_buffer = data;
    huart->rx_size = size;
    huart->rx_count = 0;

    /* 逐字节接收 */
    for (i = 0; i < size; i++) {
        tickstart = 0;

        /* 等待接收数据寄存器非空 (RXNE=1) */
        while (!(usart->SR & USART_SR_RXNE)) {
            if (++tickstart > timeout * 1000) {
                huart->error_code |= UART_ERROR_TIMEOUT;
                huart->state = HAL_STATE_ERROR;
                return HAL_TIMEOUT;
            }
        }

        /* 读取状态寄存器和数据寄存器 */
        sr = usart->SR;
        data[i] = (uint8_t)(usart->DR & 0xFF);
        huart->rx_count++;

        /* 检查错误标志 */
        if (sr & USART_SR_PE) {
            huart->error_code |= UART_ERROR_PE;
        }
        if (sr & USART_SR_FE) {
            huart->error_code |= UART_ERROR_FE;
        }
        if (sr & USART_SR_NE) {
            huart->error_code |= UART_ERROR_NE;
        }
        if (sr & USART_SR_ORE) {
            huart->error_code |= UART_ERROR_ORE;
        }
    }

    /* 如果有错误,返回错误状态 */
    if (huart->error_code != UART_ERROR_NONE) {
        huart->state = HAL_STATE_ERROR;
        return HAL_ERROR;
    }

    huart->state = HAL_STATE_READY;

    return HAL_OK;
}

/**
 * @brief 动态修改波特率
 * @param huart UART句柄指针
 * @param baudrate 新波特率
 * @return HAL状态
 */
hal_status_t uart_set_baudrate(uart_handle_t *huart, uint32_t baudrate)
{
    usart_reg_t *usart;
    uint32_t cr1_val;
    hal_status_t status;

    if (huart == NULL) {
        return HAL_ERROR;
    }

    if (huart->instance != UART_INSTANCE_2) {
        return HAL_ERROR;
    }

    usart = USART2;

    /* 保存当前CR1值 */
    cr1_val = usart->CR1;

    /* 临时禁用USART以修改波特率 */
    usart->CR1 = cr1_val & ~USART_CR1_UE;

    /* 设置新波特率 */
    status = uart_set_brr(usart, baudrate);

    if (status == HAL_OK) {
        /* 更新配置 */
        huart->config.baudrate = baudrate;
    }

    /* 恢复USART使能 */
    usart->CR1 = cr1_val;

    return status;
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
    if (huart == NULL) {
        return false;
    }

    return (huart->state == HAL_STATE_BUSY);
}

/**
 * @brief 清除错误标志
 * @param huart UART句柄指针
 */
void uart_clear_error(uart_handle_t *huart)
{
    usart_reg_t *usart;
    uint32_t sr;

    if (huart == NULL) {
        return;
    }

    if (huart->instance != UART_INSTANCE_2) {
        return;
    }

    usart = USART2;

    /* 读取SR然后读取DR来清除错误标志 */
    sr = usart->SR;
    (void)usart->DR;

    /* 清除句柄中的错误码 */
    huart->error_code = UART_ERROR_NONE;

    /* 如果之前是错误状态,恢复为就绪 */
    if (huart->state == HAL_STATE_ERROR) {
        huart->state = HAL_STATE_READY;
    }

    UNUSED(sr);
}

/**
 * @brief 获取错误码
 * @param huart UART句柄指针
 * @return 错误码
 */
uint32_t uart_get_error(const uart_handle_t *huart)
{
    if (huart == NULL) {
        return UART_ERROR_NONE;
    }

    return huart->error_code;
}
