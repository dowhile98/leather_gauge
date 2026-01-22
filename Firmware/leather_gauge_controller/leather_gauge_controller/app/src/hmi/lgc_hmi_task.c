/**
 *
 */

//-------------------------------------------------------------------------------
// includes
//-------------------------------------------------------------------------------
#include "lgc.h"
#include "lgc_interface_printer.h"
#include "error.h"
#include "os_port.h"
#include "dwin_core.h"
#include "usart.h"
#include "lgc_hmi.h"
#include "lgc_module_rtc.h"

//-------------------------------------------------------------------------------
// defines
//-------------------------------------------------------------------------------
#ifndef DWIN_BUFFER_SIZE
#define DWIN_BUFFER_SIZE 1024
#endif

#ifndef DWIN_PROCESS_TASK_PRI
#define DWIN_PROCESS_TASK_PRI 11
#endif

#ifndef DWIN_PROCESS_TASK_STACK
#define DWIN_PROCESS_TASK_STACK 256
#endif

#ifndef LGC_HMI_TASK_PRI
#define LGC_HMI_TASK_PRI 10
#endif

#ifndef LGC_HMI_TASK_STACK
#define LGC_HMI_TASK_STACK 256
#endif

#ifndef LGC_HMI_UPDATE_TASK_PRI
#define LGC_HMI_UPDATE_TASK_PRI 10
#endif

#ifndef LGC_HMI_UPDATE_TASK_STACK
#define LGC_HMI_UPDATE_TASK_STACK 256
#endif
//-------------------------------------------------------------------------------
// typedefs
//-------------------------------------------------------------------------------
typedef struct
{
	// sensor test flag
	bool sensor_test_active;
	// sensor test id
	uint8_t sensor_test_id;
	// sensor test value
	uint16_t sensor_test_value;
	// hh
	uint16_t hh;
	// mm
	uint16_t mm;
	// ss
	uint16_t ss;
	// day
	uint16_t day;
	// month
	uint16_t month;
	// year
	uint16_t year;
	// mutex
	OsMutex mutex;
	// page
	uint8_t current_page;
} lgc_hmi_data_t;

//-------------------------------------------------------------------------------
// global variables
//-------------------------------------------------------------------------------
static OsQueue hmi_msg;

