/*
 * lgc_interface_modbus.h
 *
 *  Created on: Jan 6, 2026
 *      Author: tecna-smart-lab
 */

#ifndef MODULES_MODBUS_LGC_INTERFACE_MODBUS_H_
#define MODULES_MODBUS_LGC_INTERFACE_MODBUS_H_

/*includes*/
#include <stdint.h>
#include "error.h"
#include "os_port.h"
#include "nanomodbus.h"
#include "lgc_typedefs.h"

/*defines*/

/*public functions*/

error_t lgc_interface_modbus_init(void );

error_t lgc_interface_modbus_set_mode(LGC_BUS_MODE_t mode);

error_t lgc_modbus_reset_burst(void);

error_t lgc_modbus_get_burst_data(uint8_t *buffer, uint16_t *len);


#endif /* MODULES_MODBUS_LGC_INTERFACE_MODBUS_H_ */
