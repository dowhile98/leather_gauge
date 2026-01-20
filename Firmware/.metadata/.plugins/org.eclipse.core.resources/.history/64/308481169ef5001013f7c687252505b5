
/* ============================================================================
 * Includes
 * ========================================================================= */
#include "lg_module_sensor.h"
#include "adc.h"
#include "tim.h"
#include "lg_module_modbus.h"
#include "lg_module_eeprom.h"
/* ============================================================================
 * global variables
 * ========================================================================= */
#ifndef LG_ADC_FS
#define LG_ADC_FS 100.0f
#endif
/* ============================================================================
 * global variables
 * ========================================================================= */
static LG_SENSOR_TypeDef_t sensor = {0};
static Biquad_t filter[LG_ADC_SENAOR_MAX_SIZE] = {0};
/* ============================================================================
 * private function prototype
 * ========================================================================= */

/* ============================================================================
 * public function definition
 * ========================================================================= */
uint8_t lg_module_sensor_init(float fc)
{
    uint8_t ret = 0;
    /*filte init*/
    for (uint8_t i = 0; i < LG_ADC_SENAOR_MAX_SIZE; i++)
    {
        Biquad_Init(&filter[i], BQ_LOWPASS, fc, LG_ADC_FS, 0.707f);
    }
    /*adc init*/
    if (HAL_ADCEx_Calibration_Start(&hadc1) != HAL_OK)
    {
        return 1;
    }

    if (HAL_ADC_Start_DMA(&hadc1, (uint32_t *)sensor.raw, LG_ADC_SENAOR_MAX_SIZE) != HAL_OK)
    {
        return 1;
    }

    /*start timer triger*/
    if (HAL_TIM_Base_Start(&htim3) != HAL_OK)
    {
        return 1;
    }

    return ret;
}

uint8_t lg_module_sensor_filter_set(float fc)
{
    for (uint8_t i = 0; i < LG_ADC_SENAOR_MAX_SIZE; i++)
    {
        /*reset filter*/
        Biquad_Reset(&filter[i]);
        /*init*/
        Biquad_Init(&filter[i], BQ_LOWPASS, fc, LG_ADC_FS, 0.707f);
    }
    return 0;
}

uint8_t lg_module_sensor_get(LG_SENSOR_TypeDef_t *out)
{
    memcpy(out, &sensor, sizeof(LG_SENSOR_TypeDef_t));

    return 0;
}
/* ============================================================================
 * private function definition
 * ========================================================================= */

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    LG_CONF_TypeDef_t conf = {0};
    /* Prevent unused argument(s) compilation warning */
    lg_module_eeprom_conf_get(&conf);
    /*filter data*/
    for (uint8_t i = 0; i < LG_ADC_SENAOR_MAX_SIZE; i++)
    {
        /*apply filter*/
        sensor.S[i] = Biquad_Apply(&filter[i], sensor.raw[i]);
        sensor.S[i] = (sensor.S[i] < 0) ? 0 : sensor.S[i];
        /*apply offset*/
        sensor.D[i] = sensor.S[i] - conf.offset[i];
        sensor.D[i] = (sensor.D[i] < 0) ? 0 : sensor.D[i];
        /*detect*/
        if (sensor.D[i] <= conf.threshold)
        {
            sensor.value |= 1 << (i); // set bit
        }
        else if (sensor.D[i] >= (conf.threshold + LB_THESHOLD_HYSTERESIS))
        {
            sensor.value &= ~(1 << i); // clear
        }
    }
    /*detect*/
    return;
}
