#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"

void LED_Init(void)
{
    __HAL_RCC_GPIOC_CLK_ENABLE(); // Enable clock for GPIOC

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_13; // Assuming the LED is connected to PC13
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP; // Push-pull output
    GPIO_InitStruct.Pull = GPIO_NOPULL; // No pull-up or pull-down resistors
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW; // Low speed
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct); // Initialize GPIOC with the specified settings

    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET); // Turn off the LED (assuming active low)
}

void LED_Shining(void)
{
    HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13); // Toggle the state of the LED
    HAL_Delay(500); // Delay for 500 milliseconds
}

void LED_Warning(void)
{
    HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13); // Toggle the state of the LED
    vTaskDelay(pdMS_TO_TICKS(500)); // Delay for 500 milliseconds
}

void LED_OFF(void)
{
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
}
