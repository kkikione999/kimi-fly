/**
 * @file i2c.c
 * @brief STM32 I2C HAL实现
 *
 * @note 本文件为Ralph-loop v2.0 HAL层实现
 *       支持I2C1与气压计(LPS22HBTR)和磁力计(QMC5883P)通信
 *
 * @hardware
 *   - I2C1: PB6 (SCL), PB7 (SDA)
 *   - 时钟源: APB1 (42MHz)
 *   - GPIO配置: 开漏输出 + 内部上拉
 */

#include "i2c.h"
#include <string.h>

/* STM32F4xx 标准库头文件 (条件编译) */
#ifdef USE_HAL_DRIVER
#include "stm32f4xx_hal.h"
#else
/* 寄存器定义 (用于裸机开发) */
#define I2C1_BASE           0x40005400U
#define I2C2_BASE           0x40005800U
#define I2C3_BASE           0x40005C00U

#define I2C1                ((I2C_TypeDef *)I2C1_BASE)
#define I2C2                ((I2C_TypeDef *)I2C2_BASE)
#define I2C3                ((I2C_TypeDef *)I2C3_BASE)

#define RCC_BASE            0x40023800U
#define RCC_APB1ENR         (*(volatile uint32_t *)(RCC_BASE + 0x40U))
#define RCC_APB1ENR_I2C1EN  (1U << 21)

#define GPIOB_BASE          0x40020400U
#define GPIOB_MODER         (*(volatile uint32_t *)(GPIOB_BASE + 0x00U))
#define GPIOB_OTYPER        (*(volatile uint32_t *)(GPIOB_BASE + 0x04U))
#define GPIOB_OSPEEDR       (*(volatile uint32_t *)(GPIOB_BASE + 0x08U))
#define GPIOB_PUPDR         (*(volatile uint32_t *)(GPIOB_BASE + 0x0CU))
#define GPIOB_AFRL          (*(volatile uint32_t *)(GPIOB_BASE + 0x20U))

#define RCC_AHB1ENR         (*(volatile uint32_t *)(RCC_BASE + 0x30U))
#define RCC_AHB1ENR_GPIOBEN (1U << 1)

/* I2C寄存器结构 */
typedef struct {
    volatile uint32_t CR1;      /* 控制寄存器1 */
    volatile uint32_t CR2;      /* 控制寄存器2 */
    volatile uint32_t OAR1;     /* 自身地址寄存器1 */
    volatile uint32_t OAR2;     /* 自身地址寄存器2 */
    volatile uint32_t DR;       /* 数据寄存器 */
    volatile uint32_t SR1;      /* 状态寄存器1 */
    volatile uint32_t SR2;      /* 状态寄存器2 */
    volatile uint32_t CCR;      /* 时钟控制寄存器 */
    volatile uint32_t TRISE;    /* 上升时间寄存器 */
    volatile uint32_t FLTR;     /* 滤波寄存器 */
} I2C_TypeDef;

/* I2C_CR1位定义 */
#define I2C_CR1_PE          (1U << 0)
#define I2C_CR1_SMBUS       (1U << 1)
#define I2C_CR1_SMBTYPE     (1U << 3)
#define I2C_CR1_ENARP       (1U << 4)
#define I2C_CR1_ENPEC       (1U << 5)
#define I2C_CR1_ENGC        (1U << 6)
#define I2C_CR1_NOSTRETCH   (1U << 7)
#define I2C_CR1_START       (1U << 8)
#define I2C_CR1_STOP        (1U << 9)
#define I2C_CR1_ACK         (1U << 10)
#define I2C_CR1_POS         (1U << 11)
#define I2C_CR1_PEC         (1U << 12)
#define I2C_CR1_ALERT       (1U << 13)
#define I2C_CR1_SWRST       (1U << 15)

/* I2C_CR2位定义 */
#define I2C_CR2_FREQ_Pos    0U
#define I2C_CR2_FREQ_Msk    (0x3FU << I2C_CR2_FREQ_Pos)
#define I2C_CR2_ITERREN     (1U << 8)
#define I2C_CR2_ITEVTEN     (1U << 9)
#define I2C_CR2_ITBUFEN     (1U << 10)
#define I2C_CR2_DMAEN       (1U << 11)
#define I2C_CR2_LAST        (1U << 12)

