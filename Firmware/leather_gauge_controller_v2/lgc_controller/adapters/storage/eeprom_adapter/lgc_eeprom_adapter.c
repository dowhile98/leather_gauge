/**
 * @file    lgc_eeprom_adapter.c
 * @brief   EEPROM Storage Adapter - Implementation
 * @author  Clean Architecture Refactor Team (TDD)
 * @date    2026-02-12
 * @version 1.0.0
 *
 * @details I2C AT24Cxx EEPROM implementation with:
 *          - CRC32 (IEEE 802.3) validation
 *          - ThreadX (TX_MUTEX) thread safety
 *          - Configuration persistence
 *
 * Copyright (c) 2026 Leather Gauge Controller Team
 */

/* ============================= Includes ============================= */
#include "lgc_eeprom_adapter.h"
#include "tx_api.h" /* ThreadX for mutex - allowed in adapters */
#include <string.h>

/* TODO: Move at24cxx driver to Third_Party/ */
#include "driver_at24cxx.h" /* AT24Cxx EEPROM driver */

/* ============================= Configuration ======================== */
#define EEPROM_CHIP_TYPE AT24C256
#define EEPROM_ADDR_PIN AT24CXX_ADDRESS_A000
#define EEPROM_CONFIG_ADDRESS 0x0000      /**< Config start address */
#define EEPROM_BATCH_START_ADDRESS 0x0100 /**< Batch data start (256 bytes offset) */
#define CRC32_POLYNOMIAL 0x04C11DB7UL     /**< IEEE 802.3 */

/* ============================= Private Types ======================== */
/**
 * @brief EEPROM adapter context (opaque to domain)
 */
typedef struct
{
    /* HAL/Middleware handles */
    at24cxx_handle_t eeprom_handle; /**< AT24Cxx driver handle */
    TX_MUTEX mutex;                 /**< ThreadX mutex for thread safety */

    /* Configuration */
    LgcStorageConfig_t config;

    /* State */
    bool is_initialized;
    uint32_t write_count; /**< Wear leveling tracking (optional) */

} EepromAdapterContext_t;

/* ============================= Private Variables ==================== */
static EepromAdapterContext_t s_eeprom_ctx = {0};

/* ============================= Private Function Prototypes ========== */
static Result_t eeprom_init(void *ctx, const LgcStorageConfig_t *config);
static Result_t eeprom_save_config(void *ctx, const LgcSystemConfig_t *config);
static Result_t eeprom_load_config(void *ctx, LgcSystemConfig_t *out_config);
static Result_t eeprom_save_batch(void *ctx, const LgcBatch_t *batch);
static Result_t eeprom_load_batch(void *ctx, uint32_t batch_number, LgcBatch_t *out_batch);
static Result_t eeprom_erase_all(void *ctx);
static Result_t eeprom_deinit(void *ctx);

/* Private helper functions */
static uint32_t compute_crc32(const uint8_t *data, size_t length);
static Result_t eeprom_write_bytes(uint16_t address, const uint8_t *data, uint16_t length);
static Result_t eeprom_read_bytes(uint16_t address, uint8_t *data, uint16_t length);

/* ============================= Interface Definition ================= */
static const ILgcStorage_t s_storage_interface = {
    .context = &s_eeprom_ctx,
    .init = eeprom_init,
    .save_config = eeprom_save_config,
    .load_config = eeprom_load_config,
    .save_batch = eeprom_save_batch,
    .load_batch = eeprom_load_batch,
    .erase_all = eeprom_erase_all,
    .deinit = eeprom_deinit};

/* ============================= Public API =========================== */

const ILgcStorage_t *LgcEepromAdapter_GetInterface(void)
{
    return &s_storage_interface;
}

/* ============================= Private Functions ==================== */

/**
 * @brief Initialize EEPROM adapter
 */
