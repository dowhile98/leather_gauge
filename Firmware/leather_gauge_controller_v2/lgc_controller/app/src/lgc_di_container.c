/**
 * @file    lgc_di_container.c
 * @brief   Dependency Injection Container - Implementation
 * @author  Clean Architecture Refactor Team
 * @date    2026-02-12
 * @version 1.1.0
 *
 * @details This is the COMPOSITION ROOT - the ONLY file that knows about
 *          concrete implementations. All dependency wiring happens here.
 *
 * Copyright (c) 2026 Leather Gauge Controller Team
 */

/* ============================= Includes ============================= */
#include "lgc_di_container.h"

/* Domain interfaces (abstractions) */
#include "../../domain/interfaces/lgc_i_sensor_reader.h"
#include "../../domain/interfaces/lgc_i_encoder.h"
#include "../../domain/interfaces/lgc_i_storage.h"
#include "../../domain/interfaces/lgc_i_display.h"
#include "../../domain/interfaces/lgc_i_printer.h"
#include "../../domain/interfaces/lgc_i_event_publisher.h"

/* Domain entities */
#include "../../domain/entities/lgc_configuration_entity.h"
#include "../../domain/entities/lgc_measurement_entity.h"

/* Concrete adapters (implementations) */
#include "../../adapters/communication/lwpkt_adapter/lgc_lwpkt_agent.h"
#include "../../adapters/peripherals/encoder_adapter/lgc_encoder_adapter.h"
#include "../../adapters/storage/eeprom_adapter/lgc_eeprom_adapter.h"
#include "../../adapters/peripherals/display_adapter/lgc_display_adapter.h"
/* #include "../../adapters/peripherals/printer_adapter/lgc_printer_adapter.h" // TODO */

/* Application services & Tasks */
#include "events/lgc_event_publisher.h"
#include "tasks/lgc_hmi_task.h"
#include "tasks/lgc_main_task.h"
#include "tasks/lgc_printer_task.h"

/* HAL & RTOS (only in app layer) */
#include "stm32f4xx_hal.h"
#include "tx_api.h"

/* Peripherals (from Core/Inc) */
extern UART_HandleTypeDef huart2; // LwPKT communication
extern UART_HandleTypeDef huart1; // DWIN display
// extern I2C_HandleTypeDef hi2c1;    // EEPROM

/* ============================= Global Instances ===================== */
/**
 * @brief Global LwPKT Agent instance pointer (for ISR callbacks)
 * @note Points to static instance in s_adapters struct after initialization
 */
LgcLwPktAgent_t *g_lwpkt_agent = NULL;

/* ============================= Static Instances ===================== */
/**
 * @brief Static adapter instances (no dynamic memory)
 */
static struct
{
    /* Communication adapters */
    LgcLwPktAgent_t lwpkt_agent; /* Active Object (OSAL-based) */

    /* Peripheral adapters */
    LgcDisplayAdapter_t display_adapter;
    /* Printer: TODO */

    /* Storage adapters */
    /* EEPROM: Static instance managed inside adapter (singleton) */

} s_adapters;

/**
 * @brief Static interface pointers (for DI)
 */
static struct
{
    ILgcSensorReader_t *sensor_reader;
    ILgcEncoder_t *encoder;
    ILgcStorage_t *storage;
    ILgcDisplay_t *display;
    ILgcPrinter_t *printer;
    ILgcEventPublisher_t *event_publisher;
} s_interfaces;

/**
 * @brief Static system configuration (loaded from EEPROM)
 */
static LgcSystemConfig_t s_system_config;

/**
 * @brief Static ThreadX tasks
 */
static struct
{
    LgcHmiTask_t hmi_task;
    /* MainTask and PrinterTask manage their own static instances */
} s_tasks;

/**
 * @brief Static application services
 */
static struct
{
    LgcEventPublisher_t event_publisher;
    LgcMeasurements_t measurements;
} s_services;

/* ============================= Private Functions ==================== */
/**
 * @brief Initialize all adapters
 */
