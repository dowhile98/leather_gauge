/**
 *
 */

//-------------------------------------------------------------------------------
// includes
//-------------------------------------------------------------------------------
#include "lgc.h"
#include "lgc_interface_printer.h"
#include "error.h"
#include "os_port.h"
#include "dwin_core.h"
#include "usart.h"
#include "lgc_hmi.h"
//-------------------------------------------------------------------------------
// defines
//-------------------------------------------------------------------------------
#ifndef DWIN_BUFFER_SIZE
#define DWIN_BUFFER_SIZE 1024
#endif

#ifndef DWIN_PROCESS_TASK_PRI
#define DWIN_PROCESS_TASK_PRI 11
#endif

#ifndef DWIN_PROCESS_TASK_STACK
#define DWIN_PROCESS_TASK_STACK 256
#endif

#ifndef LGC_HMI_TASK_PRI
#define LGC_HMI_TASK_PRI 10
#endif

#ifndef LGC_HMI_TASK_STACK
#define LGC_HMI_TASK_STACK 256
#endif

#ifndef LGC_HMI_UPDATE_TASK_PRI
#define LGC_HMI_UPDATE_TASK_PRI 10
#endif

#ifndef LGC_HMI_UPDATE_TASK_STACK
#define LGC_HMI_UPDATE_TASK_STACK 256
#endif
//-------------------------------------------------------------------------------
// global variables
//-------------------------------------------------------------------------------
static OsQueue hmi_msg;

static uint8_t dwin_fifo_mem[DWIN_BUFFER_SIZE];
static dwin_t dwin_hmi;
static OsMutex dwin_mutex;
static OsSemaphore dwin_response;
static OsSemaphore dwin_new_data_flag;
static OsSemaphore tx_cplt_flag;
static uint8_t uart_rx[128];
static dwin_interface_t dwin_hal = {0};
static OsTaskId dwin_process_task;
static OsTaskId lgc_hmi_task = {0};
static OsTaskId lgc_hmi_update_task = {0};
static uint8_t hmi_page = 0;
//-------------------------------------------------------------------------------
// private function prototype
//-------------------------------------------------------------------------------
static uint32_t lgc_dwin_uart_transmit(uint8_t *data, uint16_t len);
static uint32_t lgc_dwin_get_tick(void);
static void lgc_dwin_lock(void);
static void lgc_dwin_unlock(void);
static bool lgc_dwin_signal(uint32_t timeout);
static void lgc_dwin_signal_set(void);
static void lgc_dwin_new_data_signal(void);
static void lgc_dwin_new_data_signal_set(void);
static void lgc_dwin_uart_ErrorCallback(UART_HandleTypeDef *huart);
static void lgc_dwin_uart_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Pos);
static void on_dwin_event(dwin_evt_t *evt, void *ctx);
error_t lgc_hmi_send_msg(dwin_evt_t *evt);
static void hmi_set_current_page(uint8_t page);
//-------------------------------------------------------------------------------
// task definition
//-------------------------------------------------------------------------------
void lgc_hmi_update_task_entry(void *param)
{
    lgc_measurements_t *measurements;
    lgc_t state_data;
    measurements = osAllocMem(sizeof(lgc_measurements_t));
    if (measurements == NULL)
    {
        for (;;)
        {
            osDelayTask(1000);
        }
    }
    // TO-DO: Implement periodic updates to HMI if needed
    for (;;)
    {
        // wait for update event
        osWaitForEventBits(&events, LGC_HMI_UPDATE_REQUIRED, TRUE, TRUE, INFINITE_DELAY);
        // state machine
        switch (hmi_page)
        {
        case HMI_PAGE1:
        {
            // get measurements
            lgc_get_measurements(measurements);
            // get state data
            lgc_get_state_data(&state_data);
            // send to HMI
            dwin_write_vp_u16(&dwin_hmi, LGC_HMI_VP_ICON_SPEEP, state_data.speed_motor);
            dwin_write_vp_u16(&dwin_hmi, LGC_HMI_VP_BATCH_COUNT, measurements->current_batch_index);
            dwin_write_vp_u16(&dwin_hmi, LGC_HMI_VP_LEATHER_COUNT, measurements->current_leather_index);
            dwin_write_vp_u16(&dwin_hmi, LGC_HMI_VP_CURRENT_LEATHER_AREA, (uint16_t)(measurements->current_leather_area * 100)); // assuming area in cm², sending as integer
        }
        break;

        default:
            break;
        }
    }
}
void lgc_hmi_task_entry(void *param)
{
    dwin_evt_t msg = {0};

    for (;;)
    {
        if (osReceiveFromQueue(&hmi_msg, &msg, INFINITE_DELAY) != TRUE)
        {
            continue;
        }

        switch (msg.addr)
        {
        case 0x1000:
        {
            break;
        }
        }

        /*release memory*/
        osFreeMem(&msg.data);
        /* code */
        osDelayTask(1000);
    }
}
//-------------------------------------------------------------------------------
// private function definition
//-------------------------------------------------------------------------------
static void lgc_dwin_process_task_entry(void *param);
/*---------------------------------------------------------------------------*/
/* Implementación HAL                                                        */
/*---------------------------------------------------------------------------*/

