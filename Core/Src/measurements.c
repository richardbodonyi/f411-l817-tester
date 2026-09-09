#include "measurements.h"
#include "ssd1306.h"
#include "ssd1306_fonts.h"
#include <stdio.h>

// TODO include ssd1306.h
// TODO constants
// Vdd = 3.3, Rk = 56, Rc = 100

const uint8_t INDEX_A1 = 0,
	INDEX_A2 = 1,
	INDEX_A3 = 2;
float VDD = 3.3f, R_K = 55.4, R_C = 99;

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
} OptoMetrics;

float digitalToAnalogValue(uint16_t value) {
	return (value * 3.3f) / 4095.0f;
}

char* failureMark(uint8_t passed) {
	return passed == 1 ? "" : "* ";
}

void updateDisplay(OptoMetrics metrics) {
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

//    uint8_t passed = metrics.ctrPassed && metrics.vfPassed && metrics.darkPassed;
    snprintf(buf, sizeof(buf), "Dark Vce: %.2fV %s", metrics.Vcedark, failureMark(metrics.darkPassed));
    ssd1306_SetCursor(0, 48);
    ssd1306_WriteString(buf, Font_7x10, White);

    ssd1306_UpdateScreen();
}

void measure(TIM_HandleTypeDef *htim, uint32_t pwmChannel, uint16_t *adcValues) {
	OptoMetrics metrics;
	// set A6 PWM output to 50%
	uint16_t drive = 80; // this is not percentage!
	__HAL_TIM_SET_COMPARE(htim, pwmChannel, drive);

	// delay
	HAL_Delay(200);

	// read A1 (Vk), A2 (Vrk) and A3 (Vce) - convert readings to voltage (uint16_t (0-16xxx) to float (0.000-3.300))
	float Vk = digitalToAnalogValue(adcValues[INDEX_A2]), Vrk = digitalToAnalogValue(adcValues[INDEX_A3]);
	metrics.Vce = digitalToAnalogValue(adcValues[INDEX_A1]);

	// calculate Vf = Vdd - Vk, If = (Vk - Vrk) / Rk, Ic = (Vdd - Vce) / Rc, ctr = 100.0 * Ic / If;
//	metrics.Vf = Vk;
//	metrics.If = 0;
//	metrics.Ic = 0;
//	metrics.CTR = 0;

	metrics.Vf = VDD - Vk;
	metrics.If = 1000.0 * (Vk - Vrk) / R_K;
	metrics.Ic = 1000.0 * (VDD - metrics.Vce) / R_C;
	metrics.CTR = 100.0 * metrics.Ic / metrics.If;


	metrics.ctrPassed = 0;
	metrics.vfPassed = 0;
	metrics.darkPassed = 0;

	// Verify CTR Range (130% - 260%)
	if (metrics.CTR >= 130.0 && metrics.CTR <= 260.0) {
		// Serial.println("[PASS] CTR is within 817B Spec (130% - 260%)");
		metrics.ctrPassed = 1;
	} else if (metrics.CTR < 130.0) {
		// Serial.print("[FAIL] CTR too low! Degraded LED/phototransistor or wrong rank. ["); Serial.print(ctr, 1); Serial.println("% (130% - 260%)]");
	} else {
		// Serial.println("[FAIL] CTR > 260%. Optocoupler is Rank C or Rank D.");
	}

	// Verify Vf Range (1.1V - 1.4V)
	if (metrics.Vf>= 1.1 && metrics.Vf <= 1.4) {
//		Serial.println("[PASS] LED Forward Voltage is within standard range (1.1V - 1.4V)");
		metrics.vfPassed = 1;
	} else {
//		Serial.print("[FAIL] LED Forward Voltage out of spec! ["); Serial.print(vF, 2); Serial.println("V (1.1V - 1.4V)]");
	}

	// Dark Leakage Test (Turn OFF LED)
//	drive = 0;
//	HAL_TIM_PWM_Start_DMA(htim, pwmChannel, (uint32_t *)drive, 1);
//	HAL_Delay(100);
//	metrics.Vcedark = digitalToAnalogValue(adcValues[INDEX_A3]);
//
//	// Verify darkVce - darkVce > (V_AN * 0.95) -> OK
//	if (metrics.Vcedark > (VDD * 0.95)) {
////		Serial.println("[PASS] Dark state cutoff verified (VCE near V_AN).");
//		metrics.darkPassed = 1;
//	} else {
////		Serial.print("[FAIL] High dark leakage current detected! ["); Serial.print(darkVCE); Serial.print("V (>"); Serial.print(V_AN * 0.95, 2); Serial.println("V accepted)]");
//	}

	updateDisplay(metrics);
}


