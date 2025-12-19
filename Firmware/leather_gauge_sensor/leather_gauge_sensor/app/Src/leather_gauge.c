

/* ============================================================================
 * includes
 * ========================================================================= */
#include "leather_gauge.h"

#include "lg_module_eeprom.h"
#include "lg_module_sensor.h"
#include "lg_module_modbus.h"
/* ============================================================================
 * private functions prototype
 * ========================================================================= */

/* ============================================================================
 * public functions
 * ========================================================================= */
uint8_t lg_sensor_init(void)
{
    uint8_t ret = 0;
    LG_CONF_TypeDef_t conf;
    /*eeprom interface */
    ret = lg_module_eeprom_init();

    if (ret != 0)
    {
        return ret;
    }
    lg_module_eeprom_conf_get(&conf);
    /*sensor init*/
    ret = lg_module_sensor_init(conf.fc);

    if (ret != 0)
    {
        return ret;
    }
    /*modbus init*/
    ret = lg_module_modbus_init(conf.address);

    return ret;
}

void lg_sensor_run(void)
{
    /*init another*/

    /*loop*/
    while (1)
    {
        /* code */
        lg_module_modbus_pool();
    }
}

/* ============================================================================
 * private functions definitions
 * ========================================================================= */
