/*
 * lg_module_eeprom.c
 *
 *  Created on: Dec 16, 2025
 *      Author: tecna-smart-lab
 */

/* ============================================================================
 * includes
 * ========================================================================= */
#include "lg_module_eeprom.h"
#include "stm32g0xx_hal.h"
#include "stm32g0xx_hal_flash.h"
/* ============================================================================
 * defines
 * ========================================================================= */
// El STM32G030C8 tiene 64KB de Flash.
// Dirección Base: 0x08000000
// Última página (Página 31): 0x0800F800 - 0x0800FFFF (2KB)
#define EEPROM_START_ADDRESS ((uint32_t)0x0800F800)
#define EEPROM_PAGE_SIZE ((uint16_t)0x0800) // 2KB (2048 bytes)

#define CRC32_POLYNOMIAL 0x04C11DB7UL

#ifndef LG_MODBUS_SERVER_DEFAULT_ADDR
#define LG_MODBUS_SERVER_DEFAULT_ADDR 1
#endif

#ifndef LB_FILTER_FC_DEFAULT
#define LB_FILTER_FC_DEFAULT 10.0f
#endif
/* ============================================================================
 * global variables
 * ========================================================================= */
static LG_CONF_TypeDef_t conf;

static uint64_t ram_page_buffer[EEPROM_PAGE_SIZE / 8];

/* ============================================================================
 * Private function
 * ===========================================================================*/
static uint8_t lg_module_eeprom_write(uint32_t addr, uint8_t *buffer, uint16_t size);

static uint8_t lg_module_eeprom_read(uint32_t addr, uint8_t *buffer, uint16_t size);

static uint32_t BSP_CRC32_Calculate(const uint8_t *p_data, size_t length)
{
    uint32_t crc = 0xFFFFFFFFUL;
    size_t i, j;

    if (p_data == NULL || length == 0)
    {
        return 0;
    }

    for (i = 0; i < length; ++i)
    {
        crc ^= p_data[i];
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
/* ============================================================================
 * function definition
 * ========================================================================= */
/**
 * @brief  Inicializa el módulo (verifica alineación o integridad básica)
 * @retval 0: Éxito, 1: Error
 */
uint8_t lg_module_eeprom_init(void)
{
    uint32_t checksum = 0;
    /*load configuration*/
    lg_module_eeprom_read(0, (uint8_t *)&conf, sizeof(LG_CONF_TypeDef_t));

    /*checksum*/
    checksum = BSP_CRC32_Calculate((const uint8_t *)&conf, sizeof(LG_CONF_TypeDef_t) - 4);

    /*verify */

    if (checksum != conf.checksum)
    {
        /*write default config*/
        memset(&conf, 0, sizeof(LG_CONF_TypeDef_t));

        conf.address = LG_MODBUS_SERVER_DEFAULT_ADDR;
        conf.fc = LB_FILTER_FC_DEFAULT;
        conf.threshold = LB_THRESHOLD_DEFAULT;
        /*calculate checksum*/
        conf.checksum = BSP_CRC32_Calculate((const uint8_t *)&conf, sizeof(LG_CONF_TypeDef_t) - 4);

        /*Write data*/
        lg_module_eeprom_write(0, (uint8_t *)&conf, sizeof(LG_CONF_TypeDef_t));
    }

    return 0;
}

uint8_t lg_module_eeprom_conf_set(LG_CONF_TypeDef_t *in)
{
    /*Memory copy*/
    memcpy(&conf, in, sizeof(LG_CONF_TypeDef_t));
    /*calculate checksum*/
    conf.checksum = BSP_CRC32_Calculate((const uint8_t *)&conf, sizeof(LG_CONF_TypeDef_t) - 4);

    /*Write data*/
    lg_module_eeprom_write(0, (uint8_t *)&conf, sizeof(LG_CONF_TypeDef_t));

    return 0;
}

uint8_t lg_module_eeprom_conf_get(LG_CONF_TypeDef_t *out)
{
    memcpy(out, &conf, sizeof(LG_CONF_TypeDef_t));

    return 0;
}
/* ============================================================================
 * private function definition
 * ========================================================================= */
/**
 * @brief  Lee datos de la Flash emulada.
 * @param  addr: Offset relativo (0 a 2047) desde el inicio de la página emulada.
 * @param  buffer: Puntero donde guardar los datos.
 * @param  size: Cantidad de bytes a leer.
 * @retval 0: Éxito, 1: Error (fuera de rango)
 */
static uint8_t lg_module_eeprom_read(uint32_t addr, uint8_t *buffer, uint16_t size)
{
    // 1. Validación de límites
    if ((addr + size) > EEPROM_PAGE_SIZE)
    {
        return 1; // Error: Intento de leer fuera de la página reservada
    }

    // 2. Lectura directa (La flash es memory mapped)
    // Calculamos la dirección física absoluta
    uint32_t absolute_addr = EEPROM_START_ADDRESS + addr;

    // 3. Copiamos los datos
    memcpy(buffer, (void *)absolute_addr, size);

    return 0; // Éxito
}

/**
 * @brief  Escribe datos en la Flash emulada (Read-Modify-Write).
 * @param  addr: Offset relativo (0 a 2047).
 * @param  buffer: Datos a escribir.
 * @param  size: Cantidad de bytes.
 * @retval 0: Éxito, Other: Error HAL
 */
static uint8_t lg_module_eeprom_write(uint32_t addr, uint8_t *buffer, uint16_t size)
{
    HAL_StatusTypeDef status;
    uint32_t page_error = 0;
    FLASH_EraseInitTypeDef erase_init;

    // 1. Validación de límites
    if ((addr + size) > EEPROM_PAGE_SIZE)
    {
        return 1; // Error: Fuera de rango
    }

    // 2. Copiar TODA la página actual de Flash a RAM (Backup)
    // Usamos punteros para asegurar el copiado correcto al array de uint64_t
    memcpy(ram_page_buffer, (void *)EEPROM_START_ADDRESS, EEPROM_PAGE_SIZE);

    // 3. Modificar solo los bytes necesarios en el buffer de RAM
    // Casteamos a uint8_t* para movernos byte a byte en el buffer
    uint8_t *byte_ptr = (uint8_t *)ram_page_buffer;
    memcpy(&byte_ptr[addr], buffer, size);

    // 4. Desbloquear la Flash para escritura
    HAL_FLASH_Unlock();

    // 5. Borrar la página de Flash
    erase_init.TypeErase = FLASH_TYPEERASE_PAGES;
    erase_init.Banks = FLASH_BANK_1;
    erase_init.Page = 31; // Página 31 es la última en 64KB (0x0800F800)
    erase_init.NbPages = 1;

    status = HAL_FLASHEx_Erase(&erase_init, &page_error);
    if (status != HAL_OK)
    {
        HAL_FLASH_Lock();
        return 2; // Error al borrar
    }

    // 6. Escribir el buffer RAM actualizado de vuelta a la Flash
    // El STM32G0 requiere escritura de DoubleWord (64 bits)
    for (uint32_t i = 0; i < (EEPROM_PAGE_SIZE / 8); i++)
    {
        // Calculamos la dirección absoluta de destino
        uint32_t dest_addr = EEPROM_START_ADDRESS + (i * 8);

        // Obtenemos el dato de 64 bits del buffer
        uint64_t data = ram_page_buffer[i];

        status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, dest_addr, data);
        if (status != HAL_OK)
        {
            HAL_FLASH_Lock();
            return 3; // Error al escribir
        }
    }

    // 7. Bloquear la Flash
    HAL_FLASH_Lock();

    return 0; // Éxito
}
