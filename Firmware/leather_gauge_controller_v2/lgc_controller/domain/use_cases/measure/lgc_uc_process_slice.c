/**
 * @file    lgc_uc_process_slice.c
 * @brief   Process Slice Use Case - Implementation
 * @author  Clean Architecture Refactor Team
 * @date    2026-02-12
 * @version 1.0.0
 *
 * Copyright (c) 2026 Leather Gauge Controller Team
 */

/* ============================= Includes ============================= */
#include "lgc_uc_process_slice.h"
#include <string.h>

/* ============================= Constants ============================ */
/** Area per active bit (mm²) = 20mm × 5.5mm (REAL hardware values from legacy) */
#define AREA_PER_BIT_MM2 110.0f

/** Conversion factor: mm² to m² */
#define MM2_TO_M2 0.000001f

/** Conversion factors for units (ft² variants from legacy) */
#define M2_TO_FT2_STANDARD 10.7639f /**< Standard ft² conversion */
#define M2_TO_FT2_VARIANT1 30.48f   /**< Variant 1 */
#define M2_TO_FT2_VARIANT2 30.0f    /**< Variant 2 */
#define M2_TO_FT2_VARIANT3 28.0f    /**< Variant 3 */

/** Area per active bit in m² */
#define AREA_PER_BIT_M2 (AREA_PER_BIT_MM2 * MM2_TO_M2)

/* ============================= Private Functions ==================== */
/**
 * @brief Count total active bits across all sensors
 *
 * @param[in] sensor_data Sensor array
 * @return Total active bits (0-110)
 */
static uint32_t count_total_active_bits(const LgcSensorArray_t *sensor_data)
{
    uint32_t total = 0;

    for (uint8_t i = 0; i < LGC_SENSOR_COUNT; i++)
    {
        total += LgcSensorReading_CountActiveBits(&sensor_data->sensors[i]);
    }

    return total;
}

/* ============================= Public Functions ===================== */

float LgcUC_CalculateSliceArea(const LgcSensorArray_t *sensor_data)
{
    if (sensor_data == NULL)
    {
        return 0.0f;
    }

    uint32_t active_bits = count_total_active_bits(sensor_data);

    // area_m² = active_bits × 20mm × 5.5mm × (1m/1000mm)²
    //         = active_bits × 110mm² × 0.000001
    //         = active_bits × 0.00011 m²
    return (float)active_bits * AREA_PER_BIT_M2;
}

bool LgcUC_DetectLeather(
    const LgcSensorArray_t *sensor_data,
    uint8_t threshold)
{
    if (sensor_data == NULL || threshold == 0)
    {
        return false;
    }

    // Leather detected if ANY sensor exceeds threshold
    for (uint8_t i = 0; i < LGC_SENSOR_COUNT; i++)
    {
        if (LgcSensorReading_HasLeather(&sensor_data->sensors[i], threshold))
        {
            return true;
        }
    }

    return false;
}

bool LgcUC_IsPieceFinished(
    const LgcActiveMeasurement_t *active,
    uint8_t hysteresis)
{
    if (active == NULL || !active->is_measuring)
    {
        return false;
    }

    // Piece finished when consecutive empty slices >= hysteresis
    return (active->empty_slice_count >= hysteresis);
}

Result_t LgcUC_ProcessSlice(
    const LgcSensorArray_t *sensor_data,
    const LgcSystemConfig_t *config,
    LgcActiveMeasurement_t *active,
    LgcSliceResult_t *result)
{
    /* ===== Parameter Validation ===== */
    LGC_VALIDATE_PTR(sensor_data);
    LGC_VALIDATE_PTR(config);
    LGC_VALIDATE_PTR(active);
    LGC_VALIDATE_PTR(result);

    if (!config->is_valid)
    {
        return ERR_INVALID_PARAM;
    }

    /* ===== Initialize Result ===== */
    memset(result, 0, sizeof(LgcSliceResult_t));

    /* ===== Calculate Slice Area ===== */
    float slice_area_m2 = LgcUC_CalculateSliceArea(sensor_data);
    result->active_bits_total = count_total_active_bits(sensor_data);

    /* ===== Unit Conversion (Legacy Logic) ===== */
    if (config->unit == LGC_UNIT_M2)
    {
        // Metric: Keep m²
        result->slice_area_dm2 = slice_area_m2;
    }
    else
    {
        // Imperial: Convert to ft² using configured factor
        float conversion_factor = M2_TO_FT2_STANDARD; // Default

        switch (config->conversion_factor)
        {
        case 0:
            conversion_factor = M2_TO_FT2_STANDARD;
            break;
        case 1:
            conversion_factor = M2_TO_FT2_VARIANT1;
            break;
        case 2:
            conversion_factor = M2_TO_FT2_VARIANT2;
            break;
        case 3:
            conversion_factor = M2_TO_FT2_VARIANT3;
            break;
        default:
            conversion_factor = M2_TO_FT2_STANDARD;
            break;
        }

        result->slice_area_dm2 = slice_area_m2 * conversion_factor;
    }

    /* ===== Detect Leather Presence ===== */
    result->leather_detected = LgcUC_DetectLeather(
        sensor_data,
        config->leather_threshold);

    /* ===== Update Active Measurement State ===== */

    if (result->leather_detected)
    {
        // Leather present: accumulate area
        if (!active->is_measuring)
        {
            // Start new piece
            active->is_measuring = true;
            active->leather_detected = true;
            active->current_area_dm2 = 0.0f;
            active->slice_count = 0;
            active->start_position = sensor_data->encoder_position;
        }

        // Accumulate area
        active->current_area_dm2 += result->slice_area_dm2;
        active->slice_count++;

        // Reset empty slice counter
        active->empty_slice_count = 0;
    }
    else
    {
        // No leather detected
        if (active->is_measuring)
        {
            // Increment empty slice counter
            active->empty_slice_count++;

            // Check if piece finished (hysteresis)
            result->piece_finished = LgcUC_IsPieceFinished(
                active,
                config->hysteresis);

            if (result->piece_finished)
            {
                // Piece finished - stop measuring
                active->is_measuring = false;
                active->leather_detected = false;
            }
        }
    }

    return ERR_OK;
}
