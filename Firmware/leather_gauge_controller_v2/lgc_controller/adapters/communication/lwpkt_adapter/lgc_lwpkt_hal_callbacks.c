/**
 * @file    lgc_lwpkt_hal_callbacks.c
 * @brief   HAL UART callbacks for LwPKT Agent (ISR integration)
 * @author  Clean Architecture Refactor Team
 * @date    2026-02-12
 * @version 2.0.0
 *
 * @details This file bridges STM32 HAL UART interrupts with the LwPKT Active Object.
 *          Uses HAL_UARTEx_ReceiveToIdle_DMA() with event-based callback.
 *
 * **Integration:**
 *   ```c
 *   // In Core/Src/stm32f4xx_it.c or similar
 *   extern LgcLwPktAgent_t *g_lwpkt_agent;  // Global instance from DI Container
 *
 *   void USART2_IRQHandler(void)  // Assuming LwPKT on USART2
 *   {
 *       HAL_UART_IRQHandler(&huart2);  // HAL processes interrupt
 *       // HAL calls HAL_UARTEx_RxEventCallback() below
 *   }
 *   ```
 *
 * Copyright (c) 2026 Leather Gauge Controller Team
 */

/* ============================= Includes ============================= */
#include "lgc_lwpkt_agent.h"
#include "stm32f4xx_hal.h"

/* External instances (declared in DI Container) */
extern UART_HandleTypeDef huart2;      /* Assuming LwPKT on UART2 */
extern LgcLwPktAgent_t *g_lwpkt_agent; /* Global agent instance POINTER */

/* ============================= HAL Callbacks ======================== */
/**
 * @brief UART RX Event callback (DMA event-based mode)
 * @param[in] huart UART handle
 * @param[in] Size  Number of bytes received
 *
 * @note ISR context: Keep execution time minimal (<50μs target)
 * @note Called on: IDLE line detection OR buffer full
 * @note Automatically restarts DMA reception after callback
 */
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    if (huart == &huart2 && g_lwpkt_agent != NULL) /* LwPKT UART */
    {
        /* Write received data to ring buffer */
        LgcLwPktAgent_RxISRCallback(g_lwpkt_agent, g_lwpkt_agent->rx_buffer, Size);

        /* Restart DMA reception (event-based mode requires manual restart) */
        HAL_UARTEx_ReceiveToIdle_DMA(huart, g_lwpkt_agent->rx_buffer, sizeof(g_lwpkt_agent->rx_buffer));
    }
}

/**
 * @brief UART error callback
 * @param[in] huart UART handle
 *
 * @note Handle framing errors, overrun, etc.
 */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if (huart == &huart2 && g_lwpkt_agent != NULL) /* LwPKT UART */
    {
        /* Log error */
        uint32_t error = HAL_UART_GetError(huart);

        if (error & HAL_UART_ERROR_ORE) /* Overrun error */
        {
            __HAL_UART_CLEAR_OREFLAG(huart);
        }

        if (error & HAL_UART_ERROR_FE) /* Framing error */
        {
            __HAL_UART_CLEAR_FEFLAG(huart);
        }

        /* Restart DMA reception if stopped */
        if (huart->RxState == HAL_UART_STATE_READY)
        {
            HAL_UARTEx_ReceiveToIdle_DMA(huart, g_lwpkt_agent->rx_buffer, sizeof(g_lwpkt_agent->rx_buffer));
        }

        /* Increment error counter */
        g_lwpkt_agent->error_count++;
    }
}
