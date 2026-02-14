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
#include "lgc_i_sensor_cache.h"
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

extern error_t lgc_modbus_read_holding_regs(uint8_t dev, uint16_t address, uint16_t *regs, size_t len);

extern error_t lgc_modbus_read_coils(uint8_t dev, uint16_t address, uint8_t *coils, size_t len);

extern error_t lgc_modbus_write_holding_regs(uint8_t dev, uint16_t address, uint16_t *regs, size_t len);

extern void lgc_buttons_callback(uint8_t di, uint32_t evt);

extern void lgc_set_stop_condition(uint8_t stop);

extern void lgc_get_measurements(lgc_measurements_t *out_measurements);

extern void lgc_get_state_data(lgc_t *out_data);

extern void lgc_increment_batch_index(void);

extern void lgc_clear_measurement_last_leather(void);

extern OsEvent events;
//-------------------------------------------------------------------------------
// public functions
//-------------------------------------------------------------------------------
error_t lgc_system_init(void *memory);

void lgc_main_task_entry(void *param);

/**
 * @brief Get sensor cache interface for non-blocking sensor reads
 *
 * @return ILgcSensorCache_t* Interface pointer
 */
ILgcSensorCache_t *lgc_get_sensor_cache_interface(void);

/**
 * @brief Trigger Modbus read cycle (called from encoder callback)
 *
 * Signals the Modbus task to begin a new sensor polling round.
 * Non-blocking - returns immediately after signaling.
 */
void lgc_trigger_modbus_cycle(void);
//-------------------------------------------------------------------------------
//
//-------------------------------------------------------------------------------

//-------------------------------------------------------------------------------
//
//-------------------------------------------------------------------------------

#endif