static uint8_t dwin_fifo_mem[DWIN_BUFFER_SIZE];
static dwin_t dwin_hmi;
static OsMutex dwin_mutex;
static OsSemaphore dwin_response;
static OsSemaphore dwin_new_data_flag;
static OsSemaphore tx_cplt_flag;
static uint8_t uart_rx[128];
static dwin_interface_t dwin_hal = {0};
static OsTaskId dwin_process_task;
static OsTaskId lgc_hmi_task = {0};
static OsTaskId lgc_hmi_update_task = {0};
static lgc_hmi_data_t hmi_data;
//-------------------------------------------------------------------------------
// private function prototype
//-------------------------------------------------------------------------------
static uint32_t lgc_dwin_uart_transmit(uint8_t *data, uint16_t len);
static uint32_t lgc_dwin_get_tick(void);
static void lgc_dwin_lock(void);
static void lgc_dwin_unlock(void);
static bool lgc_dwin_signal(uint32_t timeout);
static void lgc_dwin_signal_set(void);
static void lgc_dwin_new_data_signal(void);
static void lgc_dwin_new_data_signal_set(void);
static void lgc_dwin_uart_ErrorCallback(UART_HandleTypeDef *huart);
static void lgc_dwin_uart_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Pos);
void lgc_dwin_uart_TxCpltCallback(UART_HandleTypeDef *huart);
static void on_dwin_event(dwin_evt_t *evt, void *ctx);
error_t lgc_hmi_send_msg(dwin_evt_t *evt);
static void hmi_set_current_page(uint8_t page);
//-------------------------------------------------------------------------------
// task definition
//-------------------------------------------------------------------------------
void lgc_hmi_update_task_entry(void *param)
{
	/*local variables*/
	lgc_measurements_t *measurements;
	lgc_t state_data;
	RTC_DateTime_t datetime;
	LGC_CONF_TypeDef_t conf = {0};
	uint16_t value = 0;
	uint16_t vp_addr = 0;
	/*alloc memory for measurements*/
	measurements = osAllocMem(sizeof(lgc_measurements_t));
	if (measurements == NULL)
	{
		for (;;)
		{
			osDelayTask(1000);
		}
	}
	osDelayTask(500); // wait for system to stabilize

	// set initial page
	hmi_set_current_page(HMI_PAGE1);

	for (;;)
	{
		// wait for update event
		osWaitForEventBits(&events, LGC_HMI_UPDATE_REQUIRED | LGC_HMI_SENSOR_TEST_UPDATE, FALSE, TRUE, 1000);
		// state machine
		switch (hmi_data.current_page)
		{
		case HMI_PAGE1:
		{
			// get measurements
			lgc_get_measurements(measurements);
			// get state data
			lgc_get_state_data(&state_data);
			/*get rtc data*/
			lgc_module_rtc_get(&datetime);
			/*get current configuration*/
			lgc_module_conf_get(&conf);
			// update HMI variables
			//->guard
			dwin_write_vp_u16(&dwin_hmi, LGC_HMI_VP_STATE, state_data.guard_motor);
			//->speed
			dwin_write_vp_u16(&dwin_hmi, LGC_HMI_VP_ICON_SPEEP, state_data.speed_motor);
			// set date (YYYY / MM / DD)
			dwin_write_vp_u16(&dwin_hmi, LGC_HMI_VP_CONFIG_YEAR, datetime.year);
			dwin_write_vp_u16(&dwin_hmi, LGC_HMI_VP_CONFIG_MONTH, datetime.month);
			dwin_write_vp_u16(&dwin_hmi, LGC_HMI_VP_CONFIG_DAY, datetime.day);
			// dwin_write_vp_u16(&dwin_hmi, LGC_HMI_VP_CONFIG_DAY, 15);
			//  batch count
			dwin_write_vp_u16(&dwin_hmi, LGC_HMI_VP_BATCH_COUNT, measurements->current_batch_index);
			// leather count
			dwin_write_vp_u16(&dwin_hmi, LGC_HMI_VP_LEATHER_COUNT, measurements->current_leather_index);
			//->current leather area
			dwin_write_vp_u16(&dwin_hmi, LGC_HMI_VP_CURRENT_LEATHER_AREA, (uint16_t)(measurements->current_leather_area * 100)); // assuming area in cm², sending as integer
			// dwin_write_vp_u16(&dwin_hmi, LGC_HMI_VP_CURRENT_LEATHER_AREA, (uint16_t)(95));
			//->motor feedback
			dwin_write_vp_u16(&dwin_hmi, LGC_HMI_VP_FEEDBACK_MOTOR, state_data.feedback_motor);
			// ->total area count (accumulated area of current batch in progress)
			//  Note: current_batch_index is the batch currently being measured (0-based)
			//  Display the current batch's accumulated area, not the previous one
			dwin_write_vp_u16(&dwin_hmi, LGC_HMI_VP_ACUMULATED_LEATHER_AREA, (uint16_t)(measurements->batch_measurement[measurements->current_batch_index] * 100)); // assuming area in cm², sending as integer
			/*Current configuration*/
			//->client name
			dwin_write_text(&dwin_hmi, LGC_HMI_VP_CONFIG_TEXT_NAME_CLIENT, conf.client_name);
			// leather color
			dwin_write_text(&dwin_hmi, LGC_HMI_VP_CONFIG_TEXT_NAME_COLOR, conf.color);
			// leather id
			dwin_write_text(&dwin_hmi, LGC_HMI_VP_CONFIG_TEXT_NAME_LEATHER, conf.leather_id);
			// batch number
			dwin_write_vp_u16(&dwin_hmi, LGC_HMI_VP_CONFIG_NUMBER_NAME_LEATHER, conf.batch);
			// units
			dwin_write_vp_u16(&dwin_hmi, LGC_HMI_VP_CONFIG_UNITS, conf.units);
			break;
		}
		case HMI_PAGE3:
		case HMI_PAGE4:
		{
			// sensor test update
			if (lgc_modbus_read_holding_regs(hmi_data.sensor_test_id % 12, 45, &value, 1) == NO_ERROR)
			{
				osAcquireMutex(&hmi_data.mutex);
				hmi_data.sensor_test_value = value;
				osReleaseMutex(&hmi_data.mutex);
				// write to HMI
				dwin_write_vp_u16(&dwin_hmi, LGC_HMI_VP_TEST_BIT_SENSOR, hmi_data.sensor_test_value);
			}
			// wait some time to avoid flooding
			if (osWaitForEventBits(&events, LGC_HMI_UPDATE_REQUIRED, FALSE, FALSE, 400) != TRUE)
			{
				// set another sensor test update
				osSetEventBits(&events, LGC_HMI_SENSOR_TEST_UPDATE);
			}
			break;
		}
		case HMI_PAGE12:
		case HMI_PAGE13:
		case HMI_PAGE14:
		case HMI_PAGE15:
		case HMI_PAGE16:
		case HMI_PAGE17:
		{
			vp_addr = 0x1601 + (hmi_data.current_page - HMI_PAGE12) * 50;
			// batch report pages
			// get measurements
			lgc_get_measurements(measurements);
			// send data
			for (uint8_t i = 0; i < 50; i++)
			{
				dwin_write_vp_u16(&dwin_hmi, vp_addr + i, (uint16_t)(measurements->leather_measurement[i + (hmi_data.current_page - HMI_PAGE12) * 50] * 100)); // assuming area in cm², sending as integer
			}

			break;
		}

		default:
			break;
		}
	}
}
void lgc_hmi_task_entry(void *param)
{
	/*local variables*/
	dwin_evt_t msg = {0};
	uint16_t value = 0;
	LGC_CONF_TypeDef_t conf = {0};
	char text[32] = {0};
	RTC_DateTime_t datetime;
	/*main loop*/
	for (;;)
	{
		if (osReceiveFromQueue(&hmi_msg, &msg, INFINITE_DELAY) != TRUE)
		{
			continue;
		}

		switch (msg.addr)
		{
		case LGC_HMI_TOUCH_PAGE_ADDR:
		{
			/*get value*/
			value = (msg.data[0] << 8) | msg.data[1];
			/*set current page*/
			hmi_set_current_page((uint8_t)value);
			// verify page
			if (value == HMI_PAGE20 || value == HMI_PAGE6)
			{
				// get conf
				lgc_module_conf_get(&conf);
				// set stop event
				osSetEventBits(&events, LGC_EVENT_STOP);

				// update conf on hmi (write values)
				if (value == HMI_PAGE6)
				{
					// update hmi
					//->client name
					dwin_write_text(&dwin_hmi, LGC_HMI_VP_CONFIG_TEXT_NAME_CLIENT, conf.client_name);
					// leather color
					dwin_write_text(&dwin_hmi, LGC_HMI_VP_CONFIG_TEXT_NAME_COLOR, conf.color);
					// leather id
					dwin_write_text(&dwin_hmi, LGC_HMI_VP_CONFIG_TEXT_NAME_LEATHER, conf.leather_id);
					// batch number
					dwin_write_vp_u16(&dwin_hmi, LGC_HMI_VP_CONFIG_NUMBER_NAME_LEATHER, conf.batch);
					// units
					dwin_write_vp_u16(&dwin_hmi, LGC_HMI_VP_CONFIG_UNITS, conf.units);
				}
				else
				{
					// update ft2 conversion
					switch (conf.conversion)
					{
					case 0:
						dwin_write_vp_u16(&dwin_hmi, 0x121A, 1);
						dwin_write_vp_u16(&dwin_hmi, 0x122A, 0);
						dwin_write_vp_u16(&dwin_hmi, 0x123A, 0);
						break;
					case 1:
						dwin_write_vp_u16(&dwin_hmi, 0x121A, 0);
						dwin_write_vp_u16(&dwin_hmi, 0x122A, 1);
						dwin_write_vp_u16(&dwin_hmi, 0x123A, 0);
						break;
					case 2:
						dwin_write_vp_u16(&dwin_hmi, 0x121A, 0);
						dwin_write_vp_u16(&dwin_hmi, 0x122A, 0);
						dwin_write_vp_u16(&dwin_hmi, 0x123A, 1);
						break;
					default:
						break;
					}
				}
			}

			else if (value == HMI_PAGE3 && hmi_data.sensor_test_active == false)
			{
				osAcquireMutex(&hmi_data.mutex);
				hmi_data.sensor_test_id = 1; // initial sensor test value
				hmi_data.sensor_test_active = true;
				osReleaseMutex(&hmi_data.mutex);
				// set initial sensor test value
				dwin_write_vp_u16(&dwin_hmi, LGC_HMI_VP_TEST_CHOICED_SENSOR, hmi_data.sensor_test_id);
			}

			break;
		}
		// clear one leather area
		case 0x1501:
		{
			// clear index
			lgc_clear_measurement_last_leather();
			// set hmi update
			osSetEventBits(&events, LGC_HMI_UPDATE_REQUIRED);
		}
		// print report
		case LGC_HMI_VP_PRINT:
		{
			// send print command
			osSetEventBits(&events, LGC_EVENT_PRINT_BATCH);
			// increment batch index
			// clear leather count
			lgc_increment_batch_index();
			// set hmi update
			osSetEventBits(&events, LGC_HMI_UPDATE_REQUIRED);
			break;
		}
		// Sensor test
		case LGC_HMI_VP_TEST_CHOICED_SENSOR:
		{
			/*get value*/
			value = (msg.data[0] << 8) | msg.data[1];
			/*store value*/
			osAcquireMutex(&hmi_data.mutex);
			hmi_data.sensor_test_id = value;
			hmi_data.sensor_test_active = true;
			osReleaseMutex(&hmi_data.mutex);
			// update sensor threadhold
			lgc_modbus_read_holding_regs(hmi_data.sensor_test_id % 12, 12, &value, 1);

			value = value / 40; // scale factor for slider
			// update HMI values
			dwin_write_vp_u16(&dwin_hmi, 0x1108, value);
			dwin_write_vp_u16(&dwin_hmi, 0x1109, value);
			// set update event
			osSetEventBits(&events, LGC_HMI_SENSOR_TEST_UPDATE);
			break;
		}
		// save data
		case LGC_HMI_VP_CONFIG_SAVE_CMD:
		{
			if (hmi_data.sensor_test_active)
			{
				/*get value: only test sensor comunication*/
				if (lgc_modbus_read_holding_regs(hmi_data.sensor_test_id % 12, 12, &value, 1) != NO_ERROR)
				{
					value = 2;
				}
				else
				{
					value = 1;
				}
				/*store value*/
			}
			else if (hmi_data.current_page == HMI_PAGE7)
			{
				// get day
				dwin_read_u16(&dwin_hmi, 0x1341, &value, 1000);
				osAcquireMutex(&hmi_data.mutex);
				hmi_data.day = value;
				osReleaseMutex(&hmi_data.mutex);
				// get month
				dwin_read_u16(&dwin_hmi, 0x1342, &value, 1000);
				osAcquireMutex(&hmi_data.mutex);
				hmi_data.month = value;
				osReleaseMutex(&hmi_data.mutex);
				// get year
				dwin_read_u16(&dwin_hmi, 0x1343, &value, 1000);
				osAcquireMutex(&hmi_data.mutex);
				hmi_data.year = value;
				osReleaseMutex(&hmi_data.mutex);
				// set date time
				lgc_module_rtc_get(&datetime);
				datetime.day = hmi_data.day;
				datetime.month = hmi_data.month;
				datetime.year = hmi_data.year;
				lgc_module_rtc_set(&datetime);
				// save ok
				value = 1;
			}
			else if (hmi_data.current_page == HMI_PAGE10)
			{
				// read hh value
				dwin_read_u16(&dwin_hmi, 0x1346, &value, 1000);
				osAcquireMutex(&hmi_data.mutex);
				hmi_data.hh = value;
				osReleaseMutex(&hmi_data.mutex);
				// read mm value
				dwin_read_u16(&dwin_hmi, 0x1347, &value, 1000);
				osAcquireMutex(&hmi_data.mutex);
				hmi_data.mm = value;
				osReleaseMutex(&hmi_data.mutex);
				// read ss value
				dwin_read_u16(&dwin_hmi, 0x1348, &value, 1000);
				osAcquireMutex(&hmi_data.mutex);
				hmi_data.ss = value;
				osReleaseMutex(&hmi_data.mutex);
				// get current date
				lgc_module_rtc_get(&datetime);
				// update
				datetime.hours = hmi_data.hh;
				datetime.minutes = hmi_data.mm;
				datetime.seconds = hmi_data.ss;
				// set date time
				lgc_module_rtc_set(&datetime);
				
				// save ok
				value = 1;
			}
			// another save data
			else
			{
				lgc_module_conf_set(&conf);
				value = 1;
			}
			// update result
			dwin_write_vp_u16(&dwin_hmi, LGC_HMI_VP_CONFIG_SAVE_RESULT, value);
			osDelayTask(1000);
			dwin_write_vp_u16(&dwin_hmi, LGC_HMI_VP_CONFIG_SAVE_RESULT, 0);
			break;
		}
		case LGC_HMI_VP_CONFIG_UNITS:
		{
			value = (msg.data[0] << 8) | msg.data[1];
			conf.units = (uint8_t)value;
			break;
		}
		case LGC_HMI_VP_TEST_SLIDER_THRESHOLD_SENSOR:
		{

			// update 0x1109
			value = (msg.data[0] << 8) | msg.data[1];
			dwin_write_vp_u16(&dwin_hmi, 0x1109, value);
			// set to sensor offset
			value = value * 40.96; // scale factor for slider
			lgc_modbus_write_holding_regs(hmi_data.sensor_test_id % 12, 12, &value, 1);
			// set update event
			osSetEventBits(&events, LGC_HMI_SENSOR_TEST_UPDATE);
			break;
		}
		case 0x130F: // client id name return keyboard
		{
			memset(text, 0, sizeof(text));
			// get text value
			dwin_read_text(&dwin_hmi, 0x1310, text, 10, 1000);
			// save text
			lgc_module_conf_get(&conf);
			strcpy(conf.client_name, text);
			lgc_module_conf_set(&conf);
			break;
		}
		case 0x131F: // color id return for keyboard
		{
			memset(text, 0, sizeof(text));
			// get text value
			dwin_read_text(&dwin_hmi, 0x1320, text, 10, 1000);
			// save text
			lgc_module_conf_get(&conf);
			strcpy(conf.color, text);
			lgc_module_conf_set(&conf);
			break;
		}
		case 0x132F: // leather id return for keyboard
		{
			memset(text, 0, sizeof(text));
			// get text value
			dwin_read_text(&dwin_hmi, 0x1330, text, 20, 1000);
			// save text
			lgc_module_conf_get(&conf);
			strcpy(conf.leather_id, text);
			lgc_module_conf_set(&conf);
			break;
		}
		case 0x1340: // batch number return for keyboard
		{
			// get text value
			value = (msg.data[0] << 8) | msg.data[1];
			if (value > 300)
			{
				value = 300;
				// write back corrected value
				dwin_write_vp_u16(&dwin_hmi, 0x1340, value);
			}
			else if (value <= 0)
			{
				value = 1;
				// write back corrected value
				dwin_write_vp_u16(&dwin_hmi, 0x1340, value);
			}
			// save text
			lgc_module_conf_get(&conf);
			conf.batch = value;
			lgc_module_conf_set(&conf);
			break;
		}
		
		case 0x121A:
		{
			// update config
			conf.conversion = 0;
			dwin_write_vp_u16(&dwin_hmi, 0x121A, 1);
			dwin_write_vp_u16(&dwin_hmi, 0x122A, 0);
			dwin_write_vp_u16(&dwin_hmi, 0x123A, 0);
			break;
		}
		case 0x122A:
		{
			conf.conversion = 1;
			dwin_write_vp_u16(&dwin_hmi, 0x121A, 0);
			dwin_write_vp_u16(&dwin_hmi, 0x122A, 1);
			dwin_write_vp_u16(&dwin_hmi, 0x123A, 0);
			break;
		}
		case 0x123A:
		{
			conf.conversion = 2;
			dwin_write_vp_u16(&dwin_hmi, 0x121A, 0);
			dwin_write_vp_u16(&dwin_hmi, 0x122A, 0);
			dwin_write_vp_u16(&dwin_hmi, 0x123A, 1);
			break;
		}

		default:
			break;
		}

		/*release memory*/
		osFreeMem(&msg.data);
	}
}
//-------------------------------------------------------------------------------
// private function definition
//-------------------------------------------------------------------------------
static void lgc_dwin_process_task_entry(void *param);
/*---------------------------------------------------------------------------*/
/* Implementación HAL                                                        */
/*---------------------------------------------------------------------------*/

