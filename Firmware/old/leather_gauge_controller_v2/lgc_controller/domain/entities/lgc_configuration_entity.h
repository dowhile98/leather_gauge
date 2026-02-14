/**
 * @file    lgc_configuration_entity.h
 * @brief   Configuration entity definition
 * @author  Clean Architecture Refactor Team
 * @date    2026-02-12
 * @version 1.0.0
 *
 * @details Defines system configuration structures persisted to EEPROM.
 *
 * @note    DOMAIN LAYER - Pure data structures.
 *
 * Copyright (c) 2026 Leather Gauge Controller Team
 */

#ifndef LGC_CONFIGURATION_ENTITY_H
#define LGC_CONFIGURATION_ENTITY_H

#ifdef __cplusplus
extern "C"
{
#endif

/* ============================= Includes ============================= */
#include "lgc_common_types.h"

    /* ============================= System Configuration ================= */
    /**
     * @brief System configuration (persisted to EEPROM)
     *
     * @details This structure is saved/loaded from EEPROM with CRC32 validation.
     */
    typedef struct
    {
        /* Client Information */
        char client_name[LGC_CONFIG_STRING_MAX_LEN + 1]; /**< Client name (null-terminated) */
        char color[LGC_CONFIG_STRING_MAX_LEN + 1];       /**< Leather color */
        char leather_id[LGC_CONFIG_STRING_MAX_LEN + 1];  /**< Leather ID/type */

        /* Measurement Parameters */
        LgcUnit_t unit;            /**< Display unit (dm², m², ft²) */
        float conversion_factor;   /**< Custom unit conversion factor */
        uint8_t leather_threshold; /**< Min active bits to detect leather (2-5) */
        uint8_t hysteresis;        /**< Consecutive empty slices to end piece (3) */

        /* Batch Settings */
        uint32_t batch_number;         /**< Current batch number (auto-increment) */
        uint32_t max_pieces_per_batch; /**< Max pieces before auto-finalize */

        /* Calibration */
        uint16_t zero_offset[LGC_SENSOR_COUNT]; /**< Sensor zero offsets */
        bool is_calibrated;                     /**< Calibration valid flag */
        LgcDateTime_t calibration_date;         /**< Last calibration timestamp */

        /* Data Integrity */
        uint32_t crc32; /**< CRC32 checksum (computed, not stored directly) */
        bool is_valid;  /**< Configuration validity flag */
    } LgcSystemConfig_t;

/* ============================= Defaults ============================= */
/**
 * @brief Default configuration values
 */
#define LGC_DEFAULT_CLIENT_NAME "CLIENTE"
#define LGC_DEFAULT_COLOR "SIN COLOR"
#define LGC_DEFAULT_LEATHER_ID "SIN ID"
#define LGC_DEFAULT_UNIT LGC_UNIT_DM2
#define LGC_DEFAULT_CONVERSION_FACTOR 1.0f
#define LGC_DEFAULT_THRESHOLD 2U
#define LGC_DEFAULT_HYSTERESIS LGC_LEATHER_END_HYSTERESIS
#define LGC_DEFAULT_MAX_PIECES 100U

    /* ============================= Helper Functions ===================== */
    /**
     * @brief Initialize configuration with default values
     *
     * @param[out] config Pointer to configuration structure
     *
     * @pre  config must not be NULL
     * @post config populated with defaults, is_valid = true
     */
    static inline void LgcSystemConfig_InitDefaults(LgcSystemConfig_t *config)
    {
        if (config == NULL)
        {
            return;
        }

        // Client info
        for (uint8_t i = 0; i < LGC_CONFIG_STRING_MAX_LEN; i++)
        {
            config->client_name[i] = LGC_DEFAULT_CLIENT_NAME[i];
            config->color[i] = LGC_DEFAULT_COLOR[i];
            config->leather_id[i] = LGC_DEFAULT_LEATHER_ID[i];
            if (LGC_DEFAULT_CLIENT_NAME[i] == '\0' &&
                LGC_DEFAULT_COLOR[i] == '\0' &&
                LGC_DEFAULT_LEATHER_ID[i] == '\0')
            {
                break;
            }
        }
        config->client_name[LGC_CONFIG_STRING_MAX_LEN] = '\0';
        config->color[LGC_CONFIG_STRING_MAX_LEN] = '\0';
        config->leather_id[LGC_CONFIG_STRING_MAX_LEN] = '\0';

        // Measurement params
        config->unit = LGC_DEFAULT_UNIT;
        config->conversion_factor = LGC_DEFAULT_CONVERSION_FACTOR;
        config->leather_threshold = LGC_DEFAULT_THRESHOLD;
        config->hysteresis = LGC_DEFAULT_HYSTERESIS;

        // Batch settings
        config->batch_number = 1;
        config->max_pieces_per_batch = LGC_DEFAULT_MAX_PIECES;

        // Calibration
        for (uint8_t i = 0; i < LGC_SENSOR_COUNT; i++)
        {
            config->zero_offset[i] = 0;
        }
        config->is_calibrated = false;

        // Integrity
        config->crc32 = 0;
        config->is_valid = true;
    }

    /**
     * @brief Validate configuration parameters
     *
     * @param[in] config Pointer to configuration structure
     * @return true if configuration is valid, false otherwise
     *
     * @note  Does NOT validate CRC (that's IStorage's responsibility)
     */
    static inline bool LgcSystemConfig_Validate(const LgcSystemConfig_t *config)
    {
        if (config == NULL)
        {
            return false;
        }

        // Check enum ranges
        if (config->unit > LGC_UNIT_FT2)
        {
            return false;
        }

        // Check thresholds
        if (config->leather_threshold > LGC_PHOTOCELLS_PER_SENSOR ||
            config->hysteresis == 0 ||
            config->hysteresis > 10)
        {
            return false;
        }

        // Check batch settings
        if (config->max_pieces_per_batch == 0 ||
            config->max_pieces_per_batch > LGC_MAX_PIECES_PER_BATCH)
        {
            return false;
        }

        return config->is_valid;
    }

#ifdef __cplusplus
}
#endif

#endif /* LGC_CONFIGURATION_ENTITY_H */
