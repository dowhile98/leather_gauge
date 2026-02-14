/**
 * @file    lgc_measurement_entity.h
 * @brief   Measurement entities (LeatherPiece, Batch, Measurement)
 * @author  Clean Architecture Refactor Team
 * @date    2026-02-12
 * @version 1.0.0
 *
 * @details Defines core business entities for leather measurement system.
 *          These are PURE DATA structures with NO dependencies on infrastructure.
 *
 * @note    DOMAIN LAYER - Must NOT include:
 *          - stm32f4xx_hal.h
 *          - tx_api.h
 *          - Any adapter headers
 *
 * Copyright (c) 2026 Leather Gauge Controller Team
 */

#ifndef LGC_MEASUREMENT_ENTITY_H
#define LGC_MEASUREMENT_ENTITY_H

#ifdef __cplusplus
extern "C"
{
#endif

/* ============================= Includes ============================= */
#include "lgc_common_types.h"
#include "lgc_sensor_array_entity.h"

    /* ============================= Leather Piece ======================== */
    /**
     * @brief Single leather piece measurement result
     *
     * @details Represents the final measurement of one individual leather piece.
     *          Accumulated during measurement, finalized when piece ends.
     */
    typedef struct
    {
        float area_dm2;          /**< Total area in dm² */
        uint32_t slice_count;    /**< Number of encoder pulses (slices) processed */
        uint32_t start_position; /**< Encoder position at start */
        uint32_t end_position;   /**< Encoder position at end */
        LgcDateTime_t timestamp; /**< When measurement finished */
        bool is_valid;           /**< Measurement validity flag */
    } LgcLeatherPiece_t;

    /* ============================= Batch ================================ */
    /**
     * @brief Batch of leather pieces
     *
     * @details Contains multiple leather measurements with metadata.
     *          Limited to LGC_MAX_PIECES_PER_BATCH (100 pieces).
     */
    typedef struct
    {
        LgcLeatherPiece_t pieces[LGC_MAX_PIECES_PER_BATCH]; /**< Array of pieces */
        uint32_t piece_count;                               /**< Current count */
        uint32_t batch_number;                              /**< Sequential batch ID */
        float total_area_dm2;                               /**< Sum of all pieces */
        float average_area_dm2;                             /**< Average piece area */
        LgcDateTime_t start_time;                           /**< Batch start timestamp */
        LgcDateTime_t end_time;                             /**< Batch end timestamp */
        bool is_finalized;                                  /**< Batch closed flag */
    } LgcBatch_t;

    /* ============================= Active Measurement =================== */
    /**
     * @brief Active measurement state (current leather piece being measured)
     *
     * @details This structure tracks the CURRENT measurement in progress.
     *          It's continuously updated with each encoder pulse until
     *          the leather piece ends.
     *
     * @note  This is mutable state - protect with mutex in multi-threaded env.
     */
    typedef struct
    {
        float current_area_dm2;     /**< Accumulated area so far */
        uint32_t slice_count;       /**< Slices processed */
        uint32_t start_position;    /**< Encoder position at start */
        uint32_t empty_slice_count; /**< Consecutive empty slices (for hysteresis) */
        bool is_measuring;          /**< Currently measuring flag */
        bool leather_detected;      /**< Leather currently present */
    } LgcActiveMeasurement_t;

    /* ============================= System State ========================= */
    /**
     * @brief Complete measurement system state
     *
     * @details Aggregates all measurement-related state.
     */
    typedef struct
    {
        LgcActiveMeasurement_t active; /**< Current measurement in progress */
        LgcBatch_t batch;              /**< Current batch */
        LgcSystemState_t state;        /**< System operational state */
    } LgcMeasurementSystem_t;

    /**
     * @brief Global measurements storage (Legacy-compatible)
     *
     * @details Stores all measurements in arrays matching legacy structure.
     *          Thread-safe access via mutex (provided by OS adapter).
     */
    typedef struct
    {
        /* Current state */
        uint16_t current_batch_index;     /**< Current batch index (0-199) */
        uint16_t current_leather_index;   /**< Current leather index within batch (0-299) */
        uint16_t total_leathers_measured; /**< Total leathers measured */
        float current_leather_area;       /**< Accumulator for current leather area */

        /* Measurement arrays (legacy-compatible) */
        float leather_measurement[LGC_MAX_TOTAL_PIECES];      /**< Individual leather areas (300) */
        float leather_measurement_last[LGC_MAX_TOTAL_PIECES]; /**< Previous batch backup (300) */
        float batch_measurement[LGC_MAX_BATCH_COUNT];         /**< Batch sums (200) */

        /* State flags */
        uint8_t is_measuring;       /**< Measuring state flag */
        uint8_t no_detection_count; /**< Consecutive steps with no detection */

        /* Mutex (OS-specific, injected by adapter) */
        void *mutex_handle; /**< Opaque mutex handle */
    } LgcMeasurements_t;

    /* ============================= Helper Functions ===================== */
    /**
     * @brief Initialize leather piece to default state
     *
     * @param[out] piece Pointer to leather piece structure
     *
     * @pre  piece must not be NULL
     * @post piece initialized with zeros, is_valid = false
     */
    static inline void LgcLeatherPiece_Init(LgcLeatherPiece_t *piece)
    {
        if (piece == NULL)
        {
            return;
        }

        piece->area_dm2 = 0.0f;
        piece->slice_count = 0;
        piece->start_position = 0;
        piece->end_position = 0;
        piece->is_valid = false;
    }

    /**
     * @brief Initialize batch to default state
     *
     * @param[out] batch        Pointer to batch structure
     * @param[in]  batch_number Sequential batch number
     *
     * @pre  batch must not be NULL
     * @post batch initialized, piece_count = 0, is_finalized = false
     */
    static inline void LgcBatch_Init(LgcBatch_t *batch, uint32_t batch_number)
    {
        if (batch == NULL)
        {
            return;
        }

        for (uint32_t i = 0; i < LGC_MAX_PIECES_PER_BATCH; i++)
        {
            LgcLeatherPiece_Init(&batch->pieces[i]);
        }

        batch->piece_count = 0;
        batch->batch_number = batch_number;
        batch->total_area_dm2 = 0.0f;
        batch->average_area_dm2 = 0.0f;
        batch->is_finalized = false;
    }

    /**
     * @brief Initialize active measurement to default state
     *
     * @param[out] active Pointer to active measurement structure
     *
     * @pre  active must not be NULL
     * @post active initialized, is_measuring = false
     */
    static inline void LgcActiveMeasurement_Init(LgcActiveMeasurement_t *active)
    {
        if (active == NULL)
        {
            return;
        }

        active->current_area_dm2 = 0.0f;
        active->slice_count = 0;
        active->start_position = 0;
        active->empty_slice_count = 0;
        active->is_measuring = false;
        active->leather_detected = false;
    }

    /**
     * @brief Calculate total area from batch
     *
     * @param[in] batch Pointer to batch structure
     * @return Total area in dm² (0.0 if batch is NULL or empty)
     *
     * @note  Thread-safe (pure function)
     */
    static inline float LgcBatch_CalculateTotalArea(const LgcBatch_t *batch)
    {
        if (batch == NULL || batch->piece_count == 0)
        {
            return 0.0f;
        }

        float total = 0.0f;
        for (uint32_t i = 0; i < batch->piece_count; i++)
        {
            if (batch->pieces[i].is_valid)
            {
                total += batch->pieces[i].area_dm2;
            }
        }

        return total;
    }

    /**
     * @brief Calculate average area from batch
     *
     * @param[in] batch Pointer to batch structure
     * @return Average area in dm² (0.0 if batch is NULL or empty)
     *
     * @note  Thread-safe (pure function)
     */
    static inline float LgcBatch_CalculateAverageArea(const LgcBatch_t *batch)
    {
        if (batch == NULL || batch->piece_count == 0)
        {
            return 0.0f;
        }

        float total = LgcBatch_CalculateTotalArea(batch);
        return total / (float)batch->piece_count;
    }

    /**
     * @brief Initialize global measurements structure
     *
     * @param[out] measurements Pointer to measurements structure
     *
     * @pre  measurements must not be NULL
     * @post measurements initialized with zeros
     */
    static inline void LgcMeasurements_Init(LgcMeasurements_t *measurements)
    {
        if (measurements == NULL)
        {
            return;
        }

        measurements->current_batch_index = 0;
        measurements->current_leather_index = 0;
        measurements->total_leathers_measured = 0;
        measurements->current_leather_area = 0.0f;

        /* Clear arrays */
        for (uint32_t i = 0; i < LGC_MAX_TOTAL_PIECES; i++)
        {
            measurements->leather_measurement[i] = 0.0f;
            measurements->leather_measurement_last[i] = 0.0f;
        }

        for (uint32_t i = 0; i < LGC_MAX_BATCH_COUNT; i++)
        {
            measurements->batch_measurement[i] = 0.0f;
        }

        measurements->is_measuring = 0;
        measurements->no_detection_count = 0;
        measurements->mutex_handle = NULL;
    }

#ifdef __cplusplus
}
#endif

#endif /* LGC_MEASUREMENT_ENTITY_H */