uint32_t lgc_dwin_uart_transmit(uint8_t *data, uint16_t len)
{
    HAL_UART_Transmit_DMA(&huart6, (uint8_t *)data, len);
    /*wait for transmit finish*/
    if (osWaitForSemaphore(&tx_cplt_flag, INFINITE_DELAY) != TRUE)
    {
        return 0;
    }

    return len;
}

uint32_t lgc_dwin_get_tick(void)
{
    return osGetSystemTime();
}
void lgc_dwin_lock(void)
{
    osAcquireMutex(&dwin_mutex);
}
void lgc_dwin_unlock(void)
{
    osReleaseMutex(&dwin_mutex);
}

bool lgc_dwin_signal(uint32_t timeout)
{
    return (osWaitForSemaphore(&dwin_response, timeout) == TRUE);
}

void lgc_dwin_signal_set(void)
{
    osReleaseSemaphore(&dwin_response);
}

/* Semáforo para nuevos datos (Consumidor) */
void lgc_dwin_new_data_signal(void)
{
    // Espera infinita hasta que llegue algo
    osWaitForSemaphore(&dwin_new_data_flag, INFINITE_DELAY);
}

/* Semáforo para nuevos datos (Productor - ISR) */
void lgc_dwin_new_data_signal_set(void)
{
    // IMPORTANTE: Si tu RTOS tiene funciones "FromISR", úsalas aquí.
    // ThreadX osReleaseSemaphore suele ser seguro en ISR, pero verifica tu port.
    osReleaseSemaphore(&dwin_new_data_flag);
}

static void lgc_dwin_process_task_entry(void *param)
{
    dwin_register_callback(&dwin_hmi, on_dwin_event, NULL);
    for (;;)
    {
        dwin_process(&dwin_hmi);
    }
}
/*---------------------------------------------------------------------------*/
/* hmi callbacks                                                             */
/*---------------------------------------------------------------------------*/
static void on_dwin_event(dwin_evt_t *evt, void *ctx)
{
    lgc_hmi_send_msg(evt);
}
//-------------------------------------------------------------------------------
// hadrware callback
//-------------------------------------------------------------------------------
static void lgc_dwin_uart_ErrorCallback(UART_HandleTypeDef *huart)

{
    HAL_UARTEx_ReceiveToIdle_DMA(&huart6, uart_rx, 128);
}

