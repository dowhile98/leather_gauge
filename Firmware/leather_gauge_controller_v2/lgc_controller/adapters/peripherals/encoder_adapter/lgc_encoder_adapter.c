/**
 * @file    lgc_encoder_adapter.c
 * @brief   STM32 Encoder Adapter - Implementation
 * @author  Clean Architecture Refactor Team (TDD)
 * @date    2026-02-12
 * @version 1.0.0
 *
 * @details GPIO EXTI-based encoder implementation.
 *          ISR latency: <500µs critical for measurement sync.
 *
 * Copyright (c) 2026 Leather Gauge Controller Team
 */

/* ============================= Includes ============================= */
#include "lgc_encoder_adapter.h"
#include "stm32f4xx_hal.h" /* HAL allowed ONLY in adapters */
#include "gpio.h"          /* MX-generated GPIO init */
#include <string.h>

/* ============================= Configuration ======================== */
/**
 * @brief Encoder GPIO Configuration (hardware-specific)
 * @note  Change if using different pin - MUST match MX configuration
 */
#define ENCODER_GPIO_PORT GPIOA
#define ENCODER_GPIO_PIN GPIO_PIN_0
#define ENCODER_EXTI_LINE EXTI_Line0

/* ============================= Private Types ======================== */
/**
 * @brief Encoder adapter context (opaque to domain)
 */
typedef struct
{
    /* Configuration */
    LgcEncoderConfig_t config;

    /* State */
    volatile uint32_t position; /**< Current position (pulses) - ISR updates */
    bool is_initialized;

    /* Callback */
    LgcEncoderPulseCallback_t callback; /**< User callback (ISR context) */
    void *callback_user_ctx;            /**< User context for callback */

    /* Pulse Accumulator (Legacy: Reduce measurement frequency) */
    volatile uint8_t pulse_accumulator; /**< Accumulate 5 pulses before callback */

    /* Debounce */
    uint32_t last_pulse_tick; /**< HAL_GetTick() of last valid pulse */

} EncoderAdapterContext_t;

/* ============================= Private Variables ==================== */
/**
 * @brief Static singleton context (zero dynamic allocation)
 */
static EncoderAdapterContext_t s_encoder_ctx = {0};

/* ============================= Private Function Prototypes ========== */
static Result_t encoder_init(void *ctx, const LgcEncoderConfig_t *config);
static Result_t encoder_get_position(void *ctx, uint32_t *out_position);
static Result_t encoder_reset_position(void *ctx);
static Result_t encoder_attach_callback(void *ctx, LgcEncoderPulseCallback_t callback, void *user_ctx);
static Result_t encoder_detach_callback(void *ctx);
static Result_t encoder_deinit(void *ctx);

/* ============================= Interface Definition ================= */
/**
 * @brief Static interface V-Table (singleton)
 */
static const ILgcEncoder_t s_encoder_interface = {
    .context = &s_encoder_ctx,
    .init = encoder_init,
    .get_position = encoder_get_position,
    .reset_position = encoder_reset_position,
    .attach_callback = encoder_attach_callback,
    .detach_callback = encoder_detach_callback,
    .deinit = encoder_deinit};

/* ============================= Public API =========================== */

const ILgcEncoder_t *LgcEncoderAdapter_GetInterface(void)
{
    return &s_encoder_interface;
}

/* ============================= Private Functions ==================== */

/**
 * @brief Initialize encoder adapter
 */
static Result_t encoder_init(void *ctx, const LgcEncoderConfig_t *config)
{
    if (ctx == NULL || config == NULL)
    {
        return ERR_NULL_POINTER;
    }

    EncoderAdapterContext_t *encoder = (EncoderAdapterContext_t *)ctx;

    if (encoder->is_initialized)
    {
        return ERR_BUSY; /* Already initialized */
    }

    /* Copy configuration */
    memcpy(&encoder->config, config, sizeof(LgcEncoderConfig_t));

    /* Reset state */
    encoder->position = 0;
    encoder->callback = NULL;
    encoder->callback_user_ctx = NULL;
    encoder->last_pulse_tick = 0;
    encoder->pulse_accumulator = 0; /* Reset accumulator */

    /* GPIO initialization done by MX_GPIO_Init() in main.c */
    /* EXTI interrupt configured in MX (rising edge, NVIC enabled) */

    encoder->is_initialized = true;
    return ERR_OK;
}

/**
 * @brief Get current encoder position (thread-safe)
 */
static Result_t encoder_get_position(void *ctx, uint32_t *out_position)
{
    if (ctx == NULL || out_position == NULL)
    {
        return ERR_NULL_POINTER;
    }

    EncoderAdapterContext_t *encoder = (EncoderAdapterContext_t *)ctx;

    if (!encoder->is_initialized)
    {
        return ERR_NOT_INITIALIZED;
    }

    /* Atomic read on Cortex-M4 (32-bit aligned, single instruction) */
    *out_position = encoder->position;

    return ERR_OK;
}

/**
 * @brief Reset encoder position to zero
 */
static Result_t encoder_reset_position(void *ctx)
{
    if (ctx == NULL)
    {
        return ERR_NULL_POINTER;
    }

    EncoderAdapterContext_t *encoder = (EncoderAdapterContext_t *)ctx;

    if (!encoder->is_initialized)
    {
        return ERR_NOT_INITIALIZED;
    }

    /* Atomic write (disable IRQ if multi-byte, but uint32_t is safe) */
    encoder->position = 0;

    return ERR_OK;
}

/**
 * @brief Attach pulse callback
 */
static Result_t encoder_attach_callback(
    void *ctx,
    LgcEncoderPulseCallback_t callback,
    void *user_ctx)
{
    if (ctx == NULL)
    {
        return ERR_NULL_POINTER;
    }

    EncoderAdapterContext_t *encoder = (EncoderAdapterContext_t *)ctx;

    if (!encoder->is_initialized)
    {
        return ERR_NOT_INITIALIZED;
    }

    /* Allow NULL callback (detach) */
    encoder->callback = callback;
    encoder->callback_user_ctx = user_ctx;

    return ERR_OK;
}

/**
 * @brief Detach pulse callback
 */
static Result_t encoder_detach_callback(void *ctx)
{
    if (ctx == NULL)
    {
        return ERR_NULL_POINTER;
    }

    EncoderAdapterContext_t *encoder = (EncoderAdapterContext_t *)ctx;

    encoder->callback = NULL;
    encoder->callback_user_ctx = NULL;

    return ERR_OK;
}

/**
 * @brief Deinitialize encoder
 */
static Result_t encoder_deinit(void *ctx)
{
    if (ctx == NULL)
    {
        return ERR_NULL_POINTER;
    }

    EncoderAdapterContext_t *encoder = (EncoderAdapterContext_t *)ctx;

    /* Detach callback */
    encoder->callback = NULL;
    encoder->callback_user_ctx = NULL;

    /* Reset state */
    encoder->position = 0;
    encoder->is_initialized = false;

    return ERR_OK;
}

/* ============================= HAL Callback (ISR Context) =========== */
/**
 * @brief HAL GPIO EXTI callback - Called by HAL on interrupt
 *
 * @param[in] GPIO_Pin Pin that triggered interrupt (bitmask)
 *
 * @note  ISR context - MUST be fast (<500µs)
 * @note  This function is called by stm32f4xx_it.c HAL_GPIO_EXTI_IRQHandler
 *
 * @warning Do NOT call from user code - HAL use only
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    /* Check if our encoder pin triggered the interrupt */
    if (GPIO_Pin != ENCODER_GPIO_PIN)
    {
        return; /* Not our pin, ignore */
    }

    EncoderAdapterContext_t *encoder = &s_encoder_ctx;

    if (!encoder->is_initialized)
    {
        return; /* Not initialized, ignore spurious interrupt */
    }

    /* Debouncing: Check if enough time passed since last pulse */
    if (encoder->config.debounce_ms > 0)
    {
        uint32_t now = HAL_GetTick();
        if ((now - encoder->last_pulse_tick) < encoder->config.debounce_ms)
        {
            return; /* Too soon, ignore (debounce) */
        }
        encoder->last_pulse_tick = now;
    }

    /* Increment position counter (atomic on Cortex-M4) */
    encoder->position++;

    /* ===== Pulse Accumulator (Legacy: 5 pulses = 1 measurement) ===== */
    encoder->pulse_accumulator++;

    if (encoder->pulse_accumulator >= LGC_ENCODER_PULSES_PER_FLAG)
    {
        /* Reset accumulator */
        encoder->pulse_accumulator = 0;

        /* Call user callback if attached (only every 5 pulses) */
        if (encoder->callback != NULL)
        {
            encoder->callback(encoder->position, encoder->callback_user_ctx);
        }
    }
}
