/**
 * @file    lgc_i_sensor_reader.h
 * @brief   Sensor Reader Interface (Port)
 * @author  Clean Architecture Refactor Team
 * @date    2026-02-12
 * @version 1.0.0
 *
 * @details Interface for reading sensor array data.
 *          Implementations: ModbusAdapter (legacy), LwPktAdapter (optimized).
 *
 *          This abstraction allows swapping communication protocols
 *          without changing domain logic (Open/Closed Principle).
 *
 * @note    INTERFACE LAYER (Port) - Implementations in adapters/
 *
 * Copyright (c) 2026 Leather Gauge Controller Team
 */

#ifndef LGC_I_SENSOR_READER_H
#define LGC_I_SENSOR_READER_H

#ifdef __cplusplus
extern "C"
{
#endif

/* ============================= Includes ============================= */
#include "../entities/lgc_common_types.h"
#include "../entities/lgc_sensor_array_entity.h"

    /* ============================= Configuration ======================== */
    /**
     * @brief Sensor reader configuration
     */
    typedef struct
    {
        uint32_t timeout_ms; /**< Read timeout per sensor (ms) */
        uint8_t retry_count; /**< Number of retries on failure */
        bool enable_crc;     /**< Enable CRC validation */
    } LgcSensorReaderConfig_t;

    /* ============================= Interface ============================ */
    /**
     * @brief Sensor Reader Interface (V-Table)
     *
     * @details This interface abstracts sensor communication protocols.
     *          Domain layer depends ONLY on this interface, not implementations.
     *
     * Usage Pattern:
     * @code
     * // In domain/use_cases/measure/lgc_uc_measure_area.c
     * typedef struct {
     *     ILgcSensorReader_t *sensor_reader;  // Injected dependency
     * } LgcMeasureAreaUC_t;
     *
     * Result_t LgcUC_ProcessSlice(LgcMeasureAreaUC_t *uc) {
     *     LgcSensorArray_t data;
     *     Result_t res = uc->sensor_reader->read_cascade_mode(
     *         uc->sensor_reader->context,
     *         &data
     *     );
     *     // Process data...
     * }
     * @endcode
     */
    typedef struct ILgcSensorReader_t
    {
        /**
         * @brief Opaque pointer to implementation context
         *
         * @note  Cast to concrete type in adapter implementations.
         *        Example: ModbusAdapter_t*, LwPktAdapter_t*
         */
        void *context;

        /**
         * @brief Initialize sensor reader
         *
         * @param[in] ctx    Implementation context (this->context)
         * @param[in] config Configuration parameters
         * @return ERR_OK on success, error code otherwise
         *
         * @pre  ctx must not be NULL
         * @post If successful, sensor reader ready for read operations
         */
        Result_t (*init)(void *ctx, const LgcSensorReaderConfig_t *config);

        /**
         * @brief Read all sensors sequentially (polling mode)
         *
         * @param[in]  ctx      Implementation context
         * @param[out] out_data Sensor array data (all 11 sensors)
         * @return ERR_OK on success, error code otherwise
         *
         * @pre  ctx and out_data must not be NULL
         * @pre  init() must have been called successfully
         * @post out_data populated with sensor readings
         * @post Sensors with errors marked as invalid (is_valid = false)
         *
         * @note  Blocking call - may take up to 2s for 11 sensors (Modbus)
         * @note  Thread-safe if implementation uses mutex
         *
         * @warning DEPRECATED for LwPKT - use read_cascade_mode() instead
         */
        Result_t (*read_all_sensors)(void *ctx, LgcSensorArray_t *out_data);

        /**
         * @brief Read sensors in cascade mode (optimized)
         *
         * @param[in]  ctx      Implementation context
         * @param[out] out_data Sensor array data (all 11 sensors)
         * @return ERR_OK on success, error code otherwise
         *
         * @pre  ctx and out_data must not be NULL
         * @pre  init() must have been called successfully
         * @post out_data populated with sensor readings
         *
         * @note  Cascade mode: Single broadcast → 11 sequential responses
         * @note  ~550ms for 11 sensors (LwPKT), vs 2s (Modbus polling)
         * @note  Falls back to read_all_sensors() if cascade not supported
         *
         * @see   docs/sensor/README.md for LwPKT cascade protocol details
         */
        Result_t (*read_cascade_mode)(void *ctx, LgcSensorArray_t *out_data);

        /**
         * @brief Deinitialize sensor reader
         *
         * @param[in] ctx Implementation context
         * @return ERR_OK on success
         *
         * @post Resources released, further reads will fail
         */
        Result_t (*deinit)(void *ctx);

    } ILgcSensorReader_t;

/* ============================= Helper Macros ======================== */
/**
 * @brief Safely call sensor reader method
 *
 * @param reader  Pointer to ILgcSensorReader_t
 * @param method  Method name (e.g., read_all_sensors)
 * @param ...     Method arguments (excluding ctx)
 *
 * @note  Validates reader and method pointer before calling
 */
#define LGC_SENSOR_READER_CALL(reader, method, ...) \
    (((reader) != NULL && (reader)->method != NULL) ? (reader)->method((reader)->context, ##__VA_ARGS__) : ERR_NULL_POINTER)

#ifdef __cplusplus
}
#endif

#endif /* LGC_I_SENSOR_READER_H */
