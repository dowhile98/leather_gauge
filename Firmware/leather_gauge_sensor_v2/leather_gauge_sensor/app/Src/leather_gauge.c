/* ============================================================================
 * includes
 * ========================================================================= */
#include "leather_gauge.h"

// New Architecture Includes
#include "lg_core.h"
#include "lg_adapter_comm.h"
#include "lg_adapter_sensor.h"
#include "lg_adapter_storage.h"

/* ============================================================================
 * private functions prototype
 * ========================================================================= */

/* ============================================================================
 * public functions
 * ========================================================================= */
uint8_t lg_sensor_init(void)
{
    // 1. Get Interface Implementations (Dependency Injection)
    const lg_i_comm_t *comm = lg_adapter_comm_get_interface();
    const lg_i_sensor_t *sensor = lg_adapter_sensor_get_interface();
    const lg_i_storage_t *storage = lg_adapter_storage_get_interface();
    
    // 2. Initialize Core Logic
    if (lg_core_init(comm, sensor, storage) != LG_OK) {
        return 1; // Initialization Failed
    }

    return 0;
}

void lg_sensor_run(void)
{
    /*loop*/
    while (1)
    {
        /* Run Core Logic */
        lg_core_run();
    }
}

/* ============================================================================
 * private functions definitions
 * ========================================================================= */