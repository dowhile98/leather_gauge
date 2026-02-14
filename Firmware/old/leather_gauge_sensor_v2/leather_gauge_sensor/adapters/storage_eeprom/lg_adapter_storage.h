#ifndef LG_ADAPTER_STORAGE_H
#define LG_ADAPTER_STORAGE_H

#include "lg_i_storage.h"

/**
 * @brief Get the Storage Adapter Instance
 * @return const lg_i_storage_t* Pointer to the interface implementation
 */
const lg_i_storage_t* lg_adapter_storage_get_interface(void);

#endif // LG_ADAPTER_STORAGE_H
