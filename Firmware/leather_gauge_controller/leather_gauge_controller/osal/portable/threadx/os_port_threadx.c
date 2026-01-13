/**
 * @file os_port_threadx.c
 * @brief RTOS abstraction layer (Eclipse ThreadX)
 *
 * @section License
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Copyright (C) 2010-2025 Oryx Embedded SARL. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * @author Oryx Embedded SARL (www.oryx-embedded.com)
 * @version 2.5.2
 **/

#include "os_port_config.h"
#if defined(USE_THREADX)
//Switch to the appropriate trace level
#define TRACE_LEVEL TRACE_LEVEL_OFF

//Dependencies
#include <stdio.h>
#include <stdlib.h>
#include "os_port.h"
#include "os_port_threadx.h"
#include "debug.h"

//Global variable
static TX_INTERRUPT_SAVE_AREA

//Default task parameters
const OsTaskParameters OS_TASK_DEFAULT_PARAMS =
{
		NULL,                 //Task control block
		NULL,                 //Stack
		0,                    //Size of the stack
		TX_MAX_PRIORITIES - 1 //Task priority
};


/**
 * @brief Kernel initialization
 **/

void osInitKernel(void)
{
	//Low-level initialization
	_tx_initialize_low_level();
}


/**
 * @brief Start kernel
 **/

void osStartKernel(void)
{
	//Start the scheduler
	tx_kernel_enter();
}


/**
 * @brief Create a task
 * @param[in] name NULL-terminated string identifying the task
 * @param[in] taskCode Pointer to the task entry function
 * @param[in] arg Argument passed to the task function
 * @param[in] params Task parameters
 * @return Task identifier referencing the newly created task
 **/

OsTaskId osCreateTask(const char_t *name, OsTaskCode taskCode, void *arg,
		OsTaskParameters *params)
{
	UINT status;
	OsTaskId taskId;

    if (!params || !taskCode || params->stackSize == 0)
    {
        return OS_INVALID_TASK_ID;
    }

	//allocate memory
	if(params->tcb == NULL){
		params->tcb  = (TX_THREAD *)osAllocMem(sizeof(TX_THREAD));
	}
	if(params->stack == NULL){
		params->stack = (uint32_t *)osAllocMem( params->stackSize * sizeof(uint32_t));
	}
	//Check parameters
	if(params->tcb != NULL && params->stack != NULL)
	{
		//Create a new task
		status = tx_thread_create(params->tcb, (CHAR *) name,
				(OsTaskFunction) taskCode, (ULONG) arg, params->stack,
				params->stackSize * sizeof(uint32_t), params->priority,
				params->priority, 1, TX_AUTO_START);

		//Check whether the task was successfully created
		if(status == TX_SUCCESS)
		{
			taskId = (OsTaskId) params->tcb;
		}
		else
		{
			taskId = OS_INVALID_TASK_ID;
		}
	}
	else
	{
		//Invalid parameters
		taskId = OS_INVALID_TASK_ID;
	}

	//Return the handle referencing the newly created thread
	return taskId;
}


/**
 * @brief Delete a task
 * @param[in] taskId Task identifier referencing the task to be deleted
 **/

void osDeleteTask(OsTaskId taskId)
{
	//Delete the specified task
	tx_thread_delete((TX_THREAD *) taskId);
}


/**
 * @brief Delay routine
 * @param[in] delay Amount of time for which the calling task should block
 **/

void osDelayTask(systime_t delay)
{
	//Delay the task for the specified duration
	tx_thread_sleep(OS_MS_TO_SYSTICKS(delay));
}


/**
 * @brief Yield control to the next task
 **/

void osSwitchTask(void)
{
	//Force a context switch
	tx_thread_relinquish();
}


/**
 * @brief Suspend scheduler activity
 **/

void osSuspendAllTasks(void)
{
	//Suspend all tasks
	TX_DISABLE
}


/**
 * @brief Resume scheduler activity
 **/

void osResumeAllTasks(void)
{
	//Resume all tasks
	TX_RESTORE
}


/**
 * @brief Create an event object
 * @param[in] event Pointer to the event object
 * @return The function returns TRUE if the event object was successfully
 *   created. Otherwise, FALSE is returned
 **/

bool_t osCreateEvent(OsEvent *event)
{
	return osCreateEventEx(event);
}


/**
 * @brief Delete an event object
 * @param[in] event Pointer to the event object
 **/

void osDeleteEvent(OsEvent *event)
{
	//Make sure the event object is valid
	if(event->tx_event_flags_group_id == TX_EVENT_FLAGS_ID)
	{
		//Properly dispose the event object
		tx_event_flags_delete(event);
	}
}


/**
 * @brief Set the specified event object to the signaled state
 * @param[in] event Pointer to the event object
 **/

