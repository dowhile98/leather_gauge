/**
 * @file    lgc_i_storage.h
 * @brief   Storage Interface (Port)
 * @author  Clean Architecture Refactor Team
 * @date    2026-02-12
 * @version 1.0.0
 *
 * @details Interface for persistent storage operations (EEPROM, Flash, etc.)
 *          Handles configuration and batch data persistence with CRC validation.
 *
 * @note    INTERFACE LAYER (Port) - Implementation in adapters/storage/eeprom_adapter/
 *
 * Copyright (c) 2026 Leather Gauge Controller Team
 */

#ifndef LGC_I_STORAGE_H
#define LGC_I_STORAGE_H

#ifdef __cplusplus
extern "C"
{
#endif

/* ============================= Includes ============================= */
#include "../entities/lgc_common_types.h"
#include "../entities/lgc_configuration_entity.h"
#include "../entities/lgc_measurement_entity.h"

    /* ============================= Configuration ======================== */
    /**
     * @brief Storage configuration
     */
    typedef struct
    {
        uint32_t timeout_ms; /**< Operation timeout (ms) */
        bool enable_crc;     /**< Enable CRC32 validation */
        bool auto_retry;     /**< Retry on failure */
    } LgcStorageConfig_t;

    /* ============================= Interface ============================ */
    /**
     * @brief Storage Interface (V-Table)
     *
     * @details Abstracts persistent storage (EEPROM I2C, Flash, SD card, etc.)
     */
    typedef struct ILgcStorage_t
    {
        /**
         * @brief Opaque pointer to implementation context
         */
        void *context;

        /**
         * @brief Initialize storage
         *
         * @param[in] ctx    Implementation context
         * @param[in] config Configuration parameters
         * @return ERR_OK on success
         */
        Result_t (*init)(void *ctx, const LgcStorageConfig_t *config);

        /**
         * @brief Save system configuration
         *
         * @param[in] ctx    Implementation context
         * @param[in] config Configuration to save
         * @return ERR_OK on success, error code otherwise
         *
         * @pre  ctx and config must not be NULL
         * @pre  config must be valid (LgcSystemConfig_Validate() == true)
         * @post Configuration saved with CRC32
         *
         * @note  Blocking operation (~50ms for EEPROM)
         * @note  CRC32 computed and saved automatically
         */
        Result_t (*save_config)(void *ctx, const LgcSystemConfig_t *config);

        /**
         * @brief Load system configuration
         *
         * @param[in]  ctx        Implementation context
         * @param[out] out_config Loaded configuration
         * @return ERR_OK on success, ERR_CRC_MISMATCH if CRC invalid
         *
         * @pre  ctx and out_config must not be NULL
         * @post If ERR_OK: out_config populated with valid data
         * @post If ERR_CRC_MISMATCH: out_config may be corrupted, use defaults
         *
         * @note  Blocking operation (~20ms for EEPROM)
         */
        Result_t (*load_config)(void *ctx, LgcSystemConfig_t *out_config);

        /**
         * @brief Save batch data (for export/reporting)
         *
         * @param[in] ctx   Implementation context
         * @param[in] batch Batch to save
         * @return ERR_OK on success
         *
         * @pre  ctx and batch must not be NULL
         * @pre  batch must be finalized
         * @post Batch saved to persistent storage
         *
         * @note  May take several seconds for large batches
         */
        Result_t (*save_batch)(void *ctx, const LgcBatch_t *batch);

        /**
         * @brief Load batch data by batch number
         *
         * @param[in]  ctx         Implementation context
         * @param[in]  batch_number Batch number to load
         * @param[out] out_batch   Loaded batch
         * @return ERR_OK on success, ERR_NO_DATA if not found
         *
         * @pre  ctx and out_batch must not be NULL
         * @post If ERR_OK: out_batch populated
         */
        Result_t (*load_batch)(void *ctx, uint32_t batch_number, LgcBatch_t *out_batch);

        /**
         * @brief Erase all stored data (factory reset)
         *
         * @param[in] ctx Implementation context
         * @return ERR_OK on success
         *
         * @warning Irreversible operation - all data lost!
         */
        Result_t (*erase_all)(void *ctx);

        /**
         * @brief Deinitialize storage
         *
         * @param[in] ctx Implementation context
         * @return ERR_OK on success
         */
        Result_t (*deinit)(void *ctx);

    } ILgcStorage_t;

/* ============================= Helper Macros ======================== */
#define LGC_STORAGE_CALL(storage, method, ...) \
    (((storage) != NULL && (storage)->method != NULL) ? (storage)->method((storage)->context, ##__VA_ARGS__) : ERR_NULL_POINTER)

#ifdef __cplusplus
}
#endif

#endif /* LGC_I_STORAGE_H */
