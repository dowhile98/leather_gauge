/**
 *
 */

#ifndef LGC_H
#define LGC_H

//-------------------------------------------------------------------------------
// includes
//-------------------------------------------------------------------------------
#include <stdint.h>
#include "lgc_typedefs.h"
#include "error.h"
#include "os_port.h"
#include "lgc_module_input.h"
#include "lgc_module_eeprom.h"
#include "main.h"

//-------------------------------------------------------------------------------
// defines
//-------------------------------------------------------------------------------

//-------------------------------------------------------------------------------
// extern definition
//-------------------------------------------------------------------------------
extern error_t lgc_hmi_init(void);

extern error_t lgc_printer_init(void);

extern error_t lgc_interface_modbus_init(void);

extern error_t lgc_modbus_read_holding_regs(uint8_t dev,uint16_t address, uint16_t *regs, size_t len);

extern error_t lgc_modbus_read_coils(uint8_t dev,uint16_t address, uint8_t *coils, size_t len);

extern error_t lgc_modbus_write_holding_regs(uint8_t dev,uint16_t address, uint16_t *regs, size_t len);

extern void lgc_buttons_callback(uint8_t di, uint32_t evt);

extern void lgc_set_stop_condition(uint8_t stop);

extern void lgc_get_measurements(lgc_measurements_t *out_measurements);

extern void lgc_get_state_data(lgc_t *out_data);

extern OsEvent events;
//-------------------------------------------------------------------------------
// public functions
//-------------------------------------------------------------------------------
error_t lgc_system_init(void *memory);

void lgc_main_task_entry(void *param);
//-------------------------------------------------------------------------------
// 
//-------------------------------------------------------------------------------

//-------------------------------------------------------------------------------
// 
//-------------------------------------------------------------------------------

#endif
