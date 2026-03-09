#include "stm32f1xx_hal.h"

void Buzzer_Init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE(); // Enable clock for GPIOA

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_8; // Assuming the buzzer is connected to PA8
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP; // Push-pull output
    GPIO_InitStruct.Pull = GPIO_NOPULL; // No pull-up or pull-down resistors
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW; // Low speed
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct); // Initialize GPIOA with the specified settings

    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_SET); // Turn off the buzzer (assuming active high)
}

void Buzzer_On(void)
{
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_RESET); // Turn on the buzzer
}

void Buzzer_Off(void)
{
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_SET); // Turn off the buzzer
}
