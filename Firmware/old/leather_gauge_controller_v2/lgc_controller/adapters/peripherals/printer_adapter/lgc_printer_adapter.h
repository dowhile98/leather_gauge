#ifndef LGC_PRINTER_ADAPTER_H
#define LGC_PRINTER_ADAPTER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../domain/interfaces/lgc_i_printer.h"
#include "../../Third_Party/esc_pos/ESC_POS_Printer.h" // For esc_pos_printer_t
#include "lgc_interface_printer.h" // For lgc_interface_printer_writeData
#include "os_port.h" // For osDelayTask

/**
 * @brief Printer Adapter Context structure.
 *        This structure holds the internal state for the Printer adapter.
 */
typedef struct {
    esc_pos_printer_t esc_pos_handle; // ESC/POS printer handle
    bool is_initialized;
    LgcPrinterConfig_t config;
    // Add other internal state variables as needed
} LgcPrinterAdapter_t;

/**
 * @brief Initialize the Printer Adapter.
 *
 * @param[in,out] adapter Pointer to the Printer adapter instance.
 * @return ERR_OK if initialization is successful, an error code otherwise.
 */
Result_t LgcPrinterAdapter_Init(LgcPrinterAdapter_t *adapter);

/**
 * @brief Deinitialize the Printer Adapter.
 *
 * @param[in,out] adapter Pointer to the Printer adapter instance.
 * @return ERR_OK if deinitialization is successful, an error code otherwise.
 */
Result_t LgcPrinterAdapter_Deinit(LgcPrinterAdapter_t *adapter);

/**
 * @brief Get the ILgcPrinter_t interface from the Printer Adapter instance.
 *
 * @param[in,out] adapter Pointer to the Printer adapter instance.
 * @return Pointer to the ILgcPrinter_t interface, or NULL if the adapter is not initialized.
 */
const ILgcPrinter_t *LgcPrinterAdapter_GetInterface(LgcPrinterAdapter_t *adapter);

#ifdef __cplusplus
}
#endif

#endif // LGC_PRINTER_ADAPTER_H
