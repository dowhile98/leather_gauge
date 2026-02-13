/**
 * @file    lgc_digital_inputs_adapter.h
 * @brief   Digital Inputs Adapter - Interface
 * @author  Clean Architecture Refactor Team
 * @date    2026-02-13
 * @version 1.0.0
 *
 * @details Implements ILgcDigitalInputs_t using lwbtn library.
 *          Manages an internal task for input polling and debounce.
 *
 * Copyright (c) 2026 Leather Gauge Controller Team
 */

#ifndef LGC_DIGITAL_INPUTS_ADAPTER_H
#define LGC_DIGITAL_INPUTS_ADAPTER_H

#ifdef __cplusplus
extern "C"
{
#endif

/* ============================= Includes ============================= */
#include "../../domain/interfaces/lgc_i_digital_inputs.h"
#include "../../Third_Party/lwrb/src/include/lwrb/lwrb.h"
#include "../../leather_gauge_controller/middlewares/lwbtn/lwbtn.h"
#include "os_port.h"
#include "stm32f4xx_hal.h"

    /* ============================= Constants ============================ */
#ifndef LGC_DI_TASK_STACK_SIZE
#define LGC_DI_TASK_STACK_SIZE 256U
#endif

#ifndef LGC_DI_TASK_PRIORITY
#define LGC_DI_TASK_PRIORITY 9U
#endif

    /* ============================= Types ================================ */
    /**
     * @brief Digital Inputs Adapter Context
     */
    typedef struct
    {
        /* lwbtn resources */
        lwbtn_btn_t buttons[LGC_ID_MAX];
        
        /* OS Resources (Active Object) */
        OsTaskId task_id;
        uint32_t task_stack[LGC_DI_TASK_STACK_SIZE];
        
        /* Callbacks */
        LgcDigitalInputCallback_t callback;
        void *user_ctx;

        /* Config */
        LgcDigitalInputConfig_t config;

        /* State */
        bool is_initialized;
        bool is_running;

    } LgcDigitalInputsAdapter_t;

    /* ============================= Public API =========================== */
    /**
     * @brief Initialize digital inputs adapter
     * @param[in,out] adapter Pointer to adapter context
     * @return ERR_OK on success
     */
    Result_t LgcDigitalInputsAdapter_Init(LgcDigitalInputsAdapter_t *adapter);

    /**
     * @brief Start internal polling task (Active Object)
     * @param[in,out] adapter Pointer to adapter context
     * @return ERR_OK on success
     */
    Result_t LgcDigitalInputsAdapter_Start(LgcDigitalInputsAdapter_t *adapter);

    /**
     * @brief Stop internal polling task
     * @param[in,out] adapter Pointer to adapter context
     * @return ERR_OK on success
     */
    Result_t LgcDigitalInputsAdapter_Stop(LgcDigitalInputsAdapter_t *adapter);

    /**
     * @brief Get digital inputs interface (V-Table)
     * @param[in] adapter Pointer to adapter context
     * @return Pointer to ILgcDigitalInputs_t interface
     */
    ILgcDigitalInputs_t *LgcDigitalInputsAdapter_GetInterface(LgcDigitalInputsAdapter_t *adapter);

    /**
     * @brief Deinitialize digital inputs adapter
     * @param[in,out] adapter Pointer to adapter context
     * @return ERR_OK on success
     */
    Result_t LgcDigitalInputsAdapter_Deinit(LgcDigitalInputsAdapter_t *adapter);

#ifdef __cplusplus
}
#endif

#endif /* LGC_DIGITAL_INPUTS_ADAPTER_H */
