#include "lgc_printer_adapter.h"
#include <string.h> // For memset, strlen
#include <stdarg.h> // For va_list
#include <stdio.h>  // For vsnprintf

// Forward declarations for the interface functions
static Result_t printer_init_interface(void *ctx, const LgcPrinterConfig_t *config);
static Result_t printer_print_text(void *ctx, const char *text);
static Result_t printer_set_alignment(void *ctx, LgcPrinterAlign_t align);
static Result_t printer_set_font_size(void *ctx, LgcPrinterFontSize_t size);
static Result_t printer_feed_paper(void *ctx, uint8_t lines);
static Result_t printer_cut_paper(void *ctx);
static Result_t printer_print_barcode(void *ctx, const char *data);
static Result_t printer_deinit_interface(void *ctx);

// Internal helper for delay (required by ESC/POS library)
static void printer_delay_ms(uint32_t ms);

// Interface definition
static const ILgcPrinter_t s_printer_interface = {
    .context = NULL, // Will be set during LgcPrinterAdapter_Init
    .init = printer_init_interface,
    .print_text = printer_print_text,
    .set_alignment = printer_set_alignment,
    .set_font_size = printer_set_font_size,
    .feed_paper = printer_feed_paper,
    .cut_paper = printer_cut_paper,
    .print_barcode = printer_print_barcode,
    .deinit = printer_deinit_interface,
};

Result_t LgcPrinterAdapter_Init(LgcPrinterAdapter_t *adapter) {
    if (adapter == NULL) {
        return ERR_NULL_POINTER;
    }

    memset(adapter, 0, sizeof(LgcPrinterAdapter_t));

    // Note: lgc_interface_printer_init() is NOT called here because it is already
    // initialized in MX_USBX_Host_Init() (see app_usbx_host.c).
    // The adapter assumes the USB host stack is running.

    // Initialize ESC/POS library with the USBX write function and a delay function
    esc_pos_init(&adapter->esc_pos_handle, lgc_interface_printer_writeData, printer_delay_ms, PRINTER_80MM);

    adapter->is_initialized = true;

    // Set the context for the interface
    ((ILgcPrinter_t *)&s_printer_interface)->context = adapter;

    return ERR_OK;
}

Result_t LgcPrinterAdapter_Deinit(LgcPrinterAdapter_t *adapter) {
    if (adapter == NULL) {
        return ERR_NULL_POINTER;
    }
    if (!adapter->is_initialized) {
        return ERR_NOT_INITIALIZED;
    }

    // No specific deinit for lgc_interface_printer (USBX handles connections/disconnections)
    adapter->is_initialized = false;
    return ERR_OK;
}

const ILgcPrinter_t *LgcPrinterAdapter_GetInterface(LgcPrinterAdapter_t *adapter) {
    if (adapter == NULL || !adapter->is_initialized) {
        return NULL;
    }
    // Ensure the context is correctly set for this instance
    ((ILgcPrinter_t *)&s_printer_interface)->context = adapter;
    return &s_printer_interface;
}

static Result_t printer_init_interface(void *ctx, const LgcPrinterConfig_t *config) {
    LgcPrinterAdapter_t *adapter = (LgcPrinterAdapter_t *)ctx;
    if (adapter == NULL || config == NULL) {
        return ERR_NULL_POINTER;
    }
    if (!adapter->is_initialized) {
        return ERR_NOT_INITIALIZED;
    }

    // Store config
    memcpy(&adapter->config, config, sizeof(LgcPrinterConfig_t));

    // Reset printer and set defaults via ESC/POS
    if (esc_pos_reset(&adapter->esc_pos_handle) != 0) return ERR_HARDWARE_FAULT;
    if (esc_pos_set_defaults(&adapter->esc_pos_handle) != 0) return ERR_HARDWARE_FAULT;

    return ERR_OK;
}

static Result_t printer_print_text(void *ctx, const char *text) {
    LgcPrinterAdapter_t *adapter = (LgcPrinterAdapter_t *)ctx;
    if (adapter == NULL || text == NULL) {
        return ERR_NULL_POINTER;
    }
    if (!adapter->is_initialized) {
        return ERR_NOT_INITIALIZED;
    }

    // Check if printer is connected before attempting to print
    if (lgc_interface_printer_connected() == 0)
    {
        return ERR_HARDWARE_FAULT; // Printer not connected
    }

    // The ESC/POS print_text already handles newlines when using 

    if (esc_pos_print_text(&adapter->esc_pos_handle, (char *)text) != 0) return ERR_HARDWARE_FAULT;

    return ERR_OK;
}