static Result_t di_init_adapters(void)
{
    Result_t res;

    /* ===== Communication Adapters ===== */

    /* LwPKT Agent (Active Object - OSAL-based) */
    error_t err = LgcLwPktAgent_Init(&s_adapters.lwpkt_agent, &huart2);
    if (err != NO_ERROR)
        return ERR_ERROR;

    /* Start LwPKT Agent (begin UART DMA reception) */
    err = LgcLwPktAgent_Start(&s_adapters.lwpkt_agent);
    if (err != NO_ERROR)
        return ERR_ERROR;

    /* Expose global pointer for ISR callbacks */
    g_lwpkt_agent = &s_adapters.lwpkt_agent;

    /* ===== Peripheral Adapters ===== */

    /* Encoder adapter (GPIO EXTI) */
    /* Note: Encoder uses singleton pattern, init called via interface */

    /* Display adapter (UART DWIN) */
    res = LgcDisplayAdapter_Init(&s_adapters.display_adapter, &huart1);
    if (res != ERR_OK)
        return res;

    /* Printer adapter (USB ESC/POS) - TODO */
    /* s_interfaces.printer = PrinterAdapter_GetInterface(...); */

    /* ===== Storage Adapters ===== */

    /* EEPROM adapter (I2C AT24Cxx) */
    /* Note: EEPROM uses singleton pattern, init called via interface */

    return ERR_OK;
}

/**
 * @brief Wire interfaces (Dependency Inversion)
 */
static Result_t di_wire_interfaces(void)
{
    /* Get interface pointers from adapters */

    /* Sensor Reader (LwPKT Agent - Active Object) */
    /* TODO: Create wrapper adapter that uses LgcLwPktAgent async API */
    /* For now, set to NULL to avoid build errors */
    s_interfaces.sensor_reader = NULL;                                     /* Active Object migration in progress */
    /* if (s_interfaces.sensor_reader == NULL) return ERR_NULL_POINTER; */ /* Disabled during migration */

    /* Encoder (GPIO EXTI - Singleton) */
    s_interfaces.encoder = (ILgcEncoder_t *)LgcEncoderAdapter_GetInterface();
    if (s_interfaces.encoder == NULL)
        return ERR_NULL_POINTER;

    /* Initialize encoder with config */
    LgcEncoderConfig_t encoder_cfg = {
        .pulses_per_revolution = 1000,
        .enable_interrupts = true,
        .debounce_ms = 10};
    Result_t res = s_interfaces.encoder->init(s_interfaces.encoder->context, &encoder_cfg);
    if (res != ERR_OK)
        return res;

    /* Storage (EEPROM - Singleton) */
    s_interfaces.storage = (ILgcStorage_t *)LgcEepromAdapter_GetInterface();
    if (s_interfaces.storage == NULL)
        return ERR_NULL_POINTER;

    /* Initialize EEPROM */
    LgcStorageConfig_t storage_cfg = {
        .timeout_ms = 1000,
        .enable_crc = true,
        .auto_retry = true};
    res = s_interfaces.storage->init(s_interfaces.storage->context, &storage_cfg);
    if (res != ERR_OK)
        return res;

    /* Load System Configuration */
    res = s_interfaces.storage->load_config(s_interfaces.storage->context, &s_system_config);
    if (res == ERR_CRC_MISMATCH || res == ERR_NO_DATA)
    {
        LgcSystemConfig_InitDefaults(&s_system_config);
        s_interfaces.storage->save_config(s_interfaces.storage->context, &s_system_config);
    }
    else if (res != ERR_OK)
    {
        LgcSystemConfig_InitDefaults(&s_system_config);
    }

    if (!LgcSystemConfig_Validate(&s_system_config))
    {
        LgcSystemConfig_InitDefaults(&s_system_config);
    }

    /* Display (DWIN - UART1) */
    s_interfaces.display = LgcDisplayAdapter_GetInterface(&s_adapters.display_adapter);
    if (s_interfaces.display == NULL)
        return ERR_NULL_POINTER;

    LgcDisplayConfig_t display_cfg = {
        .timeout_ms = 1000,
        .refresh_rate_ms = 100,
        .backlight = 80,
        .enable_buzzer = true};
    res = s_interfaces.display->init(s_interfaces.display->context, &display_cfg);
    if (res != ERR_OK)
        return res;

    /* Event Publisher */
    res = LgcEventPublisher_Init(&s_services.event_publisher);
    if (res != ERR_OK)
        return res;

    s_interfaces.event_publisher = LgcEventPublisher_GetInterface(&s_services.event_publisher);
    if (s_interfaces.event_publisher == NULL)
        return ERR_NULL_POINTER;

    res = s_interfaces.event_publisher->init(s_interfaces.event_publisher->context);
    if (res != ERR_OK)
        return res;

    /* Initialize Measurements structure */
    LgcMeasurements_Init(&s_services.measurements);

    return ERR_OK;
}