/* I2C_SR1位定义 */
#define I2C_SR1_SB          (1U << 0)
#define I2C_SR1_ADDR        (1U << 1)
#define I2C_SR1_BTF         (1U << 2)
#define I2C_SR1_ADD10       (1U << 3)
#define I2C_SR1_STOPF       (1U << 4)
#define I2C_SR1_RXNE        (1U << 6)
#define I2C_SR1_TXE         (1U << 7)
#define I2C_SR1_BERR        (1U << 8)
#define I2C_SR1_ARLO        (1U << 9)
#define I2C_SR1_AF          (1U << 10)
#define I2C_SR1_OVR         (1U << 11)
#define I2C_SR1_PECERR      (1U << 12)
#define I2C_SR1_TIMEOUT     (1U << 14)
#define I2C_SR1_SMBALERT    (1U << 15)

/* I2C_SR2位定义 */
#define I2C_SR2_MSL         (1U << 0)
#define I2C_SR2_BUSY        (1U << 1)
#define I2C_SR2_TRA         (1U << 2)
#define I2C_SR2_GENCALL     (1U << 4)
#define I2C_SR2_SMBDEFAULT  (1U << 5)
#define I2C_SR2_SMBHOST     (1U << 6)
#define I2C_SR2_DUALF       (1U << 7)
#define I2C_SR2_PEC_Pos     8U
#define I2C_SR2_PEC_Msk     (0xFFU << I2C_SR2_PEC_Pos)

/* I2C_CCR位定义 */
#define I2C_CCR_CCR_Pos     0U
#define I2C_CCR_CCR_Msk     (0xFFFU << I2C_CCR_CCR_Pos)
#define I2C_CCR_DUTY        (1U << 14)
#define I2C_CCR_FS          (1U << 15)

/* I2C_TRISE位定义 */
#define I2C_TRISE_TRISE_Pos 0U
#define I2C_TRISE_TRISE_Msk (0x3FU << I2C_TRISE_TRISE_Pos)

/* I2C_OAR1位定义 */
#define I2C_OAR1_ADD0       (1U << 0)
#define I2C_OAR1_ADD7_1_Pos 1U
#define I2C_OAR1_ADD7_1_Msk (0x7FU << I2C_OAR1_ADD7_1_Pos)
#define I2C_OAR1_ADD9_8_Pos 8U
#define I2C_OAR1_ADD9_8_Msk (0x3U << I2C_OAR1_ADD9_8_Pos)
#define I2C_OAR1_ADDMODE    (1U << 15)

#endif /* USE_HAL_DRIVER */

/* ============================================================================
 * 私有宏定义
 * ============================================================================ */

#define I2C_MAX_RETRIES     3U          /* 最大重试次数 */
#define I2C_FLAG_TIMEOUT    100U        /* 标志位等待超时 (ms) */
#define I2C_MIN_ADDRESS     0x08U       /* 最小有效7位地址 */
#define I2C_MAX_ADDRESS     0x77U       /* 最大有效7位地址 */

/* APB1时钟频率 (42MHz) */
#define I2C_APB1_CLK_FREQ   42000000U

/* ============================================================================
 * 私有函数声明
 * ============================================================================ */

static hal_status_t i2c_wait_flag(i2c_handle_t *hi2c, uint32_t flag,
                                   uint32_t timeout);
static hal_status_t i2c_wait_busy(i2c_handle_t *hi2c, uint32_t timeout);
static hal_status_t i2c_generate_start(i2c_handle_t *hi2c);
static hal_status_t i2c_generate_stop(i2c_handle_t *hi2c);
static hal_status_t i2c_send_address(i2c_handle_t *hi2c, uint16_t addr,
                                      uint8_t direction);
static void i2c_clear_error_flags(I2C_TypeDef *i2c);
static hal_status_t i2c_check_error(i2c_handle_t *hi2c);
static void i2c_gpio_init(void);
static void i2c_gpio_deinit(void);
static void i2c_clk_enable(void);
static void i2c_clk_disable(void);

/* ============================================================================
 * API 实现
 * ============================================================================ */

/**
 * @brief 初始化I2C外设
 */
