#ifndef LG_I_SENSOR_H
#define LG_I_SENSOR_H

#include "lg_domain_types.h"

/**
 * @brief Sensor Interface (Port)
 * Defines the contract for interacting with the hardware sensor (ADC/Filtering).
 */
typedef struct lg_i_sensor {
    /**
     * @brief Initialize the sensor hardware
     * @param fc Filter cutoff frequency
     * @return lg_result_t
     */
    lg_result_t (*init)(float fc);

    /**
     * @brief Update filter parameters
     * @param fc New cutoff frequency
     * @return lg_result_t
     */
    lg_result_t (*set_filter)(float fc);

    /**
     * @brief Get the latest sensor data
     * @param data Pointer to data structure to fill
     * @return lg_result_t
     */
    lg_result_t (*get_data)(lg_sensor_data_t *data);

    /**
     * @brief Trigger a new measurement (if not continuous)
     * Optional, depending on implementation (DMA/Continuous vs Polling)
     * @return lg_result_t
     */
    lg_result_t (*trigger)(void);

    /**
     * @brief Process/Update loop (if polling is needed)
     * @return lg_result_t
     */
    lg_result_t (*process)(void);

} lg_i_sensor_t;

#endif // LG_I_SENSOR_H
