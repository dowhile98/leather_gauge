/**
 * @file    lgc_i_event_publisher.h
 * @brief   Event Publisher Interface (Observer Pattern)
 * @author  Clean Architecture Refactor Team (TDD)
 * @date    2026-0 2-12
 * @version 1.0.0
 *
 * @details Implements Observer Pattern in embedded C.
 *          Decouples domain logic from presentation/infrastructure.
 *
 * **Problem (Before):**
 *   - HMI polls measurement core every 50ms (2% CPU waste)
 *   - Printer polls batch status (latency up to 100ms)
 *   - Tight coupling: Core knows about HMI/Printer
 *
 * **Solution (After):**
 *   - Core publishes events when state changes
 *   - HMI/Printer subscribe to relevant events
 *   - Zero polling, ~0.1% CPU usage for events
 *   - Core has ZERO knowledge of observers
 *
 * **Architecture:**
 *   ```
 *   MeasurementCore (Subject)
 *         |
 *         | Publishes events (MEASUREMENT_UPDATED, PIECE_FINISHED, etc.)
 *         v
 *   EventPublisher (Holds list of observers)
 *         |
 *         +---> HMI Observer (updates display)
 *         +---> Printer Observer (prints on BATCH_FINISHED only)
 *         +---> Logger Observer (records events)
 *   ```
 *
 * Copyright (c) 2026 Leather Gauge Controller Team
 */

#ifndef LGC_I_EVENT_PUBLISHER_H
#define LGC_I_EVENT_PUBLISHER_H

