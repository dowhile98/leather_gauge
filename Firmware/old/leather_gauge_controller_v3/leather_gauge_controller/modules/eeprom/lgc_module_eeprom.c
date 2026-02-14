/*
 * lgc_module_eeprom.c
 *
 *  Created on: Jan 14, 2026
 *      Author: tecna-smart-lab
 */

#include "lgc_module_eeprom.h"
#include "os_port.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#define CRC32_POLYNOMIAL 0x04C11DB7UL
/*global variables*/
static OsMutex mutex;
static at24cxx_handle_t eeprom;
static LGC_CONF_TypeDef_t lgc_conf = {0};

/* static CRC32 (IEEE 802.3) implementation - table driven */
static uint32_t lgc_crc32_compute(const uint8_t *data, size_t length)
{
    uint32_t crc = 0xFFFFFFFFUL;
    size_t i, j;

    if (data == NULL || length == 0)
    {
        return 0;
    }

    for (i = 0; i < length; ++i)
    {
        crc ^= data[i];
        for (j = 0; j < 8; ++j)
        {
            if (crc & 1)
            {
                crc = (crc >> 1) ^ CRC32_POLYNOMIAL;
            }
            else
            {
                crc >>= 1;
            }
        }
    }

    crc = crc ^ 0xFFFFFFFFUL;
    return crc;
}

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
    at24cxx_set_type(&eeprom, AT24C256);

    /* set addr pin */
    at24cxx_set_addr_pin(&eeprom, AT24CXX_ADDRESS_A000);

    if (at24cxx_init(&eeprom) != NO_ERROR)
    {
        return ERROR_FAILURE;
    }
    /*create mutex*/
    if (osCreateMutex(&mutex) != TRUE)
    {
        return ERROR_FAILURE;
    }

    return NO_ERROR;
}

error_t lgc_module_conf_get(LGC_CONF_TypeDef_t *obj)
{
    /*lock mutex*/
    osAcquireMutex(&mutex);
    /*copy conf*/
    memcpy(obj, &lgc_conf, sizeof(LGC_CONF_TypeDef_t));
    /*release mutex*/
    osReleaseMutex(&mutex);

    return NO_ERROR;
}

error_t lgc_module_conf_set(LGC_CONF_TypeDef_t *obj)
{
    uint32_t crc = 0;
    /*lock mutex*/
    osAcquireMutex(&mutex);
    /*calculate crc*/
    crc = lgc_crc32_compute((uint8_t *)obj, sizeof(LGC_CONF_TypeDef_t) - sizeof(uint32_t));
    obj->crc = crc;
    /*copy conf*/
    memcpy(&lgc_conf, obj, sizeof(LGC_CONF_TypeDef_t));
    /*write to eeprom*/
    if (at24cxx_write(&eeprom, 0x0000, (uint8_t *)&lgc_conf, sizeof(LGC_CONF_TypeDef_t)) != NO_ERROR)
    {
        /*release mutex*/
        osReleaseMutex(&mutex);
        return ERROR_FAILURE;
    }
    /*release mutex*/
    osReleaseMutex(&mutex);

    return NO_ERROR;
}

error_t lgc_module_conf_load(void)
{
    uint32_t crc = 0;
    /*lock mutex*/
    osAcquireMutex(&mutex);
    /*read from eeprom*/
    if (at24cxx_read(&eeprom, 0x0000, (uint8_t *)&lgc_conf, sizeof(LGC_CONF_TypeDef_t)) != NO_ERROR)
    {
        /*release mutex*/
        osReleaseMutex(&mutex);
        return ERROR_FAILURE;
    }
    /*calculate crc*/
    crc = lgc_crc32_compute((uint8_t *)&lgc_conf, sizeof(LGC_CONF_TypeDef_t) - sizeof(uint32_t));
    /*verify crc*/
    if (crc != lgc_conf.crc)
    {
        /*restore default*/
        memset(&lgc_conf, 0, sizeof(LGC_CONF_TypeDef_t));
        lgc_conf.batch = 10;
        lgc_conf.units = 0;
        // todo: add

        crc = lgc_crc32_compute((uint8_t *)&lgc_conf, sizeof(LGC_CONF_TypeDef_t) - sizeof(uint32_t));
        lgc_conf.crc = crc;
        /*write to eeprom*/
        at24cxx_write(&eeprom, 0x0000, (uint8_t *)&lgc_conf, sizeof(LGC_CONF_TypeDef_t));
        /*release mutex*/
        osReleaseMutex(&mutex);

        return NO_ERROR;
    }
    /*release mutex*/
    osReleaseMutex(&mutex);

    return NO_ERROR;
}