void osSetEvent(OsEvent *event)
{
	//Set the specified event to the signaled state
	tx_event_flags_set(event, 1, TX_OR);
}


/**
 * @brief Set the specified event object to the nonsignaled state
 * @param[in] event Pointer to the event object
 **/

void osResetEvent(OsEvent *event)
{
	ULONG actualFlags;

	//Force the specified event to the nonsignaled state
	tx_event_flags_get(event, 1, TX_AND_CLEAR, &actualFlags, 0);
}


/**
 * @brief Wait until the specified event is in the signaled state
 * @param[in] event Pointer to the event object
 * @param[in] timeout Timeout interval
 * @return The function returns TRUE if the state of the specified object is
 *   signaled. FALSE is returned if the timeout interval elapsed
 **/

bool_t osWaitForEvent(OsEvent *event, systime_t timeout)
{
	UINT status;
	ULONG actualFlags;

	//Wait until the specified event is in the signaled state or the timeout
	//interval elapses
	if(timeout == INFINITE_DELAY)
	{
		//Infinite timeout period
		status = tx_event_flags_get(event, 1, TX_OR_CLEAR, &actualFlags,
				TX_WAIT_FOREVER);
	}
	else
	{
		//Wait until the specified event becomes set
		status = tx_event_flags_get(event, 1, TX_OR_CLEAR, &actualFlags,
				OS_MS_TO_SYSTICKS(timeout));
	}

	//Check whether the specified event is set
	if(status == TX_SUCCESS)
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}


/**
 * @brief Set an event object to the signaled state from an interrupt service routine
 * @param[in] event Pointer to the event object
 * @return TRUE if setting the event to signaled state caused a task to unblock
 *   and the unblocked task has a priority higher than the currently running task
 **/

bool_t osSetEventFromIsr(OsEvent *event)
{
	//Set the specified event to the signaled state
	tx_event_flags_set(event, 1, TX_OR);

	//The return value is not relevant
	return FALSE;
}
/**
 * @brief Create an event group statically (multi-bit).
 * @param event Pointer to OsEvent instance.
 * @return TRUE if creation succeeded, FALSE otherwise.
 */
bool_t osCreateEventEx(OsEvent *event)
{
	if (event == NULL) {
		return FALSE;
	}
	UINT status = tx_event_flags_create(event, "OsEvt");
	return (status == TX_SUCCESS) ? TRUE : FALSE;
}

/**
 * @brief Delete an event group (static instance).
 * @param event Pointer to OsEvent instance.
 */
void osDeleteEventEx(OsEvent *event)
{
	if (event == NULL) {
		return;
	}
	tx_event_flags_delete(event);
}

/**
 * @brief Set bits in the event group.
 */
bool_t osSetEventBits(OsEvent *event, uint32_t mask)
{
	if (event == NULL) {
		return FALSE;
	}
	UINT status = tx_event_flags_set(event, mask, TX_OR);
	return (status == TX_SUCCESS) ? TRUE : FALSE;
}

/**
 * @brief Clear bits in the event group.
 */
bool_t osClearEventBits(OsEvent *event, uint32_t mask)
{
	if (event == NULL) {
		return FALSE;
	}
	UINT status = tx_event_flags_set(event, ~mask, TX_AND);
	return (status == TX_SUCCESS) ? TRUE : FALSE;
}

/**
 * @brief Set bits from ISR context.
 */
bool_t osSetEventBitsFromIsr(OsEvent *event, uint32_t mask)
{
	return osSetEventBits(event, mask);
}


uint32_t osGetEventBits(OsEvent *event, uint32_t mask)
{
	ULONG flags = 0;
	if(tx_event_flags_get(event, mask, TX_OR, &flags, 0) != TX_SUCCESS)
	{
		return 0;
	}
	return flags;
}
/**
 * @brief Wait for bits with options.
 */
bool_t osWaitForEventBits(OsEvent *event,
		uint32_t mask,
		bool_t waitAll,
		bool_t clearOnExit,
		systime_t timeout)
{
	if (event == NULL) {
		return FALSE;
	}
	UINT get_option;
	if (waitAll) {
		get_option = clearOnExit ? TX_AND_CLEAR : TX_AND;
	} else {
		get_option = clearOnExit ? TX_OR_CLEAR : TX_OR;
	}
	ULONG ticks;
	if (timeout == INFINITE_DELAY) {
		ticks = TX_WAIT_FOREVER;
	} else {
		ticks = OS_MS_TO_SYSTICKS(timeout);
	}
	ULONG actual_flags;
	UINT status = tx_event_flags_get(event, mask, get_option, &actual_flags, ticks);
	return (status == TX_SUCCESS) ? TRUE : FALSE;
}