hal_status_t i2c_init(i2c_handle_t *hi2c, const i2c_config_t *config)
{
    I2C_TypeDef *i2c;
    uint32_t ccr_value;
    uint32_t trise_value;

    if (hi2c == NULL || config == NULL) {
        return HAL_ERROR;
    }

    /* 保存配置 */
    hi2c->config = *config;
    hi2c->error_code = I2C_ERROR_NONE;

    /* 目前只支持I2C1 */
    hi2c->instance = I2C1;
    i2c = (I2C_TypeDef *)hi2c->instance;

    /* 使能时钟 */
    i2c_clk_enable();

    /* 禁用I2C */
    i2c->CR1 &= ~I2C_CR1_PE;

    /* 配置GPIO */
    i2c_gpio_init();

    /* 配置CR2: APB1时钟频率 (MHz) */
    i2c->CR2 = (I2C_APB1_CLK_FREQ / 1000000U) & I2C_CR2_FREQ_Msk;

    /* 配置CCR和TRISE */
    if (config->clock_speed <= I2C_SPEED_STANDARD) {
        /* 标准模式 (100kHz) */
        ccr_value = I2C_APB1_CLK_FREQ / (config->clock_speed * 2U);
        if (ccr_value < 4U) {
            ccr_value = 4U;
        }
        trise_value = (I2C_APB1_CLK_FREQ / 1000000U) + 1U;
    } else {
        /* 快速模式 (400kHz) */
        ccr_value = I2C_APB1_CLK_FREQ / (config->clock_speed * 3U);
        ccr_value |= I2C_CCR_FS;  /* 设置快速模式位 */
        if (ccr_value < 1U) {
            ccr_value = 1U;
        }
        trise_value = (I2C_APB1_CLK_FREQ / 1000000U / 300U) + 1U;
    }

    i2c->CCR = ccr_value & I2C_CCR_CCR_Msk;
    i2c->TRISE = trise_value & I2C_TRISE_TRISE_Msk;

    /* 配置自身地址 (主模式不使用) */
    if (config->own_address != 0U) {
        i2c->OAR1 = ((config->own_address << I2C_OAR1_ADD7_1_Pos) & I2C_OAR1_ADD7_1_Msk);
        if (config->addr_mode == I2C_ADDR_MODE_10BIT) {
            i2c->OAR1 |= I2C_OAR1_ADDMODE;
        }
    }

    /* 使能I2C */
    i2c->CR1 |= I2C_CR1_PE;

    /* 清除错误标志 */
    i2c_clear_error_flags(i2c);

    return HAL_OK;
}

/**
 * @brief 反初始化I2C外设
 */
hal_status_t i2c_deinit(i2c_handle_t *hi2c)
{
    I2C_TypeDef *i2c;

    if (hi2c == NULL || hi2c->instance == NULL) {
        return HAL_ERROR;
    }

    i2c = (I2C_TypeDef *)hi2c->instance;

    /* 禁用I2C */
    i2c->CR1 &= ~I2C_CR1_PE;

    /* 反初始化GPIO */
    i2c_gpio_deinit();

    /* 禁用时钟 */
    i2c_clk_disable();

    /* 清除句柄 */
    hi2c->instance = NULL;
    hi2c->error_code = I2C_ERROR_NONE;

    return HAL_OK;
}

/**
 * @brief I2C主模式发送数据
 */
hal_status_t i2c_master_transmit(i2c_handle_t *hi2c, uint16_t dev_addr,
                                  const uint8_t *data, uint16_t size,
                                  uint32_t timeout)
{
    I2C_TypeDef *i2c;
    hal_status_t status;
    uint16_t i;

    if (hi2c == NULL || hi2c->instance == NULL || data == NULL || size == 0U) {
        return HAL_ERROR;
    }

    i2c = (I2C_TypeDef *)hi2c->instance;
    hi2c->error_code = I2C_ERROR_NONE;

    /* 等待总线空闲 */
    status = i2c_wait_busy(hi2c, timeout);
    if (status != HAL_OK) {
        return status;
    }

    /* 生成START条件 */
    status = i2c_generate_start(hi2c);
    if (status != HAL_OK) {
        return status;
    }

    /* 等待SB标志 */
    status = i2c_wait_flag(hi2c, I2C_SR1_SB, timeout);
    if (status != HAL_OK) {
        i2c_generate_stop(hi2c);
        return status;
    }

    /* 发送设备地址 (写) */
    status = i2c_send_address(hi2c, dev_addr, 0U);
    if (status != HAL_OK) {
        i2c_generate_stop(hi2c);
        return status;
    }

    /* 发送数据 */
    for (i = 0U; i < size; i++) {
        /* 等待TXE标志 */
        status = i2c_wait_flag(hi2c, I2C_SR1_TXE, timeout);
        if (status != HAL_OK) {
            i2c_generate_stop(hi2c);
            return status;
        }

        /* 写入数据 */
        i2c->DR = data[i];
    }

    /* 等待BTF标志 */
    status = i2c_wait_flag(hi2c, I2C_SR1_BTF, timeout);
    if (status != HAL_OK) {
        i2c_generate_stop(hi2c);
        return status;
    }

    /* 生成STOP条件 */
    status = i2c_generate_stop(hi2c);

    return status;
}

/**
 * @brief I2C主模式接收数据
 */
