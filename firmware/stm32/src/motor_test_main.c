/**
 * @file motor_test_main.c
 * @brief Standalone brushed motor PWM test for kimi-fly STM32F411CEU6
 *
 * Boots directly into a safe sequential motor test:
 *   M1 -> M2 -> M3 -> M4
 *
 * Debug output is sent over USART1 (PA9/PA10, 460800 baud).
 * PWM outputs:
 *   M1: PA8  / TIM1_CH1
 *   M2: PA11 / TIM1_CH4
 *   M3: PB1  / TIM3_CH4
 *   M4: PB10 / TIM2_CH3
 */

#include "stm32f4xx_hal.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#define UART_BAUDRATE             460800U
#define MOTOR_PWM_PERIOD          999U
#define MOTOR_PWM_PRESCALER       1U
#define MOTOR_KICK_DUTY           220U
#define MOTOR_RUN_DUTY            150U
#define MOTOR_KICK_MS             120U
#define MOTOR_RUN_MS              900U
#define MOTOR_STOP_MS             700U
#define CYCLE_GAP_MS              2000U

typedef struct {
    const char *name;
    TIM_HandleTypeDef *htim;
    uint32_t channel;
} motor_channel_t;

static UART_HandleTypeDef huart1;
static TIM_HandleTypeDef htim1;
static TIM_HandleTypeDef htim2;
static TIM_HandleTypeDef htim3;
static char g_log_buf[160];
static uint8_t g_uart_ready = 0U;
static uint8_t g_pwm_ready = 0U;

static void SystemClock_Config(void);
static void GPIO_Init(void);
static void UART1_Init(void);
static HAL_StatusTypeDef MotorPWM_Init(void);
static void motor_test_printf(const char *fmt, ...);
static void set_motor_duty(const motor_channel_t *motor, uint16_t duty);
static void stop_all_motors(void);
static void run_motor_sequence(void);
void Error_Handler(void);

static const motor_channel_t g_motors[] = {
    {"Motor1", &htim1, TIM_CHANNEL_1},
    {"Motor2", &htim1, TIM_CHANNEL_4},
    {"Motor3", &htim3, TIM_CHANNEL_4},
    {"Motor4", &htim2, TIM_CHANNEL_3},
};

void HAL_TIM_PWM_MspInit(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM1) {
        __HAL_RCC_TIM1_CLK_ENABLE();
    } else if (htim->Instance == TIM2) {
        __HAL_RCC_TIM2_CLK_ENABLE();
    } else if (htim->Instance == TIM3) {
        __HAL_RCC_TIM3_CLK_ENABLE();
    }
}

static void motor_test_printf(const char *fmt, ...)
{
    va_list args;
    int len;

    if (g_uart_ready == 0U) {
        return;
    }

    va_start(args, fmt);
    len = vsnprintf(g_log_buf, sizeof(g_log_buf), fmt, args);
    va_end(args);

    if (len > 0 && len < (int)sizeof(g_log_buf)) {
        HAL_UART_Transmit(&huart1, (uint8_t *)g_log_buf, (uint16_t)len, 200U);
    }
}

static void set_motor_duty(const motor_channel_t *motor, uint16_t duty)
{
    if (motor == NULL) {
        return;
    }

    if (duty > MOTOR_PWM_PERIOD) {
        duty = MOTOR_PWM_PERIOD;
    }

    __HAL_TIM_SET_COMPARE(motor->htim, motor->channel, duty);
}

static void stop_all_motors(void)
{
    size_t i;

    if (g_pwm_ready == 0U) {
        return;
    }

    for (i = 0; i < (sizeof(g_motors) / sizeof(g_motors[0])); i++) {
        set_motor_duty(&g_motors[i], 0U);
    }
}

static HAL_StatusTypeDef init_single_pwm(TIM_HandleTypeDef *htim, TIM_TypeDef *instance)
{
    TIM_OC_InitTypeDef sConfig = {0};

    htim->Instance = instance;
    htim->Init.Prescaler = MOTOR_PWM_PRESCALER;
    htim->Init.CounterMode = TIM_COUNTERMODE_UP;
    htim->Init.Period = MOTOR_PWM_PERIOD;
    htim->Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim->Init.RepetitionCounter = 0U;
    htim->Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

    if (HAL_TIM_PWM_Init(htim) != HAL_OK) {
        return HAL_ERROR;
    }

    sConfig.OCMode = TIM_OCMODE_PWM1;
    sConfig.Pulse = 0U;
    sConfig.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfig.OCFastMode = TIM_OCFAST_DISABLE;
#ifdef TIM_OCNPOLARITY_HIGH
    sConfig.OCNPolarity = TIM_OCNPOLARITY_HIGH;
#endif
#ifdef TIM_OCIDLESTATE_RESET
    sConfig.OCIdleState = TIM_OCIDLESTATE_RESET;
#endif
#ifdef TIM_OCNIDLESTATE_RESET
    sConfig.OCNIdleState = TIM_OCNIDLESTATE_RESET;
#endif

    return HAL_TIM_PWM_ConfigChannel(htim, &sConfig, TIM_CHANNEL_1);
}