static Result_t eeprom_init(void *ctx, const LgcStorageConfig_t *config)
{
    if (ctx == NULL || config == NULL)
    {
        return ERR_NULL_POINTER;
    }

    EepromAdapterContext_t *eeprom = (EepromAdapterContext_t *)ctx;

    if (eeprom->is_initialized)
    {
        return ERR_BUSY;
    }

    /* Copy configuration */
    memcpy(&eeprom->config, config, sizeof(LgcStorageConfig_t));

    /* Initialize ThreadX mutex */
    UINT tx_res = tx_mutex_create(&eeprom->mutex, "EEPROM_MUTEX", TX_NO_INHERIT);
    if (tx_res != TX_SUCCESS)
    {
        return ERR_ERROR;
    }

    /* Initialize AT24Cxx driver (link functions) */
    DRIVER_AT24CXX_LINK_INIT(&eeprom->eeprom_handle, at24cxx_handle_t);
    DRIVER_AT24CXX_LINK_IIC_INIT(&eeprom->eeprom_handle, at24cxx_interface_iic_init);
    DRIVER_AT24CXX_LINK_IIC_DEINIT(&eeprom->eeprom_handle, at24cxx_interface_iic_deinit);
    DRIVER_AT24CXX_LINK_IIC_READ(&eeprom->eeprom_handle, at24cxx_interface_iic_read);
    DRIVER_AT24CXX_LINK_IIC_WRITE(&eeprom->eeprom_handle, at24cxx_interface_iic_write);
    DRIVER_AT24CXX_LINK_IIC_READ_ADDRESS16(&eeprom->eeprom_handle, at24cxx_interface_iic_read_address16);
    DRIVER_AT24CXX_LINK_IIC_WRITE_ADDRESS16(&eeprom->eeprom_handle, at24cxx_interface_iic_write_address16);
    DRIVER_AT24CXX_LINK_DELAY_MS(&eeprom->eeprom_handle, at24cxx_interface_delay_ms);
    DRIVER_AT24CXX_LINK_DEBUG_PRINT(&eeprom->eeprom_handle, at24cxx_interface_debug_print);

    /* Set chip type and address */
    at24cxx_set_type(&eeprom->eeprom_handle, EEPROM_CHIP_TYPE);
    at24cxx_set_addr_pin(&eeprom->eeprom_handle, EEPROM_ADDR_PIN);

    /* Initialize driver */
    if (at24cxx_init(&eeprom->eeprom_handle) != 0)
    {
        tx_mutex_delete(&eeprom->mutex);
        return ERR_HARDWARE_FAULT;
    }

    eeprom->is_initialized = true;
    eeprom->write_count = 0;

    return ERR_OK;
}

/**
 * @brief Save system configuration to EEPROM with CRC32
 */
static Result_t eeprom_save_config(void *ctx, const LgcSystemConfig_t *config)
{
    if (ctx == NULL || config == NULL)
    {
        return ERR_NULL_POINTER;
    }

    EepromAdapterContext_t *eeprom = (EepromAdapterContext_t *)ctx;

    if (!eeprom->is_initialized)
    {
        return ERR_NOT_INITIALIZED;
    }

    /* Acquire mutex (thread-safe) */
    UINT tx_res = tx_mutex_get(&eeprom->mutex, TX_WAIT_FOREVER);
    if (tx_res != TX_SUCCESS)
    {
        return ERR_TIMEOUT;
    }

    /* Compute CRC32 over configuration (excluding CRC field itself) */
    uint32_t crc = compute_crc32(
        (const uint8_t *)config,
        sizeof(LgcSystemConfig_t) - sizeof(uint32_t)); /* Exclude crc field */

    /* Create temporary buffer with CRC appended */
    uint8_t buffer[sizeof(LgcSystemConfig_t)];
    memcpy(buffer, config, sizeof(LgcSystemConfig_t) - sizeof(uint32_t));
    memcpy(buffer + sizeof(LgcSystemConfig_t) - sizeof(uint32_t), &crc, sizeof(crc));

    /* Write to EEPROM */
    Result_t res = eeprom_write_bytes(EEPROM_CONFIG_ADDRESS, buffer, sizeof(buffer));

    /* Release mutex */
    tx_mutex_put(&eeprom->mutex);

    if (res == ERR_OK)
    {
        eeprom->write_count++;
    }

    return res;
}

/**
 * @brief Load system configuration from EEPROM with CRC32 validation
 */
static Result_t eeprom_load_config(void *ctx, LgcSystemConfig_t *out_config)
{
    if (ctx == NULL || out_config == NULL)
    {
        return ERR_NULL_POINTER;
    }

    EepromAdapterContext_t *eeprom = (EepromAdapterContext_t *)ctx;

    if (!eeprom->is_initialized)
    {
        return ERR_NOT_INITIALIZED;
    }

    /* Acquire mutex */
    UINT tx_res = tx_mutex_get(&eeprom->mutex, TX_WAIT_FOREVER);
    if (tx_res != TX_SUCCESS)
    {
        return ERR_TIMEOUT;
    }

    /* Read from EEPROM */
    uint8_t buffer[sizeof(LgcSystemConfig_t)];
    Result_t res = eeprom_read_bytes(EEPROM_CONFIG_ADDRESS, buffer, sizeof(buffer));

    if (res != ERR_OK)
    {
        tx_mutex_put(&eeprom->mutex);
        return res;
    }

    /* Extract stored CRC */
    uint32_t stored_crc;
    memcpy(&stored_crc, buffer + sizeof(LgcSystemConfig_t) - sizeof(uint32_t), sizeof(stored_crc));

    /* Compute CRC over data */
    uint32_t computed_crc = compute_crc32(buffer, sizeof(LgcSystemConfig_t) - sizeof(uint32_t));

    /* Validate CRC */
    if (eeprom->config.enable_crc && (stored_crc != computed_crc))
    {
        tx_mutex_put(&eeprom->mutex);
        return ERR_CRC_MISMATCH;
    }

    /* Copy valid data to output */
    memcpy(out_config, buffer, sizeof(LgcSystemConfig_t));

    tx_mutex_put(&eeprom->mutex);
    return ERR_OK;
}

