#ifndef LEATHER_GAUGE_TYPEDEFS_H
#define LEATHER_GAUGE_TYPEDEFS_H

/* ============================================================================
 * Includes
 * ========================================================================= */

#include <stdint.h>
#include <stdbool.h>
#include "leather_gauge_config.h"
/* ============================================================================
 * defines
 * ========================================================================= */
#ifndef LG_ADC_SENAOR_MAX_SIZE
#define LG_ADC_SENAOR_MAX_SIZE 10
#endif
/* ============================================================================
 * typedefs
 * ========================================================================= */

typedef struct LG_SENSOR_TypeDef
{
	/*adc raw values*/
	uint16_t raw[LG_ADC_SENAOR_MAX_SIZE];
	/*adc filter values*/
	float S[LG_ADC_SENAOR_MAX_SIZE];
	/*sensor digital value*/
	uint16_t value;
	/*sensor offset aply data*/
	float D[LG_ADC_SENAOR_MAX_SIZE];
} LG_SENSOR_TypeDef_t;


typedef struct __attribute__((packed)) LG_CONF_TypeDef
{
	/*zero point calibration value*/
	float offset[LG_ADC_SENAOR_MAX_SIZE];
	/*modbus addres*/
	uint8_t address;
	/*filter FC*/
	float fc; /*0.1 - 200.0 : 1 - 2000*/
	/*Threshold data*/
	uint16_t threshold; /*1-4095*/
	/*checksum*/
	uint32_t checksum;
} LG_CONF_TypeDef_t;

#endif /* LEATHER_GAUGE_TYPEDEFS_H */
