#include "lg_adapter_storage.h"
#include "stm32g0xx_hal.h"
#include "leather_gauge_config.h"
#include <string.h>

/* ============================================================================
 * Private Macros & Constants
 * ========================================================================= */
#define EEPROM_START_ADDRESS ((uint32_t)0x0800F800)
#define EEPROM_PAGE_SIZE     ((uint16_t)0x0800) // 2KB
#define CRC32_POLYNOMIAL     0x04C11DB7UL

#ifndef LG_MODBUS_SERVER_DEFAULT_ADDR
#define LG_MODBUS_SERVER_DEFAULT_ADDR 1
#endif

#ifndef LB_FILTER_FC_DEFAULT
#define LB_FILTER_FC_DEFAULT 10.0f
#endif

#ifndef LB_THRESHOLD_DEFAULT
#define LB_THRESHOLD_DEFAULT 60
#endif

/* ============================================================================
 * Private Data Types
 * ========================================================================= */
typedef struct {
    lg_config_t config;
    uint32_t checksum;
} storage_layout_t;

/* ============================================================================
 * Private Variables
 * ========================================================================= */
static uint64_t ram_page_buffer[EEPROM_PAGE_SIZE / 8];

/* ============================================================================
 * Private Function Prototypes
 * ========================================================================= */
static lg_result_t storage_init(void);
static lg_result_t storage_load_config(lg_config_t *config);
static lg_result_t storage_save_config(const lg_config_t *config);
static lg_result_t storage_factory_reset(void);

static uint32_t calculate_crc32(const uint8_t *p_data, size_t length);
static lg_result_t flash_read(uint32_t offset, void *buffer, uint16_t size);
static lg_result_t flash_write(uint32_t offset, const void *buffer, uint16_t size);

/* ============================================================================
 * Interface Definition
 * ========================================================================= */
static const lg_i_storage_t interface = {
    .init = storage_init,
    .load_config = storage_load_config,
    .save_config = storage_save_config,
    .factory_reset = storage_factory_reset
};

/* ============================================================================
 * Public Functions
 * ========================================================================= */
const lg_i_storage_t* lg_adapter_storage_get_interface(void) {
    return &interface;
}

/* ============================================================================
 * Private Functions (Implementation)
 * ========================================================================= */

static lg_result_t storage_init(void) {
    storage_layout_t stored_data;
    
    // Read Current Data
    if (flash_read(0, &stored_data, sizeof(storage_layout_t)) != LG_OK) {
        return LG_ERROR;
    }

    // Verify Checksum
    uint32_t calc_crc = calculate_crc32((const uint8_t *)&stored_data.config, sizeof(lg_config_t));
    
    if (calc_crc != stored_data.checksum) {
        // Invalid or empty, perform factory reset
        return storage_factory_reset();
    }
    
    return LG_OK;
}

static lg_result_t storage_load_config(lg_config_t *config) {
    storage_layout_t stored_data;
    if (flash_read(0, &stored_data, sizeof(storage_layout_t)) != LG_OK) {
        return LG_ERROR;
    }
    memcpy(config, &stored_data.config, sizeof(lg_config_t));
    return LG_OK;
}

static lg_result_t storage_save_config(const lg_config_t *config) {
    storage_layout_t data_to_store;
    memcpy(&data_to_store.config, config, sizeof(lg_config_t));
    data_to_store.checksum = calculate_crc32((const uint8_t *)config, sizeof(lg_config_t));
    
    return flash_write(0, &data_to_store, sizeof(storage_layout_t));
}

static lg_result_t storage_factory_reset(void) {
    lg_config_t default_conf = {0};
    
    default_conf.address = LG_MODBUS_SERVER_DEFAULT_ADDR;
    default_conf.baudrate = 115200; // Default baud
    default_conf.fc = LB_FILTER_FC_DEFAULT;
    default_conf.threshold = LB_THRESHOLD_DEFAULT;
    
    return storage_save_config(&default_conf);
}

/* ============================================================================
 * Helper Functions
 * ========================================================================= */

static uint32_t calculate_crc32(const uint8_t *p_data, size_t length) {
    uint32_t crc = 0xFFFFFFFFUL;
    size_t i, j;

    if (p_data == NULL || length == 0) return 0;

    for (i = 0; i < length; ++i) {
        crc ^= p_data[i];
        for (j = 0; j < 8; ++j) {
            if (crc & 1) crc = (crc >> 1) ^ CRC32_POLYNOMIAL;
            else crc >>= 1;
        }
    }
    return crc ^ 0xFFFFFFFFUL;
}

static lg_result_t flash_read(uint32_t offset, void *buffer, uint16_t size) {
    if ((offset + size) > EEPROM_PAGE_SIZE) return LG_INVALID_PARAM;
    memcpy(buffer, (void *)(EEPROM_START_ADDRESS + offset), size);
    return LG_OK;
}

static lg_result_t flash_write(uint32_t offset, const void *buffer, uint16_t size) {
    if ((offset + size) > EEPROM_PAGE_SIZE) return LG_INVALID_PARAM;

    // 1. Read entire page to RAM
    memcpy(ram_page_buffer, (void *)EEPROM_START_ADDRESS, EEPROM_PAGE_SIZE);

    // 2. Modify RAM buffer
    uint8_t *byte_ptr = (uint8_t *)ram_page_buffer;
    memcpy(&byte_ptr[offset], buffer, size);

    // 3. Unlock Flash
    if (HAL_FLASH_Unlock() != HAL_OK) return LG_ERROR;

    // 4. Erase Page
    FLASH_EraseInitTypeDef erase_init;
    uint32_t page_error = 0;
    erase_init.TypeErase = FLASH_TYPEERASE_PAGES;
    erase_init.Banks = FLASH_BANK_1;
    erase_init.Page = 31; // Last Page
    erase_init.NbPages = 1;

    if (HAL_FLASHEx_Erase(&erase_init, &page_error) != HAL_OK) {
        HAL_FLASH_Lock();
        return LG_ERROR;
    }

    // 5. Program Page (Double Word)
    lg_result_t ret = LG_OK;
    for (uint32_t i = 0; i < (EEPROM_PAGE_SIZE / 8); i++) {
        uint32_t dest_addr = EEPROM_START_ADDRESS + (i * 8);
        uint64_t data = ram_page_buffer[i];
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, dest_addr, data) != HAL_OK) {
            ret = LG_ERROR;
            break;
        }
    }

    HAL_FLASH_Lock();
    return ret;
}
