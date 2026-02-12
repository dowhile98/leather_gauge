/**
 * @file    lgc_uc_process_slice.h
 * @brief   Process Slice Use Case - Core measurement algorithm
 * @author  Clean Architecture Refactor Team
 * @date    2026-02-12
 * @version 1.0.0
 *
 * @details This use case implements the core leather measurement algorithm:
 *          - Calculate slice area from sensor array
 *          - Detect leather presence/absence
 *          - Apply hysteresis for piece boundaries
 *
 * @note    DOMAIN LAYER - Pure business logic, NO HAL dependencies
 *
 * Copyright (c) 2026 Leather Gauge Controller Team
 */

#ifndef LGC_UC_PROCESS_SLICE_H
#define LGC_UC_PROCESS_SLICE_H

#ifdef __cplusplus
extern "C"
{
#endif

/* ============================= Includes ============================= */
#include "../../entities/lgc_common_types.h"
#include "../../entities/lgc_sensor_array_entity.h"
#include "../../entities/lgc_measurement_entity.h"
#include "../../entities/lgc_configuration_entity.h"

    /* ============================= Use Case Output ====================== */
    /**
     * @brief Slice processing result
     */
    typedef struct
    {
        float slice_area_dm2;       /**< Calculated area for this slice (dm²) */
        bool leather_detected;      /**< Leather present in this slice */
        uint32_t active_bits_total; /**< Total active bits across all sensors */
        bool piece_finished;        /**< Piece boundary detected (hysteresis satisfied) */
    } LgcSliceResult_t;

    /* ============================= Use Case ============================= */
    /**
     * @brief Process single measurement slice
     *
     * @details Algorithm:
     *          1. Count active bits per sensor
     *          2. Calculate slice area: active_bits × 10mm × 5mm
     *          3. Check leather detection threshold
     *          4. Apply hysteresis for piece end detection
     *
     * @param[in]     sensor_data Sensor array reading (11 sensors)
     * @param[in]     config      System configuration (threshold, hysteresis)
     * @param[in,out] active      Active measurement state (updated)
     * @param[out]    result      Slice processing result
     * @return ERR_OK on success, error code otherwise
     *
     * @pre  All pointers must not be NULL
     * @pre  sensor_data must have valid readings
     * @pre  config must be validated
     * @post active->current_area_dm2 updated
     * @post active->slice_count incremented
     * @post result populated with slice info
     * @post If piece finished: active->is_measuring = false
     *
     * @note  Thread-safe if external mutex protects active
     * @note  Processing time: <500µs typical
     *
     * @see   LgcUC_MeasureArea_ProcessSlice() for complete measurement cycle
     */
    Result_t LgcUC_ProcessSlice(
        const LgcSensorArray_t *sensor_data,
        const LgcSystemConfig_t *config,
        LgcActiveMeasurement_t *active,
        LgcSliceResult_t *result);

    /**
     * @brief Calculate area for single slice
     *
     * @details Formula: area = active_bits × PHOTOCELL_SPACING × ENCODER_STEP
     *                        = active_bits × 10mm × 5mm = active_bits × 50mm²
     *                        = active_bits × 0.5 cm² = active_bits × 0.005 dm²
     *
     * @param[in] sensor_data Sensor array reading
     * @return Slice area in dm²
     *
     * @note  Pure function (no side effects)
     * @note  Thread-safe
     */
    float LgcUC_CalculateSliceArea(const LgcSensorArray_t *sensor_data);

    /**
     * @brief Detect if leather is present in slice
     *
     * @param[in] sensor_data Sensor array reading
     * @param[in] threshold   Minimum active bits per sensor to detect leather
     * @return true if leather detected, false otherwise
     *
     * @details Leather detected if ANY sensor has >= threshold active bits
     *
     * @note  Pure function
     */
    bool LgcUC_DetectLeather(
        const LgcSensorArray_t *sensor_data,
        uint8_t threshold);

    /**
     * @brief Check if piece boundary detected (end of leather)
     *
     * @param[in] active     Active measurement state
     * @param[in] hysteresis Required consecutive empty slices
     * @return true if piece finished, false otherwise
     *
     * @details Piece ends when empty_slice_count >= hysteresis (default 3)
     *
     * @note  Pure function
     */
    bool LgcUC_IsPieceFinished(
        const LgcActiveMeasurement_t *active,
        uint8_t hysteresis);

#ifdef __cplusplus
}
#endif

#endif /* LGC_UC_PROCESS_SLICE_H */
