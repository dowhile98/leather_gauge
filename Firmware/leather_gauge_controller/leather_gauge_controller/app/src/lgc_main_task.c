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
#include "lwbtn.h"
#include "lgc_module_encoder.h"

//-------------------------------------------------------------------------------
// defines
//-------------------------------------------------------------------------------

#ifndef LGC_SENSOR_READ_RETRY
#define LGC_SENSOR_READ_RETRY 4
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
#ifndef LGC_LEATHER_END_HYSTERESIS
#define LGC_LEATHER_END_HYSTERESIS 3
#endif

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

static void lgc_set_leds(uint8_t led, uint8_t state);
/**
 * @brief Process measurement data when encoder pulse is received
 *
 * Implements the leather detection and measurement algorithm with state machine.
 * Returns status code indicating measurement event.
 *
 * @param config Pointer to configuration structure with batch limit
 * @return uint8_t Status code:
 *         - 0: No leather detected (idle state)
 *         - 1: Leather measurement completed (end of leather)
 *         - 2: Batch measurement completed (batch full)
 */
static uint8_t lgc_process_measurement(LGC_CONF_TypeDef_t *config);

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
	uint8_t measurement_event; /* Event status from measurement processing */
	/*create semaphore*/
	osCreateSemaphore(&encoder_flag, 0);
	/*Mutex*/
	osCreateMutex(&mutex);

	/*encoder init*/
	lgc_module_encoder_init(lgc_encoder_callback);

	for (;;)
	{
		lgc_module_conf_get(&config); // load configuration
		// UML
		switch (lgc_get_state())
		{
		case LGC_STOP:
		{
			// verify start condition
			if (osWaitForEventBits(&events, LGC_EVENT_START | LGC_FAILURE_DETECTED, FALSE, TRUE, 50) == TRUE)
			{
				// verify which event
				if (data.start_stop_flag)
				{
					lgc_set_leds(LGC_RUNNING_LED, 1);
					// go to running
					lgc_set_state(LGC_RUNNING);
					// clear encoder flag
					osWaitForSemaphore(&encoder_flag, 0);
				}
				else if (data.guard_motor)
				{
					lgc_set_leds(LGC_RUNNING_LED, 0);
					// go to fail
					lgc_set_state(LGC_FAIL);
				}
				// set hmi update required
				osSetEventBits(&events, LGC_HMI_UPDATE_REQUIRED);
				// break
				break;
			}
			break;
		}
		case LGC_RUNNING:
		{
			/* Verify stop condition and transition to LGC_STOP */
			if (osWaitForEventBits(&events, LGC_EVENT_STOP | LGC_FAILURE_DETECTED, FALSE, TRUE, 0) == TRUE)
			{
				// verify which event
				if (data.start_stop_flag == 0)
				{
					lgc_set_leds(LGC_RUNNING_LED, 0);
					// go to stop
					lgc_set_state(LGC_STOP);

				}
				else if (data.guard_motor)
				{
					lgc_set_leds(LGC_RUNNING_LED, 0);
					// go to fail
					lgc_set_state(LGC_FAIL);
				}
				// set hmi update required
				osSetEventBits(&events, LGC_HMI_UPDATE_REQUIRED);
				// break
				break;
			}
			/* Verify encoder flag - proceed if pulse received */
			if (osWaitForSemaphore(&encoder_flag, 50) == TRUE)
			{
				//clear before data sensor
				memset(data.sensor, 0, sizeof(data.sensor));
				data.sensor_status = 0;

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
					measurement_event = lgc_process_measurement(&config);

					/* Handle measurement events
					 * 0: No event (still measuring or idle)
					 * 1: Leather measurement completed
					 * 2: Batch measurement completed
					 */
					if (measurement_event == 1)
					{
						/* TODO: Signal leather completion (e.g., update UI, log event) */

						// set hmi flag
						osSetEventBits(&events, LGC_HMI_UPDATE_REQUIRED);
					}
					else if (measurement_event == 2)
					{
						/* TODO: Signal batch completion (e.g., save to EEPROM, print results) */

						// set hmi flag and printer event
						osSetEventBits(&events, LGC_HMI_UPDATE_REQUIRED | LGC_EVENT_PRINT_BATCH);
					}
				}
			}

			break;
		}
		case LGC_FAIL:
		{
			// verify reset condition
			if (osWaitForEventBits(&events, LGC_FAILURE_CLEARED, FALSE, TRUE, 50) == TRUE)
			{
				// go to stop
				lgc_set_state(LGC_STOP);
				// set hmi update required
				osSetEventBits(&events, LGC_HMI_UPDATE_REQUIRED);
			}
			break;
		}
		}
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

