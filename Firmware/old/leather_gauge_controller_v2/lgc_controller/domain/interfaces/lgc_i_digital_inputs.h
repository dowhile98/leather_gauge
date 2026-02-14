/**
 * @file    lgc_i_digital_inputs.h
 * @brief   Digital Inputs Interface (Port)
 * @author  Clean Architecture Refactor Team
 * @date    2026-02-13
 * @version 1.0.0
 *
 * @details Interface for handling digital inputs and buttons with debounce.
 *          Supports event-driven callbacks for state changes.
 *
 * Copyright (c) 2026 Leather Gauge Controller Team
 */

#ifndef LGC_I_DIGITAL_INPUTS_H
#define LGC_I_DIGITAL_INPUTS_H

#ifdef __cplusplus
extern "C"
{
#endif

/* ============================= Includes ============================= */
#include "../entities/lgc_common_types.h"

    /* ============================= Types ================================ */
    /**
     * @brief Digital Input IDs
     */
    typedef enum
    {
        LGC_ID_START_STOP = 0, /**< Start/Stop button */
        LGC_ID_GUARD,          /**< Motor Guard sensor */
        LGC_ID_SPEEDS,         /**< Speed selector/sensor */
        LGC_ID_FEEDBACK,       /**< Motor feedback signal */
        LGC_ID_MAX
    } LgcDigitalInputId_t;

    /**
     * @brief Digital Input Events (matches lwbtn events)
     */
    typedef enum
    {
        LGC_INPUT_EVT_PRESSED = 0,
        LGC_INPUT_EVT_RELEASED,
        LGC_INPUT_EVT_KEEPALIVE,
        LGC_INPUT_EVT_CLICK,
        LGC_INPUT_EVT_DOUBLE_CLICK,
        LGC_INPUT_EVT_LONG_PRESSED,
    } LgcDigitalInputEvent_t;

    /**
     * @brief Callback function for input events
     */
    typedef void (*LgcDigitalInputCallback_t)(LgcDigitalInputId_t input_id, LgcDigitalInputEvent_t event, void *user_ctx);

    /**
     * @brief Digital Input configuration
     */
    typedef struct
    {
        uint32_t debounce_ms; /**< Debounce time in ms */
        uint32_t poll_rate_ms; /**< Polling rate for internal task */
    } LgcDigitalInputConfig_t;

    /* ============================= Interface ============================ */
    /**
     * @brief Digital Inputs Interface (V-Table)
     */
    typedef struct ILgcDigitalInputs_t
    {
        /**
         * @brief Opaque pointer to implementation context
         */
        void *context;

        /**
         * @brief Initialize digital inputs
         * @param[in] ctx    Implementation context
         * @param[in] config Configuration parameters
         * @return ERR_OK on success
         */
        Result_t (*init)(void *ctx, const LgcDigitalInputConfig_t *config);

        /**
         * @brief Get current raw state of an input
         * @param[in] ctx      Implementation context
         * @param[in] input_id Input identifier
         * @return true if active (logic 1 or pressed), false otherwise
         */
        bool (*get_state)(void *ctx, LgcDigitalInputId_t input_id);

        /**
         * @brief Register event callback
         * @param[in] ctx      Implementation context
         * @param[in] callback Function to call on event
         * @param[in] user_ctx Context passed to the callback
         * @return ERR_OK on success
         */
        Result_t (*register_callback)(void *ctx, LgcDigitalInputCallback_t callback, void *user_ctx);

        /**
         * @brief Deinitialize digital inputs
         * @param[in] ctx Implementation context
         * @return ERR_OK on success
         */
        Result_t (*deinit)(void *ctx);

    } ILgcDigitalInputs_t;

#ifdef __cplusplus
}
#endif

#endif /* LGC_I_DIGITAL_INPUTS_H */
