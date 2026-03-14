/**
 * @file lgc_report_manager.c
 * @brief Management of asynchronous batch reporting (Printer and HMI snapshots)
 */

#include "lgc_report_manager.h"
#include "lgc_interface_printer.h"
#include "ESC_POS_Printer.h"
#include "os_port.h"
#include "stm32_log.h"
#include "lwprintf.h"
#include <string.h>

/* ============================================================================
 * DEFINES
 * ============================================================================ */
#define REPORT_TASK_PRIORITY 15
#define REPORT_TASK_STACK_SIZE 2048

static const char *TAG = "REPORT_MGR";

/* ============================================================================
 * GLOBAL VARIABLES
 * ============================================================================ */
/* --- Legacy snapshot (kept for backward-compat wrapper) --- */
static LgcBatchReport_t last_finalized_batch;
static OsMutex snapshot_mutex;

/* --- Dual-snapshot storage (new architecture) --- */
static LgcBatchSnapshot_t s_current_batch; /* Lote en curso, actualizado pieza a pieza */
static LgcBatchSnapshot_t s_last_batch;    /* Último lote cerrado, inmutable */
static OsMutex s_current_mutex;
static OsMutex s_last_mutex;

static LgcLiveStatus_t current_live_status;
static OsMutex live_status_mutex;
static OsTaskId report_task_id;
static esc_pos_printer_t printer_dev;

extern OsEvent events;

/* ============================================================================
 * PRIVATE FUNCTION PROTOTYPES
 * ============================================================================ */
static void lgc_report_task_entry(void *param);
static void lgc_print_batch_report(LgcBatchSnapshot_t *report);
static uint16_t find_slot_by_visual_index(const LgcBatchSnapshot_t *snap, uint16_t visual_index);

/* ============================================================================
 * PUBLIC FUNCTION DEFINITIONS
 * ============================================================================ */

error_t lgc_report_manager_init(void)
{
    OsTaskParameters params = OS_TASK_DEFAULT_PARAMS;

    /* Legacy snapshot mutex */
    if (osCreateMutex(&snapshot_mutex) != TRUE)
    {
        return ERROR_FAILURE;
    }

    /* New dual-snapshot mutexes */
    if (osCreateMutex(&s_current_mutex) != TRUE)
    {
        return ERROR_FAILURE;
    }

    if (osCreateMutex(&s_last_mutex) != TRUE)
    {
        return ERROR_FAILURE;
    }

    if (osCreateMutex(&live_status_mutex) != TRUE)
    {
        return ERROR_FAILURE;
    }

    /* Zero-initialize snapshot buffers */
    memset(&s_current_batch, 0, sizeof(LgcBatchSnapshot_t));
    memset(&s_last_batch, 0, sizeof(LgcBatchSnapshot_t));

    /* Initialize ESC/POS Printer Driver */
    /* Note: lgc_interface_printer_writeData is provided by the USBX printer class */
    esc_pos_init(&printer_dev, lgc_interface_printer_writeData, osDelayTask, PRINTER_80MM);

    params.priority = REPORT_TASK_PRIORITY;
    params.stackSize = REPORT_TASK_STACK_SIZE;
    report_task_id = osCreateTask("Report Task", lgc_report_task_entry, NULL, &params);

    if (report_task_id == NULL)
    {
        return ERROR_FAILURE;
    }

    return NO_ERROR;
}

LgcBatchReport_t *lgc_report_get_last_snapshot(void)
{
    return &last_finalized_batch;
}

/**
 * @brief This is called by Main Task to update the snapshot (DEPRECATED wrapper)
 */
void lgc_report_update_snapshot(LgcBatchReport_t *new_snapshot)
{
    osAcquireMutex(&snapshot_mutex);
    memcpy(&last_finalized_batch, new_snapshot, sizeof(LgcBatchReport_t));
    osReleaseMutex(&snapshot_mutex);
}

/* ============================================================================
 * NEW CURRENT-BATCH API
 * ============================================================================ */

/**
 * @brief Append a finalized leather piece to the current batch snapshot.
 */
