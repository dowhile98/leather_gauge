/**
 * @file    lgc_display_adapter.h
 * @brief   DWIN Display Adapter - Interface
 * @author  Clean Architecture Refactor Team (TDD)
 * @date    2026-02-12
 * @version 1.0.0
 *
 * @details Implements ILgcDisplay_t using DWIN DGUS II protocol.
 *          Provides event-driven button handling and VP access.
 *
 * @note    ADAPTER LAYER - CAN include HAL headers.
 *
 * Copyright (c) 2026 Leather Gauge Controller Team
 */

#ifndef LGC_DISPLAY_ADAPTER_H
#define LGC_DISPLAY_ADAPTER_H

#ifdef __cplusplus
extern "C"
{
#endif

/* ============================= Includes ============================= */
#include "../../domain/interfaces/lgc_i_display.h"
#include "../../Third_Party/dwin/dwin_core.h"
#include "stm32f4xx_hal.h"
#include "os_port.h"

    /* ============================= Constants ============================ */
    /** Task stack size (words) */
#ifndef LGC_DISPLAY_TASK_STACK_SIZE
#define LGC_DISPLAY_TASK_STACK_SIZE 512U
#endif

    /** Task priority */
#ifndef LGC_DISPLAY_TASK_PRIORITY
#define LGC_DISPLAY_TASK_PRIORITY 10U
#endif

    /* ============================= Types ================================ */
    /**
     * @brief DWIN Display Adapter Context
     */
    typedef struct
    {
        /* DWIN driver instance */
        dwin_t dwin;

        /* Hardware resources */
        UART_HandleTypeDef *huart; /**< UART handle for display */

        /* Ring buffer for DMA reception */
        uint8_t rx_buffer[256];      /**< Internal DWIN FIFO buffer */
        uint8_t uart_rx_buffer[128]; /**< Raw UART DMA reception buffer */

        /* OS Primitives for DWIN HAL */
        OsMutex mutex;            /**< Mutex for thread-safe access */
        OsSemaphore response_sem; /**< Semaphore for command response */
        OsSemaphore new_data_sem; /**< Semaphore for new data notification */
        OsSemaphore tx_sem;       /**< Semaphore for UART TX completion */

        /* Task resources (Active Object) */
        OsTaskId task_id;                                /**< Task handle */
        uint32_t task_stack[LGC_DISPLAY_TASK_STACK_SIZE]; /**< Task stack */

        /* Configuration */
        LgcDisplayConfig_t config;

        /* State */
        bool is_initialized;
        bool is_running;

        /* Button event callback */
        LgcDisplayCallback_t event_callback;
        void *event_user_ctx;

    } LgcDisplayAdapter_t;

    /* ============================= Public API =========================== */
    /**
     * @brief Initialize DWIN display adapter
     *
     * @param[in,out] adapter Pointer to adapter context
     * @param[in]     huart   UART handle (must be initialized by HAL)
     * @return ERR_OK on success
     *
     * @pre  adapter and huart must not be NULL
     * @pre  UART must be initialized (MX_USART_Init)
     * @post adapter->is_initialized == true
     *
     * @note This function only initializes the adapter struct.
     *       Call display->init() to initialize DWIN protocol.
     */
    Result_t LgcDisplayAdapter_Init(
        LgcDisplayAdapter_t *adapter,
        UART_HandleTypeDef *huart);

    /**
     * @brief Start display processing task (Active Object)
     *
     * @param[in,out] adapter Pointer to adapter context
     * @return ERR_OK on success
     */
    Result_t LgcDisplayAdapter_Start(LgcDisplayAdapter_t *adapter);

    /**
     * @brief Stop display processing task
     *
     * @param[in,out] adapter Pointer to adapter context
     * @return ERR_OK on success
     */
    Result_t LgcDisplayAdapter_Stop(LgcDisplayAdapter_t *adapter);

    /**
     * @brief Get display interface (V-Table)
     *
     * @param[in] adapter Pointer to adapter context
     * @return Pointer to ILgcDisplay_t interface
     *
     * @pre  adapter must not be NULL
     * @pre  adapter->is_initialized == true
     *
     * @note Returns static interface with context set to adapter
     */
    ILgcDisplay_t *LgcDisplayAdapter_GetInterface(LgcDisplayAdapter_t *adapter);

    /**
     * @brief Deinitialize display adapter
     *
     * @param[in,out] adapter Pointer to adapter context
     * @return ERR_OK on success
     */
    Result_t LgcDisplayAdapter_Deinit(LgcDisplayAdapter_t *adapter);

    /**
     * @brief Push data from UART ISR to ring buffer
     *
     * @param[in,out] adapter Pointer to adapter context
     * @param[in]     data    Data buffer from ISR
     * @param[in]     len     Data length
     *
     * @note Call from HAL_UART_RxCpltCallback (ISR context)
     * @note Safe to call from ISR
     */
    void LgcDisplayAdapter_RxISRCallback(
        LgcDisplayAdapter_t *adapter,
        uint8_t *data,
        uint16_t len);

#ifdef __cplusplus
}
#endif

#endif /* LGC_DISPLAY_ADAPTER_H */