static void lgc_dwin_uart_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Pos)
{
    // push data
    dwin_rx_push_ex(&dwin_hmi, uart_rx, Pos);
    // notify
    dwin_rx_notify(&dwin_hmi);
    // receive another data
    HAL_UARTEx_ReceiveToIdle_DMA(&huart6, uart_rx, 128);
}
//-------------------------------------------------------------------------------
// public functions
//-------------------------------------------------------------------------------
error_t lgc_hmi_init(void)
{
    OsTaskParameters params = OS_TASK_DEFAULT_PARAMS;

    // 1. Inicializar la interfaz
    dwin_hal.uart_transmit = lgc_dwin_uart_transmit;
    dwin_hal.get_tick_ms = lgc_dwin_get_tick;
    dwin_hal.lock = lgc_dwin_lock;
    dwin_hal.unlock = lgc_dwin_unlock;
    dwin_hal.sem_signal = lgc_dwin_signal_set;
    dwin_hal.sem_wait = lgc_dwin_signal;

    /* Configuración de bloqueo en RX */
    dwin_hal.sem_new_data_wait = lgc_dwin_new_data_signal;
    dwin_hal.sem_new_data_signal = lgc_dwin_new_data_signal_set;

    if (dwin_init(&dwin_hmi, &dwin_hal, dwin_fifo_mem, DWIN_BUFFER_SIZE) != DWIN_OK)
    {
        return ERROR_FAILURE;
    }

    /* Crear primitivas OS */
    if (osCreateSemaphore(&tx_cplt_flag, 0) != TRUE)
    {
        return ERROR_FAILURE;
    }
    if (osCreateSemaphore(&dwin_response, 0) != TRUE)
    {
        return ERROR_FAILURE;
    }
    if (osCreateMutex(&dwin_mutex) != TRUE)
    {
        return ERROR_FAILURE;
    }
    if (osCreateSemaphore(&dwin_new_data_flag, 0) != TRUE)
    {
        return ERROR_FAILURE;
    }

    if (osCreateQueue(&hmi_msg, "hmi msg", sizeof(dwin_evt_t), 5) != TRUE)
    {
        return ERROR_FAILURE;
    }

    params = OS_TASK_DEFAULT_PARAMS;
    params.priority = DWIN_PROCESS_TASK_PRI;
    params.stackSize = DWIN_PROCESS_TASK_STACK;
    dwin_process_task = osCreateTask("DWIN Process", lgc_dwin_process_task_entry, NULL, &params);

    if (!dwin_process_task)
    {
        return ERROR_FAILURE;
    }

    params = OS_TASK_DEFAULT_PARAMS;
    params.priority = LGC_HMI_TASK_PRI;
    params.stackSize = LGC_HMI_TASK_STACK;
    lgc_hmi_task = osCreateTask("HMI Task", lgc_hmi_task_entry, NULL, &params);

    if (!lgc_hmi_task)
    {
        return ERROR_FAILURE;
    }

    params = OS_TASK_DEFAULT_PARAMS;
    params.priority = LGC_HMI_UPDATE_TASK_PRI;
    params.stackSize = LGC_HMI_UPDATE_TASK_STACK;
    lgc_hmi_update_task = osCreateTask("HMI Update Task", lgc_hmi_update_task_entry, NULL, &params);
    if (!lgc_hmi_update_task)
    {
        return ERROR_FAILURE;
    }
    /*uart receive data*/
    HAL_UART_RegisterCallback(&huart6, HAL_UART_ERROR_CB_ID, lgc_dwin_uart_ErrorCallback);

    HAL_UART_RegisterRxEventCallback(&huart6, lgc_dwin_uart_RxEventCallback);

    /*start receive data*/
    HAL_UARTEx_ReceiveToIdle_DMA(&huart6, uart_rx, 128);

    return NO_ERROR;
}

error_t lgc_hmi_send_msg(dwin_evt_t *evt)
{
    error_t err = NO_ERROR;

    dwin_evt_t msg = {0};

    msg.addr = evt->addr;
    msg.cmd = evt->cmd;
    msg.len = evt->len;
    msg.data_len = evt->data_len;
    /*allocate memory*/
    msg.data = osAllocMem(evt->data_len);
    if (!msg.data)
    {
        return ERROR_FAILURE;
    }
    memcpy(msg.data, evt->data, evt->data_len);

    // send message
    err = osSendToQueue(&hmi_msg, &msg, 1000);

    return err;
}

static void hmi_set_current_page(uint8_t page)
{
    hmi_page = page;
}