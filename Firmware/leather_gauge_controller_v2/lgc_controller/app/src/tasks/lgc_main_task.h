/**
 * @file    lgc_main_task.h
 * @brief   Main Measurement Task - Orchestrates encoder-driven measurement
 * @author  Clean Architecture Refactor Team
 * @date    2026-02-12
 * @version 1.0.0
 *
 * @details Main ThreadX task that:
 *          1. Waits for encoder pulse event
 *          2. Triggers sensor cascade read (550ms)
 *          3. Processes slice (updates measurement)
 *          4. Publishes events to observers (HMI, Printer)
 *
 * @note    APPLICATION LAYER - Composition Root responsibility
 *
 * Copyright (c) 2026 Leather Gauge Controller Team
 */

#ifndef LGC_MAIN_TASK_H
#define LGC_MAIN_TASK_H

#ifdef __cplusplus
extern "C"
{
#endif

/* ============================= Includes ============================= */
#include <stdint.h>
#include "../../domain/entities/lgc_common_types.h"
#include "tx_api.h" /* ThreadX */

/* ============================= Configuration ======================== */
#define LGC_MAIN_TASK_STACK_SIZE 2048     /**< Stack size (words) */
#define LGC_MAIN_TASK_PRIORITY 10         /**< Priority (10 = real-time) */
#define LGC_MAIN_TASK_PREEMPT_THRESHOLD 5 /**< Preemption threshold */

/* Event flags for encoder pulse */
#define LGC_EVENT_ENCODER_PULSE (1 << 0) /**< Encoder pulse received */
#define LGC_EVENT_USER_COMMAND (1 << 1)  /**< User command from HMI */
#define LGC_EVENT_STOP (1 << 2)          /**< Stop task */

    /* ============================= Types ================================ */
    /**
     * @brief Main task configuration
     */
    typedef struct
    {
        uint32_t encoder_timeout_ms; /**< Max time to wait for pulse (0 = infinite) */
        bool enable_diagnostics;     /**< Log timing/performance data */
    } LgcMainTaskConfig_t;

    /**
     * @brief Main task handle (opaque)
     */
    typedef struct LgcMainTask_t LgcMainTask_t;

    /* ============================= Public API =========================== */
    /**
     * @brief Create and start main task
     *
     * @param[in] config Task configuration
     * @return ERR_OK on success
     *
     * @pre  DI Container initialized (adapters wired)
     * @post Main task created and running
     *
     * @note  Call from main() after hardware init
     */
    Result_t LgcMainTask_Start(const LgcMainTaskConfig_t *config);

    /**
     * @brief Stop main task (graceful shutdown)
     *
     * @return ERR_OK on success
     *
     * @post Main task stopped, resources released
     */
    Result_t LgcMainTask_Stop(void);

    /**
     * @brief Get task statistics (diagnostics)
     *
     * @param[out] stats Statistics structure
     * @return ERR_OK on success
     */
    Result_t LgcMainTask_GetStats(void *stats); /* TODO: Define stats structure */

#ifdef __cplusplus
}
#endif

#endif /* LGC_MAIN_TASK_H */
