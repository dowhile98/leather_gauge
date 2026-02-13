/**
 * @file    lgc_di_container.h
 * @brief   Dependency Injection Container
 * @author  Clean Architecture Refactor Team
 * @date    2026-02-12
 * @version 1.0.0
 *
 * @details Composition Root - Wires all dependencies and creates system.
 *          This is the ONLY place where concrete implementations are known.
 *
 * @note    APPLICATION LAYER - Knows about ALL layers.
 *
 * Copyright (c) 2026 Leather Gauge Controller Team
 */

#ifndef LGC_DI_CONTAINER_H
#define LGC_DI_CONTAINER_H

#ifdef __cplusplus
extern "C"
{
#endif

/* ============================= Includes ============================= */
#include "../../domain/entities/lgc_common_types.h"

    /* Forward declarations (Interfaces) */
    typedef struct ILgcSensorReader_t ILgcSensorReader_t;
    typedef struct ILgcEncoder_t ILgcEncoder_t;
    typedef struct ILgcStorage_t ILgcStorage_t;
    typedef struct ILgcDisplay_t ILgcDisplay_t;
    typedef struct ILgcPrinter_t ILgcPrinter_t;
    typedef struct ILgcDigitalInputs_t ILgcDigitalInputs_t;
    typedef struct ILgcEventPublisher_t ILgcEventPublisher_t;
    typedef struct ILgcRealTimeClock_t ILgcRealTimeClock_t;
    typedef struct LgcSystemConfig_t LgcSystemConfig_t;
    typedef struct LgcMeasurements_t LgcMeasurements_t;

    /* Forward declarations (Active Objects) */
    typedef struct LgcLwPktAgent_t LgcLwPktAgent_t;

    /* ============================= External Instances ================== */
    /**
     * @brief Global LwPKT Agent instance pointer (for ISR callbacks)
     * @note Points to static instance in lgc_di_container.c after LgcDI_Init()
     */
    extern LgcLwPktAgent_t *g_lwpkt_agent;

    /* ============================= Public API =========================== */
    /**
     * @brief Initialize Dependency Injection Container
     *
     * @details Orchestrates complete system initialization:
     *          1. Create adapter instances (static allocation)
     *          2. Initialize adapters with HAL resources
     *          3. Get interface pointers (DIP)
     *          4. Inject interfaces into use cases
     *          5. Create ThreadX tasks
     *          6. Wire event publishers & observers
     *
     * @return ERR_OK on success, error code otherwise
     *
     * @pre  HAL initialized (clocks, peripheral init)
     * @pre  ThreadX kernel initialized
     * @post Complete system ready to run
     *
     * @note  Called from main() after HAL_Init()
     * @note  This is the COMPOSITION ROOT - all wiring happens here
     */
    Result_t LgcDI_Init(void);

    /**
     * @brief Start system (activate ThreadX scheduler)
     *
     * @return ERR_OK on success
     *
     * @pre  LgcDI_Init() called successfully
     * @post ThreadX scheduler running, tasks active
     *
     * @note  This function should NOT return (scheduler takes over)
     */
    Result_t LgcDI_Start(void);

    /**
     * @brief Shutdown system (graceful cleanup)
     *
     * @return ERR_OK on success
     *
     * @note  Called on factory reset or error recovery
     */
    Result_t LgcDI_Shutdown(void);

    /* ============================= Dependency Getters =================== */
    /**
     * @brief Get sensor reader interface (for injection)
     *
     * @return Pointer to ISensorReader interface (never NULL after init)
     *
     * @pre  LgcDI_Init() called successfully
     *
     * @note  Used by tasks/use cases to access sensor functionality
     */
    ILgcSensorReader_t *DIContainer_GetSensorReader(void);

    /**
     * @brief Get encoder interface (for injection)
     *
     * @return Pointer to IEncoder interface (never NULL after init)
     *
     * @pre  LgcDI_Init() called successfully
     */
    ILgcEncoder_t *DIContainer_GetEncoder(void);

    /**
     * @brief Get storage interface (for injection)
     *
     * @return Pointer to IStorage interface (never NULL after init)
     *
     * @pre  LgcDI_Init() called successfully
     */
    ILgcStorage_t *DIContainer_GetStorage(void);

    /**
     * @brief Get display interface (for injection)
     *
     * @return Pointer to IDisplay interface (NULL if not configured)
     *
     * @pre  LgcDI_Init() called successfully
     */
    ILgcDisplay_t *DIContainer_GetDisplay(void);

    /**
     * @brief Get printer interface (for injection)
     *
     * @return Pointer to IPrinter interface (NULL if not configured)
     *
     * @pre  LgcDI_Init() called successfully
     */
    ILgcPrinter_t *DIContainer_GetPrinter(void);

    /**
     * @brief Get digital inputs interface (for injection)
     *
     * @return Pointer to IDigitalInputs interface (never NULL after init)
     *
     * @pre  LgcDI_Init() called successfully
     */
    ILgcDigitalInputs_t *DIContainer_GetDigitalInputs(void);

    /**
     * @brief Get system configuration (for injection)
     *
     * @return Pointer to system config (never NULL after init)
     *
     * @pre  LgcDI_Init() called successfully
     * @post Configuration loaded from EEPROM or defaults
     *
     * @note  Config is loaded from EEPROM on init, with CRC validation
     */
    LgcSystemConfig_t *DIContainer_GetConfig(void);

    /**
     * @brief Get event publisher interface (for injection)
     *
     * @return Pointer to IEventPublisher interface (never NULL after init)
     *
     * @pre  LgcDI_Init() called successfully
     */
    ILgcEventPublisher_t *DIContainer_GetEventPublisher(void);

    /**
     * @brief Get real-time clock interface (for injection)
     *
     * @return Pointer to IRealTimeClock interface (never NULL after init)
     *
     * @pre  LgcDI_Init() called successfully
     */
    ILgcRealTimeClock_t *DIContainer_GetRealTimeClock(void);

    /**
     * @brief Get measurements data structure (for injection)
     *
     * @return Pointer to measurements structure (never NULL after init)
     *
     * @pre  LgcDI_Init() called successfully
     */
    LgcMeasurements_t *DIContainer_GetMeasurements(void);

#ifdef __cplusplus
}
#endif

#endif /* LGC_DI_CONTAINER_H */