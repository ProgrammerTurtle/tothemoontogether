/* charge.c — Charge measurement and thermistor ADC implementation */

#include "charge.h"
#include <stdio.h>

/* ── Dyze 500C NTC lookup table ─────────────────────────────── */
/* Temperature (C) vs Resistance (Ω)
 * Source: Dyze Design documentation
 * Extended range — useful data from ~50C to ~400C with 1MΩ divider */
const Therm_LUT_Entry_t DYZE_LUT[] = {
    {  10,  8100000 },
    {  20,  5200000 },
    {  25,  4500000 },
    {  30,  2830000 },
    {  60,   666000 },
    {  80,   288000 },
    { 100,   136000 },
    { 120,    70000 },
    { 140,    38500 },
    { 160,    22200 },
    { 180,    13200 },
    { 200,     8070 },
    { 220,     5120 },
    { 240,     3380 },
    { 260,     2240 },
    { 280,     1555 },
    { 300,     1100 },
    { 320,      790 },
    { 340,      578 },
    { 360,      434 },
    { 380,      333 },
    { 400,      253 },
    { 420,      196 },
    { 440,      157 },
    { 460,      125 },
    { 480,      102 },
    { 500,       84 },
};
const uint16_t DYZE_LUT_SIZE = sizeof(DYZE_LUT) / sizeof(DYZE_LUT[0]);

/* ── ADC channel read ───────────────────────────────────────── */

uint32_t Charge_ReadADC(ADC_HandleTypeDef *hadc, uint32_t channel) {
    ADC_ChannelConfTypeDef cfg = {0};
    cfg.Channel      = channel;
    cfg.Rank         = ADC_REGULAR_RANK_1;
    cfg.SamplingTime = ADC_SAMPLINGTIME_COMMON_1;
    HAL_ADC_ConfigChannel(hadc, &cfg);

    HAL_ADC_Start(hadc);
    HAL_ADC_PollForConversion(hadc, 100);
    uint32_t val = HAL_ADC_GetValue(hadc);
    HAL_ADC_Stop(hadc);
    return val;
}

/* ── Public API ─────────────────────────────────────────────── */

void Charge_Init(ADC_HandleTypeDef *hadc) {
    /* Run calibration */
    HAL_ADCEx_Calibration_Start(hadc);
    printf("[CHARGE] ADC calibrated\r\n");
}

void Charge_Read(ADC_HandleTypeDef *hadc,
                 int32_t *hi_range_mv,
                 int32_t *lo_range_uv) {

    /* High range — PA0 — resistor divider output */
    uint32_t hi_raw = Charge_ReadADC(hadc, CHARGE_HI_CHANNEL);
    /* Convert raw ADC to millivolts at divider output */
    int32_t hi_adc_mv = (int32_t)((uint64_t)hi_raw * ADC_VREF_MV / ADC_RESOLUTION);
    /* Scale back up through divider to get tip voltage */
    *hi_range_mv = hi_adc_mv * CHARGE_HI_SCALE;

    /* Low range — PA1 — LMP7721 output
     * LMP7721 referenced to 1.65V virtual ground
     * Output is centered at 1.65V, swings ±1.65V
     * Convert to microvolts relative to virtual ground */
    uint32_t lo_raw = Charge_ReadADC(hadc, CHARGE_LO_CHANNEL);
    int32_t lo_adc_mv = (int32_t)((uint64_t)lo_raw * ADC_VREF_MV / ADC_RESOLUTION);
    /* Subtract virtual ground offset (1650mV) and convert to uV */
    *lo_range_uv = (lo_adc_mv - 1650) * 1000;
    /* TODO: apply LMP7721 gain correction when gain is known */
}

/* ── Thermistor ─────────────────────────────────────────────── */

int32_t Therm_ResistanceToTemp(uint32_t resistance_ohm) {
    /* Linear interpolation through Dyze lookup table */

    /* Above max resistance — below min temperature in table */
    if (resistance_ohm >= DYZE_LUT[0].resistance_ohm)
        return (int32_t)DYZE_LUT[0].temp_c * 1000;

    /* Below min resistance — above max temperature in table */
    if (resistance_ohm <= DYZE_LUT[DYZE_LUT_SIZE - 1].resistance_ohm)
        return (int32_t)DYZE_LUT[DYZE_LUT_SIZE - 1].temp_c * 1000;

    /* Find bracket */
    for (uint16_t i = 0; i < DYZE_LUT_SIZE - 1; i++) {
        if (resistance_ohm <= DYZE_LUT[i].resistance_ohm &&
            resistance_ohm >= DYZE_LUT[i + 1].resistance_ohm) {

            /* Linear interpolation */
            int32_t t0 = DYZE_LUT[i].temp_c;
            int32_t t1 = DYZE_LUT[i + 1].temp_c;
            int32_t r0 = (int32_t)DYZE_LUT[i].resistance_ohm;
            int32_t r1 = (int32_t)DYZE_LUT[i + 1].resistance_ohm;
            int32_t r  = (int32_t)resistance_ohm;

            /* t = t0 + (t1-t0) * (r0-r) / (r0-r1), result in millidegrees */
            int32_t temp_mc = t0 * 1000
                + (int32_t)((int64_t)(t1 - t0) * 1000 * (r0 - r) / (r0 - r1));
            return temp_mc;
        }
    }
    return 0;
}

int32_t Charge_ReadThermistor(ADC_HandleTypeDef *hadc) {
    uint32_t raw = Charge_ReadADC(hadc, THERM_CHANNEL);

    /* Convert raw to millivolts */
    uint32_t v_adc_mv = (uint32_t)((uint64_t)raw * ADC_VREF_MV / ADC_RESOLUTION);

    /* Avoid divide by zero — ADC at rail means thermistor open circuit */
    if (v_adc_mv >= ADC_VREF_MV - 10) {
        printf("[THERM] Open circuit or out of range (cold)\r\n");
        return -99000; /* -99C sentinel */
    }
    if (v_adc_mv <= 10) {
        printf("[THERM] Short circuit\r\n");
        return 999000; /* 999C sentinel */
    }

    /* Calculate thermistor resistance from divider
     * V_adc = V3V3 * R_therm / (R_ref + R_therm)
     * R_therm = R_ref * V_adc / (V3V3 - V_adc) */
    uint32_t r_therm = (uint32_t)((uint64_t)THERM_R_REF_OHM * v_adc_mv
                     / (ADC_VREF_MV - v_adc_mv));

    return Therm_ResistanceToTemp(r_therm);
}
