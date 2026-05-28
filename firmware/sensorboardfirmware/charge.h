/* charge.h — Charge measurement and thermistor ADC
 * LMP7721 low range + resistor divider high range
 * Dyze 500C NTC thermistor on PA2
 */

#ifndef __CHARGE_H
#define __CHARGE_H

#include "main.h"
#include <stdint.h>

/* ADC channel numbers */
#define CHARGE_HI_CHANNEL   ADC_CHANNEL_0   /* PA0 — high range divider */
#define CHARGE_LO_CHANNEL   ADC_CHANNEL_1   /* PA1 — LMP7721 output */
#define THERM_CHANNEL       ADC_CHANNEL_2   /* PA2 — thermistor */

/* High range divider scaling
 * 1GΩ top / 1MΩ bottom = 1001:1 division
 * V_adc = V_tip / 1001
 * V_tip_mv = adc_mv * 1001 */
#define CHARGE_HI_SCALE     1001

/* ADC reference voltage millivolts */
#define ADC_VREF_MV         3300

/* ADC resolution */
#define ADC_RESOLUTION      4096  /* 12-bit */

/* Thermistor divider
 * V3V3 → R_ref (1MΩ) → ADC → Thermistor → GND
 * R_therm = R_ref * V_adc / (V3V3 - V_adc) */
#define THERM_R_REF_OHM     1000000  /* 1MΩ reference resistor */

/* Dyze 500C NTC lookup table
 * Format: {temperature_C, resistance_ohm}
 * Source: Dyze Design documentation
 * Interpolate linearly between points */
typedef struct {
    int16_t  temp_c;
    uint32_t resistance_ohm;
} Therm_LUT_Entry_t;

extern const Therm_LUT_Entry_t DYZE_LUT[];
extern const uint16_t DYZE_LUT_SIZE;

/* Public API */
void    Charge_Init(ADC_HandleTypeDef *hadc);
void    Charge_Read(ADC_HandleTypeDef *hadc,
                    int32_t *hi_range_mv,
                    int32_t *lo_range_uv);
int32_t Charge_ReadThermistor(ADC_HandleTypeDef *hadc);

/* Internal helpers */
uint32_t Charge_ReadADC(ADC_HandleTypeDef *hadc, uint32_t channel);
int32_t  Therm_ResistanceToTemp(uint32_t resistance_ohm);

#endif /* __CHARGE_H */
