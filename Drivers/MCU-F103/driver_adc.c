#include "stm32f1xx_hal.h"

extern ADC_HandleTypeDef hadc1;
extern TIM_HandleTypeDef htim3;

uint16_t adc_buffer[3];

void ADC_Start(void)
{
    HAL_ADC_Start_DMA(&hadc1, (uint32_t*)adc_buffer, 3);
    HAL_TIM_Base_Start(&htim3);
}

uint16_t ADC_GetRaw(uint8_t ch)
{
    return adc_buffer[ch];
}
