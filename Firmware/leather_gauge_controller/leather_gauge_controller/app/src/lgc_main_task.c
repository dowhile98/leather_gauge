/**
 *
 */

//-------------------------------------------------------------------------------
// includes
//-------------------------------------------------------------------------------
#include "lgc.h"
#include "lgc_interface_printer.h"
#include "os_port.h"
#include "lgc_module_input.h"
#include "lgc_module_eeprom.h"
//-------------------------------------------------------------------------------
// defines
//-------------------------------------------------------------------------------
#ifndef LGC_SENSOR_NUMBER
#define LGC_SENSOR_NUMBER 11
#endif

#ifndef LGC_SENSOR_READ_RETRY
#define LGC_SENSOR_READ_RETRY 4
#endif

#ifndef LGC_LEATHER_COUNT_MAX
#define LGC_LEATHER_COUNT_MAX 300
#endif

#ifndef LGC_LEATHER_BATCH_COUNT_MAX
#define LGC_LEATHER_BATCH_COUNT_MAX 200
#endif

/* Pixel width in mm (single sensor pixel) */
#ifndef LGC_PIXEL_WIDTH_MM
#define LGC_PIXEL_WIDTH_MM 10.0f
#endif

/* Encoder step distance in mm */
#ifndef LGC_ENCODER_STEP_MM
#define LGC_ENCODER_STEP_MM 5.0f
#endif

/* Number of photoreceptors per sensor */
#define LGC_PHOTORECEPTORS_PER_SENSOR 10

/* Hysteresis for leather detection (consecutive steps with no detection) */
#define LGC_LEATHER_END_HYSTERESIS 3

typedef struct
{
	/*state machine*/
	uint8_t state;
	/*sensor data*/
	uint16_t sensor_status;
	uint16_t sensor[LGC_SENSOR_NUMBER];
} lgc_t;

typedef struct
{
	uint16_t current_batch_index;						  /* Current batch index */
	uint16_t current_leather_index;						  /* Current leather index within batch */
	float current_leather_area;							  /* Accumulator for current leather area */
	float leather_measurement[LGC_LEATHER_COUNT_MAX];	  /* Individual leather areas */
	float batch_measurement[LGC_LEATHER_BATCH_COUNT_MAX]; /* Batch sums */
	uint8_t is_measuring;								  /* Measuring state flag */
	uint8_t no_detection_count;							  /* Consecutive steps with no detection */
} lgc_measurements_t;

typedef enum
{
	LGC_STOP = 0,
	LGC_RUNNING,
	LGC_FAIL,

} LGC_State_t;
//-------------------------------------------------------------------------------
// global variables
//-------------------------------------------------------------------------------
lgc_t data;
static lgc_measurements_t measurements;
static OsSemaphore encoder_flag;
static OsMutex mutex;
//-------------------------------------------------------------------------------
// private function prototype
//-------------------------------------------------------------------------------
static void lgc_encoder_callback(void);

static uint8_t lgc_get_state(void);

static uint8_t lgc_set_state(uint8_t state);

static void lgc_process_measurement(LGC_CONF_TypeDef_t *config);

static uint16_t lgc_count_active_bits(void);

static float lgc_calculate_slice_area(uint16_t active_bits);
//-------------------------------------------------------------------------------
// task definition
//-------------------------------------------------------------------------------
void lgc_main_task_entry(void *param)
{
	error_t err = NO_ERROR;
	uint8_t sensor_retry = 0;
	LGC_CONF_TypeDef_t config;
	/*create semaphore*/
	osCreateSemaphore(&encoder_flag, 0);
	/*Mutex*/
	osCreateMutex(&mutex);
	/*encoder init*/
	lgc_module_encoder_init(lgc_encoder_callback);

	for (;;)
	{
		// UML
		switch (lgc_get_state())
		{
		case LGC_STOP:
		{
			// verify start condition

			lgc_module_conf_get(&config); // load configuration

			break;
		}
		case LGC_RUNNING:
		{
			/* Verify encoder flag - proceed if pulse received */
			if (osWaitForSemaphore(&encoder_flag, 50) == TRUE)
			{
				/* Read all sensors with retry logic */
				for (uint8_t i = 0; i < LGC_SENSOR_NUMBER; i++)
				{
					sensor_retry = 0;
					err = NO_ERROR;

					/* Retry loop for Modbus read */
					do
					{
						err = lgc_modbus_read_holding_regs(i + 1, 45, &data.sensor[i], 1);
						if (err != NO_ERROR)
						{
							sensor_retry++;
							osDelayTask(20);
						}
						else
						{
							break;
						}
					} while (sensor_retry <= LGC_SENSOR_READ_RETRY);

					/* Update sensor status flags */
					if (err != NO_ERROR)
					{
						data.sensor_status |= (1 << i);
					}
					else
					{
						data.sensor_status &= ~(1 << i);
					}
				}

				/* Process measurement only if all sensors are healthy */
				if (data.sensor_status == NO_ERROR)
				{
					lgc_process_measurement(&config);
				}
			}

			/* TODO: Verify stop condition and transition to LGC_STOP */
			break;
		}
		case LGC_FAIL:
		{
			// verify reset condition
			break;
		}
		}
		/*state machine*/

		//    	for(uint8_t i = 0; i<LGC_SENSOR_NUMBER; i++)
		//    	{
		//    		err = lgc_modbus_read_holding_regs(i +1 , 45, &data.sensor[i], 1);
		//
		//    		if( err!= NO_ERROR)
		//    		{
		//    			osDelayTask(50);
		//    		}
		//    	}
		//        /* code */
		//        osDelayTask(1000);
	}
}
//-------------------------------------------------------------------------------
// callbacks
//-------------------------------------------------------------------------------
static void lgc_encoder_callback(void)
{
	// set flag
	osReleaseSemaphore(&encoder_flag);
}

