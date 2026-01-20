/*
 * lgc_module_encoder.c
 *
 *  Created on: Jan 14, 2026
 *      Author: tecna-smart-lab
 */

#include "lgc_module_encoder.h"

static lgc_module_encoder_callback_cb_t lgc_module_encoder_callback = NULL;

error_t lgc_module_encoder_init(lgc_module_encoder_callback_cb_t callback)
{
    if (callback == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    /*register callbacks*/
    lgc_module_encoder_callback = callback;

    return NO_ERROR;
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (lgc_module_encoder_callback != NULL)
    {
        // run callback
        lgc_module_encoder_callback();
    }
}
