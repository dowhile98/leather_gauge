/**
 *
 */

//-------------------------------------------------------------------------------
// includes
//-------------------------------------------------------------------------------
#include "lgc.h"
#include "lgc_interface_printer.h"
#include "lgc_interface_modbus.h"
#include "os_port.h"
#include "lgc_module_input.h"
#include "lgc_module_eeprom.h"
#include "lwbtn.h"
#include "lgc_module_encoder.h"
#include "lgc_module_rtc.h"
#include "lgc_report_manager.h"
//-------------------------------------------------------------------------------
// defines
//-------------------------------------------------------------------------------

/* Trigger Pin Definition - Placeholder (User should confirm) */
#ifndef MASTER_TRIGGER_GPIO_Port
#define MASTER_TRIGGER_GPIO_Port GPIOA
#define MASTER_TRIGGER_Pin GPIO_PIN_2
#endif

/* Pixel width in mm (single sensor pixel) */
#ifndef LGC_PIXEL_WIDTH_MM
#define LGC_PIXEL_WIDTH_MM 20.0f
#endif

/* Encoder step distance in mm */
#ifndef LGC_ENCODER_STEP_MM
#define LGC_ENCODER_STEP_MM 20.398f
#endif

/* Number of photoreceptors per sensor */
#define LGC_PHOTORECEPTORS_PER_SENSOR 10

/* Hysteresis for leather detection (consecutive steps with no detection) */
#ifndef LGC_LEATHER_END_HYSTERESIS
#define LGC_LEATHER_END_HYSTERESIS 3
#endif

#ifndef LGC_LEATHER_MAX_PULSE_FLAG
#define LGC_LEATHER_MAX_PULSE_FLAG 1
#endif
//-------------------------------------------------------------------------------
// global variables
//-------------------------------------------------------------------------------
lgc_t data;
static lgc_measurements_t measurements;
static LgcBatchReport_t finalized_batch;
static bool batch_close_pending = false;
static OsSemaphore encoder_flag;
static OsMutex mutex;
static uint32_t pulse_count = 0;
static uint32_t last_sensor_read_ms = 0;
static uint32_t sensor_read_time_diff_ms = 0;

//-------------------------------------------------------------------------------
// extern definition
//-------------------------------------------------------------------------------
extern uint8_t lgc_hmi_is_sensor_test_active(void);
extern void lgc_hmi_set_sensor_value(uint16_t *value);
//-------------------------------------------------------------------------------
// private function prototype
//-------------------------------------------------------------------------------
static void lgc_encoder_callback(void);

static uint8_t lgc_get_state(void);

static uint8_t lgc_set_state(uint8_t state);

static void lgc_set_leds(uint8_t led, uint8_t state);

static void lgc_trigger_chain(void);

static error_t lgc_parse_burst_data(uint8_t *buffer, uint16_t len);

static void lgc_update_live_status(void);
/**
 * @brief Finalize current batch, create a snapshot for observers, and reset live data.
 */
