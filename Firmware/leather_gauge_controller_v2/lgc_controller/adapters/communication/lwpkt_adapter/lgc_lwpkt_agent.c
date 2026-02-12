/**
 * @file    lgc_lwpkt_agent.c
 * @brief   LwPKT Active Object - Implementation (OSAL-based)
 * @author  Clean Architecture Refactor Team
 * @date    2026-02-12
 * @version 2.0.0
 *
 * Copyright (c) 2026 Leather Gauge Controller Team
 */

/* ============================= Includes ============================= */
#include "lgc_lwpkt_agent.h"
#include <string.h>

/* ============================= Private Macros ======================= */
#define COMM_TASK_WAIT_TIMEOUT_MS 100U /**< Semaphore wait timeout */
#define CASCADE_BROADCAST_ADDR 0xFF    /**< Broadcast address for cascade */
#define CASCADE_START_FLAGS 1          /**< Start cascade with FLAGS=1 (first sensor) */
#define CASCADE_MAX_SENSORS 11         /**< Maximum number of sensors in cascade */

/* ============================= Private Function Prototypes ========== */
/**
 * @brief Main task entry point (Active Object thread)
 */
static void lwpkt_comm_task_entry(void *arg);

/**
 * @brief Process RX data (lwpkt_process loop)
 */
static void process_rx_data(LgcLwPktAgent_t *agent);

/**
 * @brief Execute TX command from queue
 */
static void execute_tx_command(LgcLwPktAgent_t *agent, const LgcLwPktCommand_t *cmd);

/**
 * @brief LwPKT event callback (RX packet received, TX complete, etc.)
 */
static lwpktr_t lwpkt_event_callback(lwpkt_evt_type_t evt_type, lwpkt_t *lwpkt);

/**
 * @brief LwPKT output function (hardware transmit)
 */
static bool lwpkt_output_func(const uint8_t *data, uint16_t len, void *arg);

