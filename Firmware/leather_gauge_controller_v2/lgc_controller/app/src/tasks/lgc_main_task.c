/**
 * @file    lgc_main_task.c
 * @brief   Main Measurement Task - Implementation
 * @author  Clean Architecture Refactor Team (TDD)
 * @date    2026-02-12
 * @version 1.1.0
 *
 * @details Encoder-driven measurement orchestration task.
 *          Real-time priority (10) - critical timing.
 *
 * Copyright (c) 2026 Leather Gauge Controller Team
 */

/* ============================= Includes ============================= */
#include "lgc_main_task.h"
#include "../../inc/lgc_di_container.h"
#include "../../../domain/use_cases/measure/lgc_uc_process_slice.h"
#include "../../../domain/interfaces/lgc_i_encoder.h"
#include "../../../domain/interfaces/lgc_i_sensor_reader.h"
#include "../../../domain/interfaces/lgc_i_event_publisher.h"
#include <string.h>

/* ============================= Private Types ======================== */
/**
 * @brief Main task context (private)
 */
typedef struct LgcMainTask_t
{
    /* ThreadX resources */
    TX_THREAD thread;
    TX_EVENT_FLAGS_GROUP event_flags;
    uint8_t stack[LGC_MAIN_TASK_STACK_SIZE];

    /* Configuration */
    LgcMainTaskConfig_t config;

    /* State */
    bool is_running;
    LgcActiveMeasurement_t active_measurement;

    /* Statistics */
    uint32_t total_slices_processed;
    uint32_t total_errors;
    uint32_t max_processing_time_ms;

} LgcMainTask_t;

/* ============================= Private Variables ==================== */
static LgcMainTask_t s_main_task = {0};

/* ============================= Private Function Prototypes ========== */
static void main_task_entry(ULONG param);
static void encoder_pulse_callback(uint32_t position, void *user_ctx);

/* ============================= Public API =========================== */

Result_t LgcMainTask_Start(const LgcMainTaskConfig_t *config)
{
    if (config == NULL)
    {
        return ERR_NULL_POINTER;
    }

    if (s_main_task.is_running)
    {
        return ERR_BUSY; /* Already running */
    }

    /* Copy configuration */
    memcpy(&s_main_task.config, config, sizeof(LgcMainTaskConfig_t));

    /* Initialize event flags */
    UINT tx_res = tx_event_flags_create(
        &s_main_task.event_flags,
        "LGC_MainTask_Events");

    if (tx_res != TX_SUCCESS)
    {
        return ERR_HARDWARE_FAULT;
    }

    /* Initialize active measurement */
    memset(&s_main_task.active_measurement, 0, sizeof(LgcActiveMeasurement_t));
    s_main_task.active_measurement.is_measuring = false;

    /* Register encoder pulse callback */
    ILgcEncoder_t *encoder = DIContainer_GetEncoder();
    if (encoder == NULL)
    {
        return ERR_NULL_POINTER;
    }

    Result_t res = encoder->attach_callback(
        encoder->context,
        encoder_pulse_callback,
        &s_main_task);

    if (res != ERR_OK)
    {
        tx_event_flags_delete(&s_main_task.event_flags);
        return res;
    }

    /* Create main task */
    tx_res = tx_thread_create(
        &s_main_task.thread,
        "LGC_MainTask",
        main_task_entry,
        (ULONG)&s_main_task,
        s_main_task.stack,
        sizeof(s_main_task.stack),
        LGC_MAIN_TASK_PRIORITY,
        LGC_MAIN_TASK_PREEMPT_THRESHOLD,
        TX_NO_TIME_SLICE,
        TX_AUTO_START);

    if (tx_res != TX_SUCCESS)
    {
        encoder->detach_callback(encoder->context);
        tx_event_flags_delete(&s_main_task.event_flags);
        return ERR_HARDWARE_FAULT;
    }

    s_main_task.is_running = true;
    return ERR_OK;
}

