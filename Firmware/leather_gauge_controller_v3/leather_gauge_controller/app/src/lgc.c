/*
 * lgc.c
 *
 *  Created on: Jan 8, 2026
 *      Author: tecna-smart-lab
 */

#include "lgc.h"
#include "lgc_modbus_task.h"
#include "lgc_sensor_cache.h"

#ifndef LGC_MAIN_TASK_STACK
#define LGC_MAIN_TASK_STACK 1024
#endif

#ifndef LGC_MAIN_TASK_PRI
#define LGC_MAIN_TASK_PRI 10
#endif

#ifndef LGC_MODBUS_TASK_STACK
#define LGC_MODBUS_TASK_STACK 256
#endif

#ifndef LGC_MODBUS_TASK_PRI
#define LGC_MODBUS_TASK_PRI 9 /* Higher priority than Main Task */
#endif

extern uint8_t osPoolInit(void *pointer);

static OsTaskId lgc_main_task = NULL;
static OsTaskId lgc_modbus_task = NULL;

/* Shared instances for async Modbus architecture */
static LgcSensorCache_t s_sensor_cache;
static LgcModbusTask_t s_modbus_task;

OsEvent events;

/*private functions*/

/*public functions*/

/**
 * @brief Get sensor cache interface for Main Task
 */
ILgcSensorCache_t *lgc_get_sensor_cache_interface(void)
{
	return LgcSensorCache_GetInterface(&s_sensor_cache);
}

/**
 * @brief Trigger Modbus read cycle (called from encoder callback)
 */
void lgc_trigger_modbus_cycle(void)
{
	LgcModbusTask_TriggerCycle(&s_modbus_task);
}

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

	/* ================================================================
	 * ASYNC MODBUS ARCHITECTURE - Sensor Cache + Modbus Task
	 * ================================================================ */

	/* Initialize sensor cache */
	ret = LgcSensorCache_Init(&s_sensor_cache);
	if (ret != NO_ERROR)
	{
		return ret;
	}

	/* Initialize Modbus task with default configuration */
	ret = LgcModbusTask_Init(&s_modbus_task, &s_sensor_cache, NULL);
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
	if (ret != NO_ERROR)
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

	/* Create Modbus task (higher priority than main) */
	params.priority = LGC_MODBUS_TASK_PRI;
	params.stackSize = LGC_MODBUS_TASK_STACK;

	lgc_modbus_task = osCreateTask("modbus", lgc_modbus_task_entry, &s_modbus_task, &params);

	if (!lgc_modbus_task)
	{
		return ERROR_FAILURE;
	}

	/* Start Modbus task */
	ret = LgcModbusTask_Start(&s_modbus_task);
	if (ret != NO_ERROR)
	{
		return ret;
	}

	return ret;
}
