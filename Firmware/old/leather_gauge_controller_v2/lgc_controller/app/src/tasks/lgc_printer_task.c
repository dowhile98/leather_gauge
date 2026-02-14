/**
 * @file    lgc_printer_task.c
 * @brief   Printer Service Task (Observer) implementation
 * @author  Clean Architecture Refactor Team
 * @date    2026-02-12
 * @version 1.0.0
 *
 * Copyright (c) 2026 Leather Gauge Controller Team
 */

/* ============================= Includes ============================= */
#include "lgc_printer_task.h"
#include <string.h>
#include <stdio.h>

/* ============================= Constants ============================ */
#define PRINTER_STACK_SIZE 1024
#define PRINTER_QUEUE_SIZE 4
#define PRINTER_THREAD_PRIORITY 20
#define PRINTER_THREAD_PREEMPT 20

#define PRINTER_EVENT_BATCH_FINISHED (1U << 0)
#define PRINTER_EVENT_CONFIG_CHANGE (1U << 1)

/* ============================= Private Types ======================== */
typedef struct {
    /* Dependencies */
    ILgcPrinter_t *printer;
    ILgcEventPublisher_t *publisher;
    LgcSystemConfig_t *config;

    /* ThreadX Resources */
    TX_THREAD thread;
    TX_EVENT_FLAGS_GROUP events;
    TX_QUEUE data_queue;
    
    /* Memory Areas */
    uint8_t stack[PRINTER_STACK_SIZE];
    uint8_t queue_buffer[PRINTER_QUEUE_SIZE * sizeof(LgcEventDataBatchFinished_t)]; // Queue stores full struct

    /* State */
    bool is_running;
    bool is_initialized;

} PrinterTask_t;

/* ============================= Private Variables ==================== */
static PrinterTask_t s_printer_task = {0};
static LgcEventDataBatchFinished_t s_batch_data_buffer; /* Temp buffer for queue */

/* ============================= Private Functions ==================== */

/**
 * @brief Observer callback for Batch Finished event
 * @note  Called from Publisher context (often Main Task)
 */
static void printer_on_event(const LgcEvent_t *event, void *context)
{
    PrinterTask_t *task = (PrinterTask_t *)context;

    if (task == NULL || event == NULL) return;

    if (event->type == LGC_EVENT_BATCH_FINISHED)
    {
        /* Copy data to queue (deep copy to decouple timing) */
        /* Note: LgcEventDataBatchFinished_t is large (~400 bytes due to array of pieces) */
        /* ThreadX Queue copies by value if size fits. Here we use queue buffer. */
        /* If struct is too large for queue, use pointer passing with dynamic alloc or pool? */
        /* For now, assuming it fits (queue item size is ULONGs). */
        /* Wait: tx_queue_create item_size is in ULONGs (4 bytes). */
        /* Struct ~800 bytes (100 pieces * 4 bytes + overhead). Too big for queue by value usually. */
        /* Better Approach: Use a Shared Pool or circular buffer for large payloads? */
        /* Or just copy essential summary for now. */
        
        /* FIX: Let's assume for this version we only print Summary to save memory/complexity, 
           or use a pointer if the data persists (BUT MainTask stack might reuse it).
           Ideally: MainTask allocates event data on heap or pool. 
           Actually, MainTask uses stack variable `batch_payload`. It will disappear.
           We MUST copy it.
           
           Simplification: We only queue the summary data (count, total area, batch number).
           And maybe top 5 pieces?
           
           Let's copy the FULL struct into a static double-buffer or similar if queue is too small.
           Actually, `tx_queue_send` copies message to queue memory area.
           Message Size is checked against queue item size.
           Let's redefine queue to hold POINTERS to HEAP allocated copies? NO, heap is risky.
           
           Let's stick to SUMMARY for now to respect embedded constraints.
        */
        
        LgcEventDataBatchFinished_t *src = (LgcEventDataBatchFinished_t *)event->data;
        
        /* Send to queue. If queue full, drop (or wait 0). */
        /* We send the POINTER? No, src is on stack. */
        /* We must copy. */
        /* Let's use a static safe buffer for 1 pending print job (since printing is slow). */
        
        memcpy(&s_batch_data_buffer, src, sizeof(LgcEventDataBatchFinished_t));
        
        /* Signal task */
        tx_event_flags_set(&task->events, PRINTER_EVENT_BATCH_FINISHED, TX_OR);
    }
}

/**
 * @brief Printer Task Thread Entry
 */
static void printer_task_entry(ULONG param)
{
    PrinterTask_t *task = (PrinterTask_t *)param;
    ULONG actual_flags;
    char buffer[64];

    while (task->is_running)
    {
        /* Wait for Event */
        UINT status = tx_event_flags_get(
            &task->events, 
            PRINTER_EVENT_BATCH_FINISHED, 
            TX_OR_CLEAR, 
            &actual_flags, 
            TX_WAIT_FOREVER
        );

        if (status != TX_SUCCESS) continue;

        if (actual_flags & PRINTER_EVENT_BATCH_FINISHED)
        {
            if (task->printer == NULL) continue;

            /* Use the static buffer (Lock mutex if multiple producers, but here only 1 publisher typically) */
            LgcEventDataBatchFinished_t *data = &s_batch_data_buffer;

            /* Start Printing */
            task->printer->print_text(task->printer->context, "\n");
            task->printer->print_text(task->printer->context, "TECNAMIC LEATHER SYS\n");
            task->printer->print_text(task->printer->context, "--------------------\n");
            
            snprintf(buffer, sizeof(buffer), "BATCH: %lu\n", (unsigned long)data->batch_number);
            task->printer->print_text(task->printer->context, buffer);
            
            snprintf(buffer, sizeof(buffer), "CLIENT: %s\n", task->config->client_name);
            task->printer->print_text(task->printer->context, buffer); // Requires config access

            task->printer->print_text(task->printer->context, "--------------------\n");
            
            snprintf(buffer, sizeof(buffer), "TOTAL PIECES: %lu\n", (unsigned long)data->piece_count);
            task->printer->print_text(task->printer->context, buffer);
            
            snprintf(buffer, sizeof(buffer), "TOTAL AREA:   %.2f dm2\n", data->total_area);
            task->printer->print_text(task->printer->context, buffer);
            
            /* List Pieces (Optional, limit to 10 for demo) */
            task->printer->print_text(task->printer->context, "Last 5 Pieces:\n");
            uint32_t start_idx = (data->piece_count > 5) ? (data->piece_count - 5) : 0;
            for(uint32_t i=start_idx; i<data->piece_count; i++) {
                 snprintf(buffer, sizeof(buffer), "#%lu: %.2f\n", (unsigned long)(i+1), data->pieces[i].area);
                 task->printer->print_text(task->printer->context, buffer);
            }
            
            task->printer->print_text(task->printer->context, "\n\n\n");
            
            /* Cut Paper */
            task->printer->cut_paper(task->printer->context);
        }
    }
}

/* ============================= Public API =========================== */

Result_t LgcPrinterTask_Init(
    ILgcPrinter_t *printer,
    ILgcEventPublisher_t *publisher,
    LgcSystemConfig_t *config)
{
    if (printer == NULL || publisher == NULL || config == NULL) return ERR_NULL_POINTER;
    
    if (s_printer_task.is_initialized) return ERR_BUSY;

    memset(&s_printer_task, 0, sizeof(PrinterTask_t));
    s_printer_task.printer = printer;
    s_printer_task.publisher = publisher;
    s_printer_task.config = config;

    /* Initialize ThreadX objects */
    if (tx_event_flags_create(&s_printer_task.events, "PrinterEvents") != TX_SUCCESS) {
        return ERR_HARDWARE_FAULT;
    }

    /* Subscribe to Batch Finished */
    publisher->subscribe(
        publisher->context,
        printer_on_event,
        &s_printer_task,
        LGC_EVENT_BATCH_FINISHED
    );

    /* Create Thread */
    if (tx_thread_create(
            &s_printer_task.thread,
            "PrinterTask",
            printer_task_entry,
            (ULONG)&s_printer_task,
            s_printer_task.stack,
            sizeof(s_printer_task.stack),
            PRINTER_THREAD_PRIORITY,
            PRINTER_THREAD_PREEMPT,
            TX_NO_TIME_SLICE,
            TX_AUTO_START
        ) != TX_SUCCESS) 
    {
        return ERR_HARDWARE_FAULT;
    }
    
    s_printer_task.is_running = true;
    s_printer_task.is_initialized = true;

    return ERR_OK;
}

Result_t LgcPrinterTask_Deinit(void)
{
    if (!s_printer_task.is_initialized) return ERR_NOT_INITIALIZED;

    s_printer_task.is_running = false;
    
    /* Unsubscribe */
    s_printer_task.publisher->unsubscribe(s_printer_task.publisher->context, printer_on_event);

    tx_thread_terminate(&s_printer_task.thread);
    tx_thread_delete(&s_printer_task.thread);
    tx_event_flags_delete(&s_printer_task.events);
    
    s_printer_task.is_initialized = false;
    return ERR_OK;
}
