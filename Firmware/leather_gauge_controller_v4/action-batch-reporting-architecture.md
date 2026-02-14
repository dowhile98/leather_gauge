# Action Plan: Robust Batch Reporting & Snapshot Architecture

## Overview
Implement a "Snapshot-Consumer" pattern to decouple high-priority sensor measurement from slow reporting processes (HMI display and Thermal Printing). This ensures data integrity, prevents "dirty reads" during batch transitions, and handles leather pieces "in transit" during manual closures.

## Phase 1: Data Structure & IPC
**Target File:** `lgc_typedefs.h`

1. **Define `LgcBatchReport_t` Struct:**
   - Encapsulate all data required for a full report: `batch_id`, `timestamp`, `client_name`, `color`, `leather_id`, `pieces_area[]`, `total_pieces`, `total_area`, and `units`.
2. **Add Event Bits:**
   - `LGC_EVENT_SNAPSHOT_READY`: Signals reporting tasks that a new snapshot is available.
   - `LGC_EVENT_CLOSE_BATCH_REQ`: Manual request from HMI to close the current batch.

## Phase 2: Safe Closure Logic (State Machine)
**Target File:** `lgc_main_task.c`

1. **Implement "Pending Close" State:**
   - Add a global flag `bool batch_close_pending`.
   - When `LGC_EVENT_CLOSE_BATCH_REQ` is received, set `batch_close_pending = true`.
2. **Atomic Snapshot Function:**
   - Create `static void lgc_finalize_batch_snapshot(void)`.
   - **Logic:**
     - Wait until `measurements.is_measuring == false` (Hysteresis check).
     - Acquire `measurements.mutex`.
     - Copy `measurements` and `config` data into a global `finalized_batch` snapshot buffer.
     - Reset `measurements` counters (prepare for next batch).
     - Increment `config.batch_number` and save to EEPROM.
     - Release `measurements.mutex`.
     - Signal `LGC_EVENT_SNAPSHOT_READY`.

## Phase 3: Asynchronous Reporting Task
**Target File:** `lgc_report_manager.c` (New Module)

1. **Create `lgc_report_task`:**
   - Priority: Low (e.g., 15).
   - Entry point: `lgc_report_task_entry`.
2. **Printer Logic:**
   - Wait for `LGC_EVENT_SNAPSHOT_READY`.
   - Use the `ESC_POS_Printer` driver to format the ticket using data **only** from the snapshot buffer.
   - Handle printer errors (offline/no paper) without blocking measurement.

## Phase 4: HMI Synchronization
**Target File:** `lgc_hmi_task.c`

1. **Manual Trigger:**
   - Map `LGC_HMI_VP_PRINT` to set `LGC_EVENT_CLOSE_BATCH_REQ` instead of calling `lgc_increment_batch_index` directly.
2. **Report Pages (12-17):**
   - Update `lgc_hmi_update_task` to read from the `finalized_batch` snapshot instead of the live `measurements` structure when on reporting pages.
   - This ensures the user sees a static, consistent list of the last completed batch.

## Verification Checklist
- [ ] Verify that a manual close during measurement waits for the leather to clear the sensors.
- [ ] Confirm that `lgc_main_task` cycle time remains < 5ms even during a large `memcpy` of the snapshot.
- [ ] Test printer disconnection during report generation (Task should handle it gracefully).
- [ ] Ensure `batch_id` increments correctly in EEPROM after each snapshot.