#ifdef __cplusplus
extern "C"
{
#endif

/* ============================= Includes ============================= */
#include "../entities/lgc_common_types.h"
#include "../entities/lgc_measurement_entity.h"
#include <stdint.h>
#include <stdbool.h>

/* ============================= Constants ============================ */
/** Maximum number of observers (subscribers) */
#define LGC_MAX_OBSERVERS 8U

    /* ============================= Types ================================ */
    /**
     * @brief Event types (bitmask)
     */
    typedef enum
    {
        LGC_EVENT_NONE = 0x00,                /**< No event */
        LGC_EVENT_MEASUREMENT_UPDATED = 0x01, /**< Slice processed, area updated */
        LGC_EVENT_PIECE_STARTED = 0x02,       /**< New piece detected */
        LGC_EVENT_PIECE_FINISHED = 0x04,      /**< Piece complete (hysteresis met) */
        LGC_EVENT_BATCH_FINISHED = 0x08,      /**< Batch complete (max pieces reached) */
        LGC_EVENT_ERROR = 0x20,               /**< Error occurred */
        LGC_EVENT_ALL = 0xFF                  /**< Subscribe to all events */
    } LgcEventType_t;

    /**
     * @brief Event data payloads (union for different event types)
     */
    typedef union
    {
        /** MEASUREMENT_UPDATED data */
        struct
        {
            float current_area_m2;        /**< Current piece area (m²) */
            float accumulated_batch_m2;   /**< Batch accumulated area (m²) */
            uint32_t active_bits;         /**< Active photocells in last slice */
            uint32_t current_piece_index; /**< Current piece index in batch */
        } measurement_updated;

        /** PIECE_FINISHED data */
        struct
        {
            uint32_t piece_index;   /**< Index of finished piece */
            float piece_area_m2;    /**< Final area of piece (m²) */
            uint32_t slice_count;   /**< Total slices in this piece */
            bool is_batch_finished; /**< true if this was last piece in batch */
        } piece_finished;

        /** BATCH_FINISHED data */
        struct
        {
            uint32_t batch_number;     /**< Batch number */
            uint32_t piece_count;      /**< Total pieces in batch */
            float total_batch_area_m2; /**< Total batch area (m²) */
            const float *pieces_array; /**< Pointer to array of piece areas */
        } batch_finished;

        /** ERROR data */
        struct
        {
            Result_t error_code;       /**< Error code */
            const char *error_message; /**< Error description */
        } error;

    } LgcEventData_t;

    /**
     * @brief Event structure
     */
    typedef struct
    {
        LgcEventType_t type;   /**< Event type */
        uint32_t timestamp_ms; /**< Event timestamp (HAL_GetTick) */
        LgcEventData_t data;   /**< Event-specific data */
    } LgcEvent_t;

    /**
     * @brief Event callback function
     * @param[in] event   Event structure
     * @param[in] context User context (observer-specific)
     *
     * @note Called in publisher's task context (NOT ISR)
     * @note Keep callback execution short (<10ms)
     */
    typedef void (*LgcEventCallback_t)(const LgcEvent_t *event, void *context);

    /**
     * @brief Observer structure (internal)
     */
    typedef struct
    {
        LgcEventCallback_t callback; /**< Callback function */
        void *context;               /**< User context */
        LgcEventType_t event_mask;   /**< Bitmask of subscribed events */
        bool is_active;              /**< Observer active flag */
    } LgcObserver_t;

    /**
     * @brief Event Publisher Interface (V-Table)
     */
    typedef struct ILgcEventPublisher_t
    {
        /* ===== Context (opaque pointer to implementation) ===== */
        void *context; /**< Private publisher context */

        /* ===== Initialization ===== */
        /**
         * @brief Initialize event publisher
         * @param[in] ctx Publisher context
         * @return ERR_OK on success
         */
        Result_t (*init)(void *ctx);

        /* ===== Observer Management ===== */
        /**
         * @brief Subscribe to events
         * @param[in] ctx        Publisher context
         * @param[in] callback   Callback function (must not be NULL)
         * @param[in] user_ctx   User context (can be NULL)
         * @param[in] event_mask Bitmask of events to subscribe (e.g., LGC_EVENT_MEASUREMENT_UPDATED | LGC_EVENT_PIECE_FINISHED)
         * @return ERR_OK on success
         * @retval ERR_FULL if maximum observers reached
         *
         * @pre  callback must not be NULL
         * @post Observer added to list
         *
         * @example
         * ```c
         * // HMI subscribes to measurement updates and piece finished
         * publisher->subscribe(publisher->context, hmi_event_handler, hmi_ctx,
         *     LGC_EVENT_MEASUREMENT_UPDATED | LGC_EVENT_PIECE_FINISHED);
         *
         * // Printer subscribes ONLY to batch finished
         * publisher->subscribe(publisher->context, printer_event_handler, printer_ctx,
         *     LGC_EVENT_BATCH_FINISHED);
         * ```
         */
        Result_t (*subscribe)(
            void *ctx,
            LgcEventCallback_t callback,
            void *user_ctx,
            LgcEventType_t event_mask);

        /**
         * @brief Unsubscribe from events
         * @param[in] ctx      Publisher context
         * @param[in] callback Callback function to remove
         * @return ERR_OK on success
         */
        Result_t (*unsubscribe)(void *ctx, LgcEventCallback_t callback);

        /* ===== Event Publishing ===== */
        /**
         * @brief Publish event to all subscribers
         * @param[in] ctx   Publisher context
         * @param[in] event Event to publish
         * @return ERR_OK on success
         *
         * @pre  event must not be NULL
         * @post All matching observers notified
         *
         * @note Thread-safe (uses mutex internally)
         * @note Callbacks executed synchronously in caller's context
         *
         * @warning Do NOT call from ISR (use event flags instead)
         */
        Result_t (*publish)(void *ctx, const LgcEvent_t *event);

        /* ===== Cleanup ===== */
        /**
         * @brief Deinitialize publisher
         * @param[in] ctx Publisher context
         * @return ERR_OK on success
         */
        Result_t (*deinit)(void *ctx);

    } ILgcEventPublisher_t;

/* ============================= Helper Macros ======================== */
/**
 * @brief Initialize event structure
 */
#define LGC_EVENT_INIT(event_ptr, event_type_val)              \
    do                                                         \
    {                                                          \
        (event_ptr)->type = (event_type_val);                  \
        (event_ptr)->timestamp_ms = HAL_GetTick();             \
        memset(&(event_ptr)->data, 0, sizeof(LgcEventData_t)); \
    } while (0)

#ifdef __cplusplus
}
#endif

#endif /* LGC_I_EVENT_PUBLISHER_H */