/**
 * @brief Create a semaphore object
 * @param[in] semaphore Pointer to the semaphore object
 * @param[in] count The maximum count for the semaphore object. This value
 *   must be greater than zero
 * @return The function returns TRUE if the semaphore was successfully
 *   created. Otherwise, FALSE is returned
 **/

bool_t osCreateSemaphore(OsSemaphore *semaphore, uint_t count)
{
	UINT status;

	//Create a semaphore object
	status = tx_semaphore_create(semaphore, "SEMAPHORE", count);

	//Check whether the semaphore was successfully created
	if(status == TX_SUCCESS)
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}


/**
 * @brief Delete a semaphore object
 * @param[in] semaphore Pointer to the semaphore object
 **/

void osDeleteSemaphore(OsSemaphore *semaphore)
{
	//Make sure the semaphore object is valid
	if(semaphore->tx_semaphore_id == TX_SEMAPHORE_ID)
	{
		//Properly dispose the semaphore object
		tx_semaphore_delete(semaphore);
	}
}


/**
 * @brief Wait for the specified semaphore to be available
 * @param[in] semaphore Pointer to the semaphore object
 * @param[in] timeout Timeout interval
 * @return The function returns TRUE if the semaphore is available. FALSE is
 *   returned if the timeout interval elapsed
 **/

bool_t osWaitForSemaphore(OsSemaphore *semaphore, systime_t timeout)
{
	UINT status;

	//Wait until the semaphore is available or the timeout interval elapses
	if(timeout == INFINITE_DELAY)
	{
		//Infinite timeout period
		status = tx_semaphore_get(semaphore, TX_WAIT_FOREVER);
	}
	else
	{
		//Wait until the specified semaphore becomes available
		status = tx_semaphore_get(semaphore, OS_MS_TO_SYSTICKS(timeout));
	}

	//Check whether the specified semaphore is available
	if(status == TX_SUCCESS)
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}


/**
 * @brief Release the specified semaphore object
 * @param[in] semaphore Pointer to the semaphore object
 **/

void osReleaseSemaphore(OsSemaphore *semaphore)
{
	//Release the semaphore
	tx_semaphore_put(semaphore);
}


/**
 * @brief Create a mutex object
 * @param[in] mutex Pointer to the mutex object
 * @return The function returns TRUE if the mutex was successfully
 *   created. Otherwise, FALSE is returned
 **/

bool_t osCreateMutex(OsMutex *mutex)
{
	UINT status;

	//Create a mutex object
	status = tx_mutex_create(mutex, "MUTEX", TX_NO_INHERIT);

	//Check whether the mutex was successfully created
	if(status == TX_SUCCESS)
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}


/**
 * @brief Delete a mutex object
 * @param[in] mutex Pointer to the mutex object
 **/

void osDeleteMutex(OsMutex *mutex)
{
	//Make sure the mutex object is valid
	if(mutex->tx_mutex_id == TX_MUTEX_ID)
	{
		//Properly dispose the mutex object
		tx_mutex_delete(mutex);
	}
}


/**
 * @brief Acquire ownership of the specified mutex object
 * @param[in] mutex Pointer to the mutex object
 **/

void osAcquireMutex(OsMutex *mutex)
{
	//Obtain ownership of the mutex object
	tx_mutex_get(mutex, TX_WAIT_FOREVER);
}

uint8_t osAcquireMutexExt(OsMutex *mutex, uint32_t timeout)
{

	if(tx_mutex_get(mutex, timeout) == TX_SUCCESS)
	{
		return TRUE;
	}

	return FALSE;
}

/**
 * @brief Release ownership of the specified mutex object
 * @param[in] mutex Pointer to the mutex object
 **/

void osReleaseMutex(OsMutex *mutex)
{
	//Release ownership of the mutex object
	tx_mutex_put(mutex);
}

bool_t osCreateTimer(OsTimer *timer, OsTimerCode code, void *arg, systime_t duration, systime_t type)
{
	ULONG initial_ticks = OS_MS_TO_SYSTICKS(duration);
	ULONG period_ticks = type ? OS_MS_TO_SYSTICKS(type) : 0;


	if (initial_ticks == 0)
	{
		initial_ticks = 1; // Initial delay cannot be 0 in ThreadX
	}


	 // Create the timer but do not activate it yet.
	if(tx_timer_create(timer, (CHAR*)"timer", (VOID (*)(ULONG))code, (ULONG)arg,
	    		initial_ticks, period_ticks, TX_NO_ACTIVATE) != TX_SUCCESS)
	{
		return FALSE;
	}

	return TRUE;
}
bool_t osStartTimer(OsTimer *timer)
{
    return (tx_timer_activate(timer) == TX_SUCCESS);
}

