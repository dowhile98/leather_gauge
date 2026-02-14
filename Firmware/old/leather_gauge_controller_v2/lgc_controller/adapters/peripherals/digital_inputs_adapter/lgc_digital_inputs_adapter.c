/**
 * @file    lgc_digital_inputs_adapter.c
 * @brief   Digital Inputs Adapter - Implementation
 * @author  Clean Architecture Refactor Team
 * @date    2026-02-13
 * @version 1.0.0
 *
 * Copyright (c) 2026 Leather Gauge Controller Team
 */

/* ============================= Includes ============================= */
#include "lgc_digital_inputs_adapter.h"
#include "main.h"
#include <string.h>

/* ============================= Static Variables ===================== */
static LgcDigitalInputsAdapter_t *s_active_adapter = NULL;

/* ============================= Private Prototypes =================== */
static uint8_t lwbtn_get_state_callback(struct lwbtn *lwobj, struct lwbtn_btn *btn);
static void lwbtn_evt_callback(struct lwbtn *lwobj, struct lwbtn_btn *btn, lwbtn_evt_t evt);
static void digital_inputs_task_entry(void *arg);

/* V-Table Implementation */
static Result_t di_init_impl(void *ctx, const LgcDigitalInputConfig_t *config);
static bool di_get_state_impl(void *ctx, LgcDigitalInputId_t input_id);
static Result_t di_register_callback_impl(void *ctx, LgcDigitalInputCallback_t callback, void *user_ctx);
static Result_t di_deinit_impl(void *ctx);

/* ============================= Private Functions ==================== */

/**
 * @brief lwbtn callback to get physical input state
 */
static uint8_t lwbtn_get_state_callback(struct lwbtn *lwobj, struct lwbtn_btn *btn)
{
    (void)lwobj;
    if (s_active_adapter == NULL) return 0;

    LgcDigitalInputId_t id = (LgcDigitalInputId_t)(uintptr_t)btn->arg;
    uint8_t state = 0;

    switch (id)
    {
    case LGC_ID_START_STOP:
        state = (HAL_GPIO_ReadPin(DI_2_GPIO_Port, DI_2_Pin) == GPIO_PIN_SET) ? 1 : 0;
        break;
    case LGC_ID_GUARD:
        /* Legacy: Inverted logic for guard */
        state = (HAL_GPIO_ReadPin(DI_3_GPIO_Port, DI_3_Pin) == GPIO_PIN_SET) ? 0 : 1;
        break;
    case LGC_ID_SPEEDS:
        state = (HAL_GPIO_ReadPin(DI_4_GPIO_Port, DI_4_Pin) == GPIO_PIN_SET) ? 1 : 0;
        break;
    case LGC_ID_FEEDBACK:
        state = (HAL_GPIO_ReadPin(DI_5_GPIO_Port, DI_5_Pin) == GPIO_PIN_SET) ? 1 : 0;
        break;
    default:
        state = 0;
        break;
    }

    return state;
}

/**
 * @brief lwbtn callback for events
 */
static void lwbtn_evt_callback(struct lwbtn *lwobj, struct lwbtn_btn *btn, lwbtn_evt_t evt)
{
    (void)lwobj;
    if (s_active_adapter == NULL || s_active_adapter->callback == NULL) return;

    LgcDigitalInputId_t id = (LgcDigitalInputId_t)(uintptr_t)btn->arg;
    
    /* Map lwbtn event to domain event */
    LgcDigitalInputEvent_t domain_evt;
    switch (evt)
    {
    case LWBTN_EVT_PRESSED:      domain_evt = LGC_INPUT_EVT_PRESSED; break;
    case LWBTN_EVT_RELEASED:     domain_evt = LGC_INPUT_EVT_RELEASED; break;
    case LWBTN_EVT_KEEPALIVE:    domain_evt = LGC_INPUT_EVT_KEEPALIVE; break;
    case LWBTN_EVT_CLICK:       domain_evt = LGC_INPUT_EVT_CLICK; break;
    case LWBTN_EVT_DBLCLICK:    domain_evt = LGC_INPUT_EVT_DOUBLE_CLICK; break;
    case LWBTN_EVT_LONGPRESS:   domain_evt = LGC_INPUT_EVT_LONG_PRESSED; break;
    default: return;
    }

    s_active_adapter->callback(id, domain_evt, s_active_adapter->user_ctx);
}

/**
 * @brief Polling task for inputs
 */
static void digital_inputs_task_entry(void *arg)
{
    LgcDigitalInputsAdapter_t *adapter = (LgcDigitalInputsAdapter_t *)arg;
    uint32_t poll_rate = adapter->config.poll_rate_ms;
    if (poll_rate == 0) poll_rate = 20;

    while (adapter->is_running)
    {
        /* Process lwbtn state machine */
        lwbtn_process(osGetSystemTime());
        
        /* Delay for next poll */
        osDelayTask(poll_rate);
    }

    osDeleteTask(OS_SELF_TASK_ID);
}

