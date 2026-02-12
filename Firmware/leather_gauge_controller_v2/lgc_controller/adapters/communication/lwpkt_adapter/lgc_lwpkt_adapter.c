/**
 * @file    lgc_lwpkt_adapter.c
 * @brief   LwPKT Communication Adapter - Implementation
 * @author  Clean Architecture Refactor Team
 * @date    2026-02-12
 * @version 1.2.0 (RxEventCallback + ReceiveToIdle_DMA)
 *
 * Copyright (c) 2026 Leather Gauge Controller Team
 */

/* ============================= Includes ============================= */
#include "lgc_lwpkt_adapter.h"
#include <string.h>

/* ============================= Private Variables ==================== */
/**
 * @brief Active adapter instance for ISR callbacks (Singleton pattern)
 * @note  Necessary because HAL callbacks only provide huart handle
 */
static LgcLwPktAdapter_t * volatile s_active_adapter = NULL;

/* ============================= Private Function Prototypes ========== */
static void lwpkt_rx_event_callback(UART_HandleTypeDef *huart, uint16_t Size);
static void lwpkt_error_callback(UART_HandleTypeDef *huart);
static Result_t lwpkt_init_impl(void *ctx, const LgcSensorReaderConfig_t *config);
static Result_t lwpkt_read_cascade_impl(void *ctx, LgcSensorArray_t *out_data);
static Result_t lwpkt_deinit_impl(void *ctx);

/* ============================= Private Functions ==================== */

/**
 * @brief UART RX Event Callback (Registered via HAL_UART_RegisterRxEventCallback)
 * 
 * @param huart UART Handle
 * @param Size  Number of bytes received (DMA position)
 */
static void lwpkt_rx_event_callback(UART_HandleTypeDef *huart, uint16_t Size)
{
    /* Check if callback is for our active adapter */
    if (s_active_adapter != NULL && s_active_adapter->huart == huart)
    {
        /* Write received data to LwPKT Ring Buffer */
        /* 'Size' is the number of bytes received in the current transfer */
        
        lwrb_write(&s_active_adapter->rx_rb, s_active_adapter->rx_buffer, Size);
        
        /* Restart reception (Idle Line Detection) */
        HAL_UARTEx_ReceiveToIdle_DMA(s_active_adapter->huart, 
                                     s_active_adapter->rx_buffer, 
                                     sizeof(s_active_adapter->rx_buffer) / 2); /* Use Half buffer size per transfer? Or Full? */
                                     
        /* The Modbus example uses: &modbus_rx_temp_buffer[0], MODBUS_RX_BUFFER_SIZE / 2 */
        /* This implies they are cycling through a temp buffer. */
    }
}

/**
 * @brief UART Error Callback
 */
static void lwpkt_error_callback(UART_HandleTypeDef *huart)
{
    if (s_active_adapter != NULL && s_active_adapter->huart == huart)
    {
        s_active_adapter->last_error_count++;
        /* Restart DMA on error */
        HAL_UARTEx_ReceiveToIdle_DMA(s_active_adapter->huart, 
                                     s_active_adapter->rx_buffer, 
                                     sizeof(s_active_adapter->rx_buffer) / 2);
    }
}

/**
 * @brief LwPKT initialization callback (ISensorReader interface)
 */
static Result_t lwpkt_init_impl(void *ctx, const LgcSensorReaderConfig_t *config)
{
    LGC_VALIDATE_PTR(ctx);
    LGC_VALIDATE_PTR(config);

    LgcLwPktAdapter_t *adapter = (LgcLwPktAdapter_t *)ctx;

    if (adapter->is_initialized) return ERR_BUSY;

    /* Set active adapter for ISRs */
    s_active_adapter = adapter;

    /* Copy configuration */
    memcpy(&adapter->config, config, sizeof(LgcSensorReaderConfig_t));

    /* Initialize Ring Buffers */
    /* Use second half of buffer for RingBuffer storage to avoid overlap with DMA buffer */
    size_t half_size = sizeof(adapter->rx_buffer) / 2;
    uint8_t *rb_storage = adapter->rx_buffer + half_size; 
    
    /* Initialize LwPKT RB using upper half of buffer */
    lwrb_init(&adapter->rx_rb, rb_storage, half_size);
    
    /* Initialize TX Ring Buffer */
    lwrb_init(&adapter->tx_rb, adapter->tx_buffer, sizeof(adapter->tx_buffer));

    /* Initialize LwPKT instance */
    lwpkt_init(&adapter->lwpkt, &adapter->rx_rb, &adapter->tx_rb);

    /* Register Callbacks */
    HAL_UART_RegisterRxEventCallback(adapter->huart, lwpkt_rx_event_callback);
    HAL_UART_RegisterCallback(adapter->huart, HAL_UART_ERROR_CB_ID, lwpkt_error_callback);

    /* Start Reception (Idle Line Detection) using lower half of buffer */
    HAL_StatusTypeDef hal_res = HAL_UARTEx_ReceiveToIdle_DMA(
        adapter->huart,
        adapter->rx_buffer, /* Lower half */
        half_size);

    if (hal_res != HAL_OK) return ERR_HARDWARE_FAULT;

    adapter->is_initialized = true;
    return ERR_OK;
}

/**
 * @brief Read all sensors in cascade mode (ISensorReader interface)
 */