uint32_t lgc_dwin_uart_transmit(uint8_t *data, uint16_t len)
{
	HAL_UART_Transmit_DMA(&huart6, (uint8_t *)data, len);
	/*wait for transmit finish*/
	if (osWaitForSemaphore(&tx_cplt_flag, INFINITE_DELAY) != TRUE)
	{
		return 0;
	}

	return len;
}

uint32_t lgc_dwin_get_tick(void)
{
	return osGetSystemTime();
}
void lgc_dwin_lock(void)
{
	osAcquireMutex(&dwin_mutex);
}
void lgc_dwin_unlock(void)
{
	osReleaseMutex(&dwin_mutex);
}

bool lgc_dwin_signal(uint32_t timeout)
{
	return (osWaitForSemaphore(&dwin_response, timeout) == TRUE);
}

void lgc_dwin_signal_set(void)
{
	osReleaseSemaphore(&dwin_response);
}

/* Semáforo para nuevos datos (Consumidor) */
void lgc_dwin_new_data_signal(void)
{
	// Espera infinita hasta que llegue algo
	osWaitForSemaphore(&dwin_new_data_flag, INFINITE_DELAY);
}

/* Semáforo para nuevos datos (Productor - ISR) */
void lgc_dwin_new_data_signal_set(void)
{
	// IMPORTANTE: Si tu RTOS tiene funciones "FromISR", úsalas aquí.
	// ThreadX osReleaseSemaphore suele ser seguro en ISR, pero verifica tu port.
	osReleaseSemaphore(&dwin_new_data_flag);
}