/**
 * @brief Save batch data (stub - full implementation TBD)
 */
static Result_t eeprom_save_batch(void *ctx, const LgcBatch_t *batch)
{
    /* TODO: Implement batch persistence
     * - Calculate required EEPROM space
     * - Write batch header + measurements
     * - Add CRC32 for validation
     */
    (void)ctx;
    (void)batch;
    return ERR_NOT_INITIALIZED; /* Stub */
}

/**
 * @brief Load batch data (stub - full implementation TBD)
 */
static Result_t eeprom_load_batch(void *ctx, uint32_t batch_number, LgcBatch_t *out_batch)
{
    /* TODO: Implement batch loading
     * - Read batch header
     * - Validate CRC32
     * - Load all measurements
     */
    (void)ctx;
    (void)batch_number;
    (void)out_batch;
    return ERR_NOT_INITIALIZED; /* Stub */
}

/**
 * @brief Erase all EEPROM data
 */
static Result_t eeprom_erase_all(void *ctx)
{
    if (ctx == NULL)
    {
        return ERR_NULL_POINTER;
    }

    EepromAdapterContext_t *eeprom = (EepromAdapterContext_t *)ctx;

    if (!eeprom->is_initialized)
    {
        return ERR_NOT_INITIALIZED;
    }

    /* Write zeros to config area (simple erase) */
    uint8_t zero_buffer[sizeof(LgcSystemConfig_t)] = {0};
    return eeprom_write_bytes(EEPROM_CONFIG_ADDRESS, zero_buffer, sizeof(zero_buffer));
}

/**
 * @brief Deinitialize EEPROM adapter
 */
static Result_t eeprom_deinit(void *ctx)
{
    if (ctx == NULL)
    {
        return ERR_NULL_POINTER;
    }

    EepromAdapterContext_t *eeprom = (EepromAdapterContext_t *)ctx;

    /* Deinit driver */
    at24cxx_deinit(&eeprom->eeprom_handle);

    /* Delete mutex */
    tx_mutex_delete(&eeprom->mutex);

    eeprom->is_initialized = false;

    return ERR_OK;
}

/* ============================= Private Helpers ====================== */

/**
 * @brief Compute CRC32 (IEEE 802.3)
 */
static uint32_t compute_crc32(const uint8_t *data, size_t length)
{
    uint32_t crc = 0xFFFFFFFFUL;

    if (data == NULL || length == 0)
    {
        return 0;
    }

    for (size_t i = 0; i < length; i++)
    {
        crc ^= data[i];
        for (uint8_t bit = 0; bit < 8; bit++)
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

    return crc ^ 0xFFFFFFFFUL;
}

/**
 * @brief Write bytes to EEPROM
 */
static Result_t eeprom_write_bytes(uint16_t address, const uint8_t *data, uint16_t length)
{
    EepromAdapterContext_t *eeprom = &s_eeprom_ctx;

    /* Page writes for AT24Cxx (64-byte pages) */
    uint16_t bytes_written = 0;
    while (bytes_written < length)
    {
        uint16_t page_offset = (address + bytes_written) % 64;
        uint16_t bytes_to_write = (64 - page_offset) < (length - bytes_written)
                                      ? (64 - page_offset)
                                      : (length - bytes_written);

        if (at24cxx_write(&eeprom->eeprom_handle,
                          address + bytes_written,
                          (uint8_t *)data + bytes_written,
                          bytes_to_write) != 0)
        {
            return ERR_HARDWARE_FAULT;
        }

        bytes_written += bytes_to_write;

        /* Wait for write cycle (5ms typical) */
        tx_thread_sleep(TX_MS_TO_TICKS(10));
    }

    return ERR_OK;
}

/**
 * @brief Read bytes from EEPROM
 */
static Result_t eeprom_read_bytes(uint16_t address, uint8_t *data, uint16_t length)
{
    EepromAdapterContext_t *eeprom = &s_eeprom_ctx;

    if (at24cxx_read(&eeprom->eeprom_handle, address, data, length) != 0)
    {
        return ERR_HARDWARE_FAULT;
    }

    return ERR_OK;
}
