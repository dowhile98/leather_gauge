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

/*defines*/

/*public functions*/

/**
 * @brief Initialize Modbus interface
 * @return error_t Status of initialization
 */
error_t lgc_interface_modbus_init(void);

/**
 * @brief Read holding registers (synchronous/blocking)
 * @deprecated Use async API via lgc_modbus_task for new code
 */
error_t lgc_modbus_read_holding_regs(uint8_t dev, uint16_t address, uint16_t *regs, size_t len);

/**
 * @brief Write holding registers (synchronous/blocking)
 */
error_t lgc_modbus_write_holding_regs(uint8_t dev, uint16_t address, uint16_t *regs, size_t len);

/**
 * @brief Read coils (synchronous/blocking)
 */
error_t lgc_modbus_read_coils(uint8_t dev, uint16_t address, uint8_t *coils, size_t len);

/* ============================================================================
 * ASYNC API (for lgc_modbus_task FSM)
 * ============================================================================ */

/**
 * @brief Set destination Modbus RTU address
 * @param address Device address (1-247)
 */
void lgc_modbus_set_address(uint8_t address);

/**
 * @brief Send raw PDU (non-blocking)
 *
 * @param function_code Modbus function code
 * @param pdu           PDU buffer (without function code)
 * @param pdu_len       PDU length
 *
 * @return error_t NO_ERROR on success
 */
error_t lgc_modbus_send_raw_pdu(uint8_t function_code, const uint8_t *pdu, uint16_t pdu_len);

/**
 * @brief Receive raw PDU response
 *
 * @param pdu_out       Output buffer for received PDU
 * @param max_len       Maximum buffer size
 *
 * @return error_t NO_ERROR on success, error code on failure
 */
error_t lgc_modbus_receive_raw_pdu(uint8_t *pdu_out, uint16_t max_len);

#endif /* MODULES_MODBUS_LGC_INTERFACE_MODBUS_H_ */
