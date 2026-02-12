/**
 * @file    lgc_lwpkt_adapter.h
 * @brief   LwPKT Communication Adapter (ISensorReader implementation)
 * @author  Clean Architecture Refactor Team
 * @date    2026-02-12
 * @version 1.0.0
 *
 * @details Implements ISensorReader using LwPKT cascade protocol.
 *          67% faster than Modbus RTU (550ms vs 2s for 11 sensors).
 *
 * @note    ADAPTER LAYER - CAN include HAL headers.
 *
 * Protocol:
 *   - Single broadcast command → 11 sequential responses
 *   - DMA + Ring buffer (lwrb) for async reception
 *   - CRC-8 validation per packet
 *
 * Copyright (c) 2026 Leather Gauge Controller Team
 */

#ifndef LGC_LWPKT_ADAPTER_H
#define LGC_LWPKT_ADAPTER_H

#ifdef __cplusplus
extern "C"
{
#endif

/* ============================= Includes ============================= */
#include "../../domain/interfaces/lgc_i_sensor_reader.h"
#include "../../Third_Party/lwpkt/src/include/lwpkt/lwpkt.h"
#include "../../Third_Party/lwrb/src/include/lwrb/lwrb.h"

/* Hardware dependencies (OK in adapter layer) */
#include "stm32f4xx_hal.h"

/* ============================= Constants ============================ */
/** LwPKT frame overhead (SOF + ADDR + CMD + FLAGS + LEN + CRC) */
#define LWPKT_FRAME_OVERHEAD 7U

/** Maximum payload size (sensor data = 2 bytes) */
#define LWPKT_MAX_PAYLOAD 2U

/** Ring buffer size for DMA reception */
#define LWPKT_RX_BUFFER_SIZE 256U

/** Timeout for cascade read (ms) */
#define LWPKT_CASCADE_TIMEOUT_MS 600U

    /* ============================= Types ================================ */
    /**
     * @brief LwPKT adapter context (private implementation)
     */
    typedef struct
    {
        /* Hardware resources */
        UART_HandleTypeDef *huart; /**< UART handle (from HAL) */

        /* LwPKT instance */
        lwpkt_t lwpkt; /**< LwPKT protocol handler */

        /* DMA ring buffers */
        lwrb_t rx_rb;                            /**< Ring buffer for RX */
        uint8_t rx_buffer[LWPKT_RX_BUFFER_SIZE]; /**< RX DMA buffer */
        lwrb_t tx_rb;                            /**< Ring buffer for TX */
        uint8_t tx_buffer[LWPKT_RX_BUFFER_SIZE]; /**< TX buffer (same size) */

        /* State */
        bool is_initialized;       /**< Initialization flag */
        uint32_t last_error_count; /**< Error counter (diagnostics) */

        /* Configuration */
        LgcSensorReaderConfig_t config; /**< Reader configuration */
    } LgcLwPktAdapter_t;

    /* ============================= Public API =========================== */
    /**
     * @brief Initialize LwPKT adapter
     *
     * @param[in,out] adapter Pointer to adapter context
     * @param[in]     huart   UART handle (must be initialized by HAL)
     * @return ERR_OK on success
     *
     * @pre  huart must be initialized (HAL_UART_Init called)
     * @post Adapter ready for read operations
     * @post DMA configured for circular mode
     */
    Result_t LgcLwPktAdapter_Init(
        LgcLwPktAdapter_t *adapter,
        UART_HandleTypeDef *huart);

    /**
     * @brief Get ISensorReader interface (Dependency Inversion)
     *
     * @param[in] adapter Pointer to adapter context
     * @return Pointer to ISensorReader interface (V-Table)
     *
     * @note  Returned interface has adapter as context
     * @note  Used in DI Container for dependency injection
     *
     * Usage:
     * @code
     * static LgcLwPktAdapter_t lwpkt_adapter;
     * LgcLwPktAdapter_Init(&lwpkt_adapter, &huart2);
     *
     * ILgcSensorReader_t *sensor = LgcLwPktAdapter_GetInterface(&lwpkt_adapter);
     * LgcUC_MeasureArea_Init(&measure_uc, sensor, encoder);
     * @endcode
     */
    ILgcSensorReader_t *LgcLwPktAdapter_GetInterface(LgcLwPktAdapter_t *adapter);

    /**
     * @brief Deinitialize LwPKT adapter
     *
     * @param[in,out] adapter Pointer to adapter context
     * @return ERR_OK on success
     *
     * @post DMA stopped, resources released
     */
    Result_t LgcLwPktAdapter_Deinit(LgcLwPktAdapter_t *adapter);

    /* ============================= Callbacks ============================ */
    /**
     * @brief DMA RX complete callback (called from HAL ISR)
     *
     * @param[in] huart UART handle
     *
     * @note  Must be called from HAL_UART_RxCpltCallback()
     * @note  Updates ring buffer write pointer
     */
    void LgcLwPktAdapter_DMA_RxCpltCallback(UART_HandleTypeDef *huart);

    /**
     * @brief DMA RX half-complete callback (called from HAL ISR)
     *
     * @param[in] huart UART handle
     *
     * @note  Must be called from HAL_UART_RxHalfCpltCallback()
     */
    void LgcLwPktAdapter_DMA_RxHalfCpltCallback(UART_HandleTypeDef *huart);

#ifdef __cplusplus
}
#endif

#endif /* LGC_LWPKT_ADAPTER_H */
