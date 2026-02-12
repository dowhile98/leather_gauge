/**
 * @file    lgc_lwpkt_agent.h
 * @brief   LwPKT Active Object - Zero-Polling Communication Agent
 * @author  Clean Architecture Refactor Team (OSAL-based)
 * @date    2026-02-12
 * @version 2.0.0
 *
 * @details Implements Active Object pattern for LwPKT protocol using OSAL abstraction:
 *          - ISR-driven RX (semaphore signaling)
 *          - Queue-based TX (command queue)
 *          - Zero CPU usage when idle (blocking on semaphore)
 *          - Thread-safe message passing
 *
 * **Architecture:**
 *   ```
 *   [UART ISR] --data--> [Ring Buffer] --semaphore--> [Comm Task]
 *                                                           |
 *                                                 lwpkt_process() loop
 *                                                           |
 *   [User Code] --command--> [TX Queue] <----- osReceiveFromQueue()
 *   ```
 *
 * **OSAL Primitives Used:**
 *   - OsTask: Main communication task (priority configurable)
 *   - OsSemaphore: Binary semaphore for ISR → Task signaling
 *   - OsQueue: Message queue for TX commands
 *   - systime_t: OSAL tick count for timeouts
 *
 * Copyright (c) 2026 Leather Gauge Controller Team
 */

#ifndef LGC_LWPKT_AGENT_H
#define LGC_LWPKT_AGENT_H

