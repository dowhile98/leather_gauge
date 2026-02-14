#ifndef LG_I_STORAGE_H
#define LG_I_STORAGE_H

#include "lg_domain_types.h"

/**
 * @brief Storage Interface (Port)
 * Defines the contract for persistent storage (EEPROM).
 */
typedef struct lg_i_storage {
    /**
     * @brief Initialize storage hardware
     * @return lg_result_t
     */
    lg_result_t (*init)(void);

    /**
     * @brief Load configuration from storage
     * @param config Pointer to config structure to fill
     * @return lg_result_t
     */
    lg_result_t (*load_config)(lg_config_t *config);

    /**
     * @brief Save configuration to storage
     * @param config Pointer to config structure to save
     * @return lg_result_t
     */
    lg_result_t (*save_config)(const lg_config_t *config);

    /**
     * @brief Reset configuration to factory defaults
     * @return lg_result_t
     */
    lg_result_t (*factory_reset)(void);

} lg_i_storage_t;

#endif // LG_I_STORAGE_H
