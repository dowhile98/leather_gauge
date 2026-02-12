/**
 * @file    lg_adapter_comm.c
 * @brief   LwPKT Communication Adapter Implementation
 * @author  TDD Agent (c-pro mode)
 * @date    2026-02-09
 *
 * @note    Implements lg_i_comm.h interface using LwPKT library.
 *          Handles RS-485 half-duplex with DE pin control.
 *          Uses DMA for efficient UART RX, IT for TX.
 */

#include "lg_adapter_comm.h"
#include "lg_lwpkt_codec.h"
#include "lwpkt/lwpkt.h"
#include "lwrb/lwrb.h"
#include "usart.h"
#include "gpio.h"
#include "main.h"
#include <string.h>

/* ============================================================================
 * Private Macros & Constants
 * ========================================================================= */
#define COMM_RX_BUFFER_SIZE 256
#define COMM_TX_BUFFER_SIZE 256
#define COMM_DMA_BUFFER_SIZE 64
#define MASTER_ADDRESS 0xFF /**< Master device address (broadcast) */

/* ============================================================================
 * Private Data Types
 * ========================================================================= */
typedef struct
{
    lwpkt_t pkt;
    lwrb_t tx_rb;
    lwrb_t rx_rb;
    uint8_t tx_rb_data[COMM_TX_BUFFER_SIZE];
    uint8_t rx_rb_data[COMM_RX_BUFFER_SIZE];
    uint8_t dma_rx_buffer[COMM_DMA_BUFFER_SIZE];
    uint32_t baudrate;
    uint8_t device_address;   /**< This device's address (1-11) */
    uint8_t last_sender_addr; /**< Address of last packet sender */
    bool packet_ready;
    ILwPktCodec_t *codec; /**< Codec interface (DIP) */
} comm_context_t;

/* ============================================================================
 * Private Variables (Static Allocation)
 * ========================================================================= */
static comm_context_t s_ctx;

/* ============================================================================
 * Private Function Prototypes
 * ========================================================================= */
static lg_result_t comm_init(uint8_t address, uint32_t baudrate);
static lg_result_t comm_process(void);
static lg_result_t comm_read(lg_comm_packet_t *packet);
static lg_result_t comm_send(uint8_t cmd, const void *data, uint16_t len);
static lg_result_t comm_send_with_flags(uint8_t cmd, uint32_t flags, const void *data, uint16_t len);
static lg_result_t comm_set_address(uint8_t address);
static void lwpkt_event_callback(lwpkt_t *pkt, lwpkt_evt_type_t evt_type);

/* ============================================================================
 * Interface Definition (Static Singleton)
 * ========================================================================= */
static const lg_i_comm_t s_interface = {
    .init = comm_init,
    .process = comm_process,
    .read = comm_read,
    .send = comm_send,
    .send_with_flags = comm_send_with_flags, // 🆕 Expose FLAGS variant
    .set_address = comm_set_address};

/* ============================================================================
 * Public Functions
 * ========================================================================= */

const lg_i_comm_t *lg_adapter_comm_get_interface(void)
{
    return &s_interface;
}

/* ============================================================================
 * Private Functions (Implementation)
 * ========================================================================= */

/**
 * @brief Initialize communication adapter with LwPKT protocol.
 */
static lg_result_t comm_init(uint8_t address, uint32_t baudrate)
{
    // 0. Get codec interface (DIP - Dependency Inversion)
    s_ctx.codec = LgLwPktCodec_GetInterface();
    if (s_ctx.codec == NULL)
    {
        return LG_ERROR;
    }

    // 1. Initialize ring buffers (static allocation)
    lwrb_init(&s_ctx.tx_rb, s_ctx.tx_rb_data, sizeof(s_ctx.tx_rb_data));
    lwrb_init(&s_ctx.rx_rb, s_ctx.rx_rb_data, sizeof(s_ctx.rx_rb_data));

    // 2. Initialize LwPKT protocol handler
    lwpkt_init(&s_ctx.pkt, &s_ctx.tx_rb, &s_ctx.rx_rb);
    lwpkt_set_addr(&s_ctx.pkt, address);
    lwpkt_set_evt_fn(&s_ctx.pkt, lwpkt_event_callback);

    // 3. Configure UART baudrate (assumes HAL peripheral initialized by MX)
    huart1.Init.BaudRate = baudrate;
    if (HAL_UART_Init(&huart1) != HAL_OK)
    {
        return LG_ERROR;
    }

    // 4. Start RX DMA with idle line detection
    HAL_GPIO_WritePin(DIR_GPIO_Port, DIR_Pin, GPIO_PIN_RESET); // RS-485 RX mode
    if (HAL_UARTEx_ReceiveToIdle_DMA(&huart1, s_ctx.dma_rx_buffer, COMM_DMA_BUFFER_SIZE) != HAL_OK)
    {
        return LG_ERROR;
    }

    // 5. Store configuration
    s_ctx.baudrate = baudrate;
    s_ctx.device_address = address;
    s_ctx.last_sender_addr = MASTER_ADDRESS; // Default reply to master
    s_ctx.packet_ready = false;

    return LG_OK;
}

/**
 * @brief Process LwPKT protocol events (call from main loop).
 */
static lg_result_t comm_process(void)
{
    // Process LwPKT internal state (timeouts, retries, etc.)
    lwpkt_process(&s_ctx.pkt, HAL_GetTick());

    // Check for new packets (only if previous consumed)
    if (!s_ctx.packet_ready)
    {
        if (lwpkt_read(&s_ctx.pkt) == lwpktVALID)
        {
            s_ctx.packet_ready = true;
        }
    }

    return LG_OK;
}

