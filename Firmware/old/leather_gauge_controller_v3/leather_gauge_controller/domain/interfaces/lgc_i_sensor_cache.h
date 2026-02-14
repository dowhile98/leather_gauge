/**
 * @file lgc_i_sensor_cache.h
 * @brief Interface for asynchronous sensor data cache access
 *
 * This interface decouples the measurement algorithm from the Modbus
 * communication layer, allowing non-blocking sensor data reads.
 *
 * @note Part of Clean Architecture Domain Layer - NO HAL dependencies allowed
 *
 * @date Created: Feb 13, 2026
 * @author GitHub Copilot
 */

#ifndef DOMAIN_INTERFACES_LGC_I_SENSOR_CACHE_H_
#define DOMAIN_INTERFACES_LGC_I_SENSOR_CACHE_H_

/* ============================================================================
 * INCLUDES
 * ============================================================================ */
#include <stdint.h>
#include <stdbool.h>
#include "error.h"

/* ============================================================================
 * CONFIGURATION
 * ============================================================================ */
#ifndef LGC_SENSOR_NUMBER
#define LGC_SENSOR_NUMBER 11
#endif

/* ============================================================================
 * TYPE DEFINITIONS
 * ============================================================================ */

/**
 * @brief Sensor health status flags
 */
typedef enum
{
    LGC_SENSOR_HEALTHY = 0x00,   /**< Sensor responding normally */
    LGC_SENSOR_TIMEOUT = 0x01,   /**< Sensor communication timeout */
    LGC_SENSOR_CRC_ERROR = 0x02, /**< CRC validation failed */
    LGC_SENSOR_OFFLINE = 0x04,   /**< Sensor not responding */
} LgcSensorStatus_t;

/**
 * @brief Individual sensor data structure
 */
typedef struct
{
    uint16_t value;           /**< Raw sensor value (10-bit bitmask) */
    LgcSensorStatus_t status; /**< Sensor health status */
    uint32_t timestamp_ms;    /**< Last valid read timestamp */
} LgcSensorData_t;

/**
 * @brief Complete sensor array snapshot
 */
typedef struct
{
    LgcSensorData_t sensors[LGC_SENSOR_NUMBER]; /**< All sensor data */
    uint16_t status_bitmask;                    /**< Combined status (bit N = sensor N fault) */
    uint32_t read_cycle_ms;                     /**< Total read cycle time */
    uint32_t sequence_number;                   /**< Monotonic update counter */
} LgcSensorArray_t;

/**
 * @brief Sensor Cache Interface (V-Table Pattern)
 *
 * Provides thread-safe access to cached sensor data without blocking
 * on Modbus communication.
 */
typedef struct ILgcSensorCache_t
{
    void *context; /**< Opaque pointer to implementation */

    /**
     * @brief Get latest sensor array snapshot (non-blocking)
     *
     * @param[in]  ctx        Implementation context
     * @param[out] out_data   Pointer to output structure (copied atomically)
     *
     * @return ERR_OK on success
     * @retval ERR_NULL_POINTER if ctx or out_data is NULL
     */
    error_t (*get_all_sensors)(void *ctx, LgcSensorArray_t *out_data);

    /**
     * @brief Get single sensor data (non-blocking)
     *
     * @param[in]  ctx        Implementation context
     * @param[in]  sensor_id  Sensor index (0 to LGC_SENSOR_NUMBER-1)
     * @param[out] out_data   Pointer to output structure
     *
     * @return ERR_OK on success
     * @retval ERR_NULL_POINTER if ctx or out_data is NULL
     * @retval ERR_INVALID_PARAM if sensor_id out of range
     */
    error_t (*get_sensor)(void *ctx, uint8_t sensor_id, LgcSensorData_t *out_data);

    /**
     * @brief Check if all sensors are healthy
     *
     * @param[in] ctx Implementation context
     *
     * @return true if all sensors responding normally
     * @return false if any sensor has fault status
     */
    bool (*is_all_healthy)(void *ctx);

    /**
     * @brief Get combined sensor status bitmask
     *
     * @param[in] ctx Implementation context
     *
     * @return uint16_t Bitmask where bit N = 1 means sensor N has fault
     */
    uint16_t (*get_status_mask)(void *ctx);

    /**
     * @brief Get sequence number for change detection
     *
     * @param[in] ctx Implementation context
     *
     * @return uint32_t Monotonically increasing update counter
     */
    uint32_t (*get_sequence)(void *ctx);

} ILgcSensorCache_t;

#endif /* DOMAIN_INTERFACES_LGC_I_SENSOR_CACHE_H_ */