static void lgc_dwin_process_task_entry(void *param)
{
	dwin_register_callback(&dwin_hmi, on_dwin_event, NULL);
	for (;;)
	{
		dwin_process(&dwin_hmi);
	}
}
/*---------------------------------------------------------------------------*/
/* hmi callbacks                                                             */
/*---------------------------------------------------------------------------*/
static void on_dwin_event(dwin_evt_t *evt, void *ctx)
{
	lgc_hmi_send_msg(evt);
}
//-------------------------------------------------------------------------------
// hadrware callback
//-------------------------------------------------------------------------------
static void lgc_dwin_uart_ErrorCallback(UART_HandleTypeDef *huart)

{
	HAL_UARTEx_ReceiveToIdle_DMA(&huart6, uart_rx, 128);
}

static void lgc_dwin_uart_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Pos)
{
	// push data
	dwin_rx_push_ex(&dwin_hmi, uart_rx, Pos);
	// notify
	dwin_rx_notify(&dwin_hmi);
	// receive another data
	HAL_UARTEx_ReceiveToIdle_DMA(&huart6, uart_rx, 128);
}

void lgc_dwin_uart_TxCpltCallback(UART_HandleTypeDef *huart)
{
	osReleaseSemaphore(&tx_cplt_flag);
}
//-------------------------------------------------------------------------------
// public functions
//-------------------------------------------------------------------------------
error_t lgc_hmi_init(void)
{
	OsTaskParameters params = OS_TASK_DEFAULT_PARAMS;

	// 1. Inicializar la interfaz
	dwin_hal.uart_transmit = lgc_dwin_uart_transmit;
	dwin_hal.get_tick_ms = lgc_dwin_get_tick;
	dwin_hal.lock = lgc_dwin_lock;
	dwin_hal.unlock = lgc_dwin_unlock;
	dwin_hal.sem_signal = lgc_dwin_signal_set;
	dwin_hal.sem_wait = lgc_dwin_signal;

	/* Configuración de bloqueo en RX */
	dwin_hal.sem_new_data_wait = lgc_dwin_new_data_signal;
	dwin_hal.sem_new_data_signal = lgc_dwin_new_data_signal_set;

	if (dwin_init(&dwin_hmi, &dwin_hal, dwin_fifo_mem, DWIN_BUFFER_SIZE) != DWIN_OK)
	{
		return ERROR_FAILURE;
	}

	/* Crear primitivas OS */
	if (osCreateSemaphore(&tx_cplt_flag, 0) != TRUE)
	{
		return ERROR_FAILURE;
	}
	if (osCreateSemaphore(&dwin_response, 0) != TRUE)
	{
		return ERROR_FAILURE;
	}
	if (osCreateMutex(&dwin_mutex) != TRUE)
	{
		return ERROR_FAILURE;
	}
	if (osCreateSemaphore(&dwin_new_data_flag, 0) != TRUE)
	{
		return ERROR_FAILURE;
	}
	if (osCreateMutex(&hmi_data.mutex) != TRUE)
	{
		return ERROR_FAILURE;
	}
	if (osCreateQueue(&hmi_msg, "hmi msg", sizeof(dwin_evt_t), 5) != TRUE)
	{
		return ERROR_FAILURE;
	}

	params = OS_TASK_DEFAULT_PARAMS;
	params.priority = DWIN_PROCESS_TASK_PRI;
	params.stackSize = DWIN_PROCESS_TASK_STACK;
	dwin_process_task = osCreateTask("DWIN Process", lgc_dwin_process_task_entry, NULL, &params);

	if (!dwin_process_task)
	{
		return ERROR_FAILURE;
	}

	params = OS_TASK_DEFAULT_PARAMS;
	params.priority = LGC_HMI_TASK_PRI;
	params.stackSize = LGC_HMI_TASK_STACK;
	lgc_hmi_task = osCreateTask("HMI Task", lgc_hmi_task_entry, NULL, &params);

	if (!lgc_hmi_task)
	{
		return ERROR_FAILURE;
	}

	params = OS_TASK_DEFAULT_PARAMS;
	params.priority = LGC_HMI_UPDATE_TASK_PRI;
	params.stackSize = LGC_HMI_UPDATE_TASK_STACK;
	lgc_hmi_update_task = osCreateTask("HMI Update Task", lgc_hmi_update_task_entry, NULL, &params);
	if (!lgc_hmi_update_task)
	{
		return ERROR_FAILURE;
	}
	/*uart receive data*/
	HAL_UART_RegisterCallback(&huart6, HAL_UART_ERROR_CB_ID, lgc_dwin_uart_ErrorCallback);

	HAL_UART_RegisterCallback(&huart6, HAL_UART_TX_COMPLETE_CB_ID, lgc_dwin_uart_TxCpltCallback);

	HAL_UART_RegisterRxEventCallback(&huart6, lgc_dwin_uart_RxEventCallback);

	/*start receive data*/
	HAL_UARTEx_ReceiveToIdle_DMA(&huart6, uart_rx, 128);

	return NO_ERROR;
}

error_t lgc_hmi_send_msg(dwin_evt_t *evt)
{
	error_t err = NO_ERROR;

	dwin_evt_t msg = {0};

	msg.addr = evt->addr;
	msg.cmd = evt->cmd;
	msg.len = evt->len;
	msg.data_len = evt->data_len;
	/*allocate memory*/
	msg.data = osAllocMem(evt->data_len);
	if (!msg.data)
	{
		return ERROR_FAILURE;
	}
	memcpy(msg.data, evt->data, evt->data_len);

	// send message
	err = osSendToQueue(&hmi_msg, &msg, 1000);

	return err;
}

static void hmi_set_current_page(uint8_t page)
{
	osAcquireMutex(&hmi_data.mutex);
	hmi_data.current_page = page;
	if (page != HMI_PAGE3 && page != HMI_PAGE4)
	{
		hmi_data.sensor_test_active = false;
	}
	osReleaseMutex(&hmi_data.mutex);
	return;
}
