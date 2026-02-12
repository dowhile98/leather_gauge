#ifndef LG_ADAPTER_COMM_H
#define LG_ADAPTER_COMM_H

#include "lg_i_comm.h"

/**
 * @brief Get the LwPKT Comms Adapter Instance
 * @return const lg_i_comm_t* Pointer to the interface implementation
 */
const lg_i_comm_t* lg_adapter_comm_get_interface(void);

#endif // LG_ADAPTER_COMM_H
