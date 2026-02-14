/**
 * @file    lgc_encoder_adapter.h
 * @brief   STM32 Encoder Adapter - Interface
 * @author  Clean Architecture Refactor Team (TDD)
 * @date    2026-02-12
 * @version 1.0.0
 *
 * @details Implements ILgcEncoder_t using STM32 GPIO EXTI interrupts.
 *          Critical timing: ISR must signal event within <500µs.
 *
 * @note    ADAPTER LAYER - Can include HAL headers
 *
 * Copyright (c) 2026 Leather Gauge Controller Team
 */

#ifndef LGC_ENCODER_ADAPTER_H
#define LGC_ENCODER_ADAPTER_H

#ifdef __cplusplus
extern "C"
{
#endif

/* ============================= Includes ============================= */
#include "../../../domain/interfaces/lgc_i_encoder.h"

    /* ============================= Public API =========================== */
    /**
     * @brief Get Encoder Adapter Interface
     *
     * @return const ILgcEncoder_t* Pointer to singleton interface instance
     *
     * @note  Returns static instance - no dynamic allocation
     * @note  Context is bound to STM32 HAL GPIO (GPIOA, specific pin)
     *
     * @usage
     * @code
     * // In DI Container:
     * ILgcEncoder_t *encoder = LgcEncoderAdapter_GetInterface();
     * Result_t res = encoder->init(encoder->context, &config);
     * @endcode
     */
    const ILgcEncoder_t *LgcEncoderAdapter_GetInterface(void);

#ifdef __cplusplus
}
#endif

#endif /* LGC_ENCODER_ADAPTER_H */