/**
 * @brief Initialize and Start ThreadX tasks
 */
static Result_t di_create_tasks(void)
{
    Result_t res;

    /* 1. Main Control Task */
    LgcMainTaskConfig_t main_cfg = {
        .encoder_timeout_ms = 0, /* Infinite wait for pulse */
        .enable_diagnostics = true};

    res = LgcMainTask_Start(&main_cfg);
    if (res != ERR_OK)
        return res;

    /* 2. HMI Task */
    res = LgcHmiTask_Init(
        &s_tasks.hmi_task,
        s_interfaces.display,
        s_interfaces.event_publisher,
        &s_system_config,
        &s_services.measurements);

    if (res != ERR_OK)
        return res;

    res = LgcHmiTask_Start(&s_tasks.hmi_task);
    if (res != ERR_OK)
        return res;

    /* 3. Printer Task */
    /* Only init if printer interface is available (TODO) */
    if (s_interfaces.printer != NULL)
    {
        res = LgcPrinterTask_Init(
            s_interfaces.printer,
            s_interfaces.event_publisher,
            &s_system_config);
        /* Printer task starts automatically in Init */
    }

    return ERR_OK;
}

/* ============================= Public Functions ===================== */

Result_t LgcDI_Init(void)
{
    Result_t res;

    /* Step 1: Initialize adapters */
    res = di_init_adapters();
    if (res != ERR_OK)
        return res;

    /* Step 2: Wire interfaces */
    res = di_wire_interfaces();
    if (res != ERR_OK)
        return res;

    /* Step 3: Create and Start Tasks */
    /* Note: Use Cases are implicitly used within Tasks via DI Getters,
       no explicit init needed if they are pure logic or managed by tasks. */
    res = di_create_tasks();
    if (res != ERR_OK)
        return res;

    return ERR_OK;
}

Result_t LgcDI_Start(void)
{
    /* ThreadX scheduler started in main.c */
    return ERR_OK;
}

Result_t LgcDI_Shutdown(void)
{
    /* Graceful shutdown logic */
    LgcMainTask_Stop();
    LgcHmiTask_Stop(&s_tasks.hmi_task);
    if (s_interfaces.printer)
        LgcPrinterTask_Deinit();

    /* Save config */
    s_interfaces.storage->save_config(s_interfaces.storage->context, &s_system_config);

    return ERR_OK;
}

/* ============================= Dependency Getters =================== */

ILgcSensorReader_t *DIContainer_GetSensorReader(void)
{
    return s_interfaces.sensor_reader;
}

ILgcEncoder_t *DIContainer_GetEncoder(void)
{
    return s_interfaces.encoder;
}

ILgcStorage_t *DIContainer_GetStorage(void)
{
    return s_interfaces.storage;
}

ILgcDisplay_t *DIContainer_GetDisplay(void)
{
    return s_interfaces.display;
}

ILgcPrinter_t *DIContainer_GetPrinter(void)
{
    return s_interfaces.printer;
}

LgcSystemConfig_t *DIContainer_GetConfig(void)
{
    return &s_system_config;
}

ILgcEventPublisher_t *DIContainer_GetEventPublisher(void)
{
    return s_interfaces.event_publisher;
}

LgcMeasurements_t *DIContainer_GetMeasurements(void)
{
    return &s_services.measurements;
}
