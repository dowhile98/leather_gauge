/**
 * @file lgc_sensor_cache.c
 * @brief Sensor Cache adapter implementation
 *
 * Thread-safe sensor data storage implementing ILgcSensorCache interface.
 * Decouples Modbus communication from measurement algorithm.
 *
 * @date Created: Feb 13, 2026
 * @author GitHub Copilot
 */

/* ============================================================================
 * INCLUDES
 * ============================================================================ */
#include "lgc_sensor_cache.h"
#include <string.h>

/* ============================================================================
 * PRIVATE FUNCTION PROTOTYPES
 * ============================================================================ */
static error_t cache_get_all_sensors(void *ctx, LgcSensorArray_t *out_data);
static error_t cache_get_sensor(void *ctx, uint8_t sensor_id, LgcSensorData_t *out_data);
static bool cache_is_all_healthy(void *ctx);
static uint16_t cache_get_status_mask(void *ctx);
static uint32_t cache_get_sequence(void *ctx);

/* ============================================================================
 * STATIC VARIABLES
 * ============================================================================ */
static ILgcSensorCache_t s_interface;

/* ============================================================================
 * PUBLIC FUNCTION DEFINITIONS
 * ============================================================================ */

/**
 * @brief Initialize sensor cache
 */
error_t LgcSensorCache_Init(LgcSensorCache_t *cache)
{
    if (cache == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    /* Clear all data */
    memset(&cache->data, 0, sizeof(LgcSensorArray_t));
    cache->sequence_number = 0;
    cache->cycle_start_ms = 0;

    /* Initialize all sensors as healthy (no faults on startup) */
    for (uint8_t i = 0; i < LGC_SENSOR_NUMBER; i++)
    {
        cache->data.sensors[i].value = 0;
        cache->data.sensors[i].status = LGC_SENSOR_HEALTHY;
        cache->data.sensors[i].timestamp_ms = 0;
    }
    cache->data.status_bitmask = 0;

#ifdef USE_RTOS
    /* Create mutex for thread safety */
    if (osCreateMutex(&cache->mutex) != TRUE)
    {
        return ERROR_FAILURE;
    }
#endif

    cache->is_initialized = true;

    return NO_ERROR;
}

/**
 * @brief Get interface pointer for dependency injection
 */
ILgcSensorCache_t *LgcSensorCache_GetInterface(LgcSensorCache_t *cache)
{
    s_interface.context = cache;
    s_interface.get_all_sensors = cache_get_all_sensors;
    s_interface.get_sensor = cache_get_sensor;
    s_interface.is_all_healthy = cache_is_all_healthy;
    s_interface.get_status_mask = cache_get_status_mask;
    s_interface.get_sequence = cache_get_sequence;

    return &s_interface;
}

/**
 * @brief Update single sensor data (called by Modbus task)
 */
error_t LgcSensorCache_UpdateSensor(
    LgcSensorCache_t *cache,
    uint8_t sensor_id,
    uint16_t value,
    LgcSensorStatus_t status,
    uint32_t timestamp)
{
    if (cache == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    if (sensor_id >= LGC_SENSOR_NUMBER)
    {
        return ERROR_INVALID_PARAMETER;
    }

#ifdef USE_RTOS
    osAcquireMutex(&cache->mutex);
#endif

    /* Update sensor data */
    cache->data.sensors[sensor_id].value = value;
    cache->data.sensors[sensor_id].status = status;
    cache->data.sensors[sensor_id].timestamp_ms = timestamp;

    /* Update status bitmask */
    if (status == LGC_SENSOR_HEALTHY)
    {
        cache->data.status_bitmask &= ~(1U << sensor_id); /* Clear fault bit */
    }
    else
    {
        cache->data.status_bitmask |= (1U << sensor_id); /* Set fault bit */
    }

    /* Increment sequence number */
    cache->sequence_number++;
    cache->data.sequence_number = cache->sequence_number;

#ifdef USE_RTOS
    osReleaseMutex(&cache->mutex);
#endif

    return NO_ERROR;
}

/**
 * @brief Mark beginning of read cycle
 */
void LgcSensorCache_BeginCycle(LgcSensorCache_t *cache, uint32_t timestamp_ms)
{
    if (cache == NULL)
    {
        return;
    }

#ifdef USE_RTOS
    osAcquireMutex(&cache->mutex);
#endif

    cache->cycle_start_ms = timestamp_ms;

#ifdef USE_RTOS
    osReleaseMutex(&cache->mutex);
#endif
}

/**
 * @brief Mark end of read cycle
 */
void LgcSensorCache_EndCycle(LgcSensorCache_t *cache, uint32_t timestamp_ms)
{
    if (cache == NULL)
    {
        return;
    }

#ifdef USE_RTOS
    osAcquireMutex(&cache->mutex);
#endif

    cache->data.read_cycle_ms = timestamp_ms - cache->cycle_start_ms;

#ifdef USE_RTOS
    osReleaseMutex(&cache->mutex);
#endif
}

/* ============================================================================
 * PRIVATE FUNCTION DEFINITIONS (Interface Implementation)
 * ============================================================================ */

/**
 * @brief Get all sensor data (ILgcSensorCache implementation)
 */
static error_t cache_get_all_sensors(void *ctx, LgcSensorArray_t *out_data)
{
    LgcSensorCache_t *cache = (LgcSensorCache_t *)ctx;

    if (cache == NULL || out_data == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

#ifdef USE_RTOS
    osAcquireMutex(&cache->mutex);
#endif

    /* Atomic copy of entire structure */
    memcpy(out_data, &cache->data, sizeof(LgcSensorArray_t));

#ifdef USE_RTOS
    osReleaseMutex(&cache->mutex);
#endif

    return NO_ERROR;
}

/**
 * @brief Get single sensor data (ILgcSensorCache implementation)
 */
static error_t cache_get_sensor(void *ctx, uint8_t sensor_id, LgcSensorData_t *out_data)
{
    LgcSensorCache_t *cache = (LgcSensorCache_t *)ctx;

    if (cache == NULL || out_data == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    if (sensor_id >= LGC_SENSOR_NUMBER)
    {
        return ERROR_INVALID_PARAMETER;
    }

#ifdef USE_RTOS
    osAcquireMutex(&cache->mutex);
#endif

    /* Copy single sensor data */
    *out_data = cache->data.sensors[sensor_id];

#ifdef USE_RTOS
    osReleaseMutex(&cache->mutex);
#endif

    return NO_ERROR;
}

/**
 * @brief Check if all sensors healthy (ILgcSensorCache implementation)
 */
static bool cache_is_all_healthy(void *ctx)
{
    LgcSensorCache_t *cache = (LgcSensorCache_t *)ctx;

    if (cache == NULL)
    {
        return false;
    }

    bool healthy;

#ifdef USE_RTOS
    osAcquireMutex(&cache->mutex);
#endif

    healthy = (cache->data.status_bitmask == 0);

#ifdef USE_RTOS
    osReleaseMutex(&cache->mutex);
#endif

    return healthy;
}

/**
 * @brief Get status bitmask (ILgcSensorCache implementation)
 */
static uint16_t cache_get_status_mask(void *ctx)
{
    LgcSensorCache_t *cache = (LgcSensorCache_t *)ctx;

    if (cache == NULL)
    {
        return 0xFFFF; /* All faults if invalid */
    }

    uint16_t mask;

#ifdef USE_RTOS
    osAcquireMutex(&cache->mutex);
#endif

    mask = cache->data.status_bitmask;

#ifdef USE_RTOS
    osReleaseMutex(&cache->mutex);
#endif

    return mask;
}

/**
 * @brief Get sequence number (ILgcSensorCache implementation)
 */
static uint32_t cache_get_sequence(void *ctx)
{
    LgcSensorCache_t *cache = (LgcSensorCache_t *)ctx;

    if (cache == NULL)
    {
        return 0;
    }

    uint32_t seq;

#ifdef USE_RTOS
    osAcquireMutex(&cache->mutex);
#endif

    seq = cache->sequence_number;

#ifdef USE_RTOS
    osReleaseMutex(&cache->mutex);
#endif

    return seq;
}