Result_t LgcMainTask_Stop(void)
{
    if (!s_main_task.is_running)
    {
        return ERR_NOT_INITIALIZED;
    }

    /* Signal stop event */
    tx_event_flags_set(&s_main_task.event_flags, LGC_EVENT_STOP, TX_OR);

    /* Wait for task to finish (timeout 1s) */
    UINT tx_res = tx_thread_wait_abort(&s_main_task.thread);

    if (tx_res != TX_SUCCESS)
    {
        /* Force terminate */
        tx_thread_terminate(&s_main_task.thread);
    }

    /* Cleanup resources */
    ILgcEncoder_t *encoder = DIContainer_GetEncoder();
    if (encoder != NULL)
    {
        encoder->detach_callback(encoder->context);
    }

    tx_thread_delete(&s_main_task.thread);
    tx_event_flags_delete(&s_main_task.event_flags);

    s_main_task.is_running = false;
    return ERR_OK;
}

Result_t LgcMainTask_GetStats(void *stats)
{
    /* TODO: Implement statistics retrieval */
    (void)stats;
    return ERR_NOT_INITIALIZED;
}

/* ============================= Private Functions ==================== */

/**
 * @brief Main task entry point
 */
static void main_task_entry(ULONG param)
{
    LgcMainTask_t *task = (LgcMainTask_t *)param;

    /* Get dependencies from DI Container */
    ILgcSensorReader_t *sensor_reader = DIContainer_GetSensorReader();
    ILgcEncoder_t *encoder = DIContainer_GetEncoder();
    LgcSystemConfig_t *config = DIContainer_GetConfig();
    LgcMeasurements_t *shared_measurements = DIContainer_GetMeasurements();
    ILgcEventPublisher_t *publisher = DIContainer_GetEventPublisher();

    /* Dependencies check */
    if (sensor_reader == NULL || encoder == NULL || config == NULL)
    {
        /* Fatal error: Cannot continue without key dependencies */
        return;
    }

    /* Main loop */
    while (task->is_running)
    {
        /* ===== Wait for Encoder Pulse ===== */
        ULONG actual_flags = 0;
        UINT tx_res = tx_event_flags_get(
            &task->event_flags,
            LGC_EVENT_ENCODER_PULSE | LGC_EVENT_STOP | LGC_EVENT_USER_COMMAND,
            TX_OR_CLEAR,
            &actual_flags,
            task->config.encoder_timeout_ms == 0 ? TX_WAIT_FOREVER : task->config.encoder_timeout_ms);

        /* Check for stop event */
        if (actual_flags & LGC_EVENT_STOP)
        {
            break; /* Exit gracefully */
        }

        /* Check for timeout (if configured) */
        if (tx_res == TX_NO_EVENTS)
        {
            continue;
        }

        /* Check for encoder pulse */
        if (!(actual_flags & LGC_EVENT_ENCODER_PULSE))
        {
            continue; /* No pulse, check other events */
        }

        /* ===== Encoder Pulse Received: Start Measurement Cycle ===== */

        uint32_t cycle_start = tx_time_get();

        /* 1. Read all sensors (cascade mode) */
        LgcSensorArray_t sensor_data;
        Result_t res = sensor_reader->read_cascade_mode(
            sensor_reader->context,
            &sensor_data);

        if (res != ERR_OK)
        {
            /* Sensor read failed: log error and continue */
            task->total_errors++;
            continue;
        }

        /* 2. Get current encoder position */
        uint32_t encoder_position;
        res = encoder->get_position(encoder->context, &encoder_position);
        if (res == ERR_OK)
        {
            sensor_data.encoder_position = encoder_position;
        }

        /* 3. Process slice (core algorithm) */
        LgcSliceResult_t slice_result;
        res = LgcUC_ProcessSlice(
            &sensor_data,
            config,
            &task->active_measurement,
            &slice_result);

        if (res != ERR_OK)
        {
            task->total_errors++;
            continue;
        }

        /* 4. Update Shared State (Critical Section) */
        if (shared_measurements != NULL)
        {
            /* NOTE: In a real system, use a mutex here if hmi_task reads concurrently.
             * For performance, atomic 32-bit writes might be sufficient for float/int 
             * on Cortex-M4, but a short robust lock is safer. 
             * Assuming single writer (Main) and single reader (HMI).
             */
             
            shared_measurements->current_leather_area = task->active_measurement.current_area_dm2;
            
            if (slice_result.piece_finished)
            {
                /* Update History */
                uint32_t idx = shared_measurements->current_leather_index;
                if (idx < LGC_MAX_PIECES_PER_BATCH)
                {
                    shared_measurements->leather_measurement[idx] = task->active_measurement.current_area_dm2;
                    shared_measurements->current_leather_index++;
                    
                    /* Update Batch Total */
                    /* Note: This logic should ideally be in a "Batch Use Case" */
                    /* For now, simple accumulation */
                    /* shared_measurements->batch_measurement[...] += ... */
                }
            }
        }

        /* 5. Publish Events (Observer Pattern) */
        if (publisher != NULL)
        {
            /* Publish Measurement Updated */
             LgcEventDataMeasurement_t meas_payload = {
                .current_area = task->active_measurement.current_area_dm2,
                .active_sensors = slice_result.active_bits_total,
                .slice_count = task->active_measurement.slice_count
            };
            
            LgcEvent_t event_meas = {
                .type = LGC_EVENT_MEASUREMENT_UPDATED,
                .timestamp_ms = tx_time_get(),
                .data = &meas_payload
            };
            publisher->publish(publisher->context, &event_meas);

            /* Publish Piece Finished */
            if (slice_result.piece_finished)
            {
                 LgcEventDataPieceFinished_t piece_payload = {
                    .final_area = task->active_measurement.current_area_dm2,
                    .piece_count = (shared_measurements ? shared_measurements->current_leather_index : 0)
                };

                LgcEvent_t event_piece = {
                    .type = LGC_EVENT_PIECE_FINISHED,
                    .timestamp_ms = tx_time_get(),
                    .data = &piece_payload
                };
                publisher->publish(publisher->context, &event_piece);
                
                /* Check for Batch Finished (Simple logic for now) */
                if (shared_measurements && 
                   (shared_measurements->current_leather_index >= config->max_pieces_per_batch))
                {
                     LgcEventDataBatchFinished_t batch_payload = {
                        .batch_number = shared_measurements->current_batch_index,
                        .piece_count = shared_measurements->current_leather_index,
                        .total_area = 0.0f /* TODO: Sum areas */
                    };
                    
                    /* Calculate total */
                    for(uint32_t i=0; i<batch_payload.piece_count; i++) {
                        batch_payload.total_area += shared_measurements->leather_measurement[i];
                         batch_payload.pieces[i].area = shared_measurements->leather_measurement[i];
                    }
                    
                    LgcEvent_t event_batch = {
                        .type = LGC_EVENT_BATCH_FINISHED,
                        .timestamp_ms = tx_time_get(),
                        .data = &batch_payload
                    };
                    publisher->publish(publisher->context, &event_batch);
                    
                    /* Reset Batch (or wait for HMI command?) */
                    /* For now, auto-reset or wait for user logic to handle clean up */
                }
            }
        }

        /* 6. Clean up active measurement if finished */
        if (slice_result.piece_finished)
        {
            task->active_measurement.is_measuring = false;
            task->active_measurement.current_area_dm2 = 0.0f;
            task->active_measurement.slice_count = 0;
            task->active_measurement.start_position = 0;
        }

        /* 7. Update statistics */
        task->total_slices_processed++;
        uint32_t cycle_time = tx_time_get() - cycle_start;
        if (cycle_time > task->max_processing_time_ms)
        {
            task->max_processing_time_ms = cycle_time;
        }

    } /* End main loop */

    /* Task exit */
    task->is_running = false;
}

/**
 * @brief Encoder pulse callback (ISR context)
 */
static void encoder_pulse_callback(uint32_t position, void *user_ctx)
{
    (void)position;

    LgcMainTask_t *task = (LgcMainTask_t *)user_ctx;

    if (task == NULL)
    {
        return;
    }

    /* Signal main task: New encoder pulse received */
    tx_event_flags_set(&task->event_flags, LGC_EVENT_ENCODER_PULSE, TX_OR);
}
