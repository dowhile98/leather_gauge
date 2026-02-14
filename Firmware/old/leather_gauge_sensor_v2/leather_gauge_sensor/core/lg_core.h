#ifndef LG_CORE_H
#define LG_CORE_H

#include "lg_domain_types.h"
#include "lg_i_comm.h"
#include "lg_i_sensor.h"
#include "lg_i_storage.h"

/**
 * @brief Initialize the Leather Gauge Core Logic
 * @param comm Pointer to Communication Interface
 * @param sensor Pointer to Sensor Interface
 * @param storage Pointer to Storage Interface
 * @return lg_result_t
 */
lg_result_t lg_core_init(const lg_i_comm_t *comm, 
                         const lg_i_sensor_t *sensor, 
                         const lg_i_storage_t *storage);

/**
 * @brief Main execution loop (call this in super loop)
 * @return lg_result_t
 */
lg_result_t lg_core_run(void);

#endif // LG_CORE_H
