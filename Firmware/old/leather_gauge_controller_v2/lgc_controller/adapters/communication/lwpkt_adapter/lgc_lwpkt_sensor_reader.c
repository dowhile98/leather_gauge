/**
 * @file    lgc_lwpkt_sensor_reader.c
 * @brief   ISensorReader Wrapper Implementation
 * @author  Clean Architecture Refactor Team (TDD)
 * @date    2026-02-12
 * @version 1.0.0
 *
 * Copyright (c) 2026 Leather Gauge Controller Team
 */

/* ============================= Includes ============================= */
#include "lgc_lwpkt_sensor_reader.h"
#include <string.h>

/* ============================= Private Prototypes =================== */
static Result_t lwpkt_reader_init(void *ctx, const LgcSensorReaderConfig_t *config);
static Result_t lwpkt_reader_read_all_sensors(void *ctx, LgcSensorArray_t *out_data);
static Result_t lwpkt_reader_read_cascade_mode(void *ctx, LgcSensorArray_t *out_data);
static Result_t lwpkt_reader_deinit(void *ctx);

static void cascade_callback(error_t result, const uint8_t *data, uint16_t data_len, void *user_ctx);

/* ============================= Implementation ======================= */

Result_t LgcLwPktSensorReader_Init(
    LgcLwPktSensorReader_t *reader,
    LgcLwPktAgent_t *agent)
{
    if (reader == NULL || agent == NULL)
    {
        return ERR_NULL_POINTER;
    }

    /* Clear structure */
    memset(reader, 0, sizeof(LgcLwPktSensorReader_t));

    /* Store dependency */
    reader->agent = agent;

    /* Create synchronization primitives */
    if (osCreateSemaphore(&reader->completion_sem, 1, 0) != NO_ERROR)
    {
        return ERR_HARDWARE_FAULT;
    }

    if (osCreateMutex(&reader->mutex) != NO_ERROR)
    {
        osDeleteSemaphore(&reader->completion_sem);
        return ERR_HARDWARE_FAULT;
    }

    reader->is_initialized = true;
    return ERR_OK;
}

Result_t LgcLwPktSensorReader_Deinit(LgcLwPktSensorReader_t *reader)
{
    if (reader == NULL)
    {
        return ERR_NULL_POINTER;
    }

    if (!reader->is_initialized)
    {
        return ERR_OK;
    }

    /* Delete synchronization primitives */
    osDeleteSemaphore(&reader->completion_sem);
    osDeleteMutex(&reader->mutex);

    reader->is_initialized = false;
    return ERR_OK;
}

ILgcSensorReader_t *LgcLwPktSensorReader_GetInterface(LgcLwPktSensorReader_t *reader)
{
    static ILgcSensorReader_t interface = {
        .context = NULL,
        .init = lwpkt_reader_init,
        .read_all_sensors = lwpkt_reader_read_all_sensors,
        .read_cascade_mode = lwpkt_reader_read_cascade_mode,
        .deinit = lwpkt_reader_deinit};

    if (reader != NULL)
    {
        interface.context = reader;
    }

    return &interface;
}

/* ============================= Private Functions ==================== */

/**
 * @brief Initialize sensor reader (ISensorReader interface)
 */
static Result_t lwpkt_reader_init(void *ctx, const LgcSensorReaderConfig_t *config)
{
    LgcLwPktSensorReader_t *reader = (LgcLwPktSensorReader_t *)ctx;

    if (reader == NULL)
    {
        return ERR_NULL_POINTER;
    }

    /* Already initialized in LgcLwPktSensorReader_Init() */
    /* This method is for runtime re-configuration (not used yet) */
    return ERR_OK;
}

/**
 * @brief Read all sensors sequentially (DEPRECATED - use cascade instead)
 */
static Result_t lwpkt_reader_read_all_sensors(void *ctx, LgcSensorArray_t *out_data)
{
    /* Forward to cascade mode (LwPKT doesn't support individual polling) */
    return lwpkt_reader_read_cascade_mode(ctx, out_data);
}

/**
 * @brief Async callback for CASCADE read (executed in Agent task context)
 */
