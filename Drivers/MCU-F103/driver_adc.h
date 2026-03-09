#ifndef __DRIVER_ADC_H
#define __DRIVER_ADC_H
#include <stdint.h>

void ADC_Start(void);
uint16_t ADC_GetRaw(uint8_t ch);

#endif /* __DRIVER_ADC_H */
