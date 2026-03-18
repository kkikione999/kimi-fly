/**
 * @file i2c.c
 * @brief STM32 I2C HAL实现 (STM32Cube HAL API版本)
 *
 * @note 本文件为Ralph-loop v2.0 HAL层实现
 *       使用STM32Cube HAL API实现I2C通信
 *       支持I2C1与气压计(LPS22HBTR)和磁力计(QMC5883P)通信
 *
 * @hardware
 *   - I2C1: PB6 (SCL), PB7 (SDA)
 *   - 时钟源: APB1 (42MHz)
 *   - GPIO配置: 开漏输出 + 内部上拉
 */

#include "i2c.h"
#include <string.h>

/* STM32F4xx HAL库头文件 */
#include "stm32f4xx_hal.h"

/* ============================================================================
 * 私有宏定义
 * ============================================================================ */

#define I2C_MAX_RETRIES     3U          /* 最大重试次数 */
#define I2C_FLAG_TIMEOUT    100U        /* 标志位等待超时 (ms) */
#define I2C_MIN_ADDRESS     0x08U       /* 最小有效7位地址 */
#define I2C_MAX_ADDRESS     0x77U       /* 最大有效7位地址 */

/* ============================================================================
 * 私有变量
 * ============================================================================ */

/* I2C1 HAL句柄 */
static I2C_HandleTypeDef hi2c1;

/* ============================================================================
 * 私有函数声明
 * ============================================================================ */

static void i2c_gpio_init(void);
static void i2c_gpio_deinit(void);
static void i2c_clk_enable(void);
static void i2c_clk_disable(void);
static void i2c_hal_error_to_local(i2c_handle_t *hi2c);

/* ============================================================================
 * API 实现
 * ============================================================================ */

/**
 * @brief 初始化I2C外设
 */
hal_status_t i2c_init(i2c_handle_t *hi2c, const i2c_config_t *config)
{
    if (hi2c == NULL || config == NULL) {
        return HAL_ERROR;
    }

    /* 保存配置 */
    hi2c->config = *config;
    hi2c->error_code = I2C_ERROR_NONE;
    hi2c->timeout = 100U;  /* 默认超时100ms */

    /* 目前只支持I2C1 */
    hi2c->instance = I2C1;

    /* 使能时钟 */
    i2c_clk_enable();

    /* 配置GPIO */
    i2c_gpio_init();

    /* 配置I2C HAL句柄 */
    hi2c1.Instance = I2C1;
    hi2c1.Init.ClockSpeed = config->clock_speed;
    hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
    hi2c1.Init.OwnAddress1 = config->own_address;
    hi2c1.Init.AddressingMode = (config->addr_mode == I2C_ADDR_MODE_10BIT) ?
                                 I2C_ADDRESSINGMODE_10BIT : I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.OwnAddress2 = 0;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;

    /* 初始化I2C */
    if (HAL_I2C_Init(&hi2c1) != HAL_OK) {
        hi2c->error_code = I2C_ERROR_TIMEOUT;
        return HAL_ERROR;
    }

    return HAL_OK;
}

/**
 * @brief 反初始化I2C外设
 */
hal_status_t i2c_deinit(i2c_handle_t *hi2c)
{
    if (hi2c == NULL || hi2c->instance == NULL) {
        return HAL_ERROR;
    }

    /* 反初始化HAL I2C */
    HAL_I2C_DeInit(&hi2c1);

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
    HAL_StatusTypeDef hal_status;

    if (hi2c == NULL || hi2c->instance == NULL || data == NULL || size == 0U) {
        return HAL_ERROR;
    }

    hi2c->error_code = I2C_ERROR_NONE;

    /* 使用HAL_I2C_Master_Transmit发送数据 */
    /* dev_addr需要左移1位以符合HAL库要求 */
    hal_status = HAL_I2C_Master_Transmit(&hi2c1, (uint16_t)(dev_addr << 1),
                                          (uint8_t *)data, size, timeout);

    if (hal_status != HAL_OK) {
        i2c_hal_error_to_local(hi2c);
        return HAL_ERROR;
    }

    return HAL_OK;
}

/**
 * @brief I2C主模式接收数据
 */
