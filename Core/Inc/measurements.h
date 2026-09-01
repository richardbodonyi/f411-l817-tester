#ifndef __MEASUREMENTS_H
#define __MEASUREMENTS_H

#include "stm32f4xx_hal.h"

void measure(TIM_HandleTypeDef *htim, uint32_t pwmChannel, uint16_t *adcValues);

#endif /* __MEASUREMENTS_H */
