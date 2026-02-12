/**
 * @file    lgc_i_display.h
 * @brief   Display Interface (Port)
 * @author  Clean Architecture Refactor Team
 * @date    2026-02-12
 * @version 1.0.0
 *
 * @details Interface for HMI display communication (DWIN, Nextion, etc.)
 *          Uses Variable Pointer (VP) addressing for data exchange.
 *
 * @note    INTERFACE LAYER (Port) - Implementation in adapters/peripherals/display_adapter/
 *
 * Copyright (c) 2026 Leather Gauge Controller Team
 */

#ifndef LGC_I_DISPLAY_H
#define LGC_I_DISPLAY_H

#ifdef __cplusplus
extern "C"
{
#endif

/* ============================= Includes ============================= */
#include "../entities/lgc_common_types.h"

    /* ============================= Types ================================ */
    /**
     * @brief Display Variable Pointer (VP) address type
     *
     * @note  VP addresses are display-specific (see app/src/hmi/lgc_hmi.h)
     */
    typedef uint16_t LgcDisplayVP_t;

    /**
     * @brief Display button events (from touch screen)
     */
    typedef enum
    {
        LGC_BTN_START = 0x01,         /**< Start/Resume measurement */
        LGC_BTN_STOP = 0x02,          /**< Stop/Pause measurement */
        LGC_BTN_DELETE_LAST = 0x03,   /**< Delete last measurement */
        LGC_BTN_NEXT_BATCH = 0x04,    /**< Force next batch */
        LGC_BTN_SETTINGS = 0x05,      /**< Open settings page */
        LGC_BTN_CONFIG_SAVE = 0x10,   /**< Save configuration */
        LGC_BTN_CONFIG_CANCEL = 0x11, /**< Cancel configuration */
        LGC_BTN_UNKNOWN = 0xFF        /**< Unknown button */
    } LgcDisplayButton_t;

    /**
     * @brief Display button event structure
     */
    typedef struct
    {
        LgcDisplayButton_t button; /**< Button ID */
        uint16_t raw_vp_addr;      /**< Raw Variable Pointer address */
        uint16_t raw_value;        /**< Raw value from display */
        uint32_t timestamp_ms;     /**< Event timestamp */
    } LgcDisplayEvent_t;

    /**
     * @brief Display event callback
     * @param[in] event   Button event
     * @param[in] context User context
     */
    typedef void (*LgcDisplayCallback_t)(const LgcDisplayEvent_t *event, void *context);

    /**
     * @brief Display configuration
     */
    typedef struct
    {
        uint32_t timeout_ms;      /**< Communication timeout (ms) */
        uint32_t refresh_rate_ms; /**< Display refresh period (ms) */
        uint8_t backlight;        /**< Backlight brightness (0-100%) */
        bool enable_buzzer;       /**< Enable button click sound */
    } LgcDisplayConfig_t;

    /* ============================= Interface ============================ */
    /**
     * @brief Display Interface (V-Table)
     *
     * @details Abstracts HMI display communication protocol.
     */
    typedef struct ILgcDisplay_t
    {
        /**
         * @brief Opaque pointer to implementation context
         */
        void *context;

        /**
         * @brief Initialize display
         *
         * @param[in] ctx    Implementation context
         * @param[in] config Configuration parameters
         * @return ERR_OK on success
         */
        Result_t (*init)(void *ctx, const LgcDisplayConfig_t *config);

        /**
         * @brief Write data to display variable
         *
         * @param[in] ctx  Implementation context
         * @param[in] vp   Variable Pointer address
         * @param[in] data Data buffer to write
         * @param[in] len  Data length (bytes)
         * @return ERR_OK on success
         *
         * @pre  ctx and data must not be NULL
         * @pre  len > 0
         * @post Display variable updated
         *
         * @note  Blocking operation (~10ms)
         * @note  Data format depends on VP type (int, float, string)
         */
        Result_t (*write_variable)(
            void *ctx,
            LgcDisplayVP_t vp,
            const void *data,
            uint16_t len);

        /**
         * @brief Write uint16_t to display variable (optimized)
         */
        Result_t (*write_u16)(void *ctx, LgcDisplayVP_t vp, uint16_t value);

        /**
         * @brief Write uint32_t to display variable (optimized)
         */
        Result_t (*write_u32)(void *ctx, LgcDisplayVP_t vp, uint32_t value);

        /**
         * @brief Write float to display variable (optimized)
         */
        Result_t (*write_float)(void *ctx, LgcDisplayVP_t vp, float value);

        /**
         * @brief Write text to display variable
         */
        Result_t (*write_text)(void *ctx, LgcDisplayVP_t vp, const char *text);

        /**
         * @brief Read data from display variable
         *
         * @param[in]  ctx      Implementation context
         * @param[in]  vp       Variable Pointer address
         * @param[out] out_data Buffer for read data
         * @param[in]  max_len  Buffer size (bytes)
         * @return ERR_OK on success
         *
         * @pre  ctx and out_data must not be NULL
         * @pre  max_len > 0
         * @post If ERR_OK: out_data populated
         *
         * @note  Blocking operation (~10ms)
         */
        Result_t (*read_variable)(
            void *ctx,
            LgcDisplayVP_t vp,
            void *out_data,
            uint16_t max_len);

        /**
         * @brief Change display page
         *
         * @param[in] ctx     Implementation context
         * @param[in] page_id Page number
         * @return ERR_OK on success
         *
         * @pre  ctx must not be NULL
         * @post Display shows requested page
         */
        Result_t (*change_page)(void *ctx, uint8_t page_id);

        /**
         * @brief Attach button event callback
         * @param[in] ctx      Implementation context
         * @param[in] callback Event callback
         * @param[in] user_ctx User context for callback
         * @return ERR_OK on success
         */
        Result_t (*attach_callback)(void *ctx, LgcDisplayCallback_t callback, void *user_ctx);

        /**
         * @brief Detach button event callback
         * @param[in] ctx Implementation context
         * @return ERR_OK on success
         */
        Result_t (*detach_callback)(void *ctx);

        /**
         * @brief Process display events (non-blocking)
         * @param[in] ctx Implementation context
         * @note Call from HMI task periodically
         * @return ERR_OK on success
         */
        Result_t (*process)(void *ctx);

        /**
         * @brief Deinitialize display
         *
         * @param[in] ctx Implementation context
         * @return ERR_OK on success
         */
        Result_t (*deinit)(void *ctx);

    } ILgcDisplay_t;

/* ============================= Helper Macros ======================== */
#define LGC_DISPLAY_CALL(display, method, ...) \
    (((display) != NULL && (display)->method != NULL) ? (display)->method((display)->context, ##__VA_ARGS__) : ERR_NULL_POINTER)

#ifdef __cplusplus
}
#endif

#endif /* LGC_I_DISPLAY_H */