hal_status_t i2c_master_receive(i2c_handle_t *hi2c, uint16_t dev_addr,
                                 uint8_t *data, uint16_t size,
                                 uint32_t timeout)
{
    I2C_TypeDef *i2c;
    hal_status_t status;
    uint16_t i;

    if (hi2c == NULL || hi2c->instance == NULL || data == NULL || size == 0U) {
        return HAL_ERROR;
    }

    i2c = (I2C_TypeDef *)hi2c->instance;
    hi2c->error_code = I2C_ERROR_NONE;

    /* 等待总线空闲 */
    status = i2c_wait_busy(hi2c, timeout);
    if (status != HAL_OK) {
        return status;
    }

    if (size == 1U) {
        /* 单字节接收 */
        /* 禁用ACK */
        i2c->CR1 &= ~I2C_CR1_ACK;

        /* 生成START条件 */
        status = i2c_generate_start(hi2c);
        if (status != HAL_OK) {
            return status;
        }

        /* 等待SB标志 */
        status = i2c_wait_flag(hi2c, I2C_SR1_SB, timeout);
        if (status != HAL_OK) {
            i2c_generate_stop(hi2c);
            return status;
        }

        /* 发送设备地址 (读) */
        status = i2c_send_address(hi2c, dev_addr, 1U);
        if (status != HAL_OK) {
            i2c_generate_stop(hi2c);
            return status;
        }

        /* 生成STOP条件 (在ADDR清零前) */
        i2c->CR1 |= I2C_CR1_STOP;

        /* 等待RXNE */
        status = i2c_wait_flag(hi2c, I2C_SR1_RXNE, timeout);
        if (status != HAL_OK) {
            return status;
        }

        /* 读取数据 */
        data[0] = (uint8_t)i2c->DR;

        /* 重新使能ACK */
        i2c->CR1 |= I2C_CR1_ACK;
    } else if (size == 2U) {
        /* 双字节接收 */
        /* 使能ACK和POS */
        i2c->CR1 |= I2C_CR1_ACK;
        i2c->CR1 |= I2C_CR1_POS;

        /* 生成START条件 */
        status = i2c_generate_start(hi2c);
        if (status != HAL_OK) {
            return status;
        }

        /* 等待SB标志 */
        status = i2c_wait_flag(hi2c, I2C_SR1_SB, timeout);
        if (status != HAL_OK) {
            i2c_generate_stop(hi2c);
            return status;
        }

        /* 发送设备地址 (读) */
        status = i2c_send_address(hi2c, dev_addr, 1U);
        if (status != HAL_OK) {
            i2c_generate_stop(hi2c);
            return status;
        }

        /* 清除ADDR, 禁用ACK */
        (void)i2c->SR1;
        (void)i2c->SR2;
        i2c->CR1 &= ~I2C_CR1_ACK;

        /* 等待BTF */
        status = i2c_wait_flag(hi2c, I2C_SR1_BTF, timeout);
        if (status != HAL_OK) {
            i2c_generate_stop(hi2c);
            return status;
        }

        /* 生成STOP */
        i2c->CR1 |= I2C_CR1_STOP;

        /* 读取数据 */
        data[0] = (uint8_t)i2c->DR;
        data[1] = (uint8_t)i2c->DR;

        /* 清除POS */
        i2c->CR1 &= ~I2C_CR1_POS;

        /* 重新使能ACK */
        i2c->CR1 |= I2C_CR1_ACK;
    } else {
        /* 多字节接收 (size > 2) */
        /* 使能ACK */
        i2c->CR1 |= I2C_CR1_ACK;

        /* 生成START条件 */
        status = i2c_generate_start(hi2c);
        if (status != HAL_OK) {
            return status;
        }

        /* 等待SB标志 */
        status = i2c_wait_flag(hi2c, I2C_SR1_SB, timeout);
        if (status != HAL_OK) {
            i2c_generate_stop(hi2c);
            return status;
        }

        /* 发送设备地址 (读) */
        status = i2c_send_address(hi2c, dev_addr, 1U);
        if (status != HAL_OK) {
            i2c_generate_stop(hi2c);
            return status;
        }

        /* 接收前N-3个字节 */
        for (i = 0U; i < (size - 3U); i++) {
            /* 等待RXNE */
            status = i2c_wait_flag(hi2c, I2C_SR1_RXNE, timeout);
            if (status != HAL_OK) {
                i2c_generate_stop(hi2c);
                return status;
            }

            /* 读取数据 */
            data[i] = (uint8_t)i2c->DR;
        }

        /* 等待BTF (倒数第3字节已接收) */
        status = i2c_wait_flag(hi2c, I2C_SR1_BTF, timeout);
        if (status != HAL_OK) {
            i2c_generate_stop(hi2c);
            return status;
        }

        /* 禁用ACK */
        i2c->CR1 &= ~I2C_CR1_ACK;

        /* 读取倒数第3字节 */
        data[size - 3U] = (uint8_t)i2c->DR;

        /* 等待BTF */
        status = i2c_wait_flag(hi2c, I2C_SR1_BTF, timeout);
        if (status != HAL_OK) {
            i2c_generate_stop(hi2c);
            return status;
        }

        /* 生成STOP */
        i2c->CR1 |= I2C_CR1_STOP;

        /* 读取最后2字节 */
        data[size - 2U] = (uint8_t)i2c->DR;
        data[size - 1U] = (uint8_t)i2c->DR;

        /* 重新使能ACK */
        i2c->CR1 |= I2C_CR1_ACK;
    }

    return HAL_OK;
}

