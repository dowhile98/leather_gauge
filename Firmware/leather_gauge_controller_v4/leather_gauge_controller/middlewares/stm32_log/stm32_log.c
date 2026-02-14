/*
 * stm32_log.c
 *
 *  Created on: Nov 11, 2023
 *      Author: eplim
 */

#include "stm32_log.h"
#include "main.h"

/**
 * @brief Static instance of the lwprintf structure
 */
static lwprintf_t debug;


static uint8_t stm32_log_out(int ch, lwprintf_t* p)
{
	if(ch == '\0')
	{
		return ch;
	}

	ITM_SendChar(ch);

	return ch;
}
/**
 * @brief Initializes the STM32 logger
 * @param out_fn The output function for the logger
 * @return 0 if successful, otherwise an error code
 */
uint8_t stm32_log_init(lwprintf_output_fn out_fn)
{

	if(out_fn == NULL)
	{
		lwprintf_init_ex(&debug,(lwprintf_output_fn) stm32_log_out);
	}
	else
	{
	    lwprintf_init_ex(&debug, out_fn);
	}

    return 0;
}
/**
 * @brief Get the current timestamp for logging
 * @return The timestamp as a uint32_t
 */
__attribute__((weak)) uint32_t stm32_log_timestamp(void) {
    // Implementa la obtención de la marca de tiempo aquí (puede ser un contador o un reloj en tiempo real)
    return HAL_GetTick() ;
}

/**
 * @brief Write a log message
 * @param level The log level
 * @param tag The tag for the log message
 * @param format The format string for the log message
 * @param ... The arguments for the format string
 */
void stm32_log_write(int level, const char* tag, const char* format, ...) {
    if (level <= STM32_LOG_LEVEL) {
        va_list args;
        va_start(args, format);
        lwprintf_vprintf_ex(&debug, format, args);
        va_end(args);
    }
    return;
}

