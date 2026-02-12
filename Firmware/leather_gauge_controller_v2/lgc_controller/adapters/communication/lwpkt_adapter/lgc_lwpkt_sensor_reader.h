/**
 * @file    lgc_lwpkt_sensor_reader.h
 * @brief   ISensorReader Wrapper for LwPKT Agent
 * @author  Clean Architecture Refactor Team (TDD)
 * @date    2026-02-12
 * @version 1.0.0
 *
 * @details Adapter that wraps LwPKT Agent asynchronous API into
 *          synchronous ISensorReader interface using semaphores.
 *
 *          This allows domain layer to use async LwPKT Agent without
 *          knowing about callbacks or async patterns.
 *
 * **Architecture:**
 * ```
 *   [Domain Use Case] ──> [ISensorReader] ◄── implements ── [LwPktSensorReader]
 *                                                                    │
 *                                                                    ▼
 *                                                             [LwPKT Agent]
 *                                                             (Async API)
 * ```
 *
 * **Key Features:**
 * - ✅ **Blocks until response**: Uses semaphore to wait for async callback
 * - ✅ **Thread-safe**: Can be called from multiple tasks
 * - ✅ **Timeout handling**: Configurable timeout per operation
 * - ✅ **Error propagation**: Maps LwPKT errors to Result_t
 *
 * Copyright (c) 2026 Leather Gauge Controller Team
 */

#ifndef LGC_LWPKT_SENSOR_READER_H
#define LGC_LWPKT_SENSOR_READER_H

#ifdef __cplusplus
extern "C"
{
#endif

/* ============================= Includes ============================= */
#include "../../domain/interfaces/lgc_i_sensor_reader.h"
#include "lgc_lwpkt_agent.h"
#include "os_port.h" /* OSAL (ThreadX abstraction) */

    /* ============================= Types ================================ */
    /**
     * @brief LwPKT Sensor Reader Adapter Context
     */
    typedef struct
    {
        /* Dependencies */
        LgcLwPktAgent_t *agent; /**< LwPKT Agent (async API) */

        /* Synchronization (convert async → sync) */
        OsSemaphore completion_sem; /**< Signaled when async operation completes */
        OsMutex mutex;              /**< Protects concurrent access */

        /* Response buffer (from async callback) */
        LgcSensorArray_t response_data; /**< Last received data */
        error_t response_error;         /**< Last operation result */

        /* State */
        bool is_initialized;

    } LgcLwPktSensorReader_t;

    /* ============================= Public API =========================== */
    /**
     * @brief Initialize LwPKT Sensor Reader Adapter
     *
     * @param[in,out] reader Pointer to adapter context
     * @param[in]     agent  LwPKT Agent instance (must be initialized)
     * @return ERR_OK on success
     *
     * @pre  agent must be initialized and started
     * @post Semaphore and mutex created
     * @post Interface ready for use
     */
    Result_t LgcLwPktSensorReader_Init(
        LgcLwPktSensorReader_t *reader,
        LgcLwPktAgent_t *agent);

    /**
     * @brief Deinitialize LwPKT Sensor Reader Adapter
     *
     * @param[in,out] reader Pointer to adapter context
     * @return ERR_OK on success
     *
     * @post Semaphore and mutex deleted
     */
    Result_t LgcLwPktSensorReader_Deinit(LgcLwPktSensorReader_t *reader);

    /**
     * @brief Get ISensorReader interface (V-Table)
     *
     * @param[in] reader Pointer to adapter context
     * @return Pointer to ISensorReader interface
     *
     * @note Use this to inject into domain layer:
     *       ```c
     *       ILgcSensorReader_t *sensor = LgcLwPktSensorReader_GetInterface(&reader);
     *       LgcUC_MeasureArea_Init(&uc, sensor, encoder);
     *       ```
     */
    ILgcSensorReader_t *LgcLwPktSensorReader_GetInterface(LgcLwPktSensorReader_t *reader);

#ifdef __cplusplus
}
#endif

#endif /* LGC_LWPKT_SENSOR_READER_H */
