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
#include "ESC_POS_Printer.h"
#include "lwprintf.h"
//-------------------------------------------------------------------------------
// defines
//-------------------------------------------------------------------------------
#ifndef LGC_PRINTER_TASK_STACK
#define LGC_PRINTER_TASK_STACK 128
#endif

#ifndef LGC_PRINTER_TASK_PRI
#define LGC_PRINTER_TASK_PRI 10
#endif
//-------------------------------------------------------------------------------
// global variables
//-------------------------------------------------------------------------------
static esc_pos_printer_t printer = {0};

static OsTaskId lgc_printer_task = NULL;
//-------------------------------------------------------------------------------
// private function prototype
//-------------------------------------------------------------------------------
static void printer_delay(uint32_t ms);

static void lgc_printer_task_entry(void *params);
//-------------------------------------------------------------------------------
// public function definitions
//-------------------------------------------------------------------------------
uint8_t lgc_printer_connected(void)
{
	return lgc_interface_printer_connected();
}
//-------------------------------------------------------------------------------
// private function definition
//-------------------------------------------------------------------------------
static void printer_delay(uint32_t ms)
{
	osDelayTask(ms);
}
//-------------------------------------------------------------------------------
// public
//-------------------------------------------------------------------------------
error_t lgc_printer_init(void)
{
	OsTaskParameters params = OS_TASK_DEFAULT_PARAMS;
	error_t err = NO_ERROR;

	esc_pos_init(&printer, lgc_interface_printer_writeData, printer_delay, PRINTER_80MM);

	params.priority = LGC_PRINTER_TASK_PRI;
	params.stackSize = LGC_PRINTER_TASK_STACK;

	lgc_printer_task = osCreateTask("printer", lgc_printer_task_entry, NULL, &params);

	if (!lgc_printer_task)
	{
		err = ERROR_FAILURE;
	}

	return err;
}

static void lgc_printer_task_entry(void *params)
{
	char buffer[64];
	lgc_measurements_t *measurements;
	measurements = osAllocMem(sizeof(lgc_measurements_t));
	LGC_CONF_TypeDef_t conf = {0};
	/*wait for printer connected*/
	do
	{
		// delay
		osDelayTask(100);
		// verify
	} while (lgc_interface_printer_connected() == 0);
	// printer is ready
	osDelayTask(50);


	for (;;)
	{
		if (osWaitForEventBits(&events, LGC_EVENT_PRINT_BATCH | LGC_EVENT_PRINT_BATCH_COMPLETED, FALSE, TRUE, INFINITE_DELAY) == TRUE)
		{
			//get current config
			lgc_module_conf_get(&conf);
			// print batch data

			// get measurements
			if (measurements == NULL)
			{
				continue;
			}
			lgc_get_measurements(measurements);
			// print header
			esc_pos_set_align(&printer, ALIGN_CENTER);
			esc_pos_print_text(&printer, (char *)"\rBatch Measurement\r\n");
			esc_pos_set_align(&printer, ALIGN_LEFT);
			//batch number
			lwprintf_snprintf(buffer, sizeof(buffer), "Batch: %d\r\n", measurements->current_batch_index);
			esc_pos_print_text(&printer, buffer);
			//company info
			esc_pos_print_text(&printer, "EMPRESA  : CURPISCO S.A.C.\r\n");
			esc_pos_print_text(&printer, "DIRECCION: AV. LOS GIRASOLES 123\r\n");
			esc_pos_print_text(&printer, "TELFONO  : +51 987654321\r\n");
			esc_pos_print_text(&printer, "RUC      : 20123456789\r\n");
			//client info
			lwprintf_snprintf(buffer, sizeof(buffer), "CLIENTE  : %s\r\n", conf.client_name);
			esc_pos_print_text(&printer, buffer);
			lwprintf_snprintf(buffer, sizeof(buffer), "COLOR    : %s\r\n", conf.color);
			esc_pos_print_text(&printer, buffer);
			lwprintf_snprintf(buffer, sizeof(buffer), "LEATHER ID: %s\r\n", conf.leather_id);
			esc_pos_print_text(&printer, buffer);
			
			// units
			if (conf.units == 0)
			{
				esc_pos_print_text(&printer, "UNITS: SQUARE METERS\r\n");
			}
			else if (conf.units == 1)
			{	
				esc_pos_print_text(&printer, "UNITS: SQUARE FEET\r\n");
			}
			esc_pos_print_text(&printer, "--------------------------------\r\n");
			


			// print leathers
			for (uint16_t i = 0; i < measurements->current_batch_index; i++)
			{
				lwprintf_snprintf(buffer, sizeof(buffer), "Leather %d: %.2f sqm\r\n", i + 1, measurements->leather_measurement[i]);
				esc_pos_print_text(&printer, buffer);
			}
			// print batch total
			lwprintf_snprintf(buffer, sizeof(buffer), "\rBatch Total: %.2f sqm\r\n", measurements->batch_measurement[measurements->current_batch_index - 1]);
			esc_pos_print_text(&printer, buffer);
			// cut paper
			esc_pos_cut(&printer, false);
		}
	}
}
