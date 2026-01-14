/*
 * lgc_module_eeprom.c
 *
 *  Created on: Jan 14, 2026
 *      Author: tecna-smart-lab
 */

#include "lgc_module_eeprom.h"
#include "os_port.h"
/*global variables*/
static OsMutex mutex;
static at24cxx_handle_t eeprom;
/*public functions*/
error_t lgc_module_eeprom_init(void)
{
    /*eeprom interface init*/
    DRIVER_AT24CXX_LINK_INIT(&eeprom, at24cxx_handle_t);
    DRIVER_AT24CXX_LINK_IIC_INIT(&eeprom, at24cxx_interface_iic_init);
    DRIVER_AT24CXX_LINK_IIC_DEINIT(&eeprom, at24cxx_interface_iic_deinit);
    DRIVER_AT24CXX_LINK_IIC_READ(&eeprom, at24cxx_interface_iic_read);
    DRIVER_AT24CXX_LINK_IIC_WRITE(&eeprom, at24cxx_interface_iic_write);
    DRIVER_AT24CXX_LINK_IIC_READ_ADDRESS16(&eeprom, at24cxx_interface_iic_read_address16);
    DRIVER_AT24CXX_LINK_IIC_WRITE_ADDRESS16(&eeprom, at24cxx_interface_iic_write_address16);
    DRIVER_AT24CXX_LINK_DELAY_MS(&eeprom, at24cxx_interface_delay_ms);
    DRIVER_AT24CXX_LINK_DEBUG_PRINT(&eeprom, at24cxx_interface_debug_print);

    /* set chip type */
    at24cxx_set_typex(&eeprom, AT24C256);

    /* set addr pin */
    at24cxx_set_addr_pin(&eeprom, AT24CXX_ADDRESS_A000);


    if(at24cxx_init(&eeprom) != NO_ERROR)
    {
        return ERROR_FAILURE;
    }
    /*create mutex*/
    if(osCreateMutex(&mutex) != TRUE)
    {
        return ERROR_FAILURE;
    }

    return NO_ERROR;
}