/* ============================= Public Functions ===================== */
error_t LgcLwPktAgent_Init(LgcLwPktAgent_t *agent, UART_HandleTypeDef *huart)
{
    /* 1. Validate parameters */
    if (agent == NULL || huart == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    if (agent->is_initialized)
    {
        return ERROR_ALREADY_RUNNING; /* Already initialized */
    }

    /* 2. Store hardware handle */
    agent->huart = huart;

    /* 3. Initialize ring buffers */
    lwrb_init(&agent->rx_rb, agent->rx_buffer, sizeof(agent->rx_buffer));
    lwrb_init(&agent->tx_rb, agent->tx_buffer, sizeof(agent->tx_buffer));

    /* 4. Initialize LwPKT instance */
    if (lwpkt_init(&agent->lwpkt, &agent->tx_rb, &agent->rx_rb) != lwpktOK)
    {
        return ERROR_FAILURE;
    }

    lwpkt_set_evt_fn(&agent->lwpkt, lwpkt_event_callback);

    /* 5. Create OSAL primitives */
    /* Binary semaphore (initial count = 0) */
    if (osCreateSemaphore(&agent->rx_data_semaphore, 0) != TRUE)
    {
        return ERROR_OUT_OF_RESOURCES;
    }

    /* TX command queue (8 messages, sizeof(LgcLwPktCommand_t) each) */
    if (osCreateQueue(&agent->tx_cmd_queue, "LwPKT TX Queue",
                      sizeof(LgcLwPktCommand_t), LGC_LWPKT_TX_QUEUE_SIZE) != TRUE)
    {
        osDeleteSemaphore(&agent->rx_data_semaphore);
        return ERROR_OUT_OF_RESOURCES;
    }

    /* 6. Create task (thread) */
    OsTaskParameters task_params = {
        .stack = agent->task_stack,
        .stackSize = sizeof(agent->task_stack),
        .priority = LGC_LWPKT_TASK_PRIORITY};

    agent->task_id = osCreateTask("LwPKT Comm", lwpkt_comm_task_entry, agent, &task_params);
    if (agent->task_id == OS_INVALID_TASK_ID)
    {
        osDeleteQueue(&agent->tx_cmd_queue);
        osDeleteSemaphore(&agent->rx_data_semaphore);
        return ERROR_OUT_OF_RESOURCES;
    }

    /* 7. Initialize state */
    agent->is_initialized = true;
    agent->is_running = false; /* Not started yet */
    agent->is_command_pending = false;
    agent->rx_count = 0;
    agent->tx_count = 0;
    agent->error_count = 0;

    return NO_ERROR;
}

error_t LgcLwPktAgent_Start(LgcLwPktAgent_t *agent)
{
    if (agent == NULL || !agent->is_initialized)
    {
        return ERROR_INVALID_PARAMETER;
    }

    if (agent->is_running)
    {
        return NO_ERROR; /* Already running */
    }

    /* Start UART DMA reception (event-based: IDLE detection) */
    HAL_StatusTypeDef hal_res = HAL_UARTEx_ReceiveToIdle_DMA(agent->huart, agent->rx_buffer, sizeof(agent->rx_buffer));
    if (hal_res != HAL_OK)
    {
        return ERROR_FAILURE;
    }

    agent->is_running = true;
    return NO_ERROR;
}

error_t LgcLwPktAgent_Stop(LgcLwPktAgent_t *agent)
{
    if (agent == NULL || !agent->is_initialized)
    {
        return ERROR_INVALID_PARAMETER;
    }

    /* Stop UART DMA (abort both RX and TX) */
    HAL_UART_DMAStop(agent->huart);

    agent->is_running = false;
    return NO_ERROR;
}

error_t LgcLwPktAgent_Deinit(LgcLwPktAgent_t *agent)
{
    if (agent == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    /* Stop if running */
    if (agent->is_running)
    {
        LgcLwPktAgent_Stop(agent);
    }

    /* Delete OSAL resources */
    if (agent->task_id != OS_INVALID_TASK_ID)
    {
        osDeleteTask(agent->task_id);
    }

    osDeleteQueue(&agent->tx_cmd_queue);
    osDeleteSemaphore(&agent->rx_data_semaphore);

    /* Clear state */
    agent->is_initialized = false;

    return NO_ERROR;
}

/* ============================= TX Command API ======================= */
error_t LgcLwPktAgent_SendReadCascade(
    LgcLwPktAgent_t *agent,
    LgcSensorArray_t *out_data,
    systime_t timeout_ms)
{
    if (agent == NULL || out_data == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    if (!agent->is_initialized || !agent->is_running)
    {
        return ERROR_NOT_CONFIGURED;
    }

    /* Build command */
    LgcLwPktCommand_t cmd = {
        .type = CMD_READ_CASCADE,       /* Protocol code 0x12 */
        .addr = CASCADE_BROADCAST_ADDR, /* 0xFF = broadcast to all sensors */
        .flags = CASCADE_START_FLAGS,   /* FLAGS = 1 (first sensor responds) */
        .payload_len = 0,               /* No payload for read commands */
        .callback = NULL,               /* Blocking mode (no callback) */
        .callback_ctx = NULL,
        .timeout_ms = timeout_ms};

    /* Send command to queue (blocking with timeout) */
    if (osSendToQueue(&agent->tx_cmd_queue, &cmd, OS_MS_TO_SYSTICKS(timeout_ms)) != TRUE)
    {
        return ERROR_BUFFER_OVERFLOW; /* Queue full or timeout */
    }

    /* TODO: Wait for completion and copy data to out_data */
    /* For now, just return OK (async model) */
    return NO_ERROR;
}

error_t LgcLwPktAgent_SendCommandAsync(
    LgcLwPktAgent_t *agent,
    const LgcLwPktCommand_t *cmd)
{
    if (agent == NULL || cmd == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    if (!agent->is_initialized || !agent->is_running)
    {
        return ERROR_NOT_CONFIGURED;
    }

    /* Non-blocking send (timeout = 0) */
    if (osSendToQueue(&agent->tx_cmd_queue, cmd, 0) != TRUE)
    {
        return ERROR_BUFFER_OVERFLOW; /* Queue full */
    }

    return NO_ERROR;
}

/* ============================= ISR Interface ======================== */
void LgcLwPktAgent_RxISRCallback(
    LgcLwPktAgent_t *agent,
    const uint8_t *data,
    uint16_t len)
{
    if (agent == NULL || data == NULL || len == 0)
    {
        return;
    }

    /* Push data to ring buffer (ISR-safe) */
    lwrb_write(&agent->rx_rb, data, len);

    /* Signal task (binary semaphore - ISR-safe) */
    osReleaseSemaphore(&agent->rx_data_semaphore);
}

/* ============================= Diagnostics ========================== */
error_t LgcLwPktAgent_GetStats(
    const LgcLwPktAgent_t *agent,
    uint32_t *rx_count,
    uint32_t *tx_count,
    uint32_t *err_count)
{
    if (agent == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    if (rx_count != NULL)
    {
        *rx_count = agent->rx_count;
    }

    if (tx_count != NULL)
    {
        *tx_count = agent->tx_count;
    }

    if (err_count != NULL)
    {
        *err_count = agent->error_count;
    }

    return NO_ERROR;
}

/* ============================= Private Functions ==================== */
/**
 * @brief Main task entry point (Active Object thread)
 * @details Implements zero-polling architecture:
 *          1. Block on rx_data_semaphore (CPU sleeps)
 *          2. When signaled: Process all RX data via lwpkt_process()
 *          3. Check TX command queue (non-blocking)
 *          4. Repeat
 */
static void lwpkt_comm_task_entry(void *arg)
{
    LgcLwPktAgent_t *agent = (LgcLwPktAgent_t *)arg;
    LgcLwPktCommand_t cmd;

    while (agent->is_running)
    {
        /* ====== 1. Wait for RX data (blocking on semaphore) ====== */
        /* CPU usage → 0% when no data (task suspended) */
        if (osWaitForSemaphore(&agent->rx_data_semaphore, OS_MS_TO_SYSTICKS(COMM_TASK_WAIT_TIMEOUT_MS)) == TRUE)
        {
            /* RX data available: Process all packets in ring buffer */
            process_rx_data(agent);
        }

        /* ====== 2. Check TX command queue (non-blocking) ====== */
        /* Timeout = 0: Return immediately if queue empty */
        if (osReceiveFromQueue(&agent->tx_cmd_queue, &cmd, 0) == TRUE)
        {
            /* Execute TX command */
            execute_tx_command(agent, &cmd);
        }

        /* ====== 3. Periodic tasks (if any) ====== */
        /* TODO: Watchdog, retry logic, etc. */
    }

    /* Task exit (should never reach here) */
    osDeleteTask(OS_SELF_TASK_ID);
}

/**
 * @brief Process RX data (lwpkt_process loop)
 */
static void process_rx_data(LgcLwPktAgent_t *agent)
{
    lwpkt_t *lwpkt = &agent->lwpkt;
    lwpktr_t res;

    /* Process all available data in ring buffer */
    while ((res = lwpkt_process(lwpkt, NULL)) == lwpktOK)
    {
        /* lwpkt_process() internally calls lwpkt_read() via event callback */
        /* All packet handling is done in lwpkt_event_callback() */
        /* No additional work here */
    }

    /* Update statistics */
    if (res == lwpktWAITDATA)
    {
        /* Normal: No more data to process */
    }
    else if (res != lwpktOK)
    {
        /* Error: CRC mismatch, protocol error, etc. */
        agent->error_count++;
    }
}

/**
 * @brief Execute TX command from queue
 */
static void execute_tx_command(LgcLwPktAgent_t *agent, const LgcLwPktCommand_t *cmd)
{
    lwpktr_t res;
    error_t result = NO_ERROR;

    switch (cmd->type)
    {
    /* ===== READ COMMANDS ===== */
    case CMD_READ_CASCADE:
    {
        /* Send broadcast command: CMD=0x12 + FLAGS (sensor# to respond 1-11)
         * FLAGS field tells WHICH sensor should respond (cascade control)
         * Sensor logic: if (flags == my_address) { respond + set flags = my_address+1 }
         */
        res = lwpkt_write(&agent->lwpkt,
                          cmd->addr,        /* 0xFF = broadcast */
                          cmd->flags,       /* FLAGS: 1-11 (which sensor responds) */
                          CMD_READ_CASCADE, /* 0x12 */
                          cmd->payload,
                          cmd->payload_len);

        if (res == lwpktOK)
        {
            agent->tx_count++;
            /* Store active command for response handling */
            agent->is_command_pending = true;
            agent->active_cmd = *cmd;
        }
        else
        {
            result = ERROR_FAILURE;
            agent->error_count++;
        }
        break;
    }

    case CMD_READ_SENSOR: /* 0x10 - Read calibrated float[10] */
    case CMD_READ_RAW:    /* 0x11 - Read raw uint16_t[10] */
    case CMD_GET_STATUS:  /* 0x31 - Read digital_state uint16_t */
    {
        /* Single sensor read (no FLAGS needed) */
        res = lwpkt_write(&agent->lwpkt,
                          cmd->addr, /* Specific sensor address (1-11) */
                          0,         /* FLAGS = 0 (not used for single read) */
                          cmd->type, /* Command code */
                          cmd->payload,
                          cmd->payload_len);

        if (res == lwpktOK)
        {
            agent->tx_count++;
            agent->is_command_pending = true;
            agent->active_cmd = *cmd;
        }
        else
        {
            result = ERROR_FAILURE;
            agent->error_count++;
        }
        break;
    }

    /* ===== WRITE/CONFIG COMMANDS ===== */
    case CMD_SET_OFFSET: /* 0x21 - Write float[10] calibration offset */
    {
        /* Payload: float offset[10] (40 bytes)
         * Sensor responds with ACK (no payload) or NACK (error code)
         */
        if (cmd->payload_len == 40) /* sizeof(float) * 10 */
        {
            res = lwpkt_write(&agent->lwpkt,
                              cmd->addr,
                              0, /* FLAGS = 0 */
                              CMD_SET_OFFSET,
                              cmd->payload,
                              cmd->payload_len);

            if (res == lwpktOK)
            {
                agent->tx_count++;
                agent->is_command_pending = true;
                agent->active_cmd = *cmd;
            }
            else
            {
                result = ERROR_FAILURE;
                agent->error_count++;
            }
        }
        else
        {
            result = ERROR_INVALID_LENGTH;
        }
        break;
    }

    case CMD_SET_FILTER: /* 0x22 - Write filter cutoff frequency (float fc) */
    {
        /* Payload: float fc (4 bytes) */
        if (cmd->payload_len == 4) /* sizeof(float) */
        {
            res = lwpkt_write(&agent->lwpkt,
                              cmd->addr,
                              0, /* FLAGS = 0 */
                              CMD_SET_FILTER,
                              cmd->payload,
                              cmd->payload_len);

            if (res == lwpktOK)
            {
                agent->tx_count++;
                agent->is_command_pending = true;
                agent->active_cmd = *cmd;
            }
            else
            {
                result = ERROR_FAILURE;
                agent->error_count++;
            }
        }
        else
        {
            result = ERROR_INVALID_LENGTH;
        }
        break;
    }

    case CMD_CALIBRATE: /* 0x30 - Trigger calibration sequence */
    {
        /* No payload, sensor responds with result */
        res = lwpkt_write(&agent->lwpkt,
                          cmd->addr,
                          0, /* FLAGS = 0 */
                          CMD_CALIBRATE,
                          cmd->payload,
                          cmd->payload_len);

        if (res == lwpktOK)
        {
            agent->tx_count++;
            agent->is_command_pending = true;
            agent->active_cmd = *cmd;
        }
        else
        {
            result = ERROR_FAILURE;
            agent->error_count++;
        }
        break;
    }

    /* ===== UNSUPPORTED ===== */
    case CMD_WRITE_CONFIG:      /* 0x20 - Generic config write */
    case CMD_WRITE_CONFIG_RESP: /* 0xA0 - Response */
    /* ===== RESPONSE CODES (RX only - should NOT be sent as commands) ===== */
    case CMD_READ_CASCADE_RESP: /* 0x92 - Response (RX only) */
        /* Response code - should not be sent as command */
        result = ERROR_INVALID_PARAMETER;
        break;

    default:
        result = ERROR_INVALID_PARAMETER;
        break;
    }

    /* Invoke callback if provided */
    if (cmd->callback != NULL)
    {
        cmd->callback(result, NULL, 0, cmd->callback_ctx);
    }
}

/**
 * @brief LwPKT event callback (RX packet received, TX complete, etc.)
 * @note Called FROM lwpkt_process() in task context (NOT ISR)
 */
static lwpktr_t lwpkt_event_callback(lwpkt_evt_type_t evt_type, lwpkt_t *lwpkt)
{
    LgcLwPktAgent_t *agent = (LgcLwPktAgent_t *)lwpkt; /* Context pointer */

    switch (evt_type)
    {
    case LWPKT_EVT_PKT:
    {
        /* New packet received: Read it */
        lwpkt_t *pkt;
        if ((pkt = lwpkt_read(lwpkt)) != NULL)
        {
            agent->rx_count++;

            /* Parse response based on command type */
            if (agent->is_command_pending)
            {
                /* Check for ERROR response (cmd | 0x80) */
                if ((pkt->m.cmd & CMD_ERROR_FLAG) != 0)
                {
                    /* Error response: Extract original command and error code */
                    uint8_t original_cmd = pkt->m.cmd & 0x7F;
                    error_t err_code = (pkt->m.len > 0) ? ERROR_FAILURE : ERROR_FAILURE;

                    if (agent->active_cmd.callback != NULL)
                    {
                        agent->active_cmd.callback(
                            err_code,
                            pkt->data,
                            pkt->m.len,
                            agent->active_cmd.callback_ctx);
                    }
                    agent->is_command_pending = false;
                }
                else
                {
                    /* Normal response */
                    switch (pkt->m.cmd)
                    {
                    /* ===== CASCADE READ RESPONSE (Special code 0x92) ===== */
                    case CMD_READ_CASCADE_RESP: /* 0x92 - Payload: uint16_t digital_state (2B) + FLAGS */
                    {
                        if (pkt->m.len == 2)
                        {
                            uint16_t digital_state;
                            memcpy(&digital_state, pkt->data, sizeof(digital_state));

                            /* FLAGS indicates which sensor responded (sensor sends FLAGS = my_addr + 1)
                             * FLAGS = 0 means last sensor in chain
                             */
                            if (pkt->m.flags > 0 && pkt->m.flags <= 12) /* 1-11 = sensors, 12 = after last */
                            {
                                /* Store response: FLAGS 1-12 indicates sensor N-1 responded
                                 * Sensor 1 sends FLAGS=2, sensor 2 sends FLAGS=3, ..., sensor 11 sends FLAGS=12 or 0
                                 */
                                uint8_t sensor_index = (uint8_t)(pkt->m.flags - 2); /* FLAGS 2-12 → index 0-10 */
                                if (sensor_index < 11)
                                {
                                    agent->cascade_responses[sensor_index] = digital_state;
                                    agent->cascade_count++;
                                }
                            }
                            else if (pkt->m.flags == 0)
                            {
                                /* FLAGS=0 means last sensor (sensor 11) responded */
                                agent->cascade_responses[10] = digital_state;
                                agent->cascade_count++;
                            }

                            /* Check if cascade complete:
                             * - FLAGS = 0 (last sensor responded)
                             * - OR received 11 responses (all sensors)
                             */
                            if (pkt->m.flags == 0 || agent->cascade_count >= 11)
                            {
                                /* Cascade complete - invoke callback with all data */
                                if (agent->active_cmd.callback != NULL)
                                {
                                    agent->active_cmd.callback(
                                        NO_ERROR,
                                        (const uint8_t *)agent->cascade_responses,
                                        agent->cascade_count * sizeof(uint16_t),
                                        agent->active_cmd.callback_ctx);
                                }
                                agent->is_command_pending = false;
                                agent->cascade_count = 0;
                                agent->cascade_expected_flags = 0;
                            }
                            else
                            {
                                /* More sensors to come - wait for next response */
                                agent->cascade_expected_flags = pkt->m.flags + 1;
                            }
                        }
                        break;
                    }

                    /* ===== READ COMMANDS (response code = command code) ===== */
                    case CMD_READ_SENSOR: /* 0x10 - Payload: float[10] calibrated (40B) */
                    {
                        if (pkt->m.len == 40) /* sizeof(float) * 10 */
                        {
                            if (agent->active_cmd.callback != NULL)
                            {
                                agent->active_cmd.callback(
                                    NO_ERROR,
                                    pkt->data,
                                    pkt->m.len,
                                    agent->active_cmd.callback_ctx);
                            }
                            agent->is_command_pending = false;
                        }
                        break;
                    }

                    case CMD_READ_RAW: /* 0x11 - Payload: uint16_t[10] raw ADC (20B) */
                    {
                        if (pkt->m.len == 20) /* sizeof(uint16_t) * 10 */
                        {
                            if (agent->active_cmd.callback != NULL)
                            {
                                agent->active_cmd.callback(
                                    NO_ERROR,
                                    pkt->data,
                                    pkt->m.len,
                                    agent->active_cmd.callback_ctx);
                            }
                            agent->is_command_pending = false;
                        }
                        break;
                    }

                    case CMD_GET_STATUS: /* 0x31 - Payload: uint16_t digital_state (2B) */
                    {
                        if (pkt->m.len == 2)
                        {
                            if (agent->active_cmd.callback != NULL)
                            {
                                agent->active_cmd.callback(
                                    NO_ERROR,
                                    pkt->data,
                                    pkt->m.len,
                                    agent->active_cmd.callback_ctx);
                            }
                            agent->is_command_pending = false;
                        }
                        break;
                    }

                    /* ===== WRITE/CONFIG COMMANDS (response: empty payload = ACK) ===== */
                    case CMD_SET_OFFSET: /* 0x21 - Payload: empty (ACK) */
                    case CMD_SET_FILTER: /* 0x22 - Payload: empty (ACK) */
                    case CMD_CALIBRATE:  /* 0x30 - Payload: empty (ACK) */
                    {
                        /* Empty payload = success ACK */
                        if (agent->active_cmd.callback != NULL)
                        {
                            agent->active_cmd.callback(
                                NO_ERROR,
                                pkt->data,
                                pkt->m.len,
                                agent->active_cmd.callback_ctx);
                        }
                        agent->is_command_pending = false;
                        break;
                    }

                    default:
                        /* Unexpected response - ignore */
                        break;
                    }
                }
            }

            /* Free packet buffer */
            lwpkt_free(pkt);
        }
        break;
    }

    case LWPKT_EVT_TIMEOUT:
        /* Command timeout: Invoke error callback */
        if (agent->is_command_pending && agent->active_cmd.callback != NULL)
        {
            agent->active_cmd.callback(ERROR_TIMEOUT, NULL, 0, agent->active_cmd.callback_ctx);
        }
        agent->is_command_pending = false;
        agent->error_count++;
        break;

    default:
        /* Protocol event (no action needed) */
        break;
    }

    return lwpktOK;
}

/**
 * @brief LwPKT output function (hardware transmit)
 * @note Called FROM lwpkt_write() when packet is ready to send
 */
static bool lwpkt_output_func(const uint8_t *data, uint16_t len, void *arg)
{
    LgcLwPktAgent_t *agent = (LgcLwPktAgent_t *)arg;

    if (agent == NULL || data == NULL || len == 0)
    {
        return false;
    }

    /* Transmit via UART (blocking or DMA) */
    HAL_StatusTypeDef res = HAL_UART_Transmit(agent->huart, (uint8_t *)data, len, 100);

    return (res == HAL_OK);
}