#ifdef __cplusplus
extern "C"
{
#endif

/* ============================= Includes ============================= */
#include "../../domain/interfaces/lgc_i_sensor_reader.h"
#include "../../Third_Party/lwpkt/src/include/lwpkt/lwpkt.h"
#include "../../Third_Party/lwrb/src/include/lwrb/lwrb.h"
#include "os_port.h" /* OSAL abstraction (from leather_gauge_controller/osal/) */
#include "error.h"   /* OSAL error types */

/* Hardware dependencies (only in adapter layer) */
#include "stm32f4xx_hal.h"

/* ============================= Constants ============================ */
/** Task priority (normal = TX_MAX_PRIORITIES/2) */
#ifndef LGC_LWPKT_TASK_PRIORITY
#define LGC_LWPKT_TASK_PRIORITY OS_TASK_PRIORITY_NORMAL
#endif

/** Task stack size (words) */
#ifndef LGC_LWPKT_TASK_STACK_SIZE
#define LGC_LWPKT_TASK_STACK_SIZE 512U
#endif

/** TX command queue size (messages) */
#ifndef LGC_LWPKT_TX_QUEUE_SIZE
#define LGC_LWPKT_TX_QUEUE_SIZE 8U
#endif

/** Ring buffer sizes */
#define LGC_LWPKT_RX_BUFFER_SIZE 256U
#define LGC_LWPKT_TX_BUFFER_SIZE 256U

/** Command timeout (ms) */
#define LGC_LWPKT_CMD_TIMEOUT_MS 1000U

/* ============================= Types ================================ */
/**
 * @brief TX command types (for message queue)
 */
typedef enum
{
    LWPKT_CMD_READ_CASCADE = 0x01,     /**< Read all sensors (cascade mode) */
    LWPKT_CMD_READ_SINGLE = 0x02,      /**< Read single sensor */
    LWPKT_CMD_WRITE_CONFIG = 0x03,     /**< Write sensor configuration */
    LWPKT_CMD_RESET = 0x04,            /**< Reset communication */
} LgcLwPktCommandType_t;

/**
 * @brief TX command result callback
 * @param[in] result    Command result (ERR_OK or error code)
 * @param[in] data      Response data (if applicable)
 * @param[in] data_len  Response data length
 * @param[in] user_ctx  User context from command request
 */
typedef void (*LgcLwPktCallback_t)(
    error_t result,
    const uint8_t *data,
    uint16_t data_len,
    void *user_ctx);

/**
 * @brief TX command message (for queue)
 */
typedef struct
{
    LgcLwPktCommandType_t type;    /**< Command type */
    uint8_t addr;                  /**< Destination address */
    uint8_t payload[32];           /**< Command payload */
    uint16_t payload_len;          /**< Payload length */
    LgcLwPktCallback_t callback;   /**< Result callback (optional) */
    void *callback_ctx;            /**< User context for callback */
    systime_t timeout_ms;          /**< Command timeout */
} LgcLwPktCommand_t;

/**
 * @brief LwPKT Agent context (Active Object state)
 */
typedef struct
{
    /* Hardware resources */
    UART_HandleTypeDef *huart; /**< UART handle */

    /* LwPKT protocol */
    lwpkt_t lwpkt;  /**< LwPKT instance */
    lwrb_t rx_rb;   /**< RX ring buffer */
    lwrb_t tx_rb;   /**< TX ring buffer */
    uint8_t rx_buffer[LGC_LWPKT_RX_BUFFER_SIZE]; /**< RX DMA buffer */
    uint8_t tx_buffer[LGC_LWPKT_TX_BUFFER_SIZE]; /**< TX buffer */

    /* OSAL primitives */
    OsTaskId task_id;               /**< Task handle */
    OsSemaphore rx_data_semaphore;  /**< Binary semaphore (ISR → Task) */
    OsQueue tx_cmd_queue;           /**< TX command queue */
    uint32_t task_stack[LGC_LWPKT_TASK_STACK_SIZE]; /**< Task stack */

    /* State */
    bool is_initialized;        /**< Initialization flag */
    bool is_running;            /**< Task running flag */
    uint32_t rx_count;          /**< RX packet counter (diagnostics) */
    uint32_t tx_count;          /**< TX packet counter (diagnostics) */
    uint32_t error_count;       /**< Error counter (diagnostics) */

    /* Pending command (for TX synchronization) */
    bool is_command_pending;    /**< Command in progress flag */
    LgcLwPktCommand_t active_cmd; /**< Current active command */

} LgcLwPktAgent_t;

/* ============================= Public API =========================== */
/**
 * @brief Initialize LwPKT Agent (Active Object)
 *
 * @param[in,out] agent Pointer to agent context
 * @param[in]     huart UART handle (must be initialized by HAL)
 * @return ERR_OK on success, error code otherwise
 *
 * @pre  UART configured: DMA RX circular mode, TX polling/DMA
 * @pre  OSAL kernel initialized (osInitKernel called)
 * @post Agent task created and started
 * @post Semaphore and queue created
 *
 * @note This function creates the task but does NOT start UART DMA.
 *       Call LgcLwPktAgent_Start() to begin reception.
 */
error_t LgcLwPktAgent_Init(LgcLwPktAgent_t *agent, UART_HandleTypeDef *huart);

/**
 * @brief Start LwPKT Agent (begin UART DMA reception)
 *
 * @param[in,out] agent Pointer to agent context
 * @return ERR_OK on success, error code otherwise
 *
 * @pre  LgcLwPktAgent_Init() called successfully
 * @post UART DMA reception started
 * @post Agent task waiting for RX data (blocking on semaphore)
 */
error_t LgcLwPktAgent_Start(LgcLwPktAgent_t *agent);

/**
 * @brief Stop LwPKT Agent (stop UART DMA, suspend task)
 *
 * @param[in,out] agent Pointer to agent context
 * @return ERR_OK on success, error code otherwise
 *
 * @post UART DMA stopped
 * @post Agent task suspended
 */
error_t LgcLwPktAgent_Stop(LgcLwPktAgent_t *agent);

/**
 * @brief Deinitialize LwPKT Agent (cleanup resources)
 *
 * @param[in,out] agent Pointer to agent context
 * @return ERR_OK on success, error code otherwise
 *
 * @post Task deleted
 * @post Semaphore and queue deleted
 */
error_t LgcLwPktAgent_Deinit(LgcLwPktAgent_t *agent);

/* ============================= TX Command API ======================= */
/**
 * @brief Send READ_CASCADE command (read all 11 sensors)
 *
 * @param[in,out] agent       Pointer to agent context
 * @param[out]    out_data    Buffer for sensor data
 * @param[in]     timeout_ms  Command timeout (ms)
 * @return ERR_OK on success, ERR_TIMEOUT, ERR_BUSY, etc.
 *
 * @pre  Agent initialized and started
 * @post Command enqueued to TX queue
 * @post Task will execute command when scheduled
 *
 * @note This function blocks until command completes or timeout
 * @note Thread-safe: Can be called from multiple tasks
 */
error_t LgcLwPktAgent_SendReadCascade(
    LgcLwPktAgent_t *agent,
    LgcSensorArray_t *out_data,
    systime_t timeout_ms);

/**
 * @brief Send command asynchronously (non-blocking with callback)
 *
 * @param[in,out] agent    Pointer to agent context
 * @param[in]     cmd      Command structure
 * @return ERR_OK if enqueued, ERR_BUFFER_FULL if queue full
 *
 * @pre  cmd.callback must not be NULL
 * @post Command enqueued to TX queue
 * @post Callback invoked when command completes
 *
 * @note Non-blocking: Returns immediately after enqueue
 * @note Callback executed in agent task context (not ISR)
 */
error_t LgcLwPktAgent_SendCommandAsync(
    LgcLwPktAgent_t *agent,
    const LgcLwPktCommand_t *cmd);

/* ============================= ISR Interface ======================== */
/**
 * @brief RX ISR callback (call from HAL_UART_RxCpltCallback)
 *
 * @param[in,out] agent Pointer to agent context
 * @param[in]     data  Received data
 * @param[in]     len   Data length
 *
 * @note ISR-safe: Uses osSetEventBitsFromIsr or equivalent
 * @note Fast execution: Only pushes to ring buffer and signals semaphore
 *
 * @example
 * ```c
 * void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
 * {
 *     if (huart == &huart2)  // LwPKT UART
 *     {
 *         extern LgcLwPktAgent_t g_lwpkt_agent;
 *         LgcLwPktAgent_RxISRCallback(&g_lwpkt_agent, rx_buffer, rx_len);
 *     }
 * }
 * ```
 */
void LgcLwPktAgent_RxISRCallback(
    LgcLwPktAgent_t *agent,
    const uint8_t *data,
    uint16_t len);

/* ============================= Diagnostics ========================== */
/**
 * @brief Get agent statistics
 *
 * @param[in]  agent     Pointer to agent context
 * @param[out] rx_count  RX packet counter
 * @param[out] tx_count  TX packet counter
 * @param[out] err_count Error counter
 * @return ERR_OK on success
 */
error_t LgcLwPktAgent_GetStats(
    const LgcLwPktAgent_t *agent,
    uint32_t *rx_count,
    uint32_t *tx_count,
    uint32_t *err_count);

#ifdef __cplusplus
}
#endif

#endif /* LGC_LWPKT_AGENT_H */