static Result_t printer_set_alignment(void *ctx, LgcPrinterAlign_t align) {
    LgcPrinterAdapter_t *adapter = (LgcPrinterAdapter_t *)ctx;
    if (adapter == NULL) {
        return ERR_NULL_POINTER;
    }
    if (!adapter->is_initialized) {
        return ERR_NOT_INITIALIZED;
    }

    align_t esc_pos_align;
    switch (align) {
        case PRINTER_ALIGN_LEFT:   esc_pos_align = ALIGN_LEFT; break;
        case PRINTER_ALIGN_CENTER: esc_pos_align = ALIGN_CENTER; break;
        case PRINTER_ALIGN_RIGHT:  esc_pos_align = ALIGN_RIGHT; break;
        default: return ERR_INVALID_PARAM;
    }
    if (esc_pos_set_align(&adapter->esc_pos_handle, esc_pos_align) != 0) return ERR_HARDWARE_FAULT;
    return ERR_OK;
}

static Result_t printer_set_font_size(void *ctx, LgcPrinterFontSize_t size) {
    LgcPrinterAdapter_t *adapter = (LgcPrinterAdapter_t *)ctx;
    if (adapter == NULL) {
        return ERR_NULL_POINTER;
    }
    if (!adapter->is_initialized) {
        return ERR_NOT_INITIALIZED;
    }

    text_size_t esc_pos_size;
    switch (size) {
        case PRINTER_FONT_NORMAL: esc_pos_size = SIZE_MEDIUM; break; // Map normal to medium for simplicity
        case PRINTER_FONT_LARGE:  esc_pos_size = SIZE_LARGE; break;
        case PRINTER_FONT_SMALL:  esc_pos_size = SIZE_SMALL; break;
        default: return ERR_INVALID_PARAM;
    }
    if (esc_pos_set_size(&adapter->esc_pos_handle, esc_pos_size) != 0) return ERR_HARDWARE_FAULT;
    return ERR_OK;
}


static Result_t printer_feed_paper(void *ctx, uint8_t lines) {
    LgcPrinterAdapter_t *adapter = (LgcPrinterAdapter_t *)ctx;
    if (adapter == NULL) {
        return ERR_NULL_POINTER;
    }
    if (!adapter->is_initialized) {
        return ERR_NOT_INITIALIZED;
    }

    if (esc_pos_feed(&adapter->esc_pos_handle, lines) != 0) return ERR_HARDWARE_FAULT;
    return ERR_OK;
}

static Result_t printer_cut_paper(void *ctx) {
    LgcPrinterAdapter_t *adapter = (LgcPrinterAdapter_t *)ctx;
    if (adapter == NULL) {
        return ERR_NULL_POINTER;
    }
    if (!adapter->is_initialized) {
        return ERR_NOT_INITIALIZED;
    }

    if (esc_pos_cut(&adapter->esc_pos_handle, true) != 0) return ERR_HARDWARE_FAULT; // Always partial cut for now
    return ERR_OK;
}

static Result_t printer_print_barcode(void *ctx, const char *data) {
    LgcPrinterAdapter_t *adapter = (LgcPrinterAdapter_t *)ctx;
    if (adapter == NULL) {
        return ERR_NULL_POINTER;
    }
    if (!adapter->is_initialized) {
        return ERR_NOT_INITIALIZED;
    }
    // Stub implementation - printing barcode is complex and printer-specific
    // For now, only basic Code128 with default height
    if (esc_pos_print_barcode(&adapter->esc_pos_handle, (char*)data, BARCODE_CODE128, 50) != 0) return ERR_HARDWARE_FAULT;
    return ERR_OK;
}

static Result_t printer_deinit_interface(void *ctx) {
    // Adapter deinit is handled by LgcPrinterAdapter_Deinit
    return LgcPrinterAdapter_Deinit((LgcPrinterAdapter_t *)ctx);
}

static void printer_delay_ms(uint32_t ms)
{
    osDelayTask(ms); // Assumes OSAL osDelayTask is available
}