static HAL_StatusTypeDef MotorPWM_Init(void)
{
    TIM_OC_InitTypeDef sConfig = {0};

    g_pwm_ready = 0U;

    if (init_single_pwm(&htim1, TIM1) != HAL_OK) {
        return HAL_ERROR;
    }
    if (HAL_TIM_PWM_ConfigChannel(&htim1, &(TIM_OC_InitTypeDef){
            .OCMode = TIM_OCMODE_PWM1,
            .Pulse = 0U,
            .OCPolarity = TIM_OCPOLARITY_HIGH,
            .OCFastMode = TIM_OCFAST_DISABLE
        }, TIM_CHANNEL_4) != HAL_OK) {
        return HAL_ERROR;
    }

    htim2.Instance = TIM2;
    htim2.Init.Prescaler = MOTOR_PWM_PRESCALER;
    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim2.Init.Period = MOTOR_PWM_PERIOD;
    htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_PWM_Init(&htim2) != HAL_OK) {
        return HAL_ERROR;
    }

    htim3.Instance = TIM3;
    htim3.Init.Prescaler = MOTOR_PWM_PRESCALER;
    htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim3.Init.Period = MOTOR_PWM_PERIOD;
    htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_PWM_Init(&htim3) != HAL_OK) {
        return HAL_ERROR;
    }

    sConfig.OCMode = TIM_OCMODE_PWM1;
    sConfig.Pulse = 0U;
    sConfig.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfig.OCFastMode = TIM_OCFAST_DISABLE;

    if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfig, TIM_CHANNEL_3) != HAL_OK) {
        return HAL_ERROR;
    }
    if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfig, TIM_CHANNEL_4) != HAL_OK) {
        return HAL_ERROR;
    }

    if (HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1) != HAL_OK) {
        return HAL_ERROR;
    }
    if (HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_4) != HAL_OK) {
        return HAL_ERROR;
    }
    if (HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_3) != HAL_OK) {
        return HAL_ERROR;
    }
    if (HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_4) != HAL_OK) {
        return HAL_ERROR;
    }

    g_pwm_ready = 1U;
    stop_all_motors();
    return HAL_OK;
}

static void GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;

    GPIO_InitStruct.Pin = GPIO_PIN_8 | GPIO_PIN_11;
    GPIO_InitStruct.Alternate = GPIO_AF1_TIM1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_1;
    GPIO_InitStruct.Alternate = GPIO_AF2_TIM3;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_10;
    GPIO_InitStruct.Alternate = GPIO_AF1_TIM2;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_14;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_RESET);
}

static void UART1_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_USART1_CLK_ENABLE();

    GPIO_InitStruct.Pin = GPIO_PIN_9 | GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    huart1.Instance = USART1;
    huart1.Init.BaudRate = UART_BAUDRATE;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart1);
    g_uart_ready = 1U;
}

static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
    RCC_OscInitStruct.PLL.PLLM = 16U;
    RCC_OscInitStruct.PLL.PLLN = 336U;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
    RCC_OscInitStruct.PLL.PLLQ = 7U;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                  RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
        Error_Handler();
    }
}

static void run_motor_sequence(void)
{
    stop_all_motors();
    HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_14);
    motor_test_printf("[MOTOR_TEST] outputs held at 0, all motors stopped\r\n");
    HAL_Delay(CYCLE_GAP_MS);
}

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    GPIO_Init();
    UART1_Init();

    if (MotorPWM_Init() != HAL_OK) {
        Error_Handler();
    }

    HAL_Delay(1500U);

    motor_test_printf("\r\n========================================\r\n");
    motor_test_printf("  MOTOR SAFE HOLD v1.1 (STM32F411 @ 42kHz)\r\n");
    motor_test_printf("========================================\r\n");
    motor_test_printf("PWM initialized, compare registers forced to 0\r\n");
    motor_test_printf("All motors remain stopped until a different firmware is flashed.\r\n\r\n");

    while (1) {
        run_motor_sequence();
    }
}

void SysTick_Handler(void)
{
    HAL_IncTick();
}

void Error_Handler(void)
{
    stop_all_motors();
    motor_test_printf("[MOTOR_TEST] Error_Handler\r\n");

    while (1) {
        HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_14);
        HAL_Delay(100U);
    }
}