/**
 * @brief 向I2C设备寄存器写入数据
 */
hal_status_t i2c_mem_write(i2c_handle_t *hi2c, uint16_t dev_addr,
                            uint16_t mem_addr, uint8_t mem_addr_size,
                            const uint8_t *data, uint16_t size,
                            uint32_t timeout)
{
    uint8_t mem_addr_buf[2];
    hal_status_t status;
    I2C_TypeDef *i2c;
    uint16_t i;

    if (hi2c == NULL || hi2c->instance == NULL || data == NULL || size == 0U ||
        (mem_addr_size != 1U && mem_addr_size != 2U)) {
        return HAL_ERROR;
    }

    i2c = (I2C_TypeDef *)hi2c->instance;
    hi2c->error_code = I2C_ERROR_NONE;

    /* 等待总线空闲 */
    status = i2c_wait_busy(hi2c, timeout);
    if (status != HAL_OK) {
        return status;
    }

    /* 生成START条件 */
    status = i2c_generate_start(hi2c);
    if (status != HAL_OK) {
        return status;
    }

    /* 等待SB标志 */
    status = i2c_wait_flag(hi2c, I2C_SR1_SB, timeout);
    if (status != HAL_OK) {
        i2c_generate_stop(hi2c);
        return status;
    }

    /* 发送设备地址 (写) */
    status = i2c_send_address(hi2c, dev_addr, 0U);
    if (status != HAL_OK) {
        i2c_generate_stop(hi2c);
        return status;
    }

    /* 发送内存地址 */
    if (mem_addr_size == 2U) {
        /* 16位地址: 高字节在前 */
        mem_addr_buf[0] = (uint8_t)((mem_addr >> 8) & 0xFFU);
        mem_addr_buf[1] = (uint8_t)(mem_addr & 0xFFU);
    } else {
        mem_addr_buf[0] = (uint8_t)(mem_addr & 0xFFU);
    }

    for (i = 0U; i < mem_addr_size; i++) {
        /* 等待TXE */
        status = i2c_wait_flag(hi2c, I2C_SR1_TXE, timeout);
        if (status != HAL_OK) {
            i2c_generate_stop(hi2c);
            return status;
        }

        i2c->DR = mem_addr_buf[i];
    }

    /* 发送数据 */
    for (i = 0U; i < size; i++) {
        /* 等待TXE */
        status = i2c_wait_flag(hi2c, I2C_SR1_TXE, timeout);
        if (status != HAL_OK) {
            i2c_generate_stop(hi2c);
            return status;
        }

        i2c->DR = data[i];
    }

    /* 等待BTF */
    status = i2c_wait_flag(hi2c, I2C_SR1_BTF, timeout);
    if (status != HAL_OK) {
        i2c_generate_stop(hi2c);
        return status;
    }

    /* 生成STOP */
    status = i2c_generate_stop(hi2c);

    return status;
}

/**
 * @brief 从I2C设备寄存器读取数据
 */
