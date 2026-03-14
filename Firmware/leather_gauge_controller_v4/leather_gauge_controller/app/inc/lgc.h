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

extern error_t lgc_report_manager_init(void);

extern error_t lgc_interface_modbus_init(void);

extern error_t lgc_interface_modbus_set_mode(LGC_BUS_MODE_t mode);

extern error_t lgc_modbus_get_burst_data(uint8_t *buffer, uint16_t *len);

extern error_t lgc_modbus_read_holding_regs(uint8_t dev, uint16_t address, uint16_t *regs, size_t len);

extern error_t lgc_modbus_read_coils(uint8_t dev, uint16_t address, uint8_t *coils, size_t len);

extern error_t lgc_modbus_write_holding_regs(uint8_t dev, uint16_t address, uint16_t *regs, size_t len);

extern void lgc_buttons_callback(uint8_t di, uint32_t evt);

extern void lgc_set_stop_condition(uint8_t stop);

extern void lgc_get_measurements(lgc_measurements_t *out_measurements);

extern void lgc_get_state_data(lgc_t *out_data);

extern void lgc_increment_batch_index(void);

/** @deprecated Use lgc_delete_leather_by_visual_index() instead */
extern void lgc_clear_measurement_last_leather(void);

/**
 * @brief Delete a leather piece from the current batch by its visual (display) index.
 *
 * The visual index is 1-based and matches the numbering shown on the DWIN display.
 * Soft-deletes the piece: marks it as deleted in the current batch snapshot,
 * subtracts its area from accumulators, and decrements the leather counter so
 * the batch closure condition (active_count >= config.batch) is honoured.
 *
 * @param visual_index  1-based index as displayed to the operator.
 */
extern void lgc_delete_leather_by_visual_index(uint16_t visual_index);

extern uint8_t lgc_p10_init(void);

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
