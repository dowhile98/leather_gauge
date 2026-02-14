/**
 * @file lgc_sensor_cache.h
 * @brief Sensor Cache adapter header - Thread-safe sensor data storage
 *
 * Implements ILgcSensorCache interface for non-blocking sensor data access.
 * The Modbus task writes, and the Main task reads without blocking.
 *
 * @note Part of Clean Architecture Adapters Layer
 *
 * @date Created: Feb 13, 2026
 * @author GitHub Copilot
 */

#ifndef ADAPTERS_SENSOR_CACHE_LGC_SENSOR_CACHE_H_
#define ADAPTERS_SENSOR_CACHE_LGC_SENSOR_CACHE_H_

/* ============================================================================
 * INCLUDES
 * ============================================================================ */
#include "lgc_i_sensor_cache.h"
#include "error.h"

#ifdef USE_RTOS
#include "os_port.h"
#endif

/* ============================================================================
 * TYPE DEFINITIONS
 * ============================================================================ */

/**
 * @brief Sensor Cache implementation structure
 */
typedef struct
{
    LgcSensorArray_t data;    /**< Cached sensor data (double-buffered read) */
    uint32_t sequence_number; /**< Update counter */
    uint32_t cycle_start_ms;  /**< Current cycle start timestamp */

#ifdef USE_RTOS
    OsMutex mutex; /**< Thread-safety mutex */
#endif

    bool is_initialized; /**< Initialization flag */
} LgcSensorCache_t;

/* ============================================================================
 * PUBLIC FUNCTION PROTOTYPES
 * ============================================================================ */

/**
 * @brief Initialize sensor cache
 *
 * @param[in,out] cache Pointer to cache structure
 *
 * @return ERR_OK on success
 * @retval ERROR_INVALID_PARAMETER if cache is NULL
 */
error_t LgcSensorCache_Init(LgcSensorCache_t *cache);

/**
 * @brief Get interface pointer for dependency injection
 *
 * @param[in] cache Pointer to initialized cache
 *
 * @return ILgcSensorCache_t* Interface pointer
 */
ILgcSensorCache_t *LgcSensorCache_GetInterface(LgcSensorCache_t *cache);

/**
 * @brief Update single sensor data (called by Modbus task)
 *
 * @param[in,out] cache       Pointer to cache
 * @param[in]     sensor_id   Sensor index (0 to LGC_SENSOR_NUMBER-1)
 * @param[in]     value       Raw sensor value
 * @param[in]     status      Sensor health status
 * @param[in]     timestamp   Timestamp of reading
 *
 * @return ERR_OK on success
 * @retval ERROR_INVALID_PARAMETER if cache is NULL or sensor_id invalid
 */
error_t LgcSensorCache_UpdateSensor(
    LgcSensorCache_t *cache,
    uint8_t sensor_id,
    uint16_t value,
    LgcSensorStatus_t status,
    uint32_t timestamp);

/**
 * @brief Mark beginning of read cycle
 *
 * @param[in,out] cache         Pointer to cache
 * @param[in]     timestamp_ms  Cycle start timestamp
 */
void LgcSensorCache_BeginCycle(LgcSensorCache_t *cache, uint32_t timestamp_ms);

/**
 * @brief Mark end of read cycle
 *
 * @param[in,out] cache         Pointer to cache
 * @param[in]     timestamp_ms  Cycle end timestamp
 */
void LgcSensorCache_EndCycle(LgcSensorCache_t *cache, uint32_t timestamp_ms);

#endif /* ADAPTERS_SENSOR_CACHE_LGC_SENSOR_CACHE_H_ */
