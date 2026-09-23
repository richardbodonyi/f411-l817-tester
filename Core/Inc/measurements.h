#ifndef __MEASUREMENTS_H
#define __MEASUREMENTS_H

#include "stm32f4xx_hal.h"

#define ADC_VALUE_COUNT 4

void measureOptocoupler(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin, ADC_HandleTypeDef *hadc, uint16_t *adcValues);

void measureRegulator(ADC_HandleTypeDef *hadc, uint16_t *adcValues);

#endif /* __MEASUREMENTS_H */
