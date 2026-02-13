/**
 * @file    lgc_hmi_task.c
 * @brief   HMI Task Implementation
 * @author  Clean Architecture Refactor Team (TDD)
 * @date    2026-02-12
 * @version 1.0.0
 *
 * Copyright (c) 2026 Leather Gauge Controller Team
 */

/* ============================= Includes ============================= */
#include "lgc_hmi_task.h"
#include "lgc_hmi_vp_addresses.h" /* Centralized VP address definitions */
#include <string.h>
#include <stdio.h>

/* ============================= Event Flags ========================== */
#define HMI_EVENT_DISPLAY_BUTTON (1U << 0)
#define HMI_EVENT_MEASUREMENT_UPDATE (1U << 1)
#define HMI_EVENT_PIECE_FINISHED (1U << 2)
#define HMI_EVENT_BATCH_FINISHED (1U << 3)

/* ============================= Private Functions ==================== */

/**
 * @brief Update display with current measurement
 */
/**
 * @brief Update display with current measurement
 * @note Uses VP_AREA_TO_UINT16 macro to convert float → uint16_t (×100)
 */
static void hmi_update_display_measurement(LgcHmiTask_t *task)
{
    /* Update current leather area (convert float dm² → uint16_t ×100) */
    uint16_t current_area_display = VP_AREA_TO_UINT16(task->measurements->current_leather_area);
    task->display->write_u16(
        task->display->context,
        VP_CURRENT_AREA,
        current_area_display);

    /* Update accumulated batch area */
    uint16_t accumulated_area_display = VP_AREA_TO_UINT16(
        task->measurements->batch_measurement[task->measurements->current_batch_index]);
    task->display->write_u16(
        task->display->context,
        VP_ACCUMULATED_AREA,
        accumulated_area_display);

    /* Update counters */
    task->display->write_u16(
        task->display->context,
        VP_LEATHER_COUNT,
        (uint16_t)task->measurements->current_leather_index);

    task->display->write_u16(
        task->display->context,
        VP_BATCH_COUNT,
        (uint16_t)task->measurements->current_batch_index);
}

/**
 * @brief Update display with configuration
 */
static void hmi_update_display_config(LgcHmiTask_t *task)
{
    /* Update text fields */
    task->display->write_text(
        task->display->context,
        VP_CONFIG_NAME_CLIENT,
        task->system_config->client_name);

    task->display->write_text(
        task->display->context,
        VP_CONFIG_NAME_COLOR,
        task->system_config->color);

    task->display->write_text(
        task->display->context,
        VP_CONFIG_NAME_LEATHER,
        task->system_config->leather_id);

    /* Update batch size */
    task->display->write_u16(
        task->display->context,
        VP_CONFIG_BATCH_SIZE,
        (uint16_t)task->system_config->max_pieces_per_batch);
}

/**
 * @brief Event callback from MeasurementCore (Observer pattern)
 */
static void hmi_on_measurement_event(const LgcEvent_t *event, void *context)
{
    LgcHmiTask_t *task = (LgcHmiTask_t *)context;

    if (task == NULL || event == NULL)
    {
        return;
    }

    /* Set event flags to wake up HMI task */
    switch (event->type)
    {
    case LGC_EVENT_MEASUREMENT_UPDATED:
        tx_event_flags_set(&task->events, HMI_EVENT_MEASUREMENT_UPDATE, TX_OR);
        break;

    case LGC_EVENT_PIECE_FINISHED:
        tx_event_flags_set(&task->events, HMI_EVENT_PIECE_FINISHED, TX_OR);
        break;

    case LGC_EVENT_BATCH_FINISHED:
        tx_event_flags_set(&task->events, HMI_EVENT_BATCH_FINISHED, TX_OR);
        break;

    default:
        break;
    }
}

/**
 * @brief Button callback from Display Adapter
 */