hal_status_t i2c_mem_read(i2c_handle_t *hi2c, uint16_t dev_addr,
                           uint16_t mem_addr, uint8_t mem_addr_size,
                           uint8_t *data, uint16_t size,
                           uint32_t timeout)
{
    uint8_t mem_addr_buf[2];
    hal_status_t status;
    I2C_TypeDef *i2c;
    uint16_t i;

    if (hi2c == NULL || hi2c->instance == NULL || data == NULL || size == 0U ||
        (mem_addr_size != 1U && mem_addr_size != 2U)) {
        return HAL_ERROR;
    }

    i2c = (I2C_TypeDef *)hi2c->instance;
    hi2c->error_code = I2C_ERROR_NONE;

    /* 等待总线空闲 */
    status = i2c_wait_busy(hi2c, timeout);
    if (status != HAL_OK) {
        return status;
    }

    /* 生成START条件 */
    status = i2c_generate_start(hi2c);
    if (status != HAL_OK) {
        return status;
    }

    /* 等待SB标志 */
    status = i2c_wait_flag(hi2c, I2C_SR1_SB, timeout);
    if (status != HAL_OK) {
        i2c_generate_stop(hi2c);
        return status;
    }

    /* 发送设备地址 (写) */
    status = i2c_send_address(hi2c, dev_addr, 0U);
    if (status != HAL_OK) {
        i2c_generate_stop(hi2c);
        return status;
    }

    /* 发送内存地址 */
    if (mem_addr_size == 2U) {
        mem_addr_buf[0] = (uint8_t)((mem_addr >> 8) & 0xFFU);
        mem_addr_buf[1] = (uint8_t)(mem_addr & 0xFFU);
    } else {
        mem_addr_buf[0] = (uint8_t)(mem_addr & 0xFFU);
    }

    for (i = 0U; i < mem_addr_size; i++) {
        /* 等待TXE */
        status = i2c_wait_flag(hi2c, I2C_SR1_TXE, timeout);
        if (status != HAL_OK) {
            i2c_generate_stop(hi2c);
            return status;
        }

        i2c->DR = mem_addr_buf[i];
    }

    /* 等待TXE */
    status = i2c_wait_flag(hi2c, I2C_SR1_TXE, timeout);
    if (status != HAL_OK) {
        i2c_generate_stop(hi2c);
        return status;
    }

    /* 重新生成START (重复起始条件) */
    status = i2c_generate_start(hi2c);
    if (status != HAL_OK) {
        return status;
    }

    /* 等待SB标志 */
    status = i2c_wait_flag(hi2c, I2C_SR1_SB, timeout);
    if (status != HAL_OK) {
        i2c_generate_stop(hi2c);
        return status;
    }

    /* 发送设备地址 (读) */
    status = i2c_send_address(hi2c, dev_addr, 1U);
    if (status != HAL_OK) {
        i2c_generate_stop(hi2c);
        return status;
    }

    /* 接收数据 */
    if (size == 1U) {
        /* 单字节 */
        i2c->CR1 &= ~I2C_CR1_ACK;
        i2c->CR1 |= I2C_CR1_STOP;

        status = i2c_wait_flag(hi2c, I2C_SR1_RXNE, timeout);
        if (status != HAL_OK) {
            return status;
        }

        data[0] = (uint8_t)i2c->DR;
        i2c->CR1 |= I2C_CR1_ACK;
    } else if (size == 2U) {
        /* 双字节 */
        i2c->CR1 |= I2C_CR1_ACK;
        i2c->CR1 |= I2C_CR1_POS;

        (void)i2c->SR1;
        (void)i2c->SR2;
        i2c->CR1 &= ~I2C_CR1_ACK;

        status = i2c_wait_flag(hi2c, I2C_SR1_BTF, timeout);
        if (status != HAL_OK) {
            i2c_generate_stop(hi2c);
            return status;
        }

        i2c->CR1 |= I2C_CR1_STOP;
        data[0] = (uint8_t)i2c->DR;
        data[1] = (uint8_t)i2c->DR;

        i2c->CR1 &= ~I2C_CR1_POS;
        i2c->CR1 |= I2C_CR1_ACK;
    } else {
        /* 多字节 */
        i2c->CR1 |= I2C_CR1_ACK;

        for (i = 0U; i < (size - 3U); i++) {
            status = i2c_wait_flag(hi2c, I2C_SR1_RXNE, timeout);
            if (status != HAL_OK) {
                i2c_generate_stop(hi2c);
                return status;
            }
            data[i] = (uint8_t)i2c->DR;
        }

        status = i2c_wait_flag(hi2c, I2C_SR1_BTF, timeout);
        if (status != HAL_OK) {
            i2c_generate_stop(hi2c);
            return status;
        }

        i2c->CR1 &= ~I2C_CR1_ACK;
        data[size - 3U] = (uint8_t)i2c->DR;

        status = i2c_wait_flag(hi2c, I2C_SR1_BTF, timeout);
        if (status != HAL_OK) {
            i2c_generate_stop(hi2c);
            return status;
        }

        i2c->CR1 |= I2C_CR1_STOP;
        data[size - 2U] = (uint8_t)i2c->DR;
        data[size - 1U] = (uint8_t)i2c->DR;

        i2c->CR1 |= I2C_CR1_ACK;
    }

    return HAL_OK;
}

/**
 * @brief 扫描I2C总线上的设备
 */
