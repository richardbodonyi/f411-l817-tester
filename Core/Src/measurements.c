#include "measurements.h"
#include "ssd1306.h"
#include "ssd1306_fonts.h"
#include <stdio.h>

const uint8_t INDEX_A1 = 0,
	INDEX_A2 = 1,
	INDEX_A3 = 2,
	INDEX_A4 = 3;

const float VDD = 3.3f,
		R_A = 187,
		R_C = 99,
		R1 = 14.98,
		R2 = 9.97,
		regulatorRatio = (R1 + R2) / R2;

const char REGULATOR_WITHIN_SPECS[] = "Within specs",
		REGULATOR_SHORTED[] = "Shorted",
		REGULATOR_DEGRADED[] = "Degraded";

const char REGULATOR_SUMMARY[2][20] = {"2.45-2.55V in specs", "0-0.7V short"};

enum RegulatorResult {
	DEGRADED,
	SHORTED,
	WITHIN_SPECS
};

typedef struct {
    float Vf;       // V
    float If;       // mA
    float Vce;      // V
    float Ic;       // mA
    float CTR;      // %
    float Vcedark;  // V
    uint8_t ctrPassed;
	uint8_t vfPassed;
	uint8_t darkPassed;
	float Vreg;
} OptocouplerMetrics;

typedef struct {
	float Vreg;
	enum RegulatorResult result;
} RegulatorMetrics;

float digitalToAnalogValue(uint16_t value) {
	return (value * 3.3f) / 4095.0f;
}

char* failureMark(uint8_t passed) {
	return passed == 1 ? "" : "* ";
}

void displayOptocouplerMetrics(OptocouplerMetrics metrics) {
    char buffer[32];
    ssd1306_Fill(Black);

    snprintf(buffer, sizeof(buffer), "Vf:%.2fV %sIf:%.1fmA", metrics.Vf, failureMark(metrics.vfPassed), metrics.If);
    ssd1306_SetCursor(0, 0);
    ssd1306_WriteString(buffer, Font_7x10, White);

    snprintf(buffer, sizeof(buffer), "Vce:%.2fV Ic:%.1f", metrics.Vce, metrics.Ic);
    ssd1306_SetCursor(0, 16);
    ssd1306_WriteString(buffer, Font_7x10, White);

    snprintf(buffer, sizeof(buffer), "CTR: %.1f %% %s", metrics.CTR, failureMark(metrics.ctrPassed));
    ssd1306_SetCursor(0, 32);
    ssd1306_WriteString(buffer, Font_7x10, White);

    // uint8_t passed = metrics.ctrPassed && metrics.vfPassed; // && metrics.darkPassed;
    snprintf(buffer, sizeof(buffer), "Dark Vce: %.2fV %s", metrics.Vcedark, failureMark(metrics.darkPassed));
    ssd1306_SetCursor(0, 48);
    ssd1306_WriteString(buffer, Font_7x10, White);

    ssd1306_UpdateScreen();
}

void measureOptocoupler(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin, ADC_HandleTypeDef *hadc, uint16_t *adcValues) {
	OptocouplerMetrics metrics;

	HAL_GPIO_WritePin(GPIOx, GPIO_Pin, GPIO_PIN_SET);
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

	metrics.Vcedark = digitalToAnalogValue(adcValues[INDEX_A1]);

	if (metrics.Vcedark > (VDD * 0.95)) {
		metrics.darkPassed = 1;
	}

	displayOptocouplerMetrics(metrics);
}

void displayRegulatorMetrics(RegulatorMetrics metrics) {
    char buffer[32];
    ssd1306_Fill(Black);

    char* status;
    if (metrics.result == WITHIN_SPECS) {
    	status = REGULATOR_WITHIN_SPECS;
    }
    else if (metrics.result == SHORTED) {
    	status = REGULATOR_SHORTED;
    }
    else {
    	status = REGULATOR_DEGRADED;
    }

    snprintf(buffer, sizeof(buffer), "Vreg: %.2fV", metrics.Vreg);
    ssd1306_SetCursor(0, 0);
    ssd1306_WriteString(buffer, Font_7x10, White);

	ssd1306_SetCursor(0, 16);
	ssd1306_WriteString(status, Font_7x10, White);

	for (uint8_t i = 0; i < 2; i++) {
		ssd1306_SetCursor(0, 32 + i * 16);
		ssd1306_WriteString((char*) REGULATOR_SUMMARY[i], Font_7x10, White);
	}

    ssd1306_UpdateScreen();
}

float convertRegulatorMeasurementToV(float measurement) {
	return measurement * regulatorRatio;
}

void measureRegulator(ADC_HandleTypeDef *hadc, uint16_t *adcValues) {
	RegulatorMetrics metrics;
	metrics.Vreg = convertRegulatorMeasurementToV(digitalToAnalogValue(adcValues[INDEX_A4]));

	if (metrics.Vreg <= 0.7) {
		metrics.result = SHORTED;
	}
	else if (metrics.Vreg >= 2.45 && metrics.Vreg <= 2.55) {
		metrics.result = WITHIN_SPECS;
	}
	else {
		metrics.result = DEGRADED;
	}

	displayRegulatorMetrics(metrics);
}