/* ============================= Interface Implementation ============= */

static Result_t di_init_impl(void *ctx, const LgcDigitalInputConfig_t *config)
{
    LgcDigitalInputsAdapter_t *adapter = (LgcDigitalInputsAdapter_t *)ctx;
    LGC_VALIDATE_PTR(adapter);
    LGC_VALIDATE_PTR(config);

    if (adapter->is_initialized) return ERR_BUSY;

    memcpy(&adapter->config, config, sizeof(LgcDigitalInputConfig_t));

    /* Initialize buttons metadata for lwbtn */
    for (int i = 0; i < LGC_ID_MAX; i++)
    {
        adapter->buttons[i].arg = (void *)(uintptr_t)i;
    }

    /* Initialize lwbtn library */
    lwbtn_init(adapter->buttons, LGC_ID_MAX, lwbtn_get_state_callback, lwbtn_evt_callback);

    adapter->is_initialized = true;
    return ERR_OK;
}

static bool di_get_state_impl(void *ctx, LgcDigitalInputId_t input_id)
{
    (void)ctx;
    if (input_id >= LGC_ID_MAX) return false;
    
    /* We can call the helper directly for raw state */
    lwbtn_btn_t tmp_btn;
    tmp_btn.arg = (void *)(uintptr_t)input_id;
    return lwbtn_get_state_callback(NULL, &tmp_btn) != 0;
}

static Result_t di_register_callback_impl(void *ctx, LgcDigitalInputCallback_t callback, void *user_ctx)
{
    LgcDigitalInputsAdapter_t *adapter = (LgcDigitalInputsAdapter_t *)ctx;
    LGC_VALIDATE_PTR(adapter);

    adapter->callback = callback;
    adapter->user_ctx = user_ctx;

    return ERR_OK;
}

static Result_t di_deinit_impl(void *ctx)
{
    LgcDigitalInputsAdapter_t *adapter = (LgcDigitalInputsAdapter_t *)ctx;
    LGC_VALIDATE_PTR(adapter);

    if (!adapter->is_initialized) return ERR_NOT_INITIALIZED;

    LgcDigitalInputsAdapter_Stop(adapter);

    adapter->is_initialized = false;
    return ERR_OK;
}

/* ============================= Public API =========================== */

Result_t LgcDigitalInputsAdapter_Init(LgcDigitalInputsAdapter_t *adapter)
{
    LGC_VALIDATE_PTR(adapter);
    memset(adapter, 0, sizeof(LgcDigitalInputsAdapter_t));
    s_active_adapter = adapter;
    return ERR_OK;
}

Result_t LgcDigitalInputsAdapter_Start(LgcDigitalInputsAdapter_t *adapter)
{
    LGC_VALIDATE_PTR(adapter);
    if (!adapter->is_initialized) return ERR_NOT_INITIALIZED;
    if (adapter->is_running) return ERR_OK;

    OsTaskParameters params = OS_TASK_DEFAULT_PARAMS;
    params.priority = LGC_DI_TASK_PRIORITY;
    params.stackSize = LGC_DI_TASK_STACK_SIZE;

    adapter->is_running = true;
    adapter->task_id = osCreateTask("DigitalInputs", digital_inputs_task_entry, adapter, &params);

    if (adapter->task_id == OS_INVALID_TASK_ID)
    {
        adapter->is_running = false;
        return ERR_HARDWARE_FAULT;
    }

    return ERR_OK;
}

Result_t LgcDigitalInputsAdapter_Stop(LgcDigitalInputsAdapter_t *adapter)
{
    LGC_VALIDATE_PTR(adapter);
    if (!adapter->is_running) return ERR_OK;

    adapter->is_running = false;
    /* Wait or delete task */
    osDeleteTask(adapter->task_id);
    adapter->task_id = NULL;

    return ERR_OK;
}

ILgcDigitalInputs_t *LgcDigitalInputsAdapter_GetInterface(LgcDigitalInputsAdapter_t *adapter)
{
    static ILgcDigitalInputs_t interface = {
        .context = NULL,
        .init = di_init_impl,
        .get_state = di_get_state_impl,
        .register_callback = di_register_callback_impl,
        .deinit = di_deinit_impl
    };

    if (adapter != NULL)
    {
        interface.context = adapter;
    }

    return &interface;
}

Result_t LgcDigitalInputsAdapter_Deinit(LgcDigitalInputsAdapter_t *adapter)
{
    return di_deinit_impl(adapter);
}