static void hmi_on_button_event(const LgcDisplayEvent_t *event, void *context)
{
    LgcHmiTask_t *task = (LgcHmiTask_t *)context;

    if (task == NULL || event == NULL)
    {
        return;
    }

    /* Push button event to queue */
    ULONG button_data = (ULONG)event->button;
    tx_queue_send(&task->button_queue, &button_data, TX_NO_WAIT);

    /* Set event flag to wake up task */
    tx_event_flags_set(&task->events, HMI_EVENT_DISPLAY_BUTTON, TX_OR);
}

/**
 * @brief Process button command
 */
static void hmi_process_button(LgcHmiTask_t *task, LgcDisplayButton_t button)
{
    switch (button)
    {
    case LGC_BTN_START:
        /* TODO: Send START command to MeasurementCore via command interface */
        task->display->write_u16(task->display->context, VP_STATE, 1); /* State: Running */
        break;

    case LGC_BTN_STOP:
        /* TODO: Send STOP command to MeasurementCore */
        task->display->write_u16(task->display->context, VP_STATE, 0); /* State: Stopped */
        break;

    case LGC_BTN_DELETE_LAST:
        /* TODO: Send DELETE_LAST command to MeasurementCore */
        if (task->measurements->current_leather_index > 0)
        {
            task->measurements->current_leather_index--;
            /* Clear last measurement */
            task->measurements->leather_measurement[task->measurements->current_leather_index] = 0.0f;
            /* Update display */
            hmi_update_display_measurement(task);
        }
        break;

    case LGC_BTN_NEXT_BATCH:
        /* TODO: Send NEXT_BATCH command to MeasurementCore */
        /* This should trigger BATCH_FINISHED event */
        break;

    case LGC_BTN_SETTINGS:
        /* Jump to settings page */
        task->display->change_page(task->display->context, 10); /* Settings page */
        hmi_update_display_config(task);
        break;

    case LGC_BTN_CONFIG_SAVE:
        /* TODO: Send SAVE_CONFIG command */
        /* Update configuration on display */
        hmi_update_display_config(task);
        break;

    case LGC_BTN_CONFIG_CANCEL:
        /* Jump back to main page */
        task->display->change_page(task->display->context, 0); /* Main page */
        break;

    default:
        /* Unknown button, ignore */
        break;
    }
}

/**
 * @brief HMI task entry point
 */
static void hmi_task_entry(ULONG param)
{
    LgcHmiTask_t *task = (LgcHmiTask_t *)param;
    ULONG actual_flags = 0;
    ULONG button_data = 0;

    while (task->is_running)
    {
        /* Wait for events (blocking, saves CPU) */
        UINT tx_res = tx_event_flags_get(
            &task->events,
            HMI_EVENT_DISPLAY_BUTTON | HMI_EVENT_MEASUREMENT_UPDATE |
                HMI_EVENT_PIECE_FINISHED | HMI_EVENT_BATCH_FINISHED,
            TX_OR_CLEAR,
            &actual_flags,
            LGC_HMI_UPDATE_RATE_MS); /* Timeout for periodic display process */

        /* Handle button events (from queue) */
        if (actual_flags & HMI_EVENT_DISPLAY_BUTTON)
        {
            while (tx_queue_receive(&task->button_queue, &button_data, TX_NO_WAIT) == TX_SUCCESS)
            {
                hmi_process_button(task, (LgcDisplayButton_t)button_data);
            }
        }

        /* Handle measurement update event */
        if (actual_flags & HMI_EVENT_MEASUREMENT_UPDATE)
        {
            hmi_update_display_measurement(task);
        }

        /* Handle piece finished event */
        if (actual_flags & HMI_EVENT_PIECE_FINISHED)
        {
            hmi_update_display_measurement(task);
            /* TODO: Play sound, show animation, etc. */
        }

        /* Handle batch finished event */
        if (actual_flags & HMI_EVENT_BATCH_FINISHED)
        {
            hmi_update_display_measurement(task);
            /* TODO: Show batch summary, ask for print, etc. */
        }

        /* Periodic display processing (if timeout occurred) */
        if (tx_res == TX_NO_EVENTS)
        {
            /* No events, but timeout occurred */
            /* Process display anyway (for DMA data) */
        }
    }
}

/* ============================= Public API =========================== */

