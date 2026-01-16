/**
 *
 */

#ifndef LGC_TYPEDEFS_H
#define LGC_TYPEDEFS_H

//-------------------------------------------------------------------------------
// includes
//-------------------------------------------------------------------------------
#include <stdint.h>
#include "os_port.h"
#include "error.h"
//-------------------------------------------------------------------------------
// defines
//-------------------------------------------------------------------------------

#ifndef LGC_SENSOR_NUMBER
#define LGC_SENSOR_NUMBER 11
#endif

#ifndef LGC_LEATHER_COUNT_MAX
#define LGC_LEATHER_COUNT_MAX 300
#endif

#ifndef LGC_LEATHER_BATCH_COUNT_MAX
#define LGC_LEATHER_BATCH_COUNT_MAX 200
#endif

//-------------------------------------------------------------------------------
// typedefs
//-------------------------------------------------------------------------------
typedef enum
{
    LGC_DI_START_STOP = 0,
    LGC_DI_GUARD,
    LGC_DI_SPEEDS,
    LGC_DI_FEEDBACK,
    LGC_INPUT_MAX,
} LGC_INPUTS_TypeDef_t;

typedef struct
{
    /*state machine*/
    uint8_t state;
    /*sensor data*/
    uint16_t sensor_status;
    /*sensor data*/
    uint16_t sensor[LGC_SENSOR_NUMBER];
    /*set start/stop flag*/
    uint8_t start_stop_flag;
    /*guard motor*/
    uint8_t guard_motor;
    /*speed motor*/
    uint8_t speed_motor;
    /*feedback motor*/
    uint8_t feedback_motor;

} lgc_t;

typedef struct
{
    uint16_t current_batch_index;                         /* Current batch index */
    uint16_t current_leather_index;                       /* Current leather index within batch */
    float current_leather_area;                           /* Accumulator for current leather area */
    float leather_measurement[LGC_LEATHER_COUNT_MAX];     /* Individual leather areas */
    float batch_measurement[LGC_LEATHER_BATCH_COUNT_MAX]; /* Batch sums */
    uint8_t is_measuring;                                 /* Measuring state flag */
    uint8_t no_detection_count;                           /* Consecutive steps with no detection */
} lgc_measurements_t;

typedef enum
{
    LGC_STOP = 0,
    LGC_RUNNING,
    LGC_FAIL,

} LGC_State_t;

typedef enum
{
    LGC_EVENT_STOP = 1,
    LGC_EVENT_START = 1 << 2,
    LGC_FAILURE_DETECTED = 1 << 3,
    LGC_FAILURE_CLEARED = 1 << 4,
    LGC_HMI_UPDATE_REQUIRED = 1 << 5,
    LGC_EVENT_PRINT_BATCH = 1 << 6,
} LGC_Events_t;

typedef enum
{
    LGC_RUNNING_LED = 0,
    LGC_SPEED_LOW_LED,
    LGC_SPEED_HIGH_LED,
} LGC_LEDS_TypeDef_t;
//-------------------------------------------------------------------------------
// private function prototype
//-------------------------------------------------------------------------------

//-------------------------------------------------------------------------------
// task definition
//-------------------------------------------------------------------------------

//-------------------------------------------------------------------------------
// private function definition
//-------------------------------------------------------------------------------

#endif