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
	/*wait for printer connected*/
	do
	{
		// delay
		osDelayTask(100);
		// verify
	} while (lgc_interface_printer_connected() == 0);
	// printer is ready
	osDelayTask(50);

	esc_pos_set_align(&printer, ALIGN_CENTER);
	esc_pos_print_text(&printer, (char *)"\rHOL MUNDO\r\n");
	esc_pos_print_text(&printer, (char *)"\rHOL MUNDO1\r\n");
	esc_pos_print_text(&printer, (char *)"\rHOL MUNDO2\r\n");
	esc_pos_print_text(&printer, (char *)"\rHOL MUNDO3\r\n");
	esc_pos_print_text(&printer, (char *)"\rHOL MUNDO4\r\n");
	esc_pos_set_align(&printer, ALIGN_LEFT);
	esc_pos_cut(&printer, false);

	for (;;)
	{
		if (osWaitForEventBits(&events, LGC_EVENT_PRINT_BATCH, FALSE, TRUE, INFINITE_DELAY) == TRUE)
		{
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
