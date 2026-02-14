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
#define REPORT_TASK_PRIORITY    15
#define REPORT_TASK_STACK_SIZE  2048

static const char *TAG = "REPORT_MGR";

/* ============================================================================
 * GLOBAL VARIABLES
 * ============================================================================ */
static LgcBatchReport_t last_finalized_batch;
static LgcLiveStatus_t current_live_status;
static OsMutex snapshot_mutex;
static OsMutex live_status_mutex;
static OsTaskId report_task_id;
static esc_pos_printer_t printer_dev;

extern OsEvent events;

/* ============================================================================
 * PRIVATE FUNCTION PROTOTYPES
 * ============================================================================ */
static void lgc_report_task_entry(void *param);
static void lgc_print_batch_report(LgcBatchReport_t *report);

/* ============================================================================
 * PUBLIC FUNCTION DEFINITIONS
 * ============================================================================ */

error_t lgc_report_manager_init(void)
{
    OsTaskParameters params = OS_TASK_DEFAULT_PARAMS;

    if (osCreateMutex(&snapshot_mutex) != TRUE)
    {
        return ERROR_FAILURE;
    }

    if (osCreateMutex(&live_status_mutex) != TRUE)
    {
        return ERROR_FAILURE;
    }

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

LgcBatchReport_t* lgc_report_get_last_snapshot(void)
{
    return &last_finalized_batch;
}

/**
 * @brief This is called by Main Task to update the snapshot
 */
void lgc_report_update_snapshot(LgcBatchReport_t *new_snapshot)
{
    osAcquireMutex(&snapshot_mutex);
    memcpy(&last_finalized_batch, new_snapshot, sizeof(LgcBatchReport_t));
    osReleaseMutex(&snapshot_mutex);
}

void lgc_report_get_live_status(LgcLiveStatus_t *out_status)
{
    if (out_status == NULL) return;
    osAcquireMutex(&live_status_mutex);
    memcpy(out_status, &current_live_status, sizeof(LgcLiveStatus_t));
    osReleaseMutex(&live_status_mutex);
}

void lgc_report_update_live_status(LgcLiveStatus_t *new_status)
{
    if (new_status == NULL) return;
    osAcquireMutex(&live_status_mutex);
    memcpy(&current_live_status, new_status, sizeof(LgcLiveStatus_t));
    osReleaseMutex(&live_status_mutex);
}

/* ============================================================================
 * PRIVATE FUNCTION DEFINITIONS
 * ============================================================================ */

static void lgc_report_task_entry(void *param)
{
    STM32_LOGI(TAG, "Report Task Started");

    for (;;)
    {
        /* Wait for snapshot ready event */
        if (osWaitForEventBits(&events, LGC_EVENT_SNAPSHOT_READY, TRUE, TRUE, INFINITE_DELAY) == TRUE)
        {
            STM32_LOGI(TAG, "New snapshot ready, starting print process...");

            /* Check if printer is connected */
            if (lgc_interface_printer_connected())
            {
                osAcquireMutex(&snapshot_mutex);
                if (last_finalized_batch.is_valid)
                {
                    lgc_print_batch_report(&last_finalized_batch);
                }
                osReleaseMutex(&snapshot_mutex);
            }
            else
            {
                STM32_LOGE(TAG, "Printer not connected, skipping print");
            }
        }
    }
}

static void lgc_print_batch_report(LgcBatchReport_t *report)
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
    
    lwprintf_snprintf(buffer, sizeof(buffer), "Lote ID: %lu", (unsigned long)report->batch_id);
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
    
    for (uint16_t i = 0; i < report->total_pieces; i++)
    {
        lwprintf_snprintf(buffer, sizeof(buffer), "%d", i + 1);
        char area_str[16];
        lwprintf_snprintf(area_str, sizeof(area_str), "%.2f", report->pieces_area[i]);
        esc_pos_print_table_row(&printer_dev, buffer, area_str, report->units == 0 ? "ft2" : "m2");
    }
    
    esc_pos_print_separator(&printer_dev, '=');
    
    esc_pos_set_size(&printer_dev, SIZE_MEDIUM);
    lwprintf_snprintf(buffer, sizeof(buffer), "TOTAL: %.2f %s", report->total_area, report->units == 0 ? "ft2" : "m2");
    esc_pos_print_line(&printer_dev, buffer);
    
    esc_pos_set_size(&printer_dev, SIZE_SMALL);
    esc_pos_feed(&printer_dev, 3);
    esc_pos_cut(&printer_dev, false);
}
