/**
 * @file lgc_module_rtc.h
 * @brief Real-Time Clock module with thread-safe access
 * @author tecna-smart-lab
 * @date Jan 19, 2026
 *
 * This module provides thread-safe access to the STM32 RTC peripheral.
 * All operations are protected with mutex for concurrent access from multiple tasks.
 */

#ifndef MODULES_RTC_LGC_MODULE_RTC_H_
#define MODULES_RTC_LGC_MODULE_RTC_H_

#ifdef __cplusplus
extern "C"
{
#endif

/* ============================================================================
 * INCLUDES
 * ============================================================================ */
#include "error.h"
#include <stdint.h>
#include <stdbool.h>

    /* ============================================================================
     * TYPES
     * ============================================================================ */

    /**
     * @brief RTC date and time structure
     */
    typedef struct
    {
        uint16_t year;   /**< Year (2000-2099) */
        uint8_t month;   /**< Month (1-12) */
        uint8_t day;     /**< Day (1-31) */
        uint8_t weekday; /**< Day of week (1=Monday, 7=Sunday) */
        uint8_t hours;   /**< Hours (0-23) */
        uint8_t minutes; /**< Minutes (0-59) */
        uint8_t seconds; /**< Seconds (0-59) */
    } RTC_DateTime_t;

    /**
     * @brief RTC initialization configuration
     */
    typedef struct
    {
        RTC_DateTime_t initial_datetime; /**< Initial date/time to set */
        bool use_initial_datetime;       /**< If true, set initial_datetime on init */
    } RTC_Config_t;

    /* ============================================================================
     * PUBLIC FUNCTION PROTOTYPES
     * ============================================================================ */

    /**
     * @brief Initialize RTC module
     *
     * Initializes the RTC module with thread-safe mutex protection.
     * If config->use_initial_datetime is true, sets the RTC to the specified date/time.
     *
     * @param config Configuration structure (may be NULL to skip initial time set)
     * @return error_t NO_ERROR on success, error code otherwise
     *
     * @note Must be called before any other RTC module functions
     * @note Thread-safe after initialization
     */
    error_t lgc_module_rtc_init(const RTC_Config_t *config);

    /**
     * @brief Set RTC date and time
     *
     * Updates the RTC with new date and time values.
     *
     * @param datetime Pointer to datetime structure with new values
     * @return error_t NO_ERROR on success, error code otherwise
     * @retval ERROR_INVALID_PARAMETER if datetime is NULL or contains invalid values
     * @retval ERROR_FAILURE if HAL operation fails
     *
     * @note Thread-safe, uses internal mutex
     * @note Year range: 2000-2099
     * @note Month range: 1-12
     * @note Day range: 1-31
     * @note Hours range: 0-23
     * @note Minutes range: 0-59
     * @note Seconds range: 0-59
     * @note Weekday range: 1-7 (1=Monday, 7=Sunday)
     */
    error_t lgc_module_rtc_set(const RTC_DateTime_t *datetime);

    /**
     * @brief Get current RTC date and time
     *
     * Reads the current date and time from the RTC.
     *
     * @param datetime Pointer to datetime structure to store current values
     * @return error_t NO_ERROR on success, error code otherwise
     * @retval ERROR_INVALID_PARAMETER if datetime is NULL
     * @retval ERROR_FAILURE if HAL operation fails
     *
     * @note Thread-safe, uses internal mutex
     */
    error_t lgc_module_rtc_get(RTC_DateTime_t *datetime);

    /**
     * @brief Deinitialize RTC module
     *
     * Cleans up RTC module resources.
     *
     * @return error_t NO_ERROR on success, error code otherwise
     *
     * @note After calling this, lgc_module_rtc_init() must be called again
     */
    error_t lgc_module_rtc_deinit(void);

    /**
     * @brief Check if RTC module is initialized
     *
     * @return true if initialized, false otherwise
     */
    bool lgc_module_rtc_is_initialized(void);

#ifdef __cplusplus
}
#endif

#endif /* MODULES_RTC_LGC_MODULE_RTC_H_ */
