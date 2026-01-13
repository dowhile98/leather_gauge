/**
 * @file os_port_threadx.h
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



#ifndef _OS_PORT_THREADX_H
#define _OS_PORT_THREADX_H

#include "os_port_config.h"
#if defined(USE_THREADX)
//Dependencies
#include "tx_port.h"
#include "tx_user.h"
#include "tx_api.h"
#include "tx_thread.h"
#include "tx_semaphore.h"
#include "tx_event_flags.h"
#include "tx_mutex.h"
#include "tx_initialize.h"

//Invalid task identifier
#define OS_INVALID_TASK_ID NULL
//Self task identifier
#define OS_SELF_TASK_ID NULL

//Task priority (normal)
#ifndef OS_TASK_PRIORITY_NORMAL
   #define OS_TASK_PRIORITY_NORMAL (TX_MAX_PRIORITIES / 2)
#endif

//Task priority (high)
#ifndef OS_TASK_PRIORITY_HIGH
   #define OS_TASK_PRIORITY_HIGH (OS_TASK_PRIORITY_NORMAL - 1)
#endif

//Milliseconds to system ticks
#ifndef OS_MS_TO_SYSTICKS
   #define OS_MS_TO_SYSTICKS(n) (n)
#endif

//System ticks to milliseconds
#ifndef OS_SYSTICKS_TO_MS
   #define OS_SYSTICKS_TO_MS(n) (n)
#endif

//Retrieve 64-bit system time (not implemented)
#ifndef osGetSystemTime64
   #define osGetSystemTime64() osGetSystemTime()
#endif

//Task prologue
#define osEnterTask()
//Task epilogue
#define osExitTask()
//Interrupt service routine prologue
#define osEnterIsr()
//Interrupt service routine epilogue
#define osExitIsr(flag)

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief Task function
 **/

typedef VOID (*OsTaskFunction)(ULONG param);


/**
 * @brief System time
 **/

typedef uint32_t systime_t;


/**
 * @brief Task identifier
 **/

typedef TX_THREAD *OsTaskId;


/**
 * @brief Task parameters
 **/

typedef struct
{
   TX_THREAD *tcb;
   uint32_t *stack;
   size_t stackSize;
   uint_t priority;
} OsTaskParameters;


/**
 * @brief Event object
 **/

typedef TX_EVENT_FLAGS_GROUP OsEvent;


/**
 * @brief Semaphore object
 **/

typedef TX_SEMAPHORE OsSemaphore;


/**
 * @brief Mutex object
 **/

typedef TX_MUTEX OsMutex;


/**
 * @brief Task routine
 **/

typedef void (*OsTaskCode)(void *arg);
/**
 * @brief timer routine
 */
typedef void (*OsTimerCode)(void const *);
/*
 * @brief Timer object
 */
typedef TX_TIMER OsTimer;

/**
 * @brief queue object
 */
typedef TX_QUEUE OsQueue;

//Default task parameters
extern const OsTaskParameters OS_TASK_DEFAULT_PARAMS;

//Kernel management
void osInitKernel(void);
void osStartKernel(void);

//Task management
OsTaskId osCreateTask(const char_t *name, OsTaskCode taskCode, void *arg,
   OsTaskParameters *params);

void osDeleteTask(OsTaskId taskId);
void osDelayTask(systime_t delay);
void osSwitchTask(void);
void osSuspendAllTasks(void);
void osResumeAllTasks(void);

//Event management
bool_t osCreateEvent(OsEvent *event);
void osDeleteEvent(OsEvent *event);
void osSetEvent(OsEvent *event);
void osResetEvent(OsEvent *event);
bool_t osWaitForEvent(OsEvent *event, systime_t timeout);
bool_t osSetEventFromIsr(OsEvent *event);
/* ===== Extended multi-bit event API ===== */

/**
 * @brief Create an event group supporting multiple bits.
 * @param event Pointer to OsEvent (instance or pointer, per configuration).
 * @return TRUE if created successfully, FALSE otherwise.
 *
 * After creation, the event group starts with all bits = 0.
 */
bool_t osCreateEventEx(OsEvent *event);

