/*
 * lgc_module_input.c
 *
 *  Created on: Jan 14, 2026
 *      Author: tecna-smart-lab
 */

#include "lgc_module_input.h"
#include "lwbtn.h"
#include "main.h"
/*defines*/
#ifndef LGC_INPUT_TASK_PRI
#define LGC_INPUT_TASK_PRI 9
#endif

#ifndef LGC_INPUT_TASK_STACK
#define LGC_INPUT_TASK_STACK 128
#endif
/*global variables*/
static uint32_t lgcs_input_keys[LGC_INPUT_MAX] = {
    LGC_DI_START_STOP,
    LGC_DI_GUARD,
    LGC_DI_SPEEDS,
    LGC_DI_FEEDBACK,
};
static lgc_module_input_callback_cb_t lgc_module_input_callback = NULL;
static lwbtn_btn_t inputs[LGC_INPUT_MAX] = {
    {.arg = (void *)&lgcs_input_keys[LGC_DI_START_STOP]},
    {.arg = (void *)&lgcs_input_keys[LGC_DI_GUARD]},
    {.arg = (void *)&lgcs_input_keys[LGC_DI_SPEEDS]},
    {.arg = (void *)&lgcs_input_keys[LGC_DI_FEEDBACK]},
};
OsTaskId lgc_btns_task = NULL;

/*private function prototypes   */
static uint8_t lwbtn_get_state(struct lwbtn *lwobj, struct lwbtn_btn *btn);
static void lwbtn_evt(struct lwbtn *lwobj, struct lwbtn_btn *btn, lwbtn_evt_t evt);
static void lgc_btns_task_entry(void *param);

/*public function*/

error_t lgc_module_input_init(lgc_module_input_callback_cb_t callback)
{
    OsTaskParameters params = OS_TASK_DEFAULT_PARAMS;

    if (callback == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }
    /*register callbacks*/
    lgc_module_input_callback = callback;
    /*init lwbtn*/
    lwbtn_init(inputs, LGC_INPUT_MAX, lwbtn_get_state, lwbtn_evt);


    /*create task*/
    params.priority = LGC_INPUT_TASK_PRI;
    params.stackSize = LGC_INPUT_TASK_STACK;
    lgc_btns_task = osCreateTask("LGC BTN", lgc_btns_task_entry, NULL, &params);

    if (lgc_btns_task == NULL)
    {
        return ERROR_FAILURE;
    }
    return NO_ERROR;
}

/*private function*/
static uint8_t lwbtn_get_state(struct lwbtn *lwobj, struct lwbtn_btn *btn)
{
    uint8_t state = 0;
    uint32_t di = *((uint32_t *)btn->arg);
    static uint32_t ticks = 0;

    /*initial ticks*/
    if ((HAL_GetTick() - ticks) < 1000)
    {
        return 0;
    }
    // read input state
    switch (di)
    {
    case LGC_DI_START_STOP:
        // read hardware input
        state = HAL_GPIO_ReadPin(DI_2_GPIO_Port, DI_2_Pin);
        break;
    case LGC_DI_GUARD:
        // read hardware input
        state = HAL_GPIO_ReadPin(DI_3_GPIO_Port, DI_3_Pin);
        break;
    case LGC_DI_SPEEDS:
        // read hardware input
        state = HAL_GPIO_ReadPin(DI_4_GPIO_Port, DI_4_Pin);
        break;
    case LGC_DI_FEEDBACK:
        // read hardware input
        state = HAL_GPIO_ReadPin(DI_5_GPIO_Port, DI_5_Pin);
        break;
    default:
        state = 0;
        break;
    }
    return state;
}
static void lwbtn_evt(struct lwbtn *lwobj, struct lwbtn_btn *btn, lwbtn_evt_t evt)
{
    uint32_t di = *((uint32_t *)btn->arg);
    if (lgc_module_input_callback != NULL)
    {
        // run callback
        lgc_module_input_callback(di, evt);
    }
}

static void lgc_btns_task_entry(void *param)
{

    for (;;)
    {
        // process buttons
        lwbtn_process(HAL_GetTick());
        // delay
        osDelayTask(20);
    }
}