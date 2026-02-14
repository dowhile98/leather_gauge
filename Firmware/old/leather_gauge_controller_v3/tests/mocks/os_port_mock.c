/**
 * @file os_port_mock.c
 * @brief Mock OSAL implementation for unit tests
 *
 * Provides stub implementations of OSAL functions for
 * compiling tests without RTOS dependencies.
 *
 * @date Created: Feb 13, 2026
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/* Stub type definitions */
typedef uint32_t systime_t;
typedef int bool_t;

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

/* Mock system time counter */
static uint32_t s_mock_time = 0;

/**
 * @brief Get mock system time
 */
systime_t osGetSystemTime(void)
{
    return s_mock_time;
}

/**
 * @brief Set mock system time (for test control)
 */
void mock_set_system_time(uint32_t time_ms)
{
    s_mock_time = time_ms;
}

/**
 * @brief Advance mock system time
 */
void mock_advance_time(uint32_t delta_ms)
{
    s_mock_time += delta_ms;
}