/**
 * @brief Delete an event group.
 * @param event Pointer to OsEvent.
 *
 * After deletion, if dynamic allocation was used, the internal memory is freed
 * and *event is set to NULL.
 */
void osDeleteEventEx(OsEvent *event);

/**
 * @brief Set bits in the event group.
 *        Bits set to 1 in 'mask' will be OR-ed into the group.
 * @param event Pointer to OsEvent.
 * @param mask Bitmask indicating which bits to set (e.g., (1U<<n)).
 * @return TRUE if operation succeeded, FALSE otherwise.
 */
bool_t osSetEventBits(OsEvent *event, uint32_t mask);

/**
 * @brief Clear bits in the event group.
 *        Bits set to 1 in 'mask' will be cleared (0) in the group.
 * @param event Pointer to OsEvent.
 * @param mask Bitmask indicating which bits to clear.
 * @return TRUE if operation succeeded, FALSE otherwise.
 */
bool_t osClearEventBits(OsEvent *event, uint32_t mask);

/**
 * @brief Set bits from ISR context.
 *        Same as osSetEventBits but safe to call from ISR if RTOS allows.
 * @param event Pointer to OsEvent.
 * @param mask Bitmask indicating which bits to set.
 * @return TRUE if operation succeeded, FALSE otherwise.
 */
bool_t osSetEventBitsFromIsr(OsEvent *event, uint32_t mask);

/**
 * @brief Wait for specific bits in the event group.
 * @param event Pointer to OsEvent.
 * @param mask Bitmask of bits to wait on.
 * @param waitAll If TRUE, wait until all bits in mask are set. If FALSE, wait until any bit in mask is set.
 * @param clearOnExit If TRUE, clear the bits specified in mask when the wait condition is met. If FALSE, leave bits intact.
 * @param timeout Timeout in systime_t (e.g., milliseconds or ticks). Use OS_WAIT_FOREVER to block indefinitely.
 * @return TRUE if condition met within timeout, FALSE on timeout or error.
 */
bool_t osWaitForEventBits(OsEvent *event,
                          uint32_t mask,
                          bool_t waitAll,
                          bool_t clearOnExit,
                          systime_t timeout);

uint32_t osGetEventBits(OsEvent *event, uint32_t mask);
//Semaphore management
bool_t osCreateSemaphore(OsSemaphore *semaphore, uint_t count);
void osDeleteSemaphore(OsSemaphore *semaphore);
bool_t osWaitForSemaphore(OsSemaphore *semaphore, systime_t timeout);
void osReleaseSemaphore(OsSemaphore *semaphore);

//Mutex management
bool_t osCreateMutex(OsMutex *mutex);
void osDeleteMutex(OsMutex *mutex);
void osAcquireMutex(OsMutex *mutex);
uint8_t osAcquireMutexExt(OsMutex *mutex, uint32_t timeout);
void osReleaseMutex(OsMutex *mutex);
//timer
bool_t osCreateTimer(OsTimer *timer, OsTimerCode code, void *arg, systime_t duration, systime_t type);
bool_t osStartTimer(OsTimer *timer);
bool_t osStopTimer(OsTimer *timer);
bool_t osDeleteTimer(OsTimer *timer);
bool_t osChangeTimerPeriod(OsTimer *timer, systime_t newPeriod);
//queue

//System time
systime_t osGetSystemTime(void);

//Memory management
void *osAllocMem(size_t size);
void osFreeMem(void *p);
void *osCallocMem(size_t num, size_t size);
void *osReallocMem(void *ptr, size_t new_size);

// ===== Queue Management =====
bool_t osCreateQueue(OsQueue *queue, const char *name, size_t msgSize,
                    size_t queueSize);
bool_t osDeleteQueue(OsQueue *queue);
bool_t osSendToQueue(OsQueue *queue, const void *msg, systime_t timeout);
bool_t osReceiveFromQueue(OsQueue *queue, void *msg, systime_t timeout);
bool_t osSendToQueueFromIsr(OsQueue *queue, const void *msg);
bool_t osFlushQueue(OsQueue *queue);


//C++ guard
#ifdef __cplusplus
}
#endif

#endif

#endif

