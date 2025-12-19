/**
 *
 */
#ifndef LG_MODULE_MODBUS_H
#define LG_MODULE_MODBUS_H

/* ============================================================================
 * includes
 * ========================================================================= */
#include <string.h>
#include "leather_gauge_typedefs.h"
#include "nanomodbus.h"

/* ============================================================================
 * public function prototype
 * ========================================================================= */
typedef enum lg_module_modbus_addr
{
    /* data */
    /*configuration holding registers------------------------------------------*/
    /*offset address*/
    OFFSET_S1_ADDR = 0x0,
    OFFSET_S2_ADDR,
    OFFSET_S3_ADDR,
    OFFSET_S4_ADDR,
    OFFSET_S5_ADDR,
    OFFSET_S6_ADDR,
    OFFSET_S7_ADDR,
    OFFSET_S8_ADDR,
    OFFSET_S9_ADDR,
    OFFSET_S10_ADDR,
    /*Server address*/
    SERVER_ADDR,
    /*another data*/
    FILTER_FC_ADDR,
    /*umbral data*/
    SENSOR_THRESHOLD_ADDR,
    /*Factory reset*/
    FACTORY_RESET_ADDR,
    /*Calibration flag*/
    CALIB_FLAG_ADDR,
    /*sensor current data ------------------------------------------------------*/
    /*ADC Raw values*/
    S1_ADDR,
    S2_ADDR,
    S3_ADDR,
    S4_ADDR,
    S5_ADDR,
    S6_ADDR,
    S7_ADDR,
    S8_ADDR,
    S9_ADDR,
    S10_ADDR,
    /*ADC Offset apply*/
    D1_ADDR,
    D2_ADDR,
    D3_ADDR,
    D4_ADDR,
    D5_ADDR,
    D6_ADDR,
    D7_ADDR,
    D8_ADDR,
    D9_ADDR,
    D10_ADDR,

    /*adc values*/
    A1_ADDR,
    A2_ADDR,
    A3_ADDR,
    A4_ADDR,
    A5_ADDR,
    A6_ADDR,
    A7_ADDR,
    A8_ADDR,
    A9_ADDR,
    A10_ADDR,
    /*DIGITAL VALUE SENSOR*/
    DI_VALUE_ADDR,
    LB_MODBUS_ADDR_MAX,
} lg_module_modbus_addr_t;

/* ============================================================================
 * public function prototype
 * ========================================================================= */
uint8_t lg_module_modbus_init(uint8_t addr);

uint8_t lg_module_modbus_set_addr(uint8_t addr);

uint8_t lg_module_modbus_pool(void);

#endif /* LG_MODULE_MODBUS_H */
