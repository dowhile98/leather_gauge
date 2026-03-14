/* ============================================================================
 * Includes
 * ========================================================================= */
#include "lg_module_sensor.h"
#include "adc.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"
#include "leather_gauge.h"
#include "lg_module_modbus.h"
#include "lg_module_eeprom.h"

/* TEST PATCH removed: normal STREAM frame bytes are used. */

/* ============================================================================
 * global variables
 * ========================================================================= */
#ifndef LG_ADC_FS
#define LG_ADC_FS 100.0f
#endif

#define DELAY_X 5500

/* ============================================================================
 * global variables
 * ========================================================================= */
static LG_SENSOR_TypeDef_t sensor = {0};
static Biquad_t filter[LG_ADC_SENAOR_MAX_SIZE] = {0};
static uint8_t stream_frame[3];
volatile uint8_t tx_flag = 0;

/* ============================================================================
 * public function definition
 * ========================================================================= */
uint8_t lg_module_sensor_init(float fc)
{
    uint8_t ret = 0;

    /* filter init */
    for (uint8_t i = 0; i < LG_ADC_SENAOR_MAX_SIZE; i++)
    {
        Biquad_Init(&filter[i], BQ_LOWPASS, fc, LG_ADC_FS, 0.707f);
    }

    /* adc init */
    if (HAL_ADCEx_Calibration_Start(&hadc1) != HAL_OK)
    {
        return 1;
    }

    if (HAL_ADC_Start_DMA(&hadc1, (uint32_t *)sensor.raw, LG_ADC_SENAOR_MAX_SIZE) != HAL_OK)
    {
        return 1;
    }

    /* start timer trigger */
    if (HAL_TIM_Base_Start(&htim3) != HAL_OK)
    {
        return 1;
    }

    return ret;
}

uint8_t lg_module_sensor_filter_set(float fc)
{
    for (uint8_t i = 0; i < LG_ADC_SENAOR_MAX_SIZE; i++)
    {
        /* reset filter */
        Biquad_Reset(&filter[i]);

        /* init */
        Biquad_Init(&filter[i], BQ_LOWPASS, fc, LG_ADC_FS, 0.707f);
    }

    return 0;
}

uint8_t lg_module_sensor_get(LG_SENSOR_TypeDef_t *out)
{
    memcpy(out, &sensor, sizeof(LG_SENSOR_TypeDef_t));
    return 0;
}

/* ============================================================================
 * private function definition
 * ========================================================================= */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    LG_CONF_TypeDef_t conf = {0};

    /* Get configuration */
    lg_module_eeprom_conf_get(&conf);

    /* filter data */
    for (uint8_t i = 0; i < LG_ADC_SENAOR_MAX_SIZE; i++)
    {
        /* apply filter */
        sensor.S[i] = Biquad_Apply(&filter[i], sensor.raw[i]);
        sensor.S[i] = (sensor.S[i] < 0) ? 0 : sensor.S[i];

        /* apply offset */
        sensor.D[i] = sensor.S[i] - conf.offset[i];
        sensor.D[i] = (sensor.D[i] < 0) ? 0 : sensor.D[i];

        /* detect */
        if (sensor.D[i] <= conf.threshold)
        {
            sensor.value |= (1 << i); // set bit
        }
        else if (sensor.D[i] >= (conf.threshold + LB_THESHOLD_HYSTERESIS))
        {
            sensor.value &= ~(1 << i); // clear bit
        }
    }

    return;
}

void HAL_GPIO_EXTI_Rising_Callback(uint16_t GPIO_Pin)
{
	//static uint32_t ticks = 0;

	if(/*((HAL_GetTick() - ticks)>=1) && */(GPIO_Pin == DIR_INPUT_Pin))//
    {
		//ticks = HAL_GetTick();
        if (tx_flag == 0)
        {
            lg_sensor_set_mode(LG_MODE_STREAM);

            // synchronization delay
            for (volatile uint32_t i = 0; i < DELAY_X; i++);

//            USART1->ISR & USART_ISR_IDLE

            LG_CONF_TypeDef_t conf;
            lg_module_eeprom_conf_get(&conf);

            stream_frame[0] = conf.address;
            stream_frame[1] = (uint8_t)((sensor.value >> 8) & 0xFF);
            stream_frame[2] = (uint8_t)(sensor.value & 0xFF);

            HAL_GPIO_WritePin(DIR_GPIO_Port, DIR_Pin, GPIO_PIN_SET);

            for (volatile uint32_t i = 0; i < 500; i++);

            HAL_StatusTypeDef st = HAL_UART_Transmit_DMA(&huart1, stream_frame, 3);
            // If the transmission was miss
            if (st != HAL_OK) {
            	/* Relesase RS485 bus */
                HAL_GPIO_WritePin(DIR_GPIO_Port, DIR_Pin, GPIO_PIN_RESET);
                /* Send afterwards a trigger pulse */
                for (volatile uint32_t i = 0; i < DELAY_X; i++);
                HAL_GPIO_WritePin(DIR_OUTPUT_GPIO_Port, DIR_OUTPUT_Pin, GPIO_PIN_SET);
                for (volatile uint32_t i = 0; i < 500; i++);
                HAL_GPIO_WritePin(DIR_OUTPUT_GPIO_Port, DIR_OUTPUT_Pin, GPIO_PIN_RESET);

                tx_flag = 0;
                return;
            }
        }
    }
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
        if (lg_sensor_get_mode() == LG_MODE_STREAM)
        {
            /* wait until last bit is sent */
        	uint32_t t0 = HAL_GetTick();
        	while (__HAL_UART_GET_FLAG(&huart1, UART_FLAG_TC) == RESET)
        	{
        		// If the last bit wasn't send in 2ms, exit loop
        	    if ((HAL_GetTick() - t0) > 2)
        	    {
        	        break;
        	    }
        	}
            /* Release RS485 bus */
            HAL_GPIO_WritePin(DIR_GPIO_Port, DIR_Pin, GPIO_PIN_RESET);

            for (volatile uint32_t i = 0; i < DELAY_X; i++);

            /* now generate Trigger Out pulse */
            HAL_GPIO_WritePin(DIR_OUTPUT_GPIO_Port, DIR_OUTPUT_Pin, GPIO_PIN_SET);

            for (volatile uint32_t i = 0; i < 500; i++);

            HAL_GPIO_WritePin(DIR_OUTPUT_GPIO_Port, DIR_OUTPUT_Pin, GPIO_PIN_RESET);

            tx_flag = 0;
        }
    }
}
