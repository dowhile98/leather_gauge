/*
 * stm32_log.h
 *
 *  Created on: Nov 11, 2023
 *      Author: eplim
 */

#ifndef STM32_LOG_STM32_LOG_H_
#define STM32_LOG_STM32_LOG_H_
#ifdef __cplusplus
extern "C" {
#endif
/*Includes ----------------------------------------------------------------------*/
#include <stdarg.h>		// Required for variable argument lists
#include <stdint.h>		// Required for standard integer types
#include "stm32_log_config.h"	// Include the configuration file for the STM32 logger
#include "lwprintf.h"		// Include the lightweight printf library header
/*Defines -----------------------------------------------------------------------*/
/**
 * @brief Log level definitions
 */
#define STM32_LOG_NONE      0	///< No logs are printed
#define STM32_LOG_ERROR     1	///< Only error logs are printed
#define STM32_LOG_WARN      2	///< Error and warning logs are printed
#define STM32_LOG_INFO      3	///< Error, warning, and info logs are printed
#define STM32_LOG_DEBUG     4	///< Error, warning, info, and debug logs are printed
#define STM32_LOG_VERBOSE   5	///< All logs are printed

/**
 * @brief Default log level
 *
 * This macro defines the default log level.  It can be overridden in the
 * project settings.
 */
#ifndef STM32_LOG_LEVEL
#define STM32_LOG_LEVEL    STM32_LOG_DEBUG	// Sets the desired logging level
#endif

/**
 * @brief Configuration for log colors
 *
 * This macro enables or disables colored output for the logs.
 */
#ifndef CONFIG_LOG_COLORS
#define CONFIG_LOG_COLORS	0	// Disable colored logs by default
#endif

/**
 * @brief Color definitions for logs
 *
 * These macros define the ANSI escape codes for different colors.
 */
#if (CONFIG_LOG_COLORS == 1)
#define STM32_LOG_COLOR_BLACK   "30"	///< Black color code
#define STM32_LOG_COLOR_RED     "31"	///< Red color code
#define STM32_LOG_COLOR_GREEN   "32"	///< Green color code
#define STM32_LOG_COLOR_BROWN   "33"	///< Brown color code
#define STM32_LOG_COLOR_BLUE    "34"	///< Blue color code
#define STM32_LOG_COLOR_PURPLE  "35"	///< Purple color code
#define STM32_LOG_COLOR_CYAN    "36"	///< Cyan color code
#define STM32_LOG_COLOR(COLOR)  "\033[0;" COLOR "m"	///< Apply color
#define STM32_LOG_BOLD(COLOR)   "\033[1;" COLOR "m"	///< Apply bold and color
#define STM32_LOG_RESET_COLOR   "\033[0m"	///< Reset color to default
#define STM32_LOG_COLOR_E       STM32_LOG_COLOR(STM32_LOG_COLOR_RED)	///< Error color
#define STM32_LOG_COLOR_W       STM32_LOG_COLOR(STM32_LOG_COLOR_BROWN)	///< Warning color
#define STM32_LOG_COLOR_I       STM32_LOG_COLOR(STM32_LOG_COLOR_GREEN)	///< Info color
#define STM32_LOG_COLOR_D
#define STM32_LOG_COLOR_V

#else //CONFIG_LOG_COLORS
#define STM32_LOG_COLOR_E
#define STM32_LOG_COLOR_W
#define STM32_LOG_COLOR_I
#define STM32_LOG_COLOR_D
#define STM32_LOG_COLOR_V
#define STM32_LOG_RESET_COLOR
#endif //CONFIG_LOG_COLORS


#define STM32_LOG_FORMAT(level, format)  STM32_LOG_COLOR_ ## level #level " (%u) %s: " format STM32_LOG_RESET_COLOR "\r\n"


#if STM32_LOG_LEVEL >= STM32_LOG_ERROR
#define STM32_LOGE(tag, format, ...)  {stm32_log_write(STM32_LOG_ERROR,   tag, STM32_LOG_FORMAT(E, format), stm32_log_timestamp(), tag, ##__VA_ARGS__);}
#else
#define STM32_LOGE(tag, format, ...)	{(void)tag;}
#endif

#if STM32_LOG_LEVEL >= STM32_LOG_WARN
#define STM32_LOGW(tag, format, ...)  {stm32_log_write(STM32_LOG_WARN,    tag, STM32_LOG_FORMAT(W, format), stm32_log_timestamp(), tag, ##__VA_ARGS__);}
#else
#define STM32_LOGW(tag, format, ...)	{(void)tag;}
#endif

#if STM32_LOG_LEVEL >= STM32_LOG_INFO
#define STM32_LOGI(tag, format, ...)  {stm32_log_write(STM32_LOG_INFO,    tag, STM32_LOG_FORMAT(I, format), stm32_log_timestamp(), tag, ##__VA_ARGS__);}
#else
#define STM32_LOGI(tag, format, ...)	{(void)tag;}
#endif

#if STM32_LOG_LEVEL >= STM32_LOG_DEBUG
#define STM32_LOGD(tag, format, ...)  {stm32_log_write(STM32_LOG_DEBUG,   tag, STM32_LOG_FORMAT(D, format), stm32_log_timestamp(), tag, ##__VA_ARGS__);}
#else
#define STM32_LOGD(tag, format, ...) {(void)tag;}
#endif

#if STM32_LOG_LEVEL >= STM32_LOG_VERBOSE
#define STM32_LOGV(tag, format, ...)  {stm32_log_write(STM32_LOG_VERBOSE, tag, STM32_LOG_FORMAT(V, format), stm32_log_timestamp(), tag, ##__VA_ARGS__);}
#else
#define STM32_LOGV(tag, format, ...)  {(void)tag;}
#endif

/*for debug cyclone*/


/*Function prototype -----------------------------------------------------------*/
/**
 * @brief Get the current timestamp for logging
 * @return The timestamp as a uint32_t
 */
uint32_t stm32_log_timestamp(void);

/**
 * @brief Write a log message
 * @param level The log level
 * @param tag The tag for the log message
 * @param format The format string for the log message
 * @param ... The arguments for the format string
 */
void stm32_log_write(int level, const char* tag, const char* format, ...);

/**
 * @brief Initialize the STM32 logger
 * @param out_fn The output function for the logger
 * @return 0 if successful, otherwise an error code
 */
uint8_t stm32_log_init(lwprintf_output_fn out_fn);


#ifdef __cplusplus
}
#endif
#endif /* STM32_LOG_STM32_LOG_H_ */
