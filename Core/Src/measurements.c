#include "measurements.h"
#include "ssd1306.h"
#include "ssd1306_fonts.h"
#include <stdio.h>

// TODO include ssd1306.h
// TODO constants
// Vdd = 3.3, Rk = 56, Rc = 100

const uint8_t INDEX_A1 = 0, INDEX_A2 = 1, INDEX_A3 = 2;
float VDD = 3.3f, R_K = 56, R_C = 100;

typedef struct {
    float Vf;       // Volts
    float If;       // mA
    float Vce;      // Volts
    float Ic;       // mA
    float CTR;      // Percentage (%)
    float Vcedark;  // Volts
} OptoMetrics;

float digitalToAnalogValue(uint16_t value) {
	return (value * 3.3f) / 4095.0f;
}

void updateDisplay(OptoMetrics metrics) {
    char buf[32];
    ssd1306_Fill(Black);

    snprintf(buf, sizeof(buf), "VF:%.2fV IF:%.1fmA", metrics.Vf, metrics.If);
    ssd1306_SetCursor(0, 0);
    ssd1306_WriteString(buf, Font_7x10, White);

    snprintf(buf, sizeof(buf), "VCE:%.2fV IC:%.1f", metrics.Vce, metrics.Ic);
    ssd1306_SetCursor(0, 16);
    ssd1306_WriteString(buf, Font_7x10, White);

    snprintf(buf, sizeof(buf), "CTR: %.1f %%", metrics.CTR);
    ssd1306_SetCursor(0, 32);
    ssd1306_WriteString(buf, Font_7x10, White);

    snprintf(buf, sizeof(buf), "Dark VCE: %.2fV", metrics.Vcedark);
    ssd1306_SetCursor(0, 48);
    ssd1306_WriteString(buf, Font_7x10, White);

    ssd1306_UpdateScreen();
}

void measure(TIM_HandleTypeDef *htim, uint32_t pwmChannel, uint16_t *adcValues) {
	OptoMetrics metrics;
	// TODO set A6 PWM output to 50%
	uint32_t drive = 50; // this is not percentage!
	HAL_TIM_PWM_Start_DMA(htim, pwmChannel, (uint32_t *)drive, 1);
	// TODO delay
	HAL_Delay(100);
	// TODO read A1 (Vk), A2 (Vrk) and A3 (Vce) - convert readings to voltage (uint16_t (0-16xxx) to float (0.000-3.300))
	float Vk = digitalToAnalogValue(adcValues[INDEX_A1]), Vrk = digitalToAnalogValue(adcValues[INDEX_A2]);
	metrics.Vce = digitalToAnalogValue(adcValues[INDEX_A3]);
	// TODO calculate Vf = Vdd - Vk, If = (Vk - Vrk) / Rk, Ic = (Vdd - Vce) / Rc, ctr = 100.0 * Ic / If;
	metrics.Vf = VDD - Vk;
	metrics.If = (Vk - Vrk) / R_K;
	metrics.Ic = (VDD - metrics.Vce) / R_C;
	metrics.CTR = 100.0 * metrics.Ic / metrics.If;

	// TODO Verify CTR Range (130% - 260%)
	// TODO Verify Vf Range (1.1V - 1.4V)
	// TODO Dark Leakage Test (Turn OFF LED)
	drive = 0;
	HAL_TIM_PWM_Start_DMA(htim, pwmChannel, (uint32_t *)drive, 1);
	HAL_Delay(100);
	metrics.Vcedark = digitalToAnalogValue(adcValues[INDEX_A3]);
	// TODO Verify darkVce - darkVce > (V_AN * 0.95) -> OK
	updateDisplay(metrics);
}


