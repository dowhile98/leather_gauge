/**
 *
 */

#ifndef LGC_TYPEDEFS_H
#define LGC_TYPEDEFS_H

//-------------------------------------------------------------------------------
// includes
//-------------------------------------------------------------------------------
#include <stdint.h>
#include <stdbool.h>
#include "os_port.h"
#include "error.h"
//-------------------------------------------------------------------------------
// defines
//-------------------------------------------------------------------------------

#ifndef LGC_SENSOR_NUMBER
#define LGC_SENSOR_NUMBER 11
#endif

#ifndef LGC_LEATHER_COUNT_MAX
#define LGC_LEATHER_COUNT_MAX 150
#endif

#ifndef LGC_LEATHER_BATCH_COUNT_MAX
#define LGC_LEATHER_BATCH_COUNT_MAX 200
#endif

//-------------------------------------------------------------------------------
// typedefs
//-------------------------------------------------------------------------------

/**
 * @brief Single leather piece measurement slot with soft-delete support.
 */
typedef struct
{
    float area;   /**< Measured area in the configured unit */
    bool deleted; /**< Soft-delete flag: true = exclude from display and print */
} LgcBatchSlot_t;

/**
 * @brief Complete batch snapshot with per-piece data and soft-delete support.
 *        Replaces LgcBatchReport_t as the canonical batch storage type.
 */
typedef struct
{
    /* Batch identification */
    uint32_t batch_id;
    uint32_t batch_index;

    /* Timestamp (from RTC at finalization) */
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hours;
    uint8_t minutes;
    uint8_t seconds;

    /* Client metadata (copied from config at finalization) */
    char client_name[16];
    char color[16];
    char leather_id[16];

    /* Individual piece measurements */
    LgcBatchSlot_t slots[LGC_LEATHER_COUNT_MAX];

    /* Counters */
    uint16_t total_slots;  /**< Slots used, including deleted entries */
    uint16_t active_count; /**< Valid (non-deleted) pieces */
    float total_area;      /**< Sum of areas for non-deleted pieces */

    /* Unit configuration */
    uint8_t units;
    uint8_t conversion;

    /* Validity flag */
    uint8_t is_valid; /**< 0 = empty/invalid, 1 = contains valid data */
} LgcBatchSnapshot_t;
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
    uint16_t total_leathers_measured;                     /* Total leathers measured */
    float current_leather_area;                           /* Accumulator for current leather area */
    float leather_measurement[LGC_LEATHER_COUNT_MAX];     /* Individual leather areas */
    float batch_measurement[LGC_LEATHER_BATCH_COUNT_MAX]; /* Batch sums */
    uint8_t is_measuring;                                 /* Measuring state flag */
    uint8_t no_detection_count;                           /* Consecutive steps with no detection */
    /*mutex*/
    OsMutex mutex;
} lgc_measurements_t;

typedef struct
{
    uint16_t batch_count;
    uint16_t leather_count;
    float current_leather_area;
    float accumulated_batch_area;
    uint8_t system_state;
    uint8_t motor_feedback;
    uint8_t motor_speed;
    uint8_t guard_status;
    uint8_t is_measuring;
} LgcLiveStatus_t;

typedef struct
{
    uint32_t batch_id;
    uint32_t batch_index;
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hours;
    uint8_t minutes;
    uint8_t seconds;
    char client_name[16];
    char color[16];
    char leather_id[16];
    float pieces_area[LGC_LEATHER_COUNT_MAX];
    uint16_t total_pieces;
    float total_area;
    uint8_t units;
    uint8_t conversion;
    uint8_t is_valid;
} LgcBatchReport_t;

typedef enum
{
    LGC_STOP = 0,
    LGC_RUNNING,
    LGC_FAIL,

} LGC_State_t;

typedef enum
{
    LGC_EVENT_STOP = 1,
    LGC_EVENT_START = 1 << 1,
    LGC_FAILURE_DETECTED = 1 << 2,
    LGC_FAILURE_CLEARED = 1 << 3,
    LGC_HMI_UPDATE_REQUIRED = 1 << 4,
    LGC_EVENT_PRINT_BATCH = 1 << 5,
    LGC_HMI_SENSOR_TEST_UPDATE = 1 << 6,
    LGC_EVENT_PRINT_BATCH_COMPLETED = 1 << 7,
    LGC_EVENT_BURST_READY = 1 << 8,
    LGC_EVENT_CLOSE_BATCH_REQ = 1 << 9,
    LGC_EVENT_ENTER_CONFIG = 1 << 10,
    LGC_EVENT_SNAPSHOT_READY = 1 << 11,
} LGC_Events_t;

typedef enum
{
    LGC_RUNNING_LED = 0,
    LGC_SPEED_LOW_LED,
    LGC_SPEED_HIGH_LED,
} LGC_LEDS_TypeDef_t;

typedef enum
{
    LGC_BUS_MODE_MODBUS = 0,
    LGC_BUS_MODE_DAISY_CHAIN
} LGC_BUS_MODE_t;
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
