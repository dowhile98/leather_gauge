/**
 * @file    lgc_printer_task.h
 * @brief   Printer Task Interface (Observer Pattern)
 * @author  Clean Architecture Refactor Team
 * @date    2026-02-12
 * @version 1.0.0
 *
 * Copyright (c) 2026 Leather Gauge Controller Team
 */

#ifndef LGC_PRINTER_TASK_H
#define LGC_PRINTER_TASK_H

#ifdef __cplusplus
extern "C" {
#endif

/* ============================= Includes ============================= */
#include "tx_api.h"
#include "../../inc/lgc_di_container.h" /* For common types */
#include "../../domain/interfaces/lgc_i_printer.h"
#include "../../domain/interfaces/lgc_i_event_publisher.h"

/* ============================= Types ================================ */

/**
 * @brief Printer Task Initialization
 *
 * @param[in] printer     Printer interface (Adapter)
 * @param[in] publisher   Event publisher (Domain)
 * @param[in] config      System configuration (Shared)
 * @return ERR_OK on success
 */
Result_t LgcPrinterTask_Init(
    ILgcPrinter_t *printer,
    ILgcEventPublisher_t *publisher,
    LgcSystemConfig_t *config
);

/**
 * @brief Deinitialize Printer Task
 */
Result_t LgcPrinterTask_Deinit(void);

#ifdef __cplusplus
}
#endif

#endif /* LGC_PRINTER_TASK_H */
