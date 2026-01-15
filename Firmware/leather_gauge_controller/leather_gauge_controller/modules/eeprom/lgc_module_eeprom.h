/*
 * lgc_module_eeprom.h
 *
 *  Created on: Dec 22, 2025
 *      Author: tecna-smart-lab
 */

#ifndef MODULES_EEPROM_LGC_MODULE_EEPROM_H_
#define MODULES_EEPROM_LGC_MODULE_EEPROM_H_

#include "error.h"
#include "driver_at24cxx.h"
#include "driver_at24cxx_interface.h"

typedef struct __attribute__((__packed__))
{
    uint8_t day;
    uint8_t month;
    uint16_t year;
} LGC_DATE_TypeDef_t;

typedef struct __attribute__((__packed__))
{
    uint8_t hours;
    uint8_t minutes;
    uint8_t seconds;
} LGC_TIME_TypeDef_t;

/*typedefs*/
typedef struct __attribute__((__packed__)) LGC_CONF_TypeDef
{
    /*client*/
    char client_name[12];
    /*color*/
    char color[10];
    /*leather id*/
    char leather_id[20];
    /*batch*/
    uint32_t batch;
    /*units*/
    uint8_t units;
    /*unit conversion*/
    uint8_t conversion;
    /*crc*/
    uint32_t crc;

} LGC_CONF_TypeDef_t;

/*public functions*/
error_t lgc_module_eeprom_init(void);

error_t lgc_module_conf_get(LGC_CONF_TypeDef_t *obj);

error_t lgc_module_conf_set(LGC_CONF_TypeDef_t *obj);

error_t lgc_module_conf_load(void);

#endif /* MODULES_EEPROM_LGC_MODULE_EEPROM_H_ */
