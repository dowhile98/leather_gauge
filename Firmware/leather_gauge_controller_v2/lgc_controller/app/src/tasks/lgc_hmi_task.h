/**
 * @file    lgc_hmi_task.h
 * @brief   HMI Task - User Interface Management
 * @author  Clean Architecture Refactor Team (TDD)
 * @date    2026-02-12
 * @version 1.0.0
 *
 * @details ThreadX task for HMI management:
 *          - Event-driven (NO polling)
 *          - Subscribes to measurement events
 *          - Processes display button events
 *          - Updates display on state changes
 *
 * **Architecture:**
 *   ```
 *   [MeasurementCore] --publish--> [EventPublisher] --notify--> [HMI Task]
 *                                                                     |
 *   [Display Button] --ISR--> [Display Adapter] --callback--> [HMI Task]
 *                                                                     |
 *                                                              [Update Display]
 *   ```
 *
 * **Priority:** 11 (lower than Main Measurement Task priority 10)
 *
 * Copyright (c) 2026 Leather Gauge Controller Team
 */

#ifndef LGC_HMI_TASK_H
#define LGC_HMI_TASK_H

#ifdef __cplusplus
extern "C"
{
#endif

/* ============================= Includes ============================= */
#include "../../domain/interfaces/lgc_i_display.h"
#include "../../domain/interfaces/lgc_i_event_publisher.h"
#include "../../domain/entities/lgc_configuration_entity.h"
#include "../../domain/entities/lgc_measurement_entity.h"
#include "tx_api.h"

/* ============================= Constants ============================ */
/** HMI task priority (lower number = higher priority) */
#define LGC_HMI_TASK_PRIORITY 11U

/** HMI task stack size (words) */
#define LGC_HMI_TASK_STACK_SIZE 512U

/** Update rate (ms) - Only for display process(), not polling */
#define LGC_HMI_UPDATE_RATE_MS 100U

    /* ============================= Types ================================ */
    /**
     * @brief HMI Task Context
     */
    typedef struct
    {
        /* Dependencies (injected) */
        ILgcDisplay_t *display;                /**< Display interface */
        ILgcEventPublisher_t *event_publisher; /**< Event publisher */
        LgcSystemConfig_t *system_config;      /**< System configuration */
        LgcMeasurements_t *measurements;       /**< Measurement data */

        /* ThreadX resources */
        TX_THREAD thread;                                       /**< Task thread */
        uint8_t stack[LGC_HMI_TASK_STACK_SIZE * sizeof(ULONG)]; /**< Task stack */
        TX_EVENT_FLAGS_GROUP events;                            /**< Event flags for button ISR */

        /* Button event queue */
        TX_QUEUE button_queue;
        ULONG button_queue_storage[32]; /**< Queue storage for 32 button events */

        /* State */
        bool is_initialized;
        bool is_running;

    } LgcHmiTask_t;

    /* ============================= Public API =========================== */
    /**
     * @brief Initialize HMI task
     * @param[in,out] task            Pointer to task context
     * @param[in]     display         Display interface (must not be NULL)
     * @param[in]     event_publisher Event publisher interface (must not be NULL)
     * @param[in]     system_config   System configuration (must not be NULL)
     * @param[in]     measurements    Measurement data (must not be NULL)
     * @return ERR_OK on success
     */
    Result_t LgcHmiTask_Init(
        LgcHmiTask_t *task,
        ILgcDisplay_t *display,
        ILgcEventPublisher_t *event_publisher,
        LgcSystemConfig_t *system_config,
        LgcMeasurements_t *measurements);

    /**
     * @brief Start HMI task
     * @param[in,out] task Pointer to task context
     * @return ERR_OK on success
     */
    Result_t LgcHmiTask_Start(LgcHmiTask_t *task);

    /**
     * @brief Stop HMI task
     * @param[in,out] task Pointer to task context
     * @return ERR_OK on success
     */
    Result_t LgcHmiTask_Stop(LgcHmiTask_t *task);

    /**
     * @brief Deinitialize HMI task
     * @param[in,out] task Pointer to task context
     * @return ERR_OK on success
     */
    Result_t LgcHmiTask_Deinit(LgcHmiTask_t *task);

#ifdef __cplusplus
}
#endif

#endif /* LGC_HMI_TASK_H */
