#include "measurements.h"
#include "ssd1306.h"
#include "ssd1306_fonts.h"
#include <stdio.h>

// TODO include ssd1306.h
// TODO constants
// Vdd = 3.3, Rk = 56, Rc = 100

const uint8_t INDEX_A1 = 0,
	INDEX_A2 = 1,
	INDEX_A3 = 2,
	INDEX_A4 = 3;
const float VDD = 3.3f, R_A = 187, R_C = 99;

//uint16_t adcValues[4];

typedef struct {
    float Vf;       // Volts
    float If;       // mA
    float Vce;      // Volts
    float Ic;       // mA
    float CTR;      // Percentage (%)
    float Vcedark;  // Volts
    uint8_t ctrPassed;
	uint8_t vfPassed;
	uint8_t darkPassed;
	float Vreg;
} Metrics;

float digitalToAnalogValue(uint16_t value) {
	return (value * 3.3f) / 4095.0f;
}

char* failureMark(uint8_t passed) {
	return passed == 1 ? "" : "* ";
}

void updateDisplay(Metrics metrics) {
    char buf[32];
    ssd1306_Fill(Black);

    snprintf(buf, sizeof(buf), "Vf:%.2fV %sIf:%.1fmA", metrics.Vf, failureMark(metrics.vfPassed), metrics.If);
    ssd1306_SetCursor(0, 0);
    ssd1306_WriteString(buf, Font_7x10, White);

    snprintf(buf, sizeof(buf), "Vce:%.2fV Ic:%.1f", metrics.Vce, metrics.Ic);
    ssd1306_SetCursor(0, 16);
    ssd1306_WriteString(buf, Font_7x10, White);

    snprintf(buf, sizeof(buf), "CTR: %.1f %% %s", metrics.CTR, failureMark(metrics.ctrPassed));
    ssd1306_SetCursor(0, 32);
    ssd1306_WriteString(buf, Font_7x10, White);

    // uint8_t passed = metrics.ctrPassed && metrics.vfPassed; // && metrics.darkPassed;
    snprintf(buf, sizeof(buf), "Dark Vce: %.2fV %s", metrics.Vcedark, failureMark(metrics.darkPassed));
//    snprintf(buf, sizeof(buf), "Pass: %s", passed ? "OK" : "x");
    ssd1306_SetCursor(0, 48);
    ssd1306_WriteString(buf, Font_7x10, White);

    ssd1306_UpdateScreen();
}

void measure(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin, ADC_HandleTypeDef *hadc, uint16_t *adcValues) {
	Metrics metrics;

	HAL_GPIO_WritePin(GPIOx, GPIO_Pin, GPIO_PIN_SET);
	HAL_Delay(100);
	HAL_ADC_Start_DMA(hadc, (uint32_t*) adcValues, 4);
	HAL_Delay(100);

	float Vk = digitalToAnalogValue(adcValues[INDEX_A2]), Va = digitalToAnalogValue(adcValues[INDEX_A3]);
	metrics.Vce = digitalToAnalogValue(adcValues[INDEX_A1]);
	metrics.Vreg = digitalToAnalogValue(adcValues[INDEX_A4]);

	metrics.Vf = Vk - Va;
	metrics.If = 1000.0 * (VDD - Vk) / R_A;
	metrics.Ic = 1000.0 * (VDD - metrics.Vce) / R_C;
	metrics.CTR = 100.0 * metrics.Ic / metrics.If;


	metrics.ctrPassed = 0;
	metrics.vfPassed = 0;
	metrics.darkPassed = 0;

	// Verify CTR Range (130% - 260%)
	if (metrics.CTR >= 130.0 && metrics.CTR <= 260.0) {
		metrics.ctrPassed = 1;
	}

	// Verify Vf Range (1.1V - 1.4V)
	if (metrics.Vf>= 1.1 && metrics.Vf <= 1.4) {
		metrics.vfPassed = 1;
	}

	// Dark Leakage Test (Turn OFF LED)
	HAL_GPIO_WritePin(GPIOx, GPIO_Pin, GPIO_PIN_RESET);
	HAL_Delay(100);
	HAL_ADC_Start_DMA(hadc, (uint32_t*) adcValues, 4);
	HAL_Delay(100);

	metrics.Vcedark = digitalToAnalogValue(adcValues[INDEX_A1]);

	// Verify darkVce - darkVce > (V_AN * 0.95) -> OK
	if (metrics.Vcedark > (VDD * 0.95)) {
		metrics.darkPassed = 1;
	}

	updateDisplay(metrics);
}


