/**
 * @file lgc_modbus_task.h
 * @brief Asynchronous Modbus Task header
 *
 * Dedicated task for non-blocking Modbus communication using
 * a state machine (FSM) and DMA+IDLE line detection.
 *
 * @date Created: Feb 13, 2026
 * @author GitHub Copilot
 */

#ifndef APP_LGC_MODBUS_TASK_H_
#define APP_LGC_MODBUS_TASK_H_

/* ============================================================================
 * INCLUDES
 * ============================================================================ */
#include "error.h"
#include "lgc_modbus_fsm_types.h"
#include "lgc_sensor_cache.h"

/* ============================================================================
 * CONFIGURATION
 * ============================================================================ */

/** @brief Modbus task priority (higher than Main task) */
#ifndef LGC_MODBUS_TASK_PRI
#define LGC_MODBUS_TASK_PRI 9
#endif

/** @brief Modbus task stack size (words) */
#ifndef LGC_MODBUS_TASK_STACK
#define LGC_MODBUS_TASK_STACK 256
#endif

/* ============================================================================
 * TYPE DEFINITIONS
 * ============================================================================ */

/**
 * @brief Modbus Task Handle
 */
typedef struct
{
    LgcModbusFsmContext_t fsm;      /**< FSM runtime context */
    LgcModbusFsmConfig_t config;    /**< Configuration */
    LgcModbusFsmStats_t stats;      /**< Statistics */
    LgcSensorCache_t *sensor_cache; /**< Pointer to shared cache */
    bool is_running;                /**< Task running flag */
} LgcModbusTask_t;

/* ============================================================================
 * PUBLIC FUNCTION PROTOTYPES
 * ============================================================================ */

/**
 * @brief Initialize Modbus task
 *
 * @param[in,out] task          Task handle
 * @param[in]     sensor_cache  Pointer to sensor cache (for writing)
 * @param[in]     config        FSM configuration (NULL for defaults)
 *
 * @return ERR_OK on success
 */
error_t LgcModbusTask_Init(
    LgcModbusTask_t *task,
    LgcSensorCache_t *sensor_cache,
    const LgcModbusFsmConfig_t *config);

/**
 * @brief Start Modbus task
 *
 * @param[in,out] task Task handle
 *
 * @return ERR_OK on success
 */
error_t LgcModbusTask_Start(LgcModbusTask_t *task);

/**
 * @brief Stop Modbus task
 *
 * @param[in,out] task Task handle
 *
 * @return ERR_OK on success
 */
error_t LgcModbusTask_Stop(LgcModbusTask_t *task);

/**
 * @brief Trigger a read cycle (called from encoder ISR or timer)
 *
 * @param[in] task Task handle
 */
void LgcModbusTask_TriggerCycle(LgcModbusTask_t *task);

/**
 * @brief Get task statistics
 *
 * @param[in]  task  Task handle
 * @param[out] stats Output statistics structure
 *
 * @return ERR_OK on success
 */
error_t LgcModbusTask_GetStats(
    const LgcModbusTask_t *task,
    LgcModbusFsmStats_t *stats);

/**
 * @brief Task entry point (called by RTOS scheduler)
 *
 * @param[in] param Task handle pointer
 */
void lgc_modbus_task_entry(void *param);

/**
 * @brief Signal RX complete from UART ISR
 *
 * Called from UART IDLE line callback to wake FSM.
 */
void LgcModbusTask_SignalRxComplete(void);

/**
 * @brief Signal TX complete from UART ISR
 *
 * Called from DMA TX complete callback.
 */
void LgcModbusTask_SignalTxComplete(void);

#endif /* APP_LGC_MODBUS_TASK_H_ */
