/**
 * @file lgc_modbus_task.c
 * @brief Asynchronous Modbus Task implementation
 *
 * Implements non-blocking Modbus RTU communication using:
 * - State machine (FSM) for sequential sensor polling
 * - DMA + IDLE line detection for efficient I/O
 * - Semaphore-based event signaling from ISRs
 *
 * @date Created: Feb 13, 2026
 * @author GitHub Copilot
 */

/* ============================================================================
 * INCLUDES
 * ============================================================================ */
#include "lgc_modbus_task.h"
#include "lgc_interface_modbus.h"
#include "os_port.h"
#include <string.h>

/* ============================================================================
 * PRIVATE DEFINES
 * ============================================================================ */
#define PDU_BUFFER_SIZE 32

/* ============================================================================
 * PRIVATE VARIABLES
 * ============================================================================ */
static OsSemaphore s_rx_semaphore;
static OsSemaphore s_tx_semaphore;
static OsSemaphore s_trigger_semaphore;
static OsEvent s_fsm_events;
static uint8_t s_pdu_rx_buffer[PDU_BUFFER_SIZE];
static uint8_t s_pdu_tx_buffer[PDU_BUFFER_SIZE];
static LgcModbusTask_t *s_task_instance = NULL;

/* ============================================================================
 * PRIVATE FUNCTION PROTOTYPES
 * ============================================================================ */
static void fsm_transition(LgcModbusTask_t *task, LgcModbusFsmState_t new_state);
static void fsm_process_idle(LgcModbusTask_t *task);
static void fsm_process_send_req(LgcModbusTask_t *task);
static void fsm_process_wait_rx(LgcModbusTask_t *task);
static void fsm_process_parse(LgcModbusTask_t *task);
static void fsm_process_next(LgcModbusTask_t *task);
static void fsm_process_error(LgcModbusTask_t *task);
static void update_statistics(LgcModbusTask_t *task, bool cycle_complete);

/* ============================================================================
 * PUBLIC FUNCTION DEFINITIONS
 * ============================================================================ */

/**
 * @brief Initialize Modbus task
 */
