#include "measurements.h"

// TODO include ssd1306.h
// TODO constants
// Vdd = 3.3, Rk = 56, Rc = 100

void measure(TIM_HandleTypeDef* htim, uint32_t channel) {
	// TODO set A6 PWM output to 50%
	uint32_t drive = 50; // this is not percent!
	HAL_TIM_PWM_Start_DMA(htim, channel, (uint32_t *)drive, 1);
	// TODO delay
	// TODO read A1 (Vk), A2 (Vrk) and A3 (Vce) - convert readings to voltage (uint16_t (0-16xxx) to float (0.000-3.300))
	// TODO calculate Vf = Vdd - Vk, If = (Vk - Vrk) / Rk, Ic = (Vdd - Vce) / Rc, ctr = 100.0 * Ic / If;
	// TODO Verify CTR Range (130% - 260%)
	// TODO Verify Vf Range (1.1V - 1.4V)
	// TODO Dark Leakage Test (Turn OFF LED)
	drive = 0;
	HAL_TIM_PWM_Start_DMA(htim, channel, (uint32_t *)drive, 1);
	// TODO Verify darkVce - darkVce > (V_AN * 0.95) -> OK
	// TODO display values in ssd1306
}