hal_status_t i2c_scan(i2c_handle_t *hi2c, uint8_t *found_addr,
                       uint8_t max_count, uint8_t *found_count)
{
    uint8_t addr;
    uint8_t count = 0U;
    hal_status_t status;

    if (hi2c == NULL || found_addr == NULL || found_count == NULL || max_count == 0U) {
        return HAL_ERROR;
    }

    *found_count = 0U;

    for (addr = I2C_MIN_ADDRESS; addr <= I2C_MAX_ADDRESS; addr++) {
        status = i2c_is_device_ready(hi2c, addr);
        if (status == HAL_OK) {
            found_addr[count++] = addr;
            if (count >= max_count) {
                break;
            }
        }
    }

    *found_count = count;
    return HAL_OK;
}

/**
 * @brief 检查设备是否响应
 */
hal_status_t i2c_is_device_ready(i2c_handle_t *hi2c, uint16_t dev_addr)
{
    I2C_TypeDef *i2c;
    hal_status_t status;
    uint32_t retry;

    if (hi2c == NULL || hi2c->instance == NULL) {
        return HAL_ERROR;
    }

    i2c = (I2C_TypeDef *)hi2c->instance;

    for (retry = 0U; retry < I2C_MAX_RETRIES; retry++) {
        /* 等待总线空闲 */
        status = i2c_wait_busy(hi2c, I2C_FLAG_TIMEOUT);
        if (status != HAL_OK) {
            continue;
        }

        /* 生成START */
        status = i2c_generate_start(hi2c);
        if (status != HAL_OK) {
            continue;
        }

        /* 等待SB */
        status = i2c_wait_flag(hi2c, I2C_SR1_SB, I2C_FLAG_TIMEOUT);
        if (status != HAL_OK) {
            i2c_generate_stop(hi2c);
            continue;
        }

        /* 发送地址 */
        status = i2c_send_address(hi2c, dev_addr, 0U);
        if (status == HAL_OK) {
            /* 设备响应 */
            i2c_generate_stop(hi2c);
            return HAL_OK;
        }

        /* 设备无响应, 停止并重试 */
        i2c_generate_stop(hi2c);
    }

    return HAL_ERROR;
}

/**
 * @brief 获取I2C错误码
 */
uint32_t i2c_get_error(i2c_handle_t *hi2c)
{
    if (hi2c == NULL) {
        return I2C_ERROR_NONE;
    }
    return hi2c->error_code;
}

/**
 * @brief 清除I2C错误码
 */
void i2c_clear_error(i2c_handle_t *hi2c)
{
    if (hi2c != NULL) {
        hi2c->error_code = I2C_ERROR_NONE;
    }
}

/* ============================================================================
 * 私有函数实现
 * ============================================================================ */

/**
 * @brief 等待I2C标志位
 */
static hal_status_t i2c_wait_flag(i2c_handle_t *hi2c, uint32_t flag,
                                   uint32_t timeout)
{
    I2C_TypeDef *i2c = (I2C_TypeDef *)hi2c->instance;
    uint32_t tick_start = 0U;  /* TODO: 使用系统滴答定时器 */
    (void)tick_start;

    while ((i2c->SR1 & flag) == 0U) {
        /* 检查错误 */
        if (i2c_check_error(hi2c) != HAL_OK) {
            return HAL_ERROR;
        }

        /* TODO: 检查超时 */
        if (timeout != HAL_MAX_DELAY) {
            /* 简化处理: 假设不会超时 */
        }
    }

    return HAL_OK;
}

/**
 * @brief 等待总线空闲
 */
static hal_status_t i2c_wait_busy(i2c_handle_t *hi2c, uint32_t timeout)
{
    I2C_TypeDef *i2c = (I2C_TypeDef *)hi2c->instance;
    uint32_t tick_start = 0U;
    (void)tick_start;

    while ((i2c->SR2 & I2C_SR2_BUSY) != 0U) {
        if (timeout != HAL_MAX_DELAY) {
            /* TODO: 检查超时 */
        }
    }

    return HAL_OK;
}

/**
 * @brief 生成START条件
 */
static hal_status_t i2c_generate_start(i2c_handle_t *hi2c)
{
    I2C_TypeDef *i2c = (I2C_TypeDef *)hi2c->instance;

    /* 使能ACK */
    i2c->CR1 |= I2C_CR1_ACK;

    /* 生成START */
    i2c->CR1 |= I2C_CR1_START;

    return HAL_OK;
}

/**
 * @brief 生成STOP条件
 */
static hal_status_t i2c_generate_stop(i2c_handle_t *hi2c)
{
    I2C_TypeDef *i2c = (I2C_TypeDef *)hi2c->instance;

    /* 生成STOP */
    i2c->CR1 |= I2C_CR1_STOP;

    return HAL_OK;
}

/**
 * @brief 发送设备地址
 */