bool_t osStopTimer(OsTimer *timer)
{
    return (tx_timer_deactivate(timer) == TX_SUCCESS);
}

bool_t osDeleteTimer(OsTimer *timer)
{
	if(tx_timer_deactivate(timer) != TX_SUCCESS)
	{
		return FALSE;
	}

	if(tx_timer_delete(timer) != TX_SUCCESS)
	{
		return FALSE;
	}

	return TRUE;

}

bool_t osChangeTimerPeriod(OsTimer *timer, systime_t newPeriod)
{
    ULONG new_period_ticks = OS_MS_TO_SYSTICKS(newPeriod);
    if (new_period_ticks == 0) new_period_ticks = 1;

    // Changing period requires current initial ticks value, which is not easily retrieved
    // without modification. A simple change is sufficient for most cases.
    ULONG initial_ticks;
    ULONG reschedule_ticks;
    UINT active;
    TX_TIMER *next_timer;

    tx_timer_info_get(timer, NULL, &active, &initial_ticks, &reschedule_ticks, &next_timer);

    return (tx_timer_change(timer, initial_ticks, new_period_ticks) == TX_SUCCESS);
}

/******************************************************************************
 * Queue Management
 ******************************************************************************/

bool_t osCreateQueue(OsQueue *queue, const char *name, size_t msgSize,
                     size_t queueSize)
{
    // ThreadX message size is in 32-bit words.
    UINT msg_size_words = (msgSize + sizeof(ULONG) - 1) / sizeof(ULONG);
    CHAR *queueStorage;
    if (msg_size_words == 0)
    {
        return FALSE;
    }
    /*reserve memory*/
    queueStorage = osAllocMem(msgSize * queueSize);

    if(queueStorage == NULL)
    {
    	return FALSE;
    }
    return (tx_queue_create(queue, (CHAR *)name, msg_size_words,
                            queueStorage, msgSize * queueSize) == TX_SUCCESS);
}

bool_t osDeleteQueue(OsQueue *queue)
{

    return (tx_queue_delete(queue) == TX_SUCCESS);
}

bool_t osSendToQueue(OsQueue *queue, const void *msg, systime_t timeout)
{
    ULONG wait_option = (timeout == INFINITE_DELAY) ? TX_WAIT_FOREVER : OS_MS_TO_SYSTICKS(timeout);
    return (tx_queue_send(queue, (void *)msg, wait_option) == TX_SUCCESS);
}

bool_t osReceiveFromQueue(OsQueue *queue, void *msg, systime_t timeout)
{
    ULONG wait_option = (timeout == INFINITE_DELAY) ? TX_WAIT_FOREVER : OS_MS_TO_SYSTICKS(timeout);
    return (tx_queue_receive(queue, msg, wait_option) == TX_SUCCESS);
}

bool_t osSendToQueueFromIsr(OsQueue *queue, const void *msg)
{
    // From an ISR, we cannot wait.
    return (tx_queue_send(queue, (void *)msg, TX_NO_WAIT) == TX_SUCCESS);
}

bool_t osFlushQueue(OsQueue *queue){

	if(tx_queue_flush(queue) == TX_SUCCESS)
	{
		return TRUE;
	}

	return FALSE;
}
/**
 * @brief Retrieve system time
 * @return Number of milliseconds elapsed since the system was last started
 **/

systime_t osGetSystemTime(void)
{
	systime_t time;

	//Get current tick count
	time = tx_time_get();

	//Convert system ticks to milliseconds
	return OS_SYSTICKS_TO_MS(time);
}


/**
 * @brief Allocate a memory block
 * @param[in] size Bytes to allocate
 * @return A pointer to the allocated memory block or NULL if
 *   there is insufficient memory available
 **/

__weak_func void *osAllocMem(size_t size)
{
	void *p;

	//Enter critical section
	osSuspendAllTasks();
	//Allocate a memory block
	p = malloc(size);
	//Leave critical section
	osResumeAllTasks();

	//Debug message
	TRACE_DEBUG("Allocating %" PRIuSIZE " bytes at 0x%08" PRIXPTR "\r\n",
			size, (uintptr_t) p);

	//Return a pointer to the newly allocated memory block
	return p;
}


/**
 * @brief Release a previously allocated memory block
 * @param[in] p Previously allocated memory block to be freed
 **/

__weak_func void osFreeMem(void *p)
{
	//Make sure the pointer is valid
	if(p != NULL)
	{
		//Debug message
		TRACE_DEBUG("Freeing memory at 0x%08" PRIXPTR "\r\n", (uintptr_t) p);

		//Enter critical section
		osSuspendAllTasks();
		//Free memory block
		free(p);
		//Leave critical section
		osResumeAllTasks();
	}
}

#endif

