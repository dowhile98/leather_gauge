/**
 * @file lgc_report_manager.h
 * @brief Management of asynchronous batch reporting (Printer and HMI snapshots)
 */

#ifndef APP_INC_LGC_REPORT_MANAGER_H_
#define APP_INC_LGC_REPORT_MANAGER_H_

#include "lgc_typedefs.h"
#include "error.h"

/* ============================================================================
 * INITIALIZATION
 * ============================================================================ */

/**
 * @brief Initialize the report manager and create its task.
 * @return error_t NO_ERROR on success, ERROR_FAILURE otherwise.
 */
error_t lgc_report_manager_init(void);

/* ============================================================================
 * CURRENT BATCH API
 * (Updated piece-by-piece while the batch is in progress)
 * ============================================================================ */

/**
 * @brief Append a new leather piece to the current batch snapshot.
 * @note  Called by Main Task every time a leather piece is finalized.
 * @param slot  Pointer to the new slot data (area + deleted=false).
 */
void lgc_report_append_current_slot(const LgcBatchSlot_t *slot);

/**
 * @brief Copy the current batch snapshot into caller-provided buffer.
 * @param[out] out  Destination buffer; must not be NULL.
 * @return NO_ERROR on success, ERROR_INVALID_PARAMETER if out is NULL.
 */
error_t lgc_report_get_current_batch(LgcBatchSnapshot_t *out);

/**
 * @brief Soft-delete a piece from the current batch by visual index.
 *
 * The visual index is 1-based and counts only non-deleted pieces, matching
 * the numbering shown on the DWIN display.
 *
 * @param[in]  visual_index     1-based visual index of the piece to delete.
 * @param[out] out_deleted_area Area of the deleted piece (for caller to update
 *                              internal accumulators). May be NULL.
 * @return NO_ERROR on success, ERROR_INVALID_PARAMETER if index not found.
 */
error_t lgc_report_delete_current_slot(uint16_t visual_index, float *out_deleted_area);

/* ============================================================================
 * BATCH FINALIZATION API
 * ============================================================================ */

/**
 * @brief Finalize the current batch: copy current → last, reset current.
 *
 * Metadata fields (timestamps, client info, IDs) from @p meta are merged
 * into the snapshot before promoting it to "last".  The caller (Main Task)
 * is responsible for filling @p meta with RTC and config data.
 *
 * @param meta  Pointer to a LgcBatchSnapshot_t whose metadata fields are
 *              populated by the caller.  The slots/counters from that
 *              struct are IGNORED — only metadata is taken from it.
 */
void lgc_report_finalize_batch(const LgcBatchSnapshot_t *meta);

/* ============================================================================
 * LAST BATCH API
 * (Read-only view of the last closed batch)
 * ============================================================================ */

/**
 * @brief Copy the last finalized batch snapshot into caller-provided buffer.
 * @param[out] out  Destination buffer; must not be NULL.
 * @return NO_ERROR on success, ERROR_INVALID_PARAMETER if out is NULL.
 */
error_t lgc_report_get_last_batch(LgcBatchSnapshot_t *out);

/* ============================================================================
 * LIVE STATUS API
 * ============================================================================ */

/**
 * @brief Get the current live status (lightweight scalars for HMI page 1).
 * @param[out] out_status  Destination buffer; must not be NULL.
 */
void lgc_report_get_live_status(LgcLiveStatus_t *out_status);

/**
 * @brief Update the live status snapshot.
 * @param new_status  Pointer to new status data; must not be NULL.
 */
void lgc_report_update_live_status(LgcLiveStatus_t *new_status);

/**
 * @brief Return the active (non-deleted) piece count of the current batch.
 *
 * Lightweight accessor — does NOT copy the whole snapshot, suitable for
 * use in any task context without stack pressure.
 *
 * @return Number of valid (non-deleted) pieces in the current batch.
 */
uint16_t lgc_report_get_current_active_count(void);

/* ============================================================================
 * LEGACY / COMPATIBILITY API  (DEPRECATED — kept for incremental migration)
 * ============================================================================ */

/**
 * @deprecated Use lgc_report_get_last_batch() instead.
 * @brief Get a pointer to the last finalized batch (old LgcBatchReport_t format).
 */
LgcBatchReport_t *lgc_report_get_last_snapshot(void);

/**
 * @deprecated Use lgc_report_finalize_batch() instead.
 * @brief Overwrite the last finalized batch snapshot buffer.
 */
void lgc_report_update_snapshot(LgcBatchReport_t *new_snapshot);

#endif /* APP_INC_LGC_REPORT_MANAGER_H_ */