error_t LgcModbusTask_Init(
    LgcModbusTask_t *task,
    LgcSensorCache_t *sensor_cache,
    const LgcModbusFsmConfig_t *config)
{
    if (task == NULL || sensor_cache == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    /* Store global instance for ISR access */
    s_task_instance = task;

    /* Link sensor cache */
    task->sensor_cache = sensor_cache;

    /* Apply configuration */
    if (config != NULL)
    {
        task->config = *config;
    }
    else
    {
        /* Use defaults */
        LgcModbusFsmConfig_t defaults = LGC_MODBUS_FSM_CONFIG_DEFAULT;
        task->config = defaults;
    }

    /* Initialize FSM context */
    memset(&task->fsm, 0, sizeof(LgcModbusFsmContext_t));
    task->fsm.state = MB_STATE_IDLE;

    /* Initialize statistics */
    memset(&task->stats, 0, sizeof(LgcModbusFsmStats_t));
    task->stats.min_cycle_time_ms = UINT32_MAX;

    /* Create synchronization primitives */
    if (osCreateSemaphore(&s_rx_semaphore, 0) != TRUE)
    {
        return ERROR_FAILURE;
    }

    if (osCreateSemaphore(&s_tx_semaphore, 0) != TRUE)
    {
        return ERROR_FAILURE;
    }

    if (osCreateSemaphore(&s_trigger_semaphore, 0) != TRUE)
    {
        return ERROR_FAILURE;
    }

    /* Event flags for FSM signaling */
    if (osCreateEvent(&s_fsm_events) != TRUE)
    {
        return ERROR_FAILURE;
    }

    task->is_running = false;

    return NO_ERROR;
}

/**
 * @brief Start Modbus task
 */
error_t LgcModbusTask_Start(LgcModbusTask_t *task)
{
    if (task == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    task->is_running = true;

    return NO_ERROR;
}

/**
 * @brief Stop Modbus task
 */
error_t LgcModbusTask_Stop(LgcModbusTask_t *task)
{
    if (task == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    task->is_running = false;

    return NO_ERROR;
}

/**
 * @brief Trigger a read cycle
 */
void LgcModbusTask_TriggerCycle(LgcModbusTask_t *task)
{
    if (task == NULL || !task->is_running)
    {
        return;
    }

    /* Non-blocking release - if already pending, this is a no-op */
    osReleaseSemaphore(&s_trigger_semaphore);
}

/**
 * @brief Get task statistics
 */
error_t LgcModbusTask_GetStats(
    const LgcModbusTask_t *task,
    LgcModbusFsmStats_t *stats)
{
    if (task == NULL || stats == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    *stats = task->stats;

    return NO_ERROR;
}

/**
 * @brief Signal RX complete from UART ISR
 */
void LgcModbusTask_SignalRxComplete(void)
{
    /* ThreadX tx_semaphore_put is ISR-safe */
    osReleaseSemaphore(&s_rx_semaphore);
}

/**
 * @brief Signal TX complete from UART ISR
 */
void LgcModbusTask_SignalTxComplete(void)
{
    /* ThreadX tx_semaphore_put is ISR-safe */
    osReleaseSemaphore(&s_tx_semaphore);
}

/**
 * @brief Task entry point
 */
void lgc_modbus_task_entry(void *param)
{
    LgcModbusTask_t *task = (LgcModbusTask_t *)param;

    if (task == NULL)
    {
        return;
    }

    for (;;)
    {
        /* Process FSM based on current state */
        switch (task->fsm.state)
        {
        case MB_STATE_IDLE:
            fsm_process_idle(task);
            break;

        case MB_STATE_SEND_REQ:
            fsm_process_send_req(task);
            break;

        case MB_STATE_WAIT_RX:
            fsm_process_wait_rx(task);
            break;

        case MB_STATE_PARSE:
            fsm_process_parse(task);
            break;

        case MB_STATE_NEXT:
            fsm_process_next(task);
            break;

        case MB_STATE_ERROR:
            fsm_process_error(task);
            break;

        default:
            fsm_transition(task, MB_STATE_IDLE);
            break;
        }
    }
}

/* ============================================================================
 * PRIVATE FUNCTION DEFINITIONS
 * ============================================================================ */

/**
 * @brief FSM state transition
 */
static void fsm_transition(LgcModbusTask_t *task, LgcModbusFsmState_t new_state)
{
    task->fsm.state = new_state;
    task->fsm.state_entry_ms = osGetSystemTime();
}

/**
 * @brief Process IDLE state - wait for trigger
 */
static void fsm_process_idle(LgcModbusTask_t *task)
{
    /* Wait for trigger signal (from encoder or timer) */
    if (osWaitForSemaphore(&s_trigger_semaphore, task->config.cycle_period_ms) == TRUE)
    {
        /* Begin new read cycle */
        task->fsm.current_sensor_idx = 0;
        task->fsm.sensors_completed = 0;
        task->fsm.cycle_start_ms = osGetSystemTime();

        /* Notify sensor cache */
        LgcSensorCache_BeginCycle(task->sensor_cache, task->fsm.cycle_start_ms);

        /* Transition to SEND_REQ */
        fsm_transition(task, MB_STATE_SEND_REQ);
    }
}

/**
 * @brief Process SEND_REQ state - transmit Modbus request
 */
static void fsm_process_send_req(LgcModbusTask_t *task)
{
    uint8_t sensor_id = task->fsm.current_sensor_idx;
    uint8_t modbus_addr = task->config.modbus_address_start + sensor_id;

    /* Check if sensor is marked offline - skip if so */
    if (task->fsm.error_tracker[sensor_id].is_offline)
    {
        /* Update cache with offline status */
        LgcSensorCache_UpdateSensor(
            task->sensor_cache,
            sensor_id,
            0,
            LGC_SENSOR_OFFLINE,
            osGetSystemTime());

        /* Skip to next sensor */
        fsm_transition(task, MB_STATE_NEXT);
        return;
    }

    /* Set Modbus address using wrapper function */
    lgc_modbus_set_address(modbus_addr);

    /* Prepare PDU for Read Holding Registers (FC 03)
     * Format: [RegAddrHi][RegAddrLo][QuantityHi][QuantityLo]
     */
    s_pdu_tx_buffer[0] = (uint8_t)(task->config.modbus_register >> 8);
    s_pdu_tx_buffer[1] = (uint8_t)(task->config.modbus_register & 0xFF);
    s_pdu_tx_buffer[2] = 0x00; /* Quantity high byte */
    s_pdu_tx_buffer[3] = 0x01; /* Quantity low byte (1 register) */

    /* Send request using wrapper function (non-blocking) */
    error_t err = lgc_modbus_send_raw_pdu(0x03, s_pdu_tx_buffer, 4);

    if (err == NO_ERROR)
    {
        /* Transition to wait for response */
        fsm_transition(task, MB_STATE_WAIT_RX);
    }
    else
    {
        /* Transmission error - go to error state */
        fsm_transition(task, MB_STATE_ERROR);
    }
}

/**
 * @brief Process WAIT_RX state - wait for response with timeout
 */
static void fsm_process_wait_rx(LgcModbusTask_t *task)
{
    /* Wait for RX semaphore with timeout */
    if (osWaitForSemaphore(&s_rx_semaphore, task->config.rx_timeout_ms) == TRUE)
    {
        /* Response received - go parse */
        fsm_transition(task, MB_STATE_PARSE);
    }
    else
    {
        /* Timeout - mark sensor fault */
        uint8_t sensor_id = task->fsm.current_sensor_idx;
        task->fsm.error_tracker[sensor_id].consecutive_failures++;
        task->stats.total_timeouts++;

        /* Check if should mark offline */
        if (task->fsm.error_tracker[sensor_id].consecutive_failures >= task->config.max_failures)
        {
            task->fsm.error_tracker[sensor_id].is_offline = 1;
        }

        /* Update cache with timeout status */
        LgcSensorCache_UpdateSensor(
            task->sensor_cache,
            sensor_id,
            0,
            LGC_SENSOR_TIMEOUT,
            osGetSystemTime());

        /* Move to next sensor */
        fsm_transition(task, MB_STATE_NEXT);
    }
}

/**
 * @brief Process PARSE state - decode received response
 */
static void fsm_process_parse(LgcModbusTask_t *task)
{
    uint8_t sensor_id = task->fsm.current_sensor_idx;

    /* Receive and parse PDU using wrapper function */
    error_t err = lgc_modbus_receive_raw_pdu(s_pdu_rx_buffer, PDU_BUFFER_SIZE);

    if (err == NO_ERROR)
    {
        /* Extract register value from response
         * Response format: [FC][ByteCount][DataHi][DataLo]
         */
        uint16_t value = ((uint16_t)s_pdu_rx_buffer[1] << 8) | s_pdu_rx_buffer[2];

        /* Update cache with valid data */
        LgcSensorCache_UpdateSensor(
            task->sensor_cache,
            sensor_id,
            value,
            LGC_SENSOR_HEALTHY,
            osGetSystemTime());

        /* Reset error tracker on success */
        task->fsm.error_tracker[sensor_id].consecutive_failures = 0;
        task->fsm.error_tracker[sensor_id].is_offline = 0;
        task->fsm.error_tracker[sensor_id].last_success_ms = osGetSystemTime();
    }
    else
    {
        /* Parse error (CRC or other) */
        if (err == ERROR_BAD_CRC)
        {
            task->stats.total_crc_errors++;
        }
        task->fsm.error_tracker[sensor_id].consecutive_failures++;

        /* Update cache with error status */
        LgcSensorCache_UpdateSensor(
            task->sensor_cache,
            sensor_id,
            0,
            (err == ERROR_BAD_CRC) ? LGC_SENSOR_CRC_ERROR : LGC_SENSOR_TIMEOUT,
            osGetSystemTime());
    }

    /* Transition to NEXT */
    fsm_transition(task, MB_STATE_NEXT);
}

/**
 * @brief Process NEXT state - advance to next sensor or complete cycle
 */
static void fsm_process_next(LgcModbusTask_t *task)
{
    task->fsm.sensors_completed++;
    task->fsm.current_sensor_idx++;

    /* Check if all sensors done */
    if (task->fsm.current_sensor_idx >= LGC_MODBUS_SENSOR_COUNT)
    {
        /* Cycle complete */
        uint32_t cycle_end_ms = osGetSystemTime();

        /* Notify sensor cache */
        LgcSensorCache_EndCycle(task->sensor_cache, cycle_end_ms);

        /* Update statistics */
        update_statistics(task, true);

        /* Return to IDLE */
        fsm_transition(task, MB_STATE_IDLE);
    }
    else
    {
        /* Inter-frame delay (T3.5) */
        osDelayTask(task->config.t35_delay_ms);

        /* Continue with next sensor */
        fsm_transition(task, MB_STATE_SEND_REQ);
    }
}

/**
 * @brief Process ERROR state - recovery
 */
static void fsm_process_error(LgcModbusTask_t *task)
{
    /* Log error and attempt recovery */
    uint8_t sensor_id = task->fsm.current_sensor_idx;
    task->fsm.error_tracker[sensor_id].consecutive_failures++;

    /* Update cache with error */
    LgcSensorCache_UpdateSensor(
        task->sensor_cache,
        sensor_id,
        0,
        LGC_SENSOR_OFFLINE,
        osGetSystemTime());

    /* Small delay for stability */
    osDelayTask(5);

    /* Try next sensor */
    fsm_transition(task, MB_STATE_NEXT);
}

/**
 * @brief Update statistics after cycle
 */
static void update_statistics(LgcModbusTask_t *task, bool cycle_complete)
{
    if (!cycle_complete)
    {
        return;
    }

    uint32_t cycle_time = osGetSystemTime() - task->fsm.cycle_start_ms;

    task->stats.total_cycles++;
    task->stats.last_cycle_time_ms = cycle_time;

    if (cycle_time < task->stats.min_cycle_time_ms)
    {
        task->stats.min_cycle_time_ms = cycle_time;
    }

    if (cycle_time > task->stats.max_cycle_time_ms)
    {
        task->stats.max_cycle_time_ms = cycle_time;
    }
}
