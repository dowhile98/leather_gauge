

/* ============================================================================
 * includes
 * ========================================================================= */
#include "leather_gauge.h"
#include "main.h"
#include "lg_module_eeprom.h"
#include "lg_module_sensor.h"
#include "lg_module_modbus.h"
/* ============================================================================
 * private variables
 * ========================================================================= */
static LG_OP_MODE_t current_mode = LG_MODE_MODBUS;
static uint32_t last_trigger_tick = 0;
extern volatile uint8_t tx_flag;
#define LG_STREAM_TIMEOUT_MS 1500
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
    
    /* Always start in MODBUS mode */
    current_mode = LG_MODE_MODBUS;

    /*sensor init*/
    ret = lg_module_sensor_init(conf.fc);

    if (ret != 0)
    {
        return ret;
    }
    /*modbus init*/
    ret = lg_module_modbus_init(conf.address, conf.baudrate);


    return ret;
}

void lg_sensor_run(void)
{
    /*loop*/
    while (1)
    {
        if (current_mode == LG_MODE_MODBUS)
        {
            lg_module_modbus_pool();
        }
        else
        {
//        	if(tx_flag)
//        	{
//        		HAL_Delay(10);
//        		/* Generate Trigger Out pulse on PB12 */
//        		HAL_GPIO_WritePin(DIR_OUTPUT_GPIO_Port, DIR_OUTPUT_Pin, GPIO_PIN_SET);
//        		/* Small delay for pulse width - could be optimized with NOPs or a timer if needed.
//        		               Given the 32us per node requirement, a few cycles is enough. */
//        		for(volatile uint32_t i = 0; i < 500; i++);
//        		HAL_GPIO_WritePin(DIR_OUTPUT_GPIO_Port, DIR_OUTPUT_Pin, GPIO_PIN_RESET);
//
//        		HAL_GPIO_WritePin(DIR_GPIO_Port, DIR_Pin, GPIO_PIN_RESET);
//
//
//        		tx_flag = 0;
//        	}
        	if(((HAL_GetTick() - last_trigger_tick)> 5) && (tx_flag == 1))
        	{
        		tx_flag = 0;
        	}
            /* In STREAM mode, check for timeout to revert to MODBUS */
            if ((HAL_GetTick() - last_trigger_tick) > LG_STREAM_TIMEOUT_MS)
            {
                current_mode = LG_MODE_MODBUS;
                lg_module_modbus_reset();
            }
        }
    }
}

LG_OP_MODE_t lg_sensor_get_mode(void)
{
    return current_mode;
}

void lg_sensor_set_mode(LG_OP_MODE_t mode)
{
    current_mode = mode;
    if (mode == LG_MODE_STREAM)
    {
        last_trigger_tick = HAL_GetTick();
    }
}

/* ============================================================================
 * private functions definitions
 * ========================================================================= */