Result_t LgcHmiTask_Init(
    LgcHmiTask_t *task,
    ILgcDisplay_t *display,
    ILgcEventPublisher_t *event_publisher,
    LgcSystemConfig_t *system_config,
    LgcMeasurements_t *measurements)
{
    LGC_VALIDATE_PTR(task);
    LGC_VALIDATE_PTR(display);
    LGC_VALIDATE_PTR(event_publisher);
    LGC_VALIDATE_PTR(system_config);
    LGC_VALIDATE_PTR(measurements);

    /* Clear structure */
    memset(task, 0, sizeof(LgcHmiTask_t));

    /* Store dependencies */
    task->display = display;
    task->event_publisher = event_publisher;
    task->system_config = system_config;
    task->measurements = measurements;

    /* Create event flags */
    UINT tx_res = tx_event_flags_create(&task->events, "hmi_events");
    if (tx_res != TX_SUCCESS)
    {
        return ERR_HARDWARE_FAULT;
    }

    /* Create button queue */
    tx_res = tx_queue_create(
        &task->button_queue,
        "hmi_button_queue",
        1, /* Message size (1 ULONG) */
        task->button_queue_storage,
        sizeof(task->button_queue_storage));

    if (tx_res != TX_SUCCESS)
    {
        tx_event_flags_delete(&task->events);
        return ERR_HARDWARE_FAULT;
    }

    /* Subscribe to measurement events */
    event_publisher->subscribe(
        event_publisher->context,
        hmi_on_measurement_event,
        task,
        LGC_EVENT_MEASUREMENT_UPDATED | LGC_EVENT_PIECE_FINISHED | LGC_EVENT_BATCH_FINISHED);

    /* Attach display button callback */
    display->attach_callback(display->context, hmi_on_button_event, task);

    task->is_initialized = true;
    task->is_running = false;

    return ERR_OK;
}

Result_t LgcHmiTask_Start(LgcHmiTask_t *task)
{
    LGC_VALIDATE_PTR(task);

    if (!task->is_initialized)
    {
        return ERR_NOT_INITIALIZED;
    }

    if (task->is_running)
    {
        return ERR_BUSY;
    }

    /* Create task thread */
    UINT tx_res = tx_thread_create(
        &task->thread,
        "hmi_task",
        hmi_task_entry,
        (ULONG)task,
        task->stack,
        sizeof(task->stack),
        LGC_HMI_TASK_PRIORITY,
        LGC_HMI_TASK_PRIORITY, /* Preemption threshold = priority (no preemption) */
        TX_NO_TIME_SLICE,
        TX_AUTO_START);

    if (tx_res != TX_SUCCESS)
    {
        return ERR_HARDWARE_FAULT;
    }

    task->is_running = true;

    /* Update display with initial values */
    hmi_update_display_config(task);
    hmi_update_display_measurement(task);

    return ERR_OK;
}

Result_t LgcHmiTask_Stop(LgcHmiTask_t *task)
{
    LGC_VALIDATE_PTR(task);

    if (!task->is_running)
    {
        return ERR_OK; /* Already stopped */
    }

    /* Signal task to stop */
    task->is_running = false;

    /* Wait for task to finish */
    tx_thread_wait_completion(&task->thread, TX_WAIT_FOREVER);

    /* Delete thread */
    tx_thread_delete(&task->thread);

    return ERR_OK;
}

Result_t LgcHmiTask_Deinit(LgcHmiTask_t *task)
{
    LGC_VALIDATE_PTR(task);

    if (!task->is_initialized)
    {
        return ERR_NOT_INITIALIZED;
    }

    /* Stop task if running */
    if (task->is_running)
    {
        LgcHmiTask_Stop(task);
    }

    /* Unsubscribe from events */
    task->event_publisher->unsubscribe(
        task->event_publisher->context,
        hmi_on_measurement_event);

    /* Detach display callback */
    task->display->detach_callback(task->display->context);

    /* Delete event flags */
    tx_event_flags_delete(&task->events);

    /* Delete queue */
    tx_queue_delete(&task->button_queue);

    task->is_initialized = false;
    return ERR_OK;
}