static void cascade_callback(error_t result, const uint8_t *data, uint16_t data_len, void *user_ctx)
{
    LgcLwPktSensorReader_t *reader = (LgcLwPktSensorReader_t *)user_ctx;

    if (reader == NULL)
    {
        return;
    }

    /* Store result */
    reader->response_error = result;

    /* Copy data if successful */
    if (result == NO_ERROR && data != NULL)
    {
        /* data contains uint16_t[11] digital states from CASCADE response */
        uint16_t *digital_states = (uint16_t *)data;
        uint8_t sensor_count = data_len / sizeof(uint16_t);

        if (sensor_count > LGC_SENSOR_NUMBER)
        {
            sensor_count = LGC_SENSOR_NUMBER;
        }

        /* Convert digital states (10-bit masks) to sensor array format */
        for (uint8_t i = 0; i < sensor_count; i++)
        {
            reader->response_data.sensors[i].sensor_id = i + 1; /* 1-based */
            reader->response_data.sensors[i].is_valid = true;
            reader->response_data.sensors[i].status = digital_states[i]; /* 10-bit mask */

            /* Optionally count active bits */
            uint8_t active_count = 0;
            for (uint8_t bit = 0; bit < 10; bit++)
            {
                if (digital_states[i] & (1 << bit))
                {
                    active_count++;
                }
            }
            reader->response_data.sensors[i].active_count = active_count;
        }

        /* Mark sensors we didn't receive as invalid */
        for (uint8_t i = sensor_count; i < LGC_SENSOR_NUMBER; i++)
        {
            reader->response_data.sensors[i].is_valid = false;
        }

        reader->response_data.count = sensor_count;
    }

    /* Signal completion (wake up waiting task) */
    osReleaseSemaphore(&reader->completion_sem);
}

/**
 * @brief Read sensors in cascade mode (ISensorReader interface)
 * @note Blocks until all 11 sensors respond or timeout
 */
static Result_t lwpkt_reader_read_cascade_mode(void *ctx, LgcSensorArray_t *out_data)
{
    LgcLwPktSensorReader_t *reader = (LgcLwPktSensorReader_t *)ctx;

    if (reader == NULL || out_data == NULL)
    {
        return ERR_NULL_POINTER;
    }

    if (!reader->is_initialized)
    {
        return ERR_UNINITIALIZED;
    }

    /* Lock (thread-safe) */
    if (osAcquireMutex(&reader->mutex) != NO_ERROR)
    {
        return ERR_BUSY;
    }

    /* Clear previous response */
    memset(&reader->response_data, 0, sizeof(reader->response_data));
    reader->response_error = NO_ERROR;

    /* Build async command */
    LgcLwPktCommand_t cmd = {
        .type = CMD_READ_CASCADE, /* 0x12 */
        .addr = 0xFF,             /* Broadcast */
        .flags = 1,               /* Start with sensor #1 */
        .payload_len = 0,
        .callback = cascade_callback, /* Async callback */
        .callback_ctx = reader,       /* Pass reader context */
        .timeout_ms = 1000            /* 1s timeout for all 11 sensors */
    };

    /* Send async command to Agent */
    error_t err = LgcLwPktAgent_SendCommandAsync(reader->agent, &cmd);
    if (err != NO_ERROR)
    {
        osReleaseMutex(&reader->mutex);
        return (err == ERROR_BUFFER_OVERFLOW) ? ERR_BUSY : ERR_HARDWARE_FAULT;
    }

    /* Wait for callback to signal completion (blocking) */
    if (osWaitForSemaphore(&reader->completion_sem, 1500) != NO_ERROR)
    {
        /* Timeout: CASCADE did not complete in time */
        osReleaseMutex(&reader->mutex);
        return ERR_TIMEOUT;
    }

    /* Check result from callback */
    if (reader->response_error != NO_ERROR)
    {
        osReleaseMutex(&reader->mutex);
        return (reader->response_error == ERROR_TIMEOUT) ? ERR_TIMEOUT : ERR_HARDWARE_FAULT;
    }

    /* Copy data to output */
    memcpy(out_data, &reader->response_data, sizeof(LgcSensorArray_t));

    /* Unlock */
    osReleaseMutex(&reader->mutex);

    return ERR_OK;
}

/**
 * @brief Deinitialize sensor reader (ISensorReader interface)
 */
static Result_t lwpkt_reader_deinit(void *ctx)
{
    return LgcLwPktSensorReader_Deinit((LgcLwPktSensorReader_t *)ctx);
}
