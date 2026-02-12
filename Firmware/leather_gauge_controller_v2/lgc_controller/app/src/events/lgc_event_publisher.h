/**
 * @file    lgc_event_publisher.h
 * @brief   Event Publisher Implementation (Observer Pattern)
 * @author  Clean Architecture Refactor Team (TDD)
 * @date    2026-02-12
 * @version 1.0.0
 *
 * Copyright (c) 2026 Leather Gauge Controller Team
 */

#ifndef LGC_EVENT_PUBLISHER_H
#define LGC_EVENT_PUBLISHER_H

#ifdef __cplusplus
extern "C"
{
#endif

/* ============================= Includes ============================= */
#include "../../domain/interfaces/lgc_i_event_publisher.h"
#include "tx_api.h" /* ThreadX */

    /* ============================= Types ================================ */
    /**
     * @brief Event Publisher Context (ThreadX-based)
     */
    typedef struct
    {
        /* Observer list */
        LgcObserver_t observers[LGC_MAX_OBSERVERS];
        uint8_t observer_count;

        /* Thread-safety */
        TX_MUTEX mutex; /**< Mutex for observer list protection */

        /* State */
        bool is_initialized;

    } LgcEventPublisher_t;

    /* ============================= Public API =========================== */
    /**
     * @brief Initialize event publisher
     * @param[in,out] publisher Pointer to publisher context
     * @return ERR_OK on success
     */
    Result_t LgcEventPublisher_Init(LgcEventPublisher_t *publisher);

    /**
     * @brief Get interface (V-Table)
     * @param[in] publisher Pointer to publisher context
     * @return Pointer to interface
     */
    ILgcEventPublisher_t *LgcEventPublisher_GetInterface(LgcEventPublisher_t *publisher);

    /**
     * @brief Deinitialize publisher
     * @param[in,out] publisher Pointer to publisher context
     * @return ERR_OK on success
     */
    Result_t LgcEventPublisher_Deinit(LgcEventPublisher_t *publisher);

#ifdef __cplusplus
}
#endif

#endif /* LGC_EVENT_PUBLISHER_H */