/**
 * @brief Read a received packet (non-blocking).
 */
static lg_result_t comm_read(lg_comm_packet_t *packet)
{
    if (packet == NULL)
    {
        return LG_INVALID_PARAM;
    }

    if (!s_ctx.packet_ready)
    {
        return LG_BUSY; // No packet available
    }

    // Extract packet fields using LwPKT API
    packet->cmd = (uint8_t)lwpkt_get_cmd(&s_ctx.pkt);
    packet->len = (uint16_t)lwpkt_get_data_len(&s_ctx.pkt);
    packet->from_addr = lwpkt_get_from_addr(&s_ctx.pkt);
    packet->flags = lwpkt_get_flags(&s_ctx.pkt); // 🆕 Extract FLAGS

    // Update last sender address for reply routing
    s_ctx.last_sender_addr = (uint8_t)packet->from_addr;

    // Safety: clamp length to buffer size
    if (packet->len > sizeof(packet->data))
    {
        packet->len = sizeof(packet->data);
    }

    // Copy payload data
    const uint8_t *pkt_data = lwpkt_get_data(&s_ctx.pkt);
    if (pkt_data != NULL && packet->len > 0)
    {
        memcpy(packet->data, pkt_data, packet->len);
    }

    // Mark packet as consumed
    s_ctx.packet_ready = false;

    return LG_OK;
}

/**
 * @brief Send a response packet.
 * @param cmd Command ID
 * @param flags FLAGS field (use 0 if not needed)
 * @param data Payload buffer
 * @param len Payload length
 */
static lg_result_t comm_send_with_flags(uint8_t cmd, uint32_t flags, const void *data, uint16_t len)
{
    if (len > 0 && data == NULL)
    {
        return LG_INVALID_PARAM;
    }

    // Send response to last sender (typically master)
    // Parameters (with FLAGS enabled): pkt, to, flags, cmd, data, len
    lwpktr_t res = lwpkt_write(
        &s_ctx.pkt,
        s_ctx.last_sender_addr, // Destination: reply to sender
        flags,                  // 🆕 FLAGS field (for cascade)
        cmd,                    // Command ID
        data,
        len);

    return (res == lwpktOK) ? LG_OK : LG_ERROR;
}

/**
 * @brief Send a response packet (without flags - backward compat).
 */
static lg_result_t comm_send(uint8_t cmd, const void *data, uint16_t len)
{
    return comm_send_with_flags(cmd, 0, data, len); // FLAGS=0
}

/**
 * @brief Update device address at runtime.
 */
static lg_result_t comm_set_address(uint8_t address)
{
    lwpkt_set_addr(&s_ctx.pkt, address);
    s_ctx.device_address = address;
    return LG_OK;
}

/**
 * @brief LwPKT event callback for RS-485 DE pin control.
 * @note  Executes in main loop context (not ISR).
 */
static void lwpkt_event_callback(lwpkt_t *pkt, lwpkt_evt_type_t evt_type)
{
    (void)pkt; // Unused parameter

    switch (evt_type)
    {
    case LWPKT_EVT_PRE_WRITE:
        // Enable RS-485 driver (TX mode)
        HAL_GPIO_WritePin(DIR_GPIO_Port, DIR_Pin, GPIO_PIN_SET);

        // Kick-start UART TX if idle
        if (huart1.gState == HAL_UART_STATE_READY)
        {
            uint8_t byte;
            if (lwrb_read(&s_ctx.tx_rb, &byte, 1) == 1)
            {
                HAL_UART_Transmit_IT(&huart1, &byte, 1);
            }
        }
        break;

    case LWPKT_EVT_POST_WRITE:
        // Note: DE pin will be cleared in TxCpltCallback when buffer empty
        break;

    default:
        break;
    }
}

/* ============================================================================
 * HAL UART Callbacks (ISR Context - Keep <50µs)
 * ========================================================================= */

/**
 * @brief UART TX complete callback (ISR).
 * @note  Continue sending from ring buffer or disable RS-485 driver.
 */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
        uint8_t byte;
        if (lwrb_read(&s_ctx.tx_rb, &byte, 1) == 1)
        {
            // More data to send
            HAL_UART_Transmit_IT(&huart1, &byte, 1);
        }
        else
        {
            // TX complete, disable RS-485 driver
            HAL_GPIO_WritePin(DIR_GPIO_Port, DIR_Pin, GPIO_PIN_RESET);
        }
    }
}

/**
 * @brief UART error callback (ISR).
 * @note  Restart DMA and reset to RX mode.
 */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
        // Restart DMA reception
        HAL_UARTEx_ReceiveToIdle_DMA(&huart1, s_ctx.dma_rx_buffer, COMM_DMA_BUFFER_SIZE);
        // Ensure in RX mode
        HAL_GPIO_WritePin(DIR_GPIO_Port, DIR_Pin, GPIO_PIN_RESET);
    }
}

/**
 * @brief UART RX event callback (DMA idle line detection).
 * @note  Copy received data to ring buffer and restart DMA.
 */
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    if (huart->Instance == USART1)
    {
        // Write received data to RX ring buffer
        lwrb_write(&s_ctx.rx_rb, s_ctx.dma_rx_buffer, Size);

        // Restart DMA for next packet
        HAL_UARTEx_ReceiveToIdle_DMA(&huart1, s_ctx.dma_rx_buffer, COMM_DMA_BUFFER_SIZE);
    }
}
