/**
 * @file    lgc_sensor_array_entity.h
 * @brief   Sensor array entity definition
 * @author  Clean Architecture Refactor Team
 * @date    2026-02-12
 * @version 1.0.0
 *
 * @details Defines the sensor array data structure used for leather detection.
 *          Each sensor has 10 photocells represented as a 16-bit bitmask.
 *
 * @note    Pure data structure - NO business logic here.
 *          This file is part of DOMAIN layer (must NOT include HAL).
 *
 * Copyright (c) 2026 Leather Gauge Controller Team
 */

#ifndef LGC_SENSOR_ARRAY_ENTITY_H
#define LGC_SENSOR_ARRAY_ENTITY_H

#ifdef __cplusplus
extern "C"
{
#endif

/* ============================= Includes ============================= */
#include "lgc_common_types.h"

    /* ============================= Sensor Reading ======================= */
    /**
     * @brief Individual sensor reading
     *
     * @details Each sensor contains 10 photocells. The status is a bitmask:
     *          - Bits 0-9: Photocell state (1 = leather detected, 0 = empty)
     *          - Bits 10-15: Reserved (must be 0)
     *
     * @note  Bit order: LSB = Photocell 0, MSB = Photocell 9
     *
     * Example:
     * @code
     * status = 0x03FF;  // All 10 photocells active (1111111111b)
     * status = 0x0000;  // All photocells empty
     * status = 0x0155;  // Pattern: 0101010101b (alternating)
     * @endcode
     */
    typedef struct
    {
        uint16_t status; /**< Bitmask: bits [0-9] = photocells state */
        uint8_t address; /**< Sensor address (1-11) */
        bool is_valid;   /**< Data validity flag (false if timeout/CRC error) */
    } LgcSensorReading_t;

    /**
     * @brief Complete sensor array (11 sensors)
     *
     * @details Represents one "slice" of measurement data captured
     *          at a single encoder position.
     */
    typedef struct
    {
        LgcSensorReading_t sensors[LGC_SENSOR_COUNT]; /**< Array of 11 sensors */
        uint32_t timestamp_ms;                        /**< Capture timestamp (ms) */
        uint32_t encoder_position;                    /**< Encoder position when captured */
    } LgcSensorArray_t;

    /* ============================= Calibration Data ===================== */
    /**
     * @brief Calibration data for zero offset compensation
     *
     * @details Stores the "no leather" baseline for each sensor.
     *          Used to compensate for ambient light variations.
     */
    typedef struct
    {
        uint16_t zero_offset[LGC_SENSOR_COUNT]; /**< Baseline reading per sensor */
        bool is_calibrated;                     /**< Calibration status */
        LgcDateTime_t calibration_date;         /**< When calibration was performed */
    } LgcCalibrationData_t;

    /* ============================= Helper Functions ===================== */
    /**
     * @brief Count active bits in a sensor reading
     *
     * @param[in] reading Pointer to sensor reading
     * @return Number of active photocells (0-10), or 0 if reading is NULL/invalid
     *
     * @note  Thread-safe (pure function, no side effects)
     */
    static inline uint8_t LgcSensorReading_CountActiveBits(const LgcSensorReading_t *reading)
    {
        if (reading == NULL || !reading->is_valid)
        {
            return 0;
        }

        uint8_t count = 0;
        uint16_t mask = reading->status & 0x03FF; // Mask to bits 0-9 only

        // Brian Kernighan's algorithm (efficient bit counting)
        while (mask)
        {
            mask &= (mask - 1);
            count++;
        }

        return count;
    }

    /**
     * @brief Check if sensor reading indicates leather presence
     *
     * @param[in] reading   Pointer to sensor reading
     * @param[in] threshold Minimum active bits to detect leather (typically 2-3)
     * @return true if leather detected, false otherwise
     *
     * @pre  reading must not be NULL
     * @post No state change (pure function)
     */
    static inline bool LgcSensorReading_HasLeather(
        const LgcSensorReading_t *reading,
        uint8_t threshold)
    {
        if (reading == NULL || !reading->is_valid)
        {
            return false;
        }

        return (LgcSensorReading_CountActiveBits(reading) >= threshold);
    }

    /**
     * @brief Reset sensor array to default state
     *
     * @param[out] array Pointer to sensor array to reset
     *
     * @pre  array must not be NULL
     * @post All sensor readings marked invalid, status = 0
     */
    static inline void LgcSensorArray_Reset(LgcSensorArray_t *array)
    {
        if (array == NULL)
        {
            return;
        }

        for (uint8_t i = 0; i < LGC_SENSOR_COUNT; i++)
        {
            array->sensors[i].status = 0;
            array->sensors[i].address = i + 1;
            array->sensors[i].is_valid = false;
        }
        array->timestamp_ms = 0;
        array->encoder_position = 0;
    }

#ifdef __cplusplus
}
#endif

#endif /* LGC_SENSOR_ARRAY_ENTITY_H */
