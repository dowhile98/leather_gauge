#ifndef LG_ADAPTER_SENSOR_H
#define LG_ADAPTER_SENSOR_H

#include "lg_i_sensor.h"

/**
 * @brief Get the STM32 Sensor Adapter Instance
 * @return const lg_i_sensor_t* Pointer to the interface implementation
 */
const lg_i_sensor_t* lg_adapter_sensor_get_interface(void);

#endif // LG_ADAPTER_SENSOR_H