void lgc_report_append_current_slot(const LgcBatchSlot_t *slot)
{
    if (slot == NULL)
        return;

    osAcquireMutex(&s_current_mutex);

    if (s_current_batch.total_slots < LGC_LEATHER_COUNT_MAX)
    {
        s_current_batch.slots[s_current_batch.total_slots] = *slot;
        s_current_batch.total_slots++;

        if (!slot->deleted)
        {
            s_current_batch.active_count++;
            s_current_batch.total_area += slot->area;
        }
        s_current_batch.is_valid = 1;
    }

    osReleaseMutex(&s_current_mutex);
}

/**
 * @brief Finalize the current batch: merge metadata, promote to last, reset current.
 */
void lgc_report_finalize_batch(const LgcBatchSnapshot_t *meta)
{
    if (meta == NULL)
        return;

    osAcquireMutex(&s_current_mutex);
    osAcquireMutex(&s_last_mutex);

    /* Stamp metadata onto the current batch before promoting */
    s_current_batch.batch_id = meta->batch_id;
    s_current_batch.batch_index = meta->batch_index;
    s_current_batch.year = meta->year;
    s_current_batch.month = meta->month;
    s_current_batch.day = meta->day;
    s_current_batch.hours = meta->hours;
    s_current_batch.minutes = meta->minutes;
    s_current_batch.seconds = meta->seconds;
    s_current_batch.units = meta->units;
    s_current_batch.conversion = meta->conversion;
    strncpy(s_current_batch.client_name, meta->client_name, sizeof(s_current_batch.client_name));
    strncpy(s_current_batch.color, meta->color, sizeof(s_current_batch.color));
    strncpy(s_current_batch.leather_id, meta->leather_id, sizeof(s_current_batch.leather_id));
    s_current_batch.is_valid = 1;

    /* Promote current → last */
    memcpy(&s_last_batch, &s_current_batch, sizeof(LgcBatchSnapshot_t));

    /* Reset current batch for the next cycle */
    memset(&s_current_batch, 0, sizeof(LgcBatchSnapshot_t));

    osReleaseMutex(&s_last_mutex);
    osReleaseMutex(&s_current_mutex);
}

/**
 * @brief Copy the current (in-progress) batch snapshot to caller buffer.
 */
error_t lgc_report_get_current_batch(LgcBatchSnapshot_t *out)
{
    if (out == NULL)
        return ERROR_INVALID_PARAMETER;

    osAcquireMutex(&s_current_mutex);
    memcpy(out, &s_current_batch, sizeof(LgcBatchSnapshot_t));
    osReleaseMutex(&s_current_mutex);

    return NO_ERROR;
}

/**
 * @brief Return the active piece count of the current batch (lightweight accessor).
 */
uint16_t lgc_report_get_current_active_count(void)
{
    osAcquireMutex(&s_current_mutex);
    uint16_t count = s_current_batch.active_count;
    osReleaseMutex(&s_current_mutex);
    return count;
}

/**
 * @brief Copy the last finalized batch snapshot to caller buffer.
 */
error_t lgc_report_get_last_batch(LgcBatchSnapshot_t *out)
{
    if (out == NULL)
        return ERROR_INVALID_PARAMETER;

    osAcquireMutex(&s_last_mutex);
    memcpy(out, &s_last_batch, sizeof(LgcBatchSnapshot_t));
    osReleaseMutex(&s_last_mutex);

    return NO_ERROR;
}

/**
 * @brief Soft-delete a piece by visual (1-based) index from the current batch.
 */
error_t lgc_report_delete_current_slot(uint16_t visual_index, float *out_deleted_area)
{
    if (visual_index == 0)
        return ERROR_INVALID_PARAMETER;

    osAcquireMutex(&s_current_mutex);

    uint16_t real_idx = find_slot_by_visual_index(&s_current_batch, visual_index);

    /* >= instead of == : bulletproof against any value >= LGC_LEATHER_COUNT_MAX */
    if (real_idx >= LGC_LEATHER_COUNT_MAX)
    {
        osReleaseMutex(&s_current_mutex);
        return ERROR_INVALID_PARAMETER; /* not found */
    }

    float area = s_current_batch.slots[real_idx].area;
    s_current_batch.slots[real_idx].deleted = true;

    if (s_current_batch.total_area >= area)
    {
        s_current_batch.total_area -= area;
    }
    else
    {
        s_current_batch.total_area = 0.0f;
    }

    if (s_current_batch.active_count > 0)
    {
        s_current_batch.active_count--;
    }

    osReleaseMutex(&s_current_mutex);

    if (out_deleted_area != NULL)
    {
        *out_deleted_area = area;
    }

    return NO_ERROR;
}