static void lgc_set_leds(uint8_t led, uint8_t state)
{
	switch (led)
	{
	case LGC_RUNNING_LED:
		/* code */
		HAL_GPIO_WritePin(DO_1_GPIO_Port, DO_1_Pin, (GPIO_PinState)state);
		HAL_GPIO_WritePin(D0_2_GPIO_Port, D0_2_Pin, (GPIO_PinState)state == GPIO_PIN_SET ? GPIO_PIN_RESET : GPIO_PIN_SET);
		//output control
		HAL_GPIO_WritePin(DO_0_GPIO_Port, DO_0_Pin, (GPIO_PinState)state);
		break;
	case LGC_SPEED_LOW_LED:
		/* code */
		HAL_GPIO_WritePin(D0_6_GPIO_Port, D0_6_Pin, (GPIO_PinState)state);
		break;
	case LGC_SPEED_HIGH_LED:
		HAL_GPIO_WritePin(D0_7_GPIO_Port, D0_7_Pin, (GPIO_PinState)state);
		/* code */
		break;

	default:
		break;
	}
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
 * Implements the leather detection and measurement algorithm with state machine.
 * Returns status code indicating measurement event.
 *
 * @param config Pointer to configuration structure with batch limit
 * @return uint8_t Status code:
 *         - 0: No leather detected (idle state)
 *         - 1: Leather measurement completed (end of leather)
 *         - 2: Batch measurement completed (batch full)
 */
static uint8_t lgc_process_measurement(LGC_CONF_TypeDef_t *config)
{
	uint16_t active_bits;
	float slice_area;
	uint8_t event_status = 0; /* Default: no event */

	/* ============================================================================
	 * STEP 1: COUNT ACTIVE PHOTORECEPTORS AND CALCULATE AREA
	 * ============================================================================ */
	active_bits = lgc_count_active_bits();
	slice_area = lgc_calculate_slice_area(active_bits);

	/* ============================================================================
	 * STEP 2: LEATHER DETECTION STATE MACHINE
	 * ============================================================================ */

	if (active_bits > 0)
	{
		/* -------- STATE: LEATHER DETECTED -------- */
		/* Leather is currently passing through sensors */

		if (!measurements.is_measuring)
		{
			/* TRANSITION: Idle → Measuring
			 * Start of new leather piece detection
			 */
			measurements.is_measuring = 1;
			measurements.current_leather_area = 0.0f;
			measurements.no_detection_count = 0;
		}

		/* ACTION: Accumulate area while leather is detected */
		measurements.current_leather_area += slice_area;
		measurements.no_detection_count = 0; /* Reset hysteresis counter */

		event_status = 0; /* No event - still measuring */
	}
	else
	{
		/* -------- STATE: NO LEATHER DETECTED -------- */
		/* No photoreceptors active (leather has passed or not yet arrived) */

		if (measurements.is_measuring)
		{
			/* Currently measuring - apply hysteresis
			 * Increment counter to detect leather end
			 */
			measurements.no_detection_count++;

			if (measurements.no_detection_count >= LGC_LEATHER_END_HYSTERESIS)
			{
				/* ====== EVENT: END OF LEATHER DETECTED ====== */
				measurements.is_measuring = 0;
				measurements.no_detection_count = 0;
				event_status = 1; /* Leather measurement completed */

				/* ==================================================
				 * SECTION A: SAVE INDIVIDUAL LEATHER MEASUREMENT
				 * ================================================== */
				if (measurements.current_leather_index < LGC_LEATHER_COUNT_MAX)
				{
					measurements.leather_measurement[measurements.current_leather_index] =
						measurements.current_leather_area;
				}

				/* ==================================================
				 * SECTION B: ACCUMULATE AREA TO CURRENT BATCH
				 * ================================================== */
				if (measurements.current_batch_index < LGC_LEATHER_BATCH_COUNT_MAX)
				{
					measurements.batch_measurement[measurements.current_batch_index] +=
						measurements.current_leather_area;
				}

				/* ==================================================
				 * SECTION C: INCREMENT LEATHER INDEX
				 * ================================================== */
				measurements.current_leather_index++;

				/* ==================================================
				 * SECTION D: BATCH MANAGEMENT AND TRANSITIONS
				 * ================================================== */
				if (measurements.current_leather_index >= config->batch)
				{
					/* ====== EVENT: END OF BATCH DETECTED ====== */
					/* Batch is full - transition to next batch */
					measurements.current_leather_index = 0;
					measurements.current_batch_index++;
					event_status = 2; /* Batch measurement completed */

					/* Prevent batch array overflow */
					if (measurements.current_batch_index >= LGC_LEATHER_BATCH_COUNT_MAX)
					{
						/* Stay at maximum valid index to prevent array access overflow */
						measurements.current_batch_index = LGC_LEATHER_BATCH_COUNT_MAX - 1;
						/* TODO: Signal critical error - batch storage full */
					}
				}

				/* Clear accumulator for next leather piece */
				measurements.current_leather_area = 0.0f;
			}
		}

		/* Default state when no leather: clear accumulator */
		if (!measurements.is_measuring)
		{
			measurements.current_leather_area = 0.0f;
		}
	}

	return event_status;
}

void lgc_buttons_callback(uint8_t di, uint32_t evt)
{
	// Handle button events here
	switch (di)
	{
	case LGC_DI_START_STOP:
	{
		// Handle START/STOP button event
		if (evt == LWBTN_EVT_ONPRESS)
		{
			// lock
			osAcquireMutex(&mutex);
			data.start_stop_flag ^= 1 & 0x1; // toggle flag
			// unlock
			osReleaseMutex(&mutex);
			// set event
			osSetEventBits(&events, data.start_stop_flag ? LGC_EVENT_START : LGC_EVENT_STOP);
		}
		break;
	}
	case LGC_DI_GUARD:
	{

		// Handle GUARD button event
		if (evt == LWBTN_EVT_ONPRESS)
		{
			// lock
			osAcquireMutex(&mutex);
			data.guard_motor = 1;
			// unlock
			osReleaseMutex(&mutex);
			// set fail flag
			osSetEventBits(&events, LGC_FAILURE_DETECTED);

		}
		else if (evt == LWBTN_EVT_ONRELEASE)
		{
			// lock
			osAcquireMutex(&mutex);
			data.guard_motor = 0;
			// unlock
			osReleaseMutex(&mutex);
			//clear fail flag
			osSetEventBits(&events, LGC_FAILURE_CLEARED);
		}
		break;
	}
	case LGC_DI_SPEEDS:
	{
		// Handle SPEEDS button event
		if (evt == LWBTN_EVT_ONPRESS)
		{
			// lock
			osAcquireMutex(&mutex);
			data.speed_motor = 1;
			// unlock
			osReleaseMutex(&mutex);
			//set led
			lgc_set_leds(LGC_SPEED_HIGH_LED, 1);
			lgc_set_leds(LGC_SPEED_LOW_LED, 0);
		}
		else if (evt == LWBTN_EVT_ONRELEASE)
		{
			// lock
			osAcquireMutex(&mutex);
			data.speed_motor = 0;
			// unlock
			osReleaseMutex(&mutex);
			//set led
			lgc_set_leds(LGC_SPEED_HIGH_LED, 0);
			lgc_set_leds(LGC_SPEED_LOW_LED, 1);
		}
		break;
	}
	case LGC_DI_FEEDBACK:
	{
		// Handle FEEDBACK button event
		if (evt == LWBTN_EVT_ONPRESS)
		{
			// lock
			osAcquireMutex(&mutex);
			data.feedback_motor = 1;
			// unlock
			osReleaseMutex(&mutex);
		}
		else if (evt == LWBTN_EVT_ONRELEASE)
		{
			// lock
			osAcquireMutex(&mutex);
			data.feedback_motor = 0;
			// unlock
			osReleaseMutex(&mutex);
		}
		break;
	}
	default:
		break;
	}
}


void lgc_set_stop_condition(uint8_t stop)
{
	// lock
	osAcquireMutex(&mutex);
	data.start_stop_flag = stop & 0x1;
	// unlock
	osReleaseMutex(&mutex);

	// set event
	osSetEventBits(&events, data.start_stop_flag ? LGC_EVENT_START : LGC_EVENT_STOP);
}

void lgc_get_measurements(lgc_measurements_t *out_measurements)
{
	// lock
	osAcquireMutex(&mutex);
	// copy measurements
	memcpy(out_measurements, &measurements, sizeof(lgc_measurements_t));
	// unlock
	osReleaseMutex(&mutex);
}

void lgc_get_state_data(lgc_t *out_data)
{
	// lock
	osAcquireMutex(&mutex);
	// copy data
	memcpy(out_data, &data, sizeof(lgc_t));
	// unlock
	osReleaseMutex(&mutex);
}