static uint8_t lgc_get_state(void)
{
	return data.state;
}

static uint8_t lgc_set_state(uint8_t state)
{
	/*Mutex protection*/
	osAcquireMutex(&mutex);
	/*set state*/
	data.state = state;
	/*return state*/
	osReleaseMutex(&mutex);

	return data.state;
}

//-------------------------------------------------------------------------------
// private function definition
//-------------------------------------------------------------------------------

/**
 * @brief Count active bits (photoreceptors detecting leather) across all sensors
 * @return uint16_t Number of active photoreceptors (0-110)
 */
static uint16_t lgc_count_active_bits(void)
{
	uint16_t active_bits = 0;

	/* Iterate through all sensors */
	for (uint8_t i = 0; i < LGC_SENSOR_NUMBER; i++)
	{
		/* Count set bits only in the first 10 bits (photoreceptors per sensor) */
		uint16_t sensor_data = data.sensor[i];

		for (uint8_t bit = 0; bit < LGC_PHOTORECEPTORS_PER_SENSOR; bit++)
		{
			if (sensor_data & (1 << bit))
			{
				active_bits++;
			}
		}
	}

	return active_bits;
}

/**
 * @brief Calculate area of a slice based on number of active bits
 * @param active_bits Number of detected photoreceptors in this step
 * @return float Area in mm² (or configured units)
 */
static float lgc_calculate_slice_area(uint16_t active_bits)
{
	/* Area = width (active_bits * pixel_width) * length (encoder_step) */
	float width = active_bits * LGC_PIXEL_WIDTH_MM;
	float area = width * LGC_ENCODER_STEP_MM;

	return area;
}

/**
 * @brief Process measurement data when encoder pulse is received
 *
 * This function implements the leather detection and measurement algorithm:
 * - Detects the start of a new piece of leather
 * - Accumulates area as the leather passes through the sensors
 * - Detects the end of the leather with hysteresis
 * - Saves measurements and manages batch accumulation
 *
 * @param config Pointer to configuration structure with batch limit
 */
static void lgc_process_measurement(LGC_CONF_TypeDef_t *config)
{
	uint16_t active_bits;
	float slice_area;

	/* Count active photoreceptors in current slice */
	active_bits = lgc_count_active_bits();
	slice_area = lgc_calculate_slice_area(active_bits);

	/* ===== LEATHER DETECTION STATE MACHINE ===== */

	if (active_bits > 0)
	{
		/* Leather is currently passing through sensors */

		if (!measurements.is_measuring)
		{
			/* START OF NEW LEATHER DETECTION */
			measurements.is_measuring = 1;
			measurements.current_leather_area = 0.0f;
			measurements.no_detection_count = 0;
		}

		/* ACCUMULATE AREA */
		measurements.current_leather_area += slice_area;
		measurements.no_detection_count = 0; /* Reset hysteresis counter */
	}
	else
	{
		/* No photoreceptors active (leather has passed or not yet arrived) */

		if (measurements.is_measuring)
		{
			/* Currently measuring - increment no-detection counter for hysteresis */
			measurements.no_detection_count++;

			if (measurements.no_detection_count >= LGC_LEATHER_END_HYSTERESIS)
			{
				/* END OF LEATHER - Hysteresis threshold reached */
				measurements.is_measuring = 0;
				measurements.no_detection_count = 0;

				/* ===== SAVE LEATHER MEASUREMENT ===== */
				if (measurements.current_leather_index < LGC_LEATHER_COUNT_MAX)
				{
					measurements.leather_measurement[measurements.current_leather_index] =
						measurements.current_leather_area;
				}

				/* ===== BATCH ACCUMULATION ===== */
				if (measurements.current_batch_index < LGC_LEATHER_BATCH_COUNT_MAX)
				{
					measurements.batch_measurement[measurements.current_batch_index] +=
						measurements.current_leather_area;
				}

				/* ===== INCREMENT LEATHER INDEX ===== */
				measurements.current_leather_index++;

				/* ===== BATCH MANAGEMENT ===== */
				if (measurements.current_leather_index >= config->batch)
					measurements.current_batch_index = LGC_LEATHER_BATCH_COUNT_MAX - 1;
				/* TODO: Signal error condition (e.g., stop measurement, print warning) */
			}
		}

		/* Clear accumulator for next leather piece */
		measurements.current_leather_area = 0.0f;
	}
}

