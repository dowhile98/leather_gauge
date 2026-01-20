/*
 * lgc.c
 *
 *  Created on: Jan 8, 2026
 *      Author: tecna-smart-lab
 */

#include "lgc.h"

#ifndef LGC_MAIN_TASK_STACK
#define LGC_MAIN_TASK_STACK 256
#endif

#ifndef LGC_MAIN_TASK_PRI
#define LGC_MAIN_TASK_PRI 10
#endif

extern uint8_t osPoolInit(void *pointer);

static OsTaskId lgc_main_task = NULL;

OsEvent events;
/*private functions*/

/*public functions*/
error_t lgc_system_init(void *memory)
{
	error_t ret = NO_ERROR;
	OsTaskParameters params = OS_TASK_DEFAULT_PARAMS;
	// memory pool init
	osPoolInit(memory);

	// init interfaces

	// HMI
	ret = lgc_hmi_init();
	if (ret != NO_ERROR)
	{
		return ret;
	}
	// PRINTER

	ret = lgc_printer_init();

	if (ret != NO_ERROR)
	{
		return ret;
	}

	/*event*/
	ret = osCreateEvent(&events);
	if (ret != TRUE)
	{
		return ret;
	}
	// modbus
	ret = lgc_interface_modbus_init();

	if (ret != NO_ERROR)
	{
		return ret;
	}
	/*inputs*/
	ret = lgc_module_input_init(lgc_buttons_callback);
	if (ret != NO_ERROR)
	{
		return ret;
	}

	/*eeprom init*/
	ret = lgc_module_eeprom_init();
	if(ret != NO_ERROR)
	{
		return ret;
	}
	
	// main task init
	params.priority = LGC_MAIN_TASK_PRI;
	params.stackSize = LGC_MAIN_TASK_STACK;

	lgc_main_task = osCreateTask("main", lgc_main_task_entry, NULL, &params);

	if (!lgc_main_task)
	{
		return ERROR_FAILURE;
	}

	return ret;
}
