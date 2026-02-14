#ifndef LEATHER_GAUGE_H
#define LEATHER_GAUGE_H

#ifdef __cplusplus
extern "C"
{
#endif

/* ============================================================================
 * Includes
 * ========================================================================= */
#include <stdint.h>
#include <stdbool.h>
#include <leather_gauge_typedefs.h>
    /* ============================================================================
     * Defines and Macros
     * ========================================================================= */

    /* ============================================================================
     * Type Definitions
     * ========================================================================= */

    /* ============================================================================
     * Function Prototypes
     * ========================================================================= */

    uint8_t lg_sensor_init(void);

    void lg_sensor_run(void);

    LG_OP_MODE_t lg_sensor_get_mode(void);

    void lg_sensor_set_mode(LG_OP_MODE_t mode);

#ifdef __cplusplus
}
#endif

#endif /* LEATHER_GAUGE_H */