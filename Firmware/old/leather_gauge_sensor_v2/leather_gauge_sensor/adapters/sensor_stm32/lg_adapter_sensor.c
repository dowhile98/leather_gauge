#include "lg_adapter_sensor.h"
#include "adc.h"
#include "tim.h"
#include "DSP_Biquad.h"
#include "leather_gauge_config.h"
#include <string.h>

/* ============================================================================
 * Private Macros & Constants
 * ========================================================================= */
#define LG_ADC_CHANNELS 10
#define LG_ADC_FS 100.0f /* Sampling Frequency */
#define HYSTERESIS 4     /* Digital Hysteresis */

/* ============================================================================
 * Private Data Types
 * ========================================================================= */
typedef struct
{
    uint16_t dma_buffer[LG_ADC_CHANNELS]; /* DMA Target Buffer */
    Biquad_t filters[LG_ADC_CHANNELS];    /* Filter Instances */
    lg_sensor_data_t current_data;        /* Latest Processed Data */
    uint16_t threshold;                   /* Digital Threshold */
    float offsets[LG_ADC_CHANNELS];       /* Calibration Offsets */
    bool data_ready;                      /* New data flag */
} sensor_context_t;

/* ============================================================================
 * Private Variables
 * ========================================================================= */
static sensor_context_t ctx;

/* ============================================================================
 * Private Function Prototypes
 * ========================================================================= */
static lg_result_t sensor_init(float fc);
static lg_result_t sensor_set_filter(float fc);
static lg_result_t sensor_get_data(lg_sensor_data_t *data);
static lg_result_t sensor_trigger(void);
static lg_result_t sensor_process(void);

/* ============================================================================
 * Interface Definition
 * ========================================================================= */
static const lg_i_sensor_t interface = {
    .init = sensor_init,
    .set_filter = sensor_set_filter,
    .get_data = sensor_get_data,
    .trigger = sensor_trigger,
    .process = sensor_process};

/* ============================================================================
 * Public Functions
 * ========================================================================= */
const lg_i_sensor_t *lg_adapter_sensor_get_interface(void)
{
    return &interface;
}

/* ============================================================================
 * Private Functions (Implementation)
 * ========================================================================= */

static lg_result_t sensor_init(float fc)
{
    // 1. Initialize Filters
    for (int i = 0; i < LG_ADC_CHANNELS; i++)
    {
        Biquad_Init(&ctx.filters[i], BQ_LOWPASS, fc, LG_ADC_FS, 0.707f);
        ctx.offsets[i] = 0.0f; // Default offset
    }

    // 2. Start ADC Calibration
    if (HAL_ADCEx_Calibration_Start(&hadc1) != HAL_OK)
    {
        return LG_ERROR;
    }

    // 3. Start ADC DMA
    if (HAL_ADC_Start_DMA(&hadc1, (uint32_t *)ctx.dma_buffer, LG_ADC_CHANNELS) != HAL_OK)
    {
        return LG_ERROR;
    }

    // 4. Start Timer Trigger
    if (HAL_TIM_Base_Start(&htim3) != HAL_OK)
    {
        return LG_ERROR;
    }

    return LG_OK;
}

static lg_result_t sensor_set_filter(float fc)
{
    for (int i = 0; i < LG_ADC_CHANNELS; i++)
    {
        Biquad_Reset(&ctx.filters[i]);
        Biquad_Init(&ctx.filters[i], BQ_LOWPASS, fc, LG_ADC_FS, 0.707f);
    }
    return LG_OK;
}

static lg_result_t sensor_get_data(lg_sensor_data_t *data)
{
    if (!data)
        return LG_INVALID_PARAM;

    // Copy safely (could disable IRQ here if needed for atomicity)
    memcpy(data, &ctx.current_data, sizeof(lg_sensor_data_t));
    return LG_OK;
}

static lg_result_t sensor_trigger(void)
{
    // Not needed for Timer-triggered ADC, but kept for interface compliance
    return LG_OK;
}

static lg_result_t sensor_process(void)
{
    // Logic moved from ISR to here?
    // Actually, for high-speed sampling, filtering often needs to stay in ISR
    // or be buffered. The original code did it in ISR.
    // Clean Architecture allows the Adapter to handle this detail.
    // If we want to keep the loop fast, we can process a flag here.

    if (ctx.data_ready)
    {
        ctx.data_ready = false;
        return LG_OK;
    }
    return LG_BUSY; // No new data
}

/* ============================================================================
 * HAL Callbacks
 * ========================================================================= */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    if (hadc->Instance == ADC1)
    {
        // Process all channels
        for (int i = 0; i < LG_ADC_CHANNELS; i++)
        {
            // 1. Raw Data
            ctx.current_data.raw[i] = ctx.dma_buffer[i];

            // 2. Filter
            float filtered = Biquad_Apply(&ctx.filters[i], (float)ctx.dma_buffer[i]);
            if (filtered < 0.0f)
                filtered = 0.0f;
            ctx.current_data.filtered[i] = filtered;

            // 3. Calibration (Offset)
            float calibrated = filtered - ctx.offsets[i];
            if (calibrated < 0.0f)
                calibrated = 0.0f;
            ctx.current_data.calibrated[i] = calibrated;

            // 4. Thresholding (Digital State)
            if (calibrated <= ctx.threshold)
            {
                ctx.current_data.digital_state |= (1 << i);
            }
            else if (calibrated >= (ctx.threshold + HYSTERESIS))
            {
                ctx.current_data.digital_state &= ~(1 << i);
            }
        }
        ctx.data_ready = true;
    }
}
