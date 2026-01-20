/**
 * @file lgc_module_rtc.c
 * @brief Real-Time Clock module implementation
 * @author tecna-smart-lab
 * @date Jan 19, 2026
 */

/* ============================================================================
 * INCLUDES
 * ============================================================================ */
#include "lgc_module_rtc.h"
#include "os_port.h"
#include "rtc.h"
#include <string.h>

/* ============================================================================
 * DEFINES
 * ============================================================================ */
#define RTC_BASE_YEAR 2000

/* ============================================================================
 * STATIC VARIABLES
 * ============================================================================ */
static bool s_initialized = false;
static OsMutex s_mutex;

/* ============================================================================
 * PRIVATE FUNCTION PROTOTYPES
 * ============================================================================ */
static error_t lgc_rtc_validate_datetime(const RTC_DateTime_t *datetime);
static uint8_t lgc_rtc_decimal_to_bcd(uint8_t decimal);
static uint8_t lgc_rtc_bcd_to_decimal(uint8_t bcd);

/* ============================================================================
 * PUBLIC FUNCTION DEFINITIONS
 * ============================================================================ */

error_t lgc_module_rtc_init(const RTC_Config_t *config)
{
    if (s_initialized)
    {
        return NO_ERROR; // Already initialized
    }

    // Create mutex for thread-safe access
    if (osCreateMutex(&s_mutex) != TRUE)
    {
        return ERROR_FAILURE;
    }

    // Set initial date/time if requested
    if (config != NULL && config->use_initial_datetime)
    {
        error_t err = lgc_module_rtc_set(&config->initial_datetime);
        if (err != NO_ERROR)
        {
            osDeleteMutex(&s_mutex);
            return err;
        }
    }

    s_initialized = true;
    return NO_ERROR;
}

error_t lgc_module_rtc_set(const RTC_DateTime_t *datetime)
{
    if (!s_initialized)
    {
        return ERROR_FAILURE;
    }

    if (datetime == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    // Validate datetime values
    error_t err = lgc_rtc_validate_datetime(datetime);
    if (err != NO_ERROR)
    {
        return err;
    }

    // Acquire mutex for thread-safe access
    osAcquireMutex(&s_mutex);

    RTC_TimeTypeDef sTime = {0};
    RTC_DateTypeDef sDate = {0};

    // Configure time
    sTime.Hours = datetime->hours;
    sTime.Minutes = datetime->minutes;
    sTime.Seconds = datetime->seconds;
    sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    sTime.StoreOperation = RTC_STOREOPERATION_RESET;

    if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK)
    {
        osReleaseMutex(&s_mutex);
        return ERROR_FAILURE;
    }

    // Configure date
    sDate.WeekDay = datetime->weekday;
    sDate.Month = datetime->month;
    sDate.Date = datetime->day;
    sDate.Year = datetime->year - RTC_BASE_YEAR; // HAL uses offset from 2000

    if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN) != HAL_OK)
    {
        osReleaseMutex(&s_mutex);
        return ERROR_FAILURE;
    }

    osReleaseMutex(&s_mutex);
    return NO_ERROR;
}

error_t lgc_module_rtc_get(RTC_DateTime_t *datetime)
{
    if (!s_initialized)
    {
        return ERROR_FAILURE;
    }

    if (datetime == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    // Acquire mutex for thread-safe access
    osAcquireMutex(&s_mutex);

    RTC_TimeTypeDef sTime = {0};
    RTC_DateTypeDef sDate = {0};

    // Get current time
    if (HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK)
    {
        osReleaseMutex(&s_mutex);
        return ERROR_FAILURE;
    }

    // Get current date (must be called after GetTime to unlock shadow registers)
    if (HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN) != HAL_OK)
    {
        osReleaseMutex(&s_mutex);
        return ERROR_FAILURE;
    }

    // Populate output structure
    datetime->hours = sTime.Hours;
    datetime->minutes = sTime.Minutes;
    datetime->seconds = sTime.Seconds;
    datetime->weekday = sDate.WeekDay;
    datetime->month = sDate.Month;
    datetime->day = sDate.Date;
    datetime->year = sDate.Year + RTC_BASE_YEAR; // Convert to absolute year

    osReleaseMutex(&s_mutex);
    return NO_ERROR;
}

error_t lgc_module_rtc_deinit(void)
{
    if (!s_initialized)
    {
        return NO_ERROR;
    }

    osDeleteMutex(&s_mutex);
    s_initialized = false;

    return NO_ERROR;
}

bool lgc_module_rtc_is_initialized(void)
{
    return s_initialized;
}

/* ============================================================================
 * PRIVATE FUNCTION DEFINITIONS
 * ============================================================================ */

/**
 * @brief Validate datetime structure values
 * @param datetime Datetime to validate
 * @return error_t NO_ERROR if valid, ERROR_INVALID_PARAMETER otherwise
 */
static error_t lgc_rtc_validate_datetime(const RTC_DateTime_t *datetime)
{
    // Validate year (2000-2099)
    if (datetime->year < 2000 || datetime->year > 2099)
    {
        return ERROR_INVALID_PARAMETER;
    }

    // Validate month (1-12)
    if (datetime->month < 1 || datetime->month > 12)
    {
        return ERROR_INVALID_PARAMETER;
    }

    // Validate day (1-31)
    if (datetime->day < 1 || datetime->day > 31)
    {
        return ERROR_INVALID_PARAMETER;
    }

    // Validate weekday (1-7)
    if (datetime->weekday < 1 || datetime->weekday > 7)
    {
        return ERROR_INVALID_PARAMETER;
    }

    // Validate hours (0-23)
    if (datetime->hours > 23)
    {
        return ERROR_INVALID_PARAMETER;
    }

    // Validate minutes (0-59)
    if (datetime->minutes > 59)
    {
        return ERROR_INVALID_PARAMETER;
    }

    // Validate seconds (0-59)
    if (datetime->seconds > 59)
    {
        return ERROR_INVALID_PARAMETER;
    }

    return NO_ERROR;
}

/**
 * @brief Convert decimal to BCD (Binary-Coded Decimal)
 * @param decimal Decimal value (0-99)
 * @return BCD encoded value
 */
static uint8_t lgc_rtc_decimal_to_bcd(uint8_t decimal)
{
    return ((decimal / 10) << 4) | (decimal % 10);
}

/**
 * @brief Convert BCD to decimal
 * @param bcd BCD encoded value
 * @return Decimal value
 */
static uint8_t lgc_rtc_bcd_to_decimal(uint8_t bcd)
{
    return ((bcd >> 4) * 10) + (bcd & 0x0F);
}