static hal_status_t i2c_send_address(i2c_handle_t *hi2c, uint16_t addr,
                                      uint8_t direction)
{
    I2C_TypeDef *i2c = (I2C_TypeDef *)hi2c->instance;
    hal_status_t status;

    /* 发送地址 (7位地址左移1位, 最低位为方向: 0=写, 1=读) */
    i2c->DR = (uint8_t)((addr << 1U) | (direction & 0x01U));

    /* 等待ADDR标志 */
    status = i2c_wait_flag(hi2c, I2C_SR1_ADDR, I2C_FLAG_TIMEOUT);
    if (status != HAL_OK) {
        return status;
    }

    /* 清除ADDR标志 (读SR1然后读SR2) */
    (void)i2c->SR1;
    (void)i2c->SR2;

    return HAL_OK;
}

/**
 * @brief 清除错误标志
 */
static void i2c_clear_error_flags(I2C_TypeDef *i2c)
{
    /* 读SR1和SR2清除错误标志 */
    (void)i2c->SR1;
    (void)i2c->SR2;
}

/**
 * @brief 检查错误
 */
static hal_status_t i2c_check_error(i2c_handle_t *hi2c)
{
    I2C_TypeDef *i2c = (I2C_TypeDef *)hi2c->instance;
    uint32_t sr1 = i2c->SR1;

    if (sr1 & I2C_SR1_BERR) {
        hi2c->error_code |= I2C_ERROR_BERR;
        i2c->SR1 &= ~I2C_SR1_BERR;
        return HAL_ERROR;
    }

    if (sr1 & I2C_SR1_ARLO) {
        hi2c->error_code |= I2C_ERROR_ARLO;
        i2c->SR1 &= ~I2C_SR1_ARLO;
        return HAL_ERROR;
    }

    if (sr1 & I2C_SR1_AF) {
        hi2c->error_code |= I2C_ERROR_AF;
        i2c->SR1 &= ~I2C_SR1_AF;
        return HAL_ERROR;
    }

    if (sr1 & I2C_SR1_OVR) {
        hi2c->error_code |= I2C_ERROR_OVR;
        i2c->SR1 &= ~I2C_SR1_OVR;
        return HAL_ERROR;
    }

    if (sr1 & I2C_SR1_TIMEOUT) {
        hi2c->error_code |= I2C_ERROR_TIMEOUT;
        i2c->SR1 &= ~I2C_SR1_TIMEOUT;
        return HAL_ERROR;
    }

    return HAL_OK;
}

/**
 * @brief 初始化I2C GPIO (PB6=SCL, PB7=SDA)
 */
static void i2c_gpio_init(void)
{
    /* 使能GPIOB时钟 */
    RCC_AHB1ENR |= RCC_AHB1ENR_GPIOBEN;

    /* PB6和PB7配置为复用功能 (AF4 = I2C1) */
    /* MODER: 10 = 复用功能 */
    GPIOB_MODER &= ~(0xFU << (6U * 2U));  /* 清除PB6, PB7模式位 */
    GPIOB_MODER |= (0xAU << (6U * 2U));   /* 设置PB6, PB7为复用功能 */

    /* OTYPER: 1 = 开漏输出 */
    GPIOB_OTYPER |= (0x3U << 6U);  /* PB6, PB7开漏 */

    /* OSPEEDR: 11 = 高速 */
    GPIOB_OSPEEDR |= (0xFU << (6U * 2U));  /* PB6, PB7高速 */

    /* PUPDR: 01 = 上拉 */
    GPIOB_PUPDR &= ~(0xFU << (6U * 2U));  /* 清除PB6, PB7上下拉位 */
    GPIOB_PUPDR |= (0x5U << (6U * 2U));   /* PB6, PB7上拉 */

    /* AFRL: AF4 = I2C1 */
    GPIOB_AFRL &= ~(0xFFU << (6U * 4U));  /* 清除PB6, PB7复用功能位 */
    GPIOB_AFRL |= (0x44U << (6U * 4U));   /* PB6, PB7 = AF4 (I2C1) */
}

/**
 * @brief 反初始化I2C GPIO
 */
static void i2c_gpio_deinit(void)
{
    /* PB6和PB7恢复为输入模式 */
    GPIOB_MODER &= ~(0xFU << (6U * 2U));

    /* 禁用GPIOB时钟 */
    RCC_AHB1ENR &= ~RCC_AHB1ENR_GPIOBEN;
}

/**
 * @brief 使能I2C时钟
 */
static void i2c_clk_enable(void)
{
    RCC_APB1ENR |= RCC_APB1ENR_I2C1EN;
}

/**
 * @brief 禁用I2C时钟
 */
static void i2c_clk_disable(void)
{
    RCC_APB1ENR &= ~RCC_APB1ENR_I2C1EN;
}
