/**
 * @file    lgc_i_printer.h
 * @brief   Printer Interface (Port)
 * @author  Clean Architecture Refactor Team
 * @date    2026-02-12
 * @version 1.0.0
 *
 * @details Interface for thermal printer (ESC/POS protocol)
 *          Used for batch report printing.
 *
 * @note    INTERFACE LAYER (Port) - Implementation in adapters/peripherals/printer_adapter/
 *
 * Copyright (c) 2026 Leather Gauge Controller Team
 */

#ifndef LGC_I_PRINTER_H
#define LGC_I_PRINTER_H

#ifdef __cplusplus
extern "C"
{
#endif

/* ============================= Includes ============================= */
#include "../entities/lgc_common_types.h"

    /* ============================= Types ================================ */
    /**
     * @brief Printer text alignment
     */
    typedef enum
    {
        PRINTER_ALIGN_LEFT = 0,
        PRINTER_ALIGN_CENTER = 1,
        PRINTER_ALIGN_RIGHT = 2,
    } LgcPrinterAlign_t;

    /**
     * @brief Printer font size
     */
    typedef enum
    {
        PRINTER_FONT_NORMAL = 0,
        PRINTER_FONT_LARGE = 1,
        PRINTER_FONT_SMALL = 2,
    } LgcPrinterFontSize_t;

    /**
     * @brief Printer configuration
     */
    typedef struct
    {
        uint32_t timeout_ms;  /**< Operation timeout (ms) */
        uint8_t line_spacing; /**< Line spacing (dots) */
    } LgcPrinterConfig_t;

    /* ============================= Interface ============================ */
    /**
     * @brief Printer Interface (V-Table)
     */
    typedef struct ILgcPrinter_t
    {
        /**
         * @brief Opaque pointer to implementation context
         */
        void *context;

        /**
         * @brief Initialize printer
         *
         * @param[in] ctx    Implementation context
         * @param[in] config Configuration parameters
         * @return ERR_OK on success
         */
        Result_t (*init)(void *ctx, const LgcPrinterConfig_t *config);

        /**
         * @brief Print text line
         *
         * @param[in] ctx  Implementation context
         * @param[in] text Null-terminated string
         * @return ERR_OK on success
         *
         * @pre  ctx and text must not be NULL
         * @post Text printed with newline
         *
         * @note  Blocking operation
         * @note  Supports printf-style formatting (implementation-dependent)
         */
        Result_t (*print_text)(void *ctx, const char *text);

        /**
         * @brief Set text alignment
         *
         * @param[in] ctx   Implementation context
         * @param[in] align Alignment mode
         * @return ERR_OK on success
         */
        Result_t (*set_alignment)(void *ctx, LgcPrinterAlign_t align);

        /**
         * @brief Set font size
         *
         * @param[in] ctx  Implementation context
         * @param[in] size Font size
         * @return ERR_OK on success
         */
        Result_t (*set_font_size)(void *ctx, LgcPrinterFontSize_t size);

        /**
         * @brief Feed paper (advance N lines)
         *
         * @param[in] ctx   Implementation context
         * @param[in] lines Number of lines to feed
         * @return ERR_OK on success
         */
        Result_t (*feed_paper)(void *ctx, uint8_t lines);

        /**
         * @brief Cut paper
         *
         * @param[in] ctx Implementation context
         * @return ERR_OK on success
         *
         * @note  May not be supported by all printers
         */
        Result_t (*cut_paper)(void *ctx);

        /**
         * @brief Print barcode (optional)
         *
         * @param[in] ctx  Implementation context
         * @param[in] data Barcode data (null-terminated)
         * @return ERR_OK on success, ERR_NOT_INITIALIZED if not supported
         */
        Result_t (*print_barcode)(void *ctx, const char *data);

        /**
         * @brief Deinitialize printer
         *
         * @param[in] ctx Implementation context
         * @return ERR_OK on success
         */
        Result_t (*deinit)(void *ctx);

    } ILgcPrinter_t;

/* ============================= Helper Macros ======================== */
#define LGC_PRINTER_CALL(printer, method, ...) \
    (((printer) != NULL && (printer)->method != NULL) ? (printer)->method((printer)->context, ##__VA_ARGS__) : ERR_NULL_POINTER)

#ifdef __cplusplus
}
#endif

#endif /* LGC_I_PRINTER_H */