static Result_t lwpkt_read_cascade_impl(void *ctx, LgcSensorArray_t *out_data)
{
    LGC_VALIDATE_PTR(ctx);
    LGC_VALIDATE_PTR(out_data);

    LgcLwPktAdapter_t *adapter = (LgcLwPktAdapter_t *)ctx;

    if (!adapter->is_initialized) return ERR_NOT_INITIALIZED;

    uint8_t cmd_payload[1] = {0x00};
    lwpkt_result_t res;

    memset(out_data, 0, sizeof(LgcSensorArray_t));
    out_data->timestamp_ms = HAL_GetTick();

    /* Send Broadcast */
    res = lwpkt_write(&adapter->lwpkt, 0xFF, 0x12, cmd_payload, 0, 1, 0);
    if (res != lwpktOK) {
        adapter->last_error_count++;
        return ERR_HARDWARE_FAULT;
    }

    uint32_t timeout_start = HAL_GetTick();
    uint8_t sensors_received = 0;
    bool cascade_complete = false;

    while (!cascade_complete && ((HAL_GetTick() - timeout_start) < adapter->config.timeout_ms))
    {
        /* Process LwPKT (Parses RB data filled by ISR) */
        lwpkt_process(&adapter->lwpkt, HAL_GetTick());

        lwpkt_packet_t *packet;
        if (lwpkt_read(&adapter->lwpkt, &packet) == lwpktOK && packet != NULL)
        {
            /* CMD_READ_CASCADE_RESP = 0x92 */
            if (packet->cmd == 0x92)
            {
               int sensor_idx = -1;
               /* Address mapping logic 1..11 -> 0..10 */
               if (packet->addr >= 1 && packet->addr <= 11) {
                   sensor_idx = packet->addr - 1;
               }
               
               if (sensor_idx >= 0 && sensor_idx < LGC_SENSOR_COUNT) {
                   if (packet->len >= 2) {
                       uint16_t val = (uint16_t)packet->data[0] | ((uint16_t)packet->data[1] << 8);
                       out_data->sensors[sensor_idx].raw_digital_status = val;
                       out_data->sensors[sensor_idx].is_valid = true;
                       out_data->sensors[sensor_idx].sensor_address = packet->addr;
                       out_data->sensors[sensor_idx].error_code = ERR_OK;
                       sensors_received++;
                   }
               }
            }
            lwpkt_free(&adapter->lwpkt, packet);
        }

        if (sensors_received >= LGC_SENSOR_COUNT) cascade_complete = true;
    }

    if (sensors_received < LGC_SENSOR_COUNT) {
        /* Mark missing */
        for (uint8_t i = 0; i < LGC_SENSOR_COUNT; i++) {
            if (!out_data->sensors[i].is_valid) {
                out_data->sensors[i].is_valid = false;
                out_data->sensors[i].error_code = ERR_TIMEOUT;
            }
        }
        adapter->last_error_count++;
        return ERR_TIMEOUT;
    }

    return ERR_OK;
}

/**
 * @brief Read all sensors sequentially (fallback, ISensorReader interface)
 */
static Result_t lwpkt_read_all_impl(void *ctx, LgcSensorArray_t *out_data)
{
    return lwpkt_read_cascade_impl(ctx, out_data);
}

/**
 * @brief Deinitialize LwPKT (ISensorReader interface)
 */
static Result_t lwpkt_deinit_impl(void *ctx)
{
    LGC_VALIDATE_PTR(ctx);
    LgcLwPktAdapter_t *adapter = (LgcLwPktAdapter_t *)ctx;

    if (!adapter->is_initialized) return ERR_NOT_INITIALIZED;
    
    s_active_adapter = NULL;

    /* Stop DMA */
    HAL_UART_DMAStop(adapter->huart);
    
    /* Unregister Callbacks */
    HAL_UART_UnRegisterRxEventCallback(adapter->huart);
    HAL_UART_UnRegisterCallback(adapter->huart, HAL_UART_ERROR_CB_ID);

    lwpkt_deinit(&adapter->lwpkt);

    adapter->is_initialized = false;
    return ERR_OK;
}

/* ============================= Public Functions ===================== */

Result_t LgcLwPktAdapter_Init(LgcLwPktAdapter_t *adapter, UART_HandleTypeDef *huart)
{
    LGC_VALIDATE_PTR(adapter);
    LGC_VALIDATE_PTR(huart);

    memset(adapter, 0, sizeof(LgcLwPktAdapter_t));
    adapter->huart = huart;

    LgcSensorReaderConfig_t default_config = {
        .timeout_ms = LWPKT_CASCADE_TIMEOUT_MS,
        .retry_count = 1,
        .enable_crc = true};

    return lwpkt_init_impl(adapter, &default_config);
}

ILgcSensorReader_t *LgcLwPktAdapter_GetInterface(LgcLwPktAdapter_t *adapter)
{
    if (adapter == NULL) return NULL;

    static ILgcSensorReader_t iface = {
        .context = NULL,
        .init = lwpkt_init_impl,
        .read_all_sensors = lwpkt_read_all_impl,
        .read_cascade_mode = lwpkt_read_cascade_impl,
        .deinit = lwpkt_deinit_impl};

    iface.context = adapter;
    return &iface;
}

Result_t LgcLwPktAdapter_Deinit(LgcLwPktAdapter_t *adapter)
{
    return lwpkt_deinit_impl(adapter);
}

/* ============================= Unused Callbacks ============================ */
void LgcLwPktAdapter_DMA_RxCpltCallback(UART_HandleTypeDef *huart) {}
void LgcLwPktAdapter_DMA_RxHalfCpltCallback(UART_HandleTypeDef *huart) {}
