/**
 * @file lgc_report_manager.h
 * @brief Management of asynchronous batch reporting (Printer and HMI snapshots)
 */

#ifndef APP_INC_LGC_REPORT_MANAGER_H_
#define APP_INC_LGC_REPORT_MANAGER_H_

#include "lgc_typedefs.h"
#include "error.h"

/**
 * @brief Initialize the report manager and create its task
 * @return error_t Status code
 */
error_t lgc_report_manager_init(void);

/**
 * @brief Get a pointer to the last finalized batch snapshot
 * @return LgcBatchReport_t* Pointer to the snapshot
 */
LgcBatchReport_t* lgc_report_get_last_snapshot(void);

/**
 * @brief Update the internal snapshot buffer with new data
 * @param new_snapshot Pointer to the source snapshot data
 */
void lgc_report_update_snapshot(LgcBatchReport_t *new_snapshot);

/**
 * @brief Get the current live status snapshot
 * @param out_status Pointer to store the live status data
 */
void lgc_report_get_live_status(LgcLiveStatus_t *out_status);

/**
 * @brief Update the live status snapshot
 * @param new_status Pointer to the new live status data
 */
void lgc_report_update_live_status(LgcLiveStatus_t *new_status);

#endif /* APP_INC_LGC_REPORT_MANAGER_H_ */