void lgc_report_get_live_status(LgcLiveStatus_t *out_status)
{
    if (out_status == NULL)
        return;
    osAcquireMutex(&live_status_mutex);
    memcpy(out_status, &current_live_status, sizeof(LgcLiveStatus_t));
    osReleaseMutex(&live_status_mutex);
}

void lgc_report_update_live_status(LgcLiveStatus_t *new_status)
{
    if (new_status == NULL)
        return;
    osAcquireMutex(&live_status_mutex);
    memcpy(&current_live_status, new_status, sizeof(LgcLiveStatus_t));
    osReleaseMutex(&live_status_mutex);
}

/* ============================================================================
 * PRIVATE FUNCTION DEFINITIONS
 * ============================================================================ */

/**
 * @brief Translate a 1-based visual index to the real slot index in the snapshot.
 *
 * Iterates over slots skipping deleted entries.  The visual index matches the
 * 1-based numbering shown on the DWIN display.
 *
 * @param snap          Pointer to the snapshot to search.
 * @param visual_index  1-based visual index.
 * @return Real array index, or LGC_LEATHER_COUNT_MAX if not found.
 */
static uint16_t find_slot_by_visual_index(const LgcBatchSnapshot_t *snap, uint16_t visual_index)
{
    /* Cap iteration to the array bound regardless of what total_slots holds */
    uint16_t limit = snap->total_slots;
    if (limit > LGC_LEATHER_COUNT_MAX)
    {
        limit = LGC_LEATHER_COUNT_MAX;
    }

    uint16_t visual = 0;
    for (uint16_t i = 0; i < limit; i++)
    {
        if (!snap->slots[i].deleted)
        {
            visual++;
            if (visual == visual_index)
            {
                return i;
            }
        }
    }
    return LGC_LEATHER_COUNT_MAX; /* sentinel: not found */
}

static void lgc_report_task_entry(void *param)
{
    STM32_LOGI(TAG, "Report Task Started");

    for (;;)
    {
        /* Wait for either a closed-batch snapshot OR a current-batch print request */
        if (osWaitForEventBits(&events,
                               LGC_EVENT_SNAPSHOT_READY | LGC_EVENT_PRINT_BATCH,
                               FALSE, /* any single bit suffices */
                               FALSE, /* do NOT auto-clear; we clear manually below */
                               INFINITE_DELAY) != TRUE)
        {
            continue;
        }

        /* Check printer once for both paths */
        if (!lgc_interface_printer_connected())
        {
            STM32_LOGE(TAG, "Printer not connected, skipping print");
            osClearEventBits(&events, LGC_EVENT_SNAPSHOT_READY | LGC_EVENT_PRINT_BATCH);
            continue;
        }

        /* --- Path A: print current batch without closing it --- */
        if (osGetEventBits(&events, LGC_EVENT_PRINT_BATCH) & LGC_EVENT_PRINT_BATCH)
        {
            osClearEventBits(&events, LGC_EVENT_PRINT_BATCH);
            STM32_LOGI(TAG, "Print current batch (no close)");

            /* Copy snapshot under mutex to minimise lock time during print */
            LgcBatchSnapshot_t snap;
            osAcquireMutex(&s_current_mutex);
            snap = s_current_batch;
            osReleaseMutex(&s_current_mutex);

            if (snap.is_valid)
            {
                lgc_print_batch_report(&snap);
            }
        }

        /* --- Path B: print last closed batch (triggered by LGC_EVENT_CLOSE_BATCH_REQ flow) --- */
        if (osGetEventBits(&events, LGC_EVENT_SNAPSHOT_READY) & LGC_EVENT_SNAPSHOT_READY)
        {
            osClearEventBits(&events, LGC_EVENT_SNAPSHOT_READY);
            STM32_LOGI(TAG, "New snapshot ready, printing last batch...");

            osAcquireMutex(&s_last_mutex);
            if (s_last_batch.is_valid)
            {
                lgc_print_batch_report(&s_last_batch);
            }
            osReleaseMutex(&s_last_mutex);
        }
    }
}

