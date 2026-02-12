/**
 * @file    lgc_i_encoder.h
 * @brief   Encoder Interface (Port)
 * @author  Clean Architecture Refactor Team
 * @date    2026-02-12
 * @version 1.0.0
 *
 * @details Interface for rotary encoder interaction.
 *          Critical for synchronization: Each pulse triggers measurement slice.
 *
 * @note    INTERFACE LAYER (Port) - Implementation in adapters/peripherals/encoder_adapter/
 *
 * Copyright (c) 2026 Leather Gauge Controller Team
 */

#ifndef LGC_I_ENCODER_H
#define LGC_I_ENCODER_H

#ifdef __cplusplus
extern "C"
{
#endif

/* ============================= Includes ============================= */
#include "../entities/lgc_common_types.h"

    /* ============================= Callback Types ======================= */
    /**
     * @brief Encoder pulse callback signature
     *
     * @param[in] position Current encoder position (pulses since init)
     * @param[in] user_ctx User-provided context (from attach_callback)
     *
     * @note  Called from ISR context - MUST be minimal and fast (<1ms)
     * @note  Recommended pattern: Set event flag, process in task context
     *
     * @warning Do NOT:
     *          - Call blocking functions (printf, tx_mutex_get with wait)
     *          - Perform heavy computation
     *          - Access non-atomic shared data without protection
     */
    typedef void (*LgcEncoderPulseCallback_t)(uint32_t position, void *user_ctx);

    /* ============================= Configuration ======================== */
    /**
     * @brief Encoder configuration
     */
    typedef struct
    {
        uint32_t pulses_per_revolution; /**< Encoder resolution (PPR) */
        bool enable_interrupts;         /**< Use ISR mode (vs polling) */
        uint8_t debounce_ms;            /**< Debounce time (ms) */
    } LgcEncoderConfig_t;

    /* ============================= Interface ============================ */
    /**
     * @brief Encoder Interface (V-Table)
     *
     * @details Abstracts encoder hardware (GPIO EXTI, Timer, etc.)
     *
     * Usage Pattern:
     * @code
     * // In domain/use_cases/measure/lgc_uc_measure_area.c
     * static void encoder_pulse_handler(uint32_t pos, void *ctx) {
     *     LgcMeasureAreaUC_t *uc = (LgcMeasureAreaUC_t *)ctx;
     *     tx_event_flags_set(&uc->events, EVENT_ENCODER_PULSE, TX_OR);
     * }
     *
     * Result_t LgcUC_MeasureArea_Init(/* ... ) {
     *     uc->encoder->attach_callback(
     *         uc->encoder->context,
     *         encoder_pulse_handler,
     *         uc
     *     );
     * }
     * @endcode
     */
    typedef struct ILgcEncoder_t
    {
        /**
         * @brief Opaque pointer to implementation context
         */
        void *context;

        /**
         * @brief Initialize encoder
         *
         * @param[in] ctx    Implementation context
         * @param[in] config Configuration parameters
         * @return ERR_OK on success
         *
         * @pre  ctx must not be NULL
         * @post Encoder ready, position = 0
         */
        Result_t (*init)(void *ctx, const LgcEncoderConfig_t *config);

        /**
         * @brief Get current encoder position
         *
         * @param[in]  ctx          Implementation context
         * @param[out] out_position Current position (pulses)
         * @return ERR_OK on success
         *
         * @pre  ctx and out_position must not be NULL
         * @post out_position contains current encoder count
         *
         * @note  Thread-safe (atomic read or mutex-protected)
         */
        Result_t (*get_position)(void *ctx, uint32_t *out_position);

        /**
         * @brief Reset encoder position to zero
         *
         * @param[in] ctx Implementation context
         * @return ERR_OK on success
         *
         * @pre  ctx must not be NULL
         * @post Encoder position = 0
         *
         * @note  Thread-safe (atomic write or mutex-protected)
         */
        Result_t (*reset_position)(void *ctx);

        /**
         * @brief Attach pulse callback (ISR mode)
         *
         * @param[in] ctx      Implementation context
         * @param[in] callback Function to call on each pulse
         * @param[in] user_ctx User context passed to callback
         * @return ERR_OK on success
         *
         * @pre  ctx and callback must not be NULL
         * @pre  init() must have been called with enable_interrupts = true
         * @post Callback registered, will be called on each pulse
         *
         * @note  Only ONE callback supported (last call wins)
         * @note  Pass NULL callback to detach
         *
         * @warning Callback executed in ISR context - must be fast!
         */
        Result_t (*attach_callback)(
            void *ctx,
            LgcEncoderPulseCallback_t callback,
            void *user_ctx);

        /**
         * @brief Detach pulse callback
         *
         * @param[in] ctx Implementation context
         * @return ERR_OK on success
         *
         * @post Callback detached, no further calls
         */
        Result_t (*detach_callback)(void *ctx);

        /**
         * @brief Deinitialize encoder
         *
         * @param[in] ctx Implementation context
         * @return ERR_OK on success
         *
         * @post Resources released, callbacks detached
         */
        Result_t (*deinit)(void *ctx);

    } ILgcEncoder_t;

/* ============================= Helper Macros ======================== */
/**
 * @brief Safely call encoder method
 */
#define LGC_ENCODER_CALL(encoder, method, ...) \
    (((encoder) != NULL && (encoder)->method != NULL) ? (encoder)->method((encoder)->context, ##__VA_ARGS__) : ERR_NULL_POINTER)

#ifdef __cplusplus
}
#endif

#endif /* LGC_I_ENCODER_H */
