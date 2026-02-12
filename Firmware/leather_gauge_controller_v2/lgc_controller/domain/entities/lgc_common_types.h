/**
 * @file    lgc_common_types.h
 * @brief   Common types and definitions for Leather Gauge Controller
 * @author  Clean Architecture Refactor Team
 * @date    2026-02-12
 * @version 1.0.0
 *
 * @details This file defines fundamental types used across all layers.
 *          NO hardware dependencies allowed (Pure C99/C11).
 *
 * @note    This file must NEVER include:
 *          - stm32f4xx_hal.h (or any HAL headers)
 *          - tx_api.h (ThreadX in domain layer violates DIP)
 *
 * Copyright (c) 2026 Leather Gauge Controller Team
 */

#ifndef LGC_COMMON_TYPES_H
#define LGC_COMMON_TYPES_H

#ifdef __cplusplus
extern "C"
{
#endif

/* ============================= Includes ============================= */
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* ============================= Constants ============================ */
/**
 * @defgroup LGC_Constants System Constants
 * @{
 */

/** Maximum number of sensors in the array */
#define LGC_SENSOR_COUNT 11U

/** Number of photocells per sensor */
#define LGC_PHOTOCELLS_PER_SENSOR 10U

/** Total photocells in the system */
#define LGC_TOTAL_PHOTOCELLS (LGC_SENSOR_COUNT * LGC_PHOTOCELLS_PER_SENSOR)

/** Maximum pieces per batch */
#define LGC_MAX_PIECES_PER_BATCH 50U

/** Maximum batches in system */
#define LGC_MAX_BATCH_COUNT 200U

/** Total maximum pieces across all batches */
#define LGC_MAX_TOTAL_PIECES 300U

/** Encoder displacement per pulse (mm) - REAL VALUE FROM LEGACY */
#define LGC_ENCODER_STEP_MM 5.5f

/** Encoder pulses accumulated before measurement - REAL VALUE FROM LEGACY */
#define LGC_ENCODER_PULSES_PER_FLAG 5U

/** Photocell spacing (mm) - REAL VALUE FROM LEGACY */
#define LGC_PHOTOCELL_SPACING_MM 20.0f

/** Hysteresis for leather detection (consecutive empty slices) */
#define LGC_LEATHER_END_HYSTERESIS 3U

/** Maximum string length for configuration fields */
#define LGC_CONFIG_STRING_MAX_LEN 12U

    /** @} */

    /* ============================= Result Types ========================= */
    /**
     * @brief Result codes for all operations
     *
     * @note  Always check return values. Non-ERR_OK indicates failure.
     */
    typedef enum
    {
        ERR_OK = 0x00,              /**< Operation successful */
        ERR_ERROR = 0x01,           /**< Generic error */
        ERR_NULL_POINTER = 0x02,    /**< NULL pointer passed */
        ERR_INVALID_PARAM = 0x03,   /**< Invalid parameter value */
        ERR_TIMEOUT = 0x04,         /**< Operation timed out */
        ERR_BUSY = 0x05,            /**< Resource busy */
        ERR_NOT_INITIALIZED = 0x06, /**< Module not initialized */
        ERR_OUT_OF_BOUNDS = 0x07,   /**< Index/value out of bounds */
        ERR_CRC_MISMATCH = 0x08,    /**< CRC validation failed */
        ERR_HARDWARE_FAULT = 0x09,  /**< Hardware error detected */
        ERR_NO_DATA = 0x0A,         /**< No data available */
        ERR_BUFFER_FULL = 0x0B,     /**< Buffer overflow */
    } Result_t;

    /* ============================= State Types ========================== */
    /**
     * @brief System operational states
     */
    typedef enum
    {
        LGC_STATE_IDLE = 0x00,        /**< Waiting for encoder pulse */
        LGC_STATE_MEASURING = 0x01,   /**< Actively measuring leather */
        LGC_STATE_PAUSED = 0x02,      /**< Measurement paused by user */
        LGC_STATE_CALIBRATING = 0x03, /**< Calibration mode */
        LGC_STATE_ERROR = 0xFF,       /**< Error state */
    } LgcSystemState_t;

    /**
     * @brief Measurement units
     */
    typedef enum
    {
        LGC_UNIT_DM2 = 0, /**< Square decimeters (dm²) */
        LGC_UNIT_M2 = 1,  /**< Square meters (m²) */
        LGC_UNIT_FT2 = 2, /**< Square feet (ft²) */
    } LgcUnit_t;

    /* ============================= Time Types =========================== */
    /**
     * @brief Date and time structure
     *
     * @note  Used for batch timestamping and RTC operations
     */
    typedef struct
    {
        uint16_t year;  /**< Year (2000-2099) */
        uint8_t month;  /**< Month (1-12) */
        uint8_t day;    /**< Day (1-31) */
        uint8_t hour;   /**< Hour (0-23) */
        uint8_t minute; /**< Minute (0-59) */
        uint8_t second; /**< Second (0-59) */
    } LgcDateTime_t;

/* ============================= Validation Macros ==================== */
/**
 * @defgroup LGC_Validation Parameter Validation Macros
 * @{
 */

/**
 * @brief Validate pointer is not NULL
 * @param ptr Pointer to check
 * @return ERR_NULL_POINTER if NULL, otherwise continues
 */
#define LGC_VALIDATE_PTR(ptr)        \
    do                               \
    {                                \
        if ((ptr) == NULL)           \
        {                            \
            return ERR_NULL_POINTER; \
        }                            \
    } while (0)

/**
 * @brief Validate numeric range
 * @param val   Value to check
 * @param min   Minimum allowed value (inclusive)
 * @param max   Maximum allowed value (inclusive)
 * @return ERR_OUT_OF_BOUNDS if out of range
 */
#define LGC_VALIDATE_RANGE(val, min, max)   \
    do                                      \
    {                                       \
        if ((val) < (min) || (val) > (max)) \
        {                                   \
            return ERR_OUT_OF_BOUNDS;       \
        }                                   \
    } while (0)

    /** @} */

#ifdef __cplusplus
}
#endif

#endif /* LGC_COMMON_TYPES_H */