static void lgc_print_batch_report(LgcBatchSnapshot_t *report)
{
    char buffer[128];

    esc_pos_set_defaults(&printer_dev);
    esc_pos_set_align(&printer_dev, ALIGN_CENTER);
    esc_pos_set_size(&printer_dev, SIZE_LARGE);
    esc_pos_print_line(&printer_dev, "REPORTE DE LOTE");

    esc_pos_set_size(&printer_dev, SIZE_SMALL);
    esc_pos_feed(&printer_dev, 1);

    /* Corporate Info */
    esc_pos_print_line(&printer_dev, "EMPRESA  : CURPISCO S.A.C.");
    esc_pos_print_line(&printer_dev, "DIRECCION: AV. LOS GIRASOLES 123");
    esc_pos_print_line(&printer_dev, "TELEFONO : +51 987654321");
    esc_pos_print_line(&printer_dev, "RUC      : 20123456789");
    esc_pos_print_separator(&printer_dev, '-');

    lwprintf_snprintf(buffer, sizeof(buffer), "Lote ID: %lu", (unsigned long)report->batch_index);
    esc_pos_print_line(&printer_dev, buffer);

    lwprintf_snprintf(buffer, sizeof(buffer), "Fecha: %02d/%02d/%04d %02d:%02d:%02d",
                      report->day, report->month, report->year,
                      report->hours, report->minutes, report->seconds);
    esc_pos_print_line(&printer_dev, buffer);

    esc_pos_set_align(&printer_dev, ALIGN_LEFT);
    esc_pos_print_separator(&printer_dev, '-');

    lwprintf_snprintf(buffer, sizeof(buffer), "Cliente: %s", report->client_name);
    esc_pos_print_line(&printer_dev, buffer);
    lwprintf_snprintf(buffer, sizeof(buffer), "Color: %s", report->color);
    esc_pos_print_line(&printer_dev, buffer);
    lwprintf_snprintf(buffer, sizeof(buffer), "ID Cuero: %s", report->leather_id);
    esc_pos_print_line(&printer_dev, buffer);

    esc_pos_print_separator(&printer_dev, '-');

    esc_pos_print_table_row(&printer_dev, "ITEM", "AREA", "UNID");

    /* Iterate slots, printing only non-deleted entries (soft-delete aware) */
    uint16_t visual_index = 0;
    uint16_t limit = report->total_slots;
    if (limit > LGC_LEATHER_COUNT_MAX)
    {
        limit = LGC_LEATHER_COUNT_MAX;
    }
    for (uint16_t i = 0; i < limit; i++)
    {
        if (!report->slots[i].deleted)
        {
            visual_index++;
            char item_str[8];
            char area_str[16];
            lwprintf_snprintf(item_str, sizeof(item_str), "%d", visual_index);
            lwprintf_snprintf(area_str, sizeof(area_str), "%.2f", report->slots[i].area);
            esc_pos_print_table_row(&printer_dev, item_str, area_str,
                                    report->units == 0 ? "ft2" : "m2");
        }
    }

    esc_pos_print_separator(&printer_dev, '=');

    esc_pos_set_size(&printer_dev, SIZE_MEDIUM);
    lwprintf_snprintf(buffer, sizeof(buffer), "TOTAL: %.2f %s",
                      report->total_area, report->units == 0 ? "ft2" : "m2");
    esc_pos_print_line(&printer_dev, buffer);

    esc_pos_set_size(&printer_dev, SIZE_SMALL);
    esc_pos_feed(&printer_dev, 3);
    esc_pos_cut(&printer_dev, false);
}