hal_status_t i2c_master_receive(i2c_handle_t *hi2c, uint16_t dev_addr,
                                 uint8_t *data, uint16_t size,
                                 uint32_t timeout)
{
    HAL_StatusTypeDef hal_status;

    if (hi2c == NULL || hi2c->instance == NULL || data == NULL || size == 0U) {
        return HAL_ERROR;
    }

    hi2c->error_code = I2C_ERROR_NONE;

    /* 使用HAL_I2C_Master_Receive接收数据 */
    /* dev_addr需要左移1位以符合HAL库要求 */
    hal_status = HAL_I2C_Master_Receive(&hi2c1, (uint16_t)(dev_addr << 1),
                                         data, size, timeout);

    if (hal_status != HAL_OK) {
        i2c_hal_error_to_local(hi2c);
        return HAL_ERROR;
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
    HAL_StatusTypeDef hal_status;
    uint16_t hal_mem_addr_size;

    if (hi2c == NULL || hi2c->instance == NULL || data == NULL || size == 0U ||
        (mem_addr_size != 1U && mem_addr_size != 2U)) {
        return HAL_ERROR;
    }

    hi2c->error_code = I2C_ERROR_NONE;

    /* 转换地址大小为HAL格式 */
    hal_mem_addr_size = (mem_addr_size == 2U) ?
                        I2C_MEMADD_SIZE_16BIT : I2C_MEMADD_SIZE_8BIT;

    /* 使用HAL_I2C_Mem_Write写入寄存器 */
    hal_status = HAL_I2C_Mem_Write(&hi2c1, (uint16_t)(dev_addr << 1),
                                    mem_addr, hal_mem_addr_size,
                                    (uint8_t *)data, size, timeout);

    if (hal_status != HAL_OK) {
        i2c_hal_error_to_local(hi2c);
        return HAL_ERROR;
    }

    return HAL_OK;
}

/**
 * @brief 从I2C设备寄存器读取数据
 */
hal_status_t i2c_mem_read(i2c_handle_t *hi2c, uint16_t dev_addr,
                           uint16_t mem_addr, uint8_t mem_addr_size,
                           uint8_t *data, uint16_t size,
                           uint32_t timeout)
{
    HAL_StatusTypeDef hal_status;
    uint16_t hal_mem_addr_size;

    if (hi2c == NULL || hi2c->instance == NULL || data == NULL || size == 0U ||
        (mem_addr_size != 1U && mem_addr_size != 2U)) {
        return HAL_ERROR;
    }

    hi2c->error_code = I2C_ERROR_NONE;

    /* 转换地址大小为HAL格式 */
    hal_mem_addr_size = (mem_addr_size == 2U) ?
                        I2C_MEMADD_SIZE_16BIT : I2C_MEMADD_SIZE_8BIT;

    /* 使用HAL_I2C_Mem_Read读取寄存器 */
    hal_status = HAL_I2C_Mem_Read(&hi2c1, (uint16_t)(dev_addr << 1),
                                   mem_addr, hal_mem_addr_size,
                                   data, size, timeout);

    if (hal_status != HAL_OK) {
        i2c_hal_error_to_local(hi2c);
        return HAL_ERROR;
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
    HAL_StatusTypeDef hal_status;
    uint32_t retry;

    if (hi2c == NULL || hi2c->instance == NULL) {
        return HAL_ERROR;
    }

    for (retry = 0U; retry < I2C_MAX_RETRIES; retry++) {
        /* 使用HAL_I2C_IsDeviceReady检查设备 */
        hal_status = HAL_I2C_IsDeviceReady(&hi2c1, (uint16_t)(dev_addr << 1),
                                            I2C_MAX_RETRIES, I2C_FLAG_TIMEOUT);

        if (hal_status == HAL_OK) {
            return HAL_OK;
        }
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
 * @brief 将HAL错误码转换为本地错误码
 */
static void i2c_hal_error_to_local(i2c_handle_t *hi2c)
{
    if (hi2c == NULL) {
        return;
    }

    /* 根据HAL错误码设置本地错误码 */
    if (hi2c1.ErrorCode & HAL_I2C_ERROR_BERR) {
        hi2c->error_code |= I2C_ERROR_BERR;
    }
    if (hi2c1.ErrorCode & HAL_I2C_ERROR_ARLO) {
        hi2c->error_code |= I2C_ERROR_ARLO;
    }
    if (hi2c1.ErrorCode & HAL_I2C_ERROR_AF) {
        hi2c->error_code |= I2C_ERROR_AF;
    }
    if (hi2c1.ErrorCode & HAL_I2C_ERROR_OVR) {
        hi2c->error_code |= I2C_ERROR_OVR;
    }
    if (hi2c1.ErrorCode & HAL_I2C_ERROR_DMA) {
        hi2c->error_code |= I2C_ERROR_DMA;
    }
    if (hi2c1.ErrorCode & HAL_I2C_ERROR_TIMEOUT) {
        hi2c->error_code |= I2C_ERROR_TIMEOUT;
    }
    if (hi2c1.ErrorCode & HAL_I2C_ERROR_SIZE) {
        hi2c->error_code |= I2C_ERROR_SIZE;
    }
}

/**
 * @brief 初始化I2C GPIO (PB6=SCL, PB7=SDA)
 */
static void i2c_gpio_init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* 使能GPIOB时钟 */
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* 配置PB6和PB7为复用开漏模式 */
    GPIO_InitStruct.Pin = GPIO_PIN_6 | GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;

    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

/**
 * @brief 反初始化I2C GPIO
 */
static void i2c_gpio_deinit(void)
{
    /* 反初始化PB6和PB7 */
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_6 | GPIO_PIN_7);

    /* 禁用GPIOB时钟 */
    __HAL_RCC_GPIOB_CLK_DISABLE();
}

/**
 * @brief 使能I2C时钟
 */
static void i2c_clk_enable(void)
{
    __HAL_RCC_I2C1_CLK_ENABLE();
}

/**
 * @brief 禁用I2C时钟
 */
static void i2c_clk_disable(void)
{
    __HAL_RCC_I2C1_CLK_DISABLE();
}