static void lgc_finalize_batch_snapshot(void)
{
	LGC_CONF_TypeDef_t config;
	RTC_DateTime_t dt;

	/* acquire measurements mutex to ensure atomic snapshot */
	osAcquireMutex(&measurements.mutex);
	uint16_t finished_index = measurements.current_batch_index;

	lgc_module_conf_get(&config);
	lgc_module_rtc_get(&dt);

	/* Populate snapshot */
	// original
	finalized_batch.batch_id = config.batch;
	// modificatrion
	finalized_batch.year = dt.year;
	finalized_batch.month = dt.month;
	finalized_batch.day = dt.day;
	finalized_batch.hours = dt.hours;
	finalized_batch.minutes = dt.minutes;
	finalized_batch.seconds = dt.seconds;

	strncpy(finalized_batch.client_name, config.client_name, sizeof(finalized_batch.client_name));
	strncpy(finalized_batch.color, config.color, sizeof(finalized_batch.color));
	strncpy(finalized_batch.leather_id, config.leather_id, sizeof(finalized_batch.leather_id));

	memcpy(finalized_batch.pieces_area, measurements.leather_measurement, sizeof(measurements.leather_measurement));

	finalized_batch.total_pieces = measurements.current_leather_index;
	finalized_batch.total_area = measurements.batch_measurement[finished_index];
	// finalized_batch.total_area = measurements.batch_measurement[measurements.current_batch_index-1];
	finalized_batch.units = config.units;
	finalized_batch.conversion = config.conversion;
	finalized_batch.is_valid = 1;
	finalized_batch.batch_index = measurements.current_batch_index;
	/* Reset Live Data for next batch */
	measurements.current_leather_index = 0;
	measurements.current_leather_area = 0.0f;
	memset(measurements.leather_measurement, 0, sizeof(measurements.leather_measurement));
	/* Also reset current batch area accumulator */
	// measurements.batch_measurement[measurements.current_batch_index] = 0.0f; // comentado
	measurements.current_batch_index += 1;
	/* Increment batch number in config and save to persistent storage */
	// config.batch++; // comentado
	// lgc_module_conf_set(&config); // comentado

	osReleaseMutex(&measurements.mutex);

	/* Build metadata for the new snapshot and promote current → last */
	{
		LgcBatchSnapshot_t meta;
		memset(&meta, 0, sizeof(LgcBatchSnapshot_t));
		meta.batch_id = finalized_batch.batch_id;
		meta.batch_index = finalized_batch.batch_index;
		meta.year = finalized_batch.year;
		meta.month = finalized_batch.month;
		meta.day = finalized_batch.day;
		meta.hours = finalized_batch.hours;
		meta.minutes = finalized_batch.minutes;
		meta.seconds = finalized_batch.seconds;
		meta.units = finalized_batch.units;
		meta.conversion = finalized_batch.conversion;
		strncpy(meta.client_name, finalized_batch.client_name, sizeof(meta.client_name));
		strncpy(meta.color, finalized_batch.color, sizeof(meta.color));
		strncpy(meta.leather_id, finalized_batch.leather_id, sizeof(meta.leather_id));
		lgc_report_finalize_batch(&meta);
	}

	/* Legacy snapshot update (backward compat for any remaining callers) */
	lgc_report_update_snapshot(&finalized_batch);
	osSetEventBits(&events, LGC_EVENT_SNAPSHOT_READY);
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
static uint8_t lgc_process_measurement(LGC_CONF_TypeDef_t *config);

static uint16_t lgc_count_active_bits(void);

static float lgc_calculate_slice_area(uint16_t active_bits);

//-------------------------------------------------------------------------------
// task definition
//-------------------------------------------------------------------------------
void lgc_main_task_entry(void *param)
{
	uint8_t raw_burst[64];
	uint16_t burst_len_rx = 0;
	LGC_CONF_TypeDef_t config;
	uint8_t measurement_event; /* Event status from measurement processing */
	RTC_Config_t rtc_config = {
		.initial_datetime = {
			.year = 2026,
			.month = 1,
			.day = 19,
			.weekday = 7, // Domingo
			.hours = 14,
			.minutes = 30,
			.seconds = 0},
		.use_initial_datetime = false};
	/*create semaphore*/
	osCreateSemaphore(&encoder_flag, 0);
	/*Mutex*/
	osCreateMutex(&mutex);

	osCreateMutex(&measurements.mutex);
	/*encoder init*/
	lgc_module_encoder_init(lgc_encoder_callback);

	/*init rtc*/
	if (lgc_module_rtc_init(&rtc_config) != NO_ERROR)
	{
		// handle error
	}

	lgc_module_conf_load();

	// verify speed select for init
	osDelayTask(300);
	if (HAL_GPIO_ReadPin(DI_4_GPIO_Port, DI_4_Pin) == 0)
	{
		// lock
		osAcquireMutex(&mutex);
		data.speed_motor = 0;
		// unlock
		osReleaseMutex(&mutex);
		// set led
		lgc_set_leds(LGC_SPEED_HIGH_LED, 0);
		lgc_set_leds(LGC_SPEED_LOW_LED, 1);
	}
	// test only
	//--------------------------------
#if 0
	lgc_interface_modbus_set_mode(LGC_BUS_MODE_DAISY_CHAIN);
	while(1)
	{
		last_sensor_read_ms = osGetSystemTime();
		/* SENSOR ACQUISITION STRATEGY: Daisy Chain Burst */
		// Reset burst accumulation before trigger
		lgc_modbus_reset_burst();
		// Trigger the sensors
		lgc_trigger_chain();


		// Wait for data (Burst Ready)
		if (osWaitForEventBits(&events, LGC_EVENT_BURST_READY, TRUE, TRUE, 80) == TRUE)
		{f

			lgc_modbus_get_burst_data(raw_burst, &burst_len_rx);
			lgc_parse_burst_data(raw_burst, burst_len_rx);
		}
		else
		{
			/* Burst failed - Mark sensors as failed to skip processing this slice */
			data.sensor_status = 0x7FF; // All 11 sensors failed
		}

		sensor_read_time_diff_ms = osGetSystemTime() - last_sensor_read_ms;

		osDelayTask(5);
	}
#endif

	lgc_interface_modbus_set_mode(LGC_BUS_MODE_MODBUS);
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
				// set running
				osAcquireMutex(&mutex);
				data.start_stop_flag = 1;
				osReleaseMutex(&mutex);
				// verify which event
				if (data.start_stop_flag)
				{
					lgc_set_leds(LGC_RUNNING_LED, 1);
					// go to running
					lgc_set_state(LGC_RUNNING);
					// Set Daisy Chain mode for production
					lgc_interface_modbus_set_mode(LGC_BUS_MODE_DAISY_CHAIN);
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
			if (osWaitForEventBits(&events, LGC_EVENT_STOP | LGC_FAILURE_DETECTED /* | LGC_EVENT_ENTER_CONFIG*/, FALSE, TRUE, 0) == TRUE)
			{
				// set cero
				osAcquireMutex(&mutex);
				data.start_stop_flag = 0;
				osReleaseMutex(&mutex);
				// verify which event
				if (data.start_stop_flag == 0)
				{
					lgc_set_leds(LGC_RUNNING_LED, 0);
					// go to stop
					lgc_set_state(LGC_STOP);
					// Ensure we return to Modbus mode if needed
					lgc_interface_modbus_set_mode(LGC_BUS_MODE_MODBUS);
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

			/* Handle manual batch closure request from HMI */
			if (osWaitForEventBits(&events, LGC_EVENT_CLOSE_BATCH_REQ, TRUE, TRUE, 0) == TRUE)
			{
				batch_close_pending = true;
			}

			/* Check if a batch closure is pending and we are NOT measuring a leather piece */
			if (batch_close_pending && measurements.is_measuring == 0)
			{
				lgc_finalize_batch_snapshot();
				batch_close_pending = false;
				/* Note: Snapshot logic already increments the batch and signals observers */
			}

			if (osWaitForSemaphore(&encoder_flag, 50) == TRUE)
			{
				last_sensor_read_ms = osGetSystemTime();

				/* SENSOR ACQUISITION STRATEGY: Daisy Chain Burst */
				// Reset burst accumulation before trigger
				lgc_modbus_reset_burst();
				// Trigger the sensors

				lgc_trigger_chain();

				// Wait for data (Burst Ready)
				if (osWaitForEventBits(&events, LGC_EVENT_BURST_READY, TRUE, TRUE, 50) == TRUE)
				{

					lgc_modbus_get_burst_data(raw_burst, &burst_len_rx);
					lgc_parse_burst_data(raw_burst, burst_len_rx);

					/*verity if sensor test active for update*/
					if (lgc_hmi_is_sensor_test_active())
					{
						lgc_hmi_set_sensor_value(data.sensor);
					}
				}
				else
				{
					/* Burst failed - Mark sensors as failed to skip processing this slice */
					data.sensor_status = 0x7FF; // All 11 sensors failed
				}

				sensor_read_time_diff_ms = osGetSystemTime() - last_sensor_read_ms;
				/* Process measurement only if all sensors are healthy */
				if (data.sensor_status == NO_ERROR)
				{
					// acquire measurements mutex
					osAcquireMutex(&measurements.mutex);
					/* Process measurement and get event status */
					measurement_event = lgc_process_measurement(&config);
					// release measurements mutex
					osReleaseMutex(&measurements.mutex);

					if (measurements.is_measuring == 1)
					{
						lgc_update_live_status();
					}

					/* Handle measurement events
					 * 0: No event (still measuring or idle)
					 * 1: Leather measurement completed
					 * 2: Batch measurement completed
					 */
					if (measurement_event == 1)
					{
						/* TODO: Signal leather completion (e.g., update UI, log event) */
						lgc_update_live_status();
						// set hmi flag
						osSetEventBits(&events, LGC_HMI_UPDATE_REQUIRED);
					}
					else if (measurement_event == 2)
					{
						lgc_update_live_status();
						/* Automatic batch full: trigger safe closure */
						lgc_finalize_batch_snapshot();

						// set hmi flag
						osSetEventBits(&events, LGC_HMI_UPDATE_REQUIRED);
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

void lgc_clear_measurement_last_leather(void)
{
	/* DEPRECATED: calls the new index-based delete on the last piece.
	 * Kept for backward compatibility. Prefer lgc_delete_leather_by_visual_index().
	 * Uses lgc_report_get_current_active_count() to avoid allocating a 1.25 KB
	 * LgcBatchSnapshot_t on the stack. */
	uint16_t active = lgc_report_get_current_active_count();
	if (active > 0)
	{
		lgc_delete_leather_by_visual_index(active); /* last visual index */
	}
}

/**
 * @brief Delete a leather piece from the current batch by visual (display) index.
 *
 * Soft-deletes the slot in the Report Manager snapshot and keeps the internal
 * measurements accumulators consistent so the batch closure condition remains
 * correct: measurements.current_leather_index == s_current_batch.active_count.
 *
 * @param visual_index  1-based index as shown on the DWIN display.
 */
void lgc_delete_leather_by_visual_index(uint16_t visual_index)
{
	float deleted_area = 0.0f;

	/* Step 1-4: soft-delete in snapshot and get the area */
	if (lgc_report_delete_current_slot(visual_index, &deleted_area) != NO_ERROR)
	{
		return; /* index not found, nothing to do */
	}

	/* Step 5: keep internal measurements consistent */
	osAcquireMutex(&measurements.mutex);

	measurements.batch_measurement[measurements.current_batch_index] -= deleted_area;
	if (measurements.batch_measurement[measurements.current_batch_index] < 0.0f)
	{
		measurements.batch_measurement[measurements.current_batch_index] = 0.0f;
	}

	if (measurements.current_leather_index > 0)
	{
		measurements.current_leather_index--;
	}

	measurements.current_leather_area = 0.0f;
	measurements.total_leathers_measured = measurements.current_leather_index;

	osReleaseMutex(&measurements.mutex);

	/* Step 6: notify HMI */
	lgc_update_live_status();
}

void lgc_increment_batch_index(void)
{
	// acquire measurements mutex
	osAcquireMutex(&measurements.mutex);
	/* Increment batch index and reset leather index */
	if (measurements.current_batch_index < LGC_LEATHER_BATCH_COUNT_MAX - 1)
	{
		measurements.current_batch_index++;
		measurements.current_leather_index = 0;
		measurements.current_leather_area = 0.0f;
	}
	// total leathers measured
	//	measurements.total_leathers_measured = measurements.current_leather_index;
	// release measurements mutex
	osReleaseMutex(&measurements.mutex);
}
//-------------------------------------------------------------------------------
// callbacks
//-------------------------------------------------------------------------------
static void lgc_encoder_callback(void)
{

	pulse_count += 1;
	if (pulse_count > LGC_LEATHER_MAX_PULSE_FLAG)
	{
		pulse_count = 0;
		// set flag
		osReleaseSemaphore(&encoder_flag);
	}
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
		// output control
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
	float area = width * LGC_ENCODER_STEP_MM * LGC_LEATHER_MAX_PULSE_FLAG;

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
#include <math.h>

static uint8_t lgc_process_measurement(LGC_CONF_TypeDef_t *config)
{
	uint16_t active_bits;
	float slice_area;
	uint8_t event_status = 0;	  /* Default: no event */
	float area_conversion = 0.0f; // ft2 factor
	/* ============================================================================
	 * STEP 1: COUNT ACTIVE PHOTORECEPTORS AND CALCULATE AREA
	 * ============================================================================ */
	active_bits = lgc_count_active_bits();
	slice_area = lgc_calculate_slice_area(active_bits); // mm2
	slice_area = slice_area / 1000000.0f;				// Convert mm² to m²
	/*The calibration was made using 30x30cm2 reference, so its necessary to
	 * always start with the corresponding conversion factor from f2 to 30x30cm2 scale*/
	slice_area = slice_area * 11.11111f; // convert to f2 scale 30x30

#if 0
	/* Unit conversion if required */
	if (config->units == 0) // ft2
	{
		switch (config->conversion)
		{
		case 0:
			/* code */
			area_conversion = 12.755102; // m2 to ft2 (28x28)
			break;
		case 1:
			area_conversion = 11.111111f; // m2 to ft2 (30x30)
			break;
		case 2:
			area_conversion = 10.7639f; // m2 to ft2 (30.48x30.48)
			break;
		default:
			area_conversion = 10.7639f; // m2 to ft2
			break;
		}
		slice_area = slice_area * area_conversion;


	}
#endif
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

				/*DELETE NOISE IN FT2 MEASURES */
#if 1
				if (measurements.current_leather_area < 0.10f && config->units == 0)
				{
					measurements.current_leather_area = 0.0f;
					return 0;
				}

				/*APPLY FACTOR CONVERSION TO DATA*/

				float x = measurements.current_leather_area;
				if (data.speed_motor)
				{ // high
					measurements.current_leather_area =
						((0.004803324f * x - 0.082321956f) * x + 1.69341995f) * x - 0.2115029f;
				}
				else
				{ // low
					measurements.current_leather_area =
						((0.00168635f * x - 0.0397947f) * x + 1.22991f) * x - 0.239696f;
				}
				/*DELETE NOISE OF THE POLINOMIAL CONVERSION*/
				if (measurements.current_leather_area < 0.50f && config->units == 0)
				{
					measurements.current_leather_area = 0.0f;
					return 0;
				}
				/* Unit conversion if required */
				if (config->units == 0) // ft2
				{
					switch (config->conversion)
					{
					case 0:
						/* code */
						area_conversion = 1.14795f; // m2 to ft2 (28x28)
						break;
					case 1:
						area_conversion = 1.0f; // m2 to ft2 (30x30)
						break;
					case 2:
						area_conversion = 0.968750f; // m2 to ft2 (30.48x30.48)
						break;
					default:
						area_conversion = 0.968750f; // m2 to ft2
						break;
					}
					measurements.current_leather_area =
						measurements.current_leather_area * area_conversion;

					/*ROUND LEATHER AREA TO MATCH MATCH REAL MEASURES*/
					float base = floorf(measurements.current_leather_area / 0.25f) * 0.25f;
					float diff = measurements.current_leather_area - base;
					if (diff > 0.15f)
					{
						base += 0.25f;
					}
					measurements.current_leather_area = base;
				}
				else // m2
				{
					measurements.current_leather_area =
						measurements.current_leather_area / 11.11111f;
				}
#endif

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
				 * SECTION C2: SYNC WITH BATCH SNAPSHOT (NEW)
				 * Append a new slot to the Report Manager's current
				 * batch so the HMI and printer can access per-piece data.
				 * ================================================== */
				{
					LgcBatchSlot_t new_slot;
					new_slot.area = measurements.current_leather_area;
					new_slot.deleted = false;
					lgc_report_append_current_slot(&new_slot);
				}

				/* ==================================================
				 * SECTION D: BATCH MANAGEMENT AND TRANSITIONS
				 * ================================================== */
				if (measurements.current_leather_index >= config->batch)
				{
					/* ====== EVENT: END OF BATCH DETECTED ====== */
					measurements.total_leathers_measured = measurements.current_leather_index;

					/* Note: Batch reset and snapshotting is now handled by lgc_finalize_batch_snapshot()
					 * called from the main task when event_status == 2 is received. */
					event_status = 2; /* Batch measurement completed */
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
			if (data.guard_motor == 0) // only if not in fail
			{
				// lock
				osAcquireMutex(&mutex);
				data.start_stop_flag ^= 1 & 0x1; // toggle flag
				// unlock
				osReleaseMutex(&mutex);
				// set event
				osSetEventBits(&events, data.start_stop_flag ? LGC_EVENT_START : LGC_EVENT_STOP);
			}
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
			// clear fail flag
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
			// set led
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
			// set led
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
	// lock using measurements mutex
	osAcquireMutex(&measurements.mutex);
	// copy measurements
	memcpy(out_measurements, &measurements, sizeof(lgc_measurements_t));
	// unlock
	osReleaseMutex(&measurements.mutex);
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

/**
 * @brief Update the live status snapshot for the HMI
 */
static void lgc_update_live_status(void)
{
	LgcLiveStatus_t status;

	osAcquireMutex(&measurements.mutex);
	status.batch_count = measurements.current_batch_index;
	status.leather_count = measurements.current_leather_index;
	/* Save the previous leather area measurement so its not show 0 every time
	 * the leather pass trough the sensors*/
#if 1
	if (measurements.current_leather_index > 0)
	{
		status.current_leather_area = measurements.leather_measurement[measurements.current_leather_index - 1];
	}
	else
	{
		status.current_leather_area = 0;
	}

#endif
	/*Undo the comment of the next line if the previous doesn't work*/
	// status.current_leather_area = measurements.current_leather_area;
	status.accumulated_batch_area = measurements.batch_measurement[measurements.current_batch_index];
	status.is_measuring = measurements.is_measuring;
	osReleaseMutex(&measurements.mutex);

	osAcquireMutex(&mutex);
	status.system_state = data.state;
	status.motor_feedback = data.feedback_motor;
	status.motor_speed = data.speed_motor;
	status.guard_status = data.guard_motor;
	osReleaseMutex(&mutex);

	lgc_report_update_live_status(&status);
}

/**
 * @brief Parse the raw 33-byte burst data from Daisy Chain mode
 *
 * Format per sensor (3 bytes): [ID] [DATA_H] [DATA_L]
 * Total length: 11 sensors * 3 bytes = 33 bytes
 */
static error_t lgc_parse_burst_data(uint8_t *buffer, uint16_t len)
{
	if (len < 33 || buffer == NULL)
		return ERROR_INVALID_PARAMETER;

	osAcquireMutex(&mutex);

	/* Reset sensor failure status for this slice */
	data.sensor_status = NO_ERROR;

	for (uint8_t i = 0; i < LGC_SENSOR_NUMBER; i++)
	{
		uint16_t offset = i * 3;
		uint8_t sensor_id = buffer[offset];

		/* Validate sensor ID consistency (expected 1 to 11) */
		if (sensor_id == (i + 1))
		{
			/* Reconstruct 16-bit sensor data */
			data.sensor[i] = (uint16_t)((buffer[offset + 1] << 8) | buffer[offset + 2]);
		}
		else
		{
			/* Sync error or sensor missing - Mark as failed */
			data.sensor[i] = 0;
			data.sensor_status |= (1 << i);
		}
	}

	osReleaseMutex(&mutex);
	return NO_ERROR;
}

/**
 * @brief Generate a physical trigger pulse for the Daisy Chain sensor acquisition
 */
static void lgc_trigger_chain(void)
{
	HAL_GPIO_WritePin(MASTER_TRIGGER_GPIO_Port, MASTER_TRIGGER_Pin, GPIO_PIN_SET);

	/* Small delay for the pulse width (~500us) */
	/* Using a simple NOP loop */
	for (volatile uint32_t i = 0; i < 6000; i++)
		;

	HAL_GPIO_WritePin(MASTER_TRIGGER_GPIO_Port, MASTER_TRIGGER_Pin, GPIO_PIN_RESET);
	// for(volatile uint32_t i = 0; i < 1000; i++);
	//__HAL_UART_FLUSH_DRREGISTER(&huart3);
}
