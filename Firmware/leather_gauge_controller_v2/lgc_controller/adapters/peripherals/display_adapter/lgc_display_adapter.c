/**
 * @file    lgc_display_adapter.c
 * @brief   DWIN Display Adapter - Implementation
 * @author  Clean Architecture Refactor Team (TDD)
 * @date    2026-02-12
 * @version 1.0.0
 *
 * Copyright (c) 2026 Leather Gauge Controller Team
 */

/* ============================= Includes ============================= */
#include "lgc_display_adapter.h"
#include <string.h>
#include <stdio.h>

/* ============================= VP Address Mapping (from legacy) ===== */
/** System state indicators */
#define VP_STATE 0x1110U
#define VP_ICON_SPEED 0x1111U
#define VP_FEEDBACK_MOTOR 0x1112U

/** Measurement counters */
#define VP_BATCH_COUNT 0x1050U
#define VP_LEATHER_COUNT 0x1051U
#define VP_CURRENT_AREA 0x1060U
#define VP_ACCUMULATED_AREA 0x1080U

/** Configuration */
#define VP_CONFIG_NAME_CLIENT 0x1310U
#define VP_CONFIG_NAME_COLOR 0x1320U
#define VP_CONFIG_NAME_LEATHER 0x1330U
#define VP_CONFIG_BATCH_SIZE 0x1340U
#define VP_CONFIG_UNITS 0x1350U

/** Date/Time */
#define VP_CONFIG_DAY 0x1341U
#define VP_CONFIG_MONTH 0x1342U
#define VP_CONFIG_YEAR 0x1343U
#define VP_CONFIG_HOUR 0x1346U
#define VP_CONFIG_MINUTE 0x1347U
#define VP_CONFIG_SECOND 0x1348U

/** Commands */
#define VP_PRINT_CMD 0x1400U
#define VP_DELETE_LAST 0x1501U
#define VP_CONFIG_SAVE_CMD 0x1002U
#define VP_CONFIG_SAVE_RESULT 0x1003U

/** Touch page */
#define VP_TOUCH_PAGE 0x1005U

/* ============================= Button VP Mapping ==================== */
/** Map VP addresses to button enums */
static LgcDisplayButton_t map_vp_to_button(uint16_t vp_addr, uint16_t value)
{
    switch (vp_addr)
    {
    case VP_CONFIG_SAVE_CMD:
        return (value == 1) ? LGC_BTN_START : LGC_BTN_CONFIG_SAVE;
    case VP_PRINT_CMD:
        return LGC_BTN_NEXT_BATCH;
    case VP_DELETE_LAST:
        return LGC_BTN_DELETE_LAST;
    case VP_TOUCH_PAGE:
        if (value >= 10 && value <= 15)
        {
            return LGC_BTN_SETTINGS;
        }
        return LGC_BTN_UNKNOWN;
    default:
        return LGC_BTN_UNKNOWN;
    }
}

/* ============================= DWIN HAL Callbacks =================== */
/**
 * @brief UART transmit wrapper for DWIN
 */
static uint32_t dwin_uart_transmit(uint8_t *data, uint16_t len)
{
    extern UART_HandleTypeDef *g_display_uart; /* Set by adapter init */
    HAL_UART_Transmit(g_display_uart, data, len, 100);
    return len;
}

/**
 * @brief Get system tick for DWIN timeouts
 */
static uint32_t dwin_get_tick_ms(void)
{
    return HAL_GetTick();
}

/**
 * @brief DWIN event callback (button presses from display)
 */
static void dwin_event_callback(dwin_evt_t *evt, void *user_ctx)
{
    LgcDisplayAdapter_t *adapter = (LgcDisplayAdapter_t *)user_ctx;

    if (adapter == NULL || evt == NULL)
    {
        return;
    }

    /* Filter only write commands (button presses) */
    if (evt->cmd != DWIN_CMD_WRITE_VP)
    {
        return;
    }

    /* Parse button event */
    uint16_t vp_addr = evt->addr;
    uint16_t value = 0;

    if (evt->data_len >= 2)
    {
        /* Extract uint16_t (big-endian from DWIN) */
        value = ((uint16_t)evt->data[0] << 8) | evt->data[1];
    }

    /* Map VP to button enum */
    LgcDisplayButton_t button = map_vp_to_button(vp_addr, value);

    if (button == LGC_BTN_UNKNOWN)
    {
        return; /* Ignore unknown buttons */
    }

    /* Call user callback if attached */
    if (adapter->event_callback != NULL)
    {
        LgcDisplayEvent_t display_evt = {
            .button = button,
            .raw_vp_addr = vp_addr,
            .raw_value = value,
            .timestamp_ms = HAL_GetTick()};

        adapter->event_callback(&display_evt, adapter->event_user_ctx);
    }
}

/* ============================= Interface Implementation ============= */

/**
 * @brief Initialize display
 */
static Result_t display_init_impl(void *ctx, const LgcDisplayConfig_t *config)
{
    LGC_VALIDATE_PTR(ctx);
    LGC_VALIDATE_PTR(config);

    LgcDisplayAdapter_t *adapter = (LgcDisplayAdapter_t *)ctx;

    if (adapter->is_initialized)
    {
        return ERR_BUSY;
    }

    /* Copy configuration */
    memcpy(&adapter->config, config, sizeof(LgcDisplayConfig_t));

    /* Setup DWIN HAL interface */
    dwin_interface_t dwin_hal = {
        .uart_transmit = dwin_uart_transmit,
        .get_tick_ms = dwin_get_tick_ms,
        .lock = NULL, /* TODO: Mutex if needed */
        .unlock = NULL,
        .sem_wait = NULL,
        .sem_signal = NULL,
        .sem_new_data_wait = NULL,
        .sem_new_data_signal = NULL};

    /* Initialize DWIN driver */
    dwin_error_t dwin_res = dwin_init(
        &adapter->dwin,
        &dwin_hal,
        adapter->rx_buffer,
        sizeof(adapter->rx_buffer));

    if (dwin_res != DWIN_OK)
    {
        return ERR_HARDWARE_FAULT;
    }

    /* Register DWIN event callback */
    dwin_register_callback(&adapter->dwin, dwin_event_callback, adapter);

    /* Set backlight */
    if (config->backlight > 0)
    {
        uint16_t backlight_value = (uint16_t)config->backlight;
        dwin_write_vp_u16(&adapter->dwin, DWIN_SYS_LED_CFG, backlight_value);
    }

    adapter->is_initialized = true;
    return ERR_OK;
}

/**
 * @brief Write variable (generic)
 */
static Result_t display_write_variable_impl(
    void *ctx,
    LgcDisplayVP_t vp,
    const void *data,
    uint16_t len)
{
    LGC_VALIDATE_PTR(ctx);
    LGC_VALIDATE_PTR(data);

    LgcDisplayAdapter_t *adapter = (LgcDisplayAdapter_t *)ctx;

    if (!adapter->is_initialized)
    {
        return ERR_NOT_INITIALIZED;
    }

    dwin_error_t res = dwin_write_vp_raw(&adapter->dwin, vp, (uint8_t *)data, len);

    return (res == DWIN_OK) ? ERR_OK : ERR_HARDWARE_FAULT;
}

/**
 * @brief Write uint16_t
 */
static Result_t display_write_u16_impl(
    void *ctx,
    LgcDisplayVP_t vp,
    uint16_t value)
{
    LGC_VALIDATE_PTR(ctx);

    LgcDisplayAdapter_t *adapter = (LgcDisplayAdapter_t *)ctx;

    if (!adapter->is_initialized)
    {
        return ERR_NOT_INITIALIZED;
    }

    dwin_error_t res = dwin_write_vp_u16(&adapter->dwin, vp, value);

    return (res == DWIN_OK) ? ERR_OK : ERR_HARDWARE_FAULT;
}

/**
 * @brief Write uint32_t
 */
static Result_t display_write_u32_impl(
    void *ctx,
    LgcDisplayVP_t vp,
    uint32_t value)
{
    LGC_VALIDATE_PTR(ctx);

    LgcDisplayAdapter_t *adapter = (LgcDisplayAdapter_t *)ctx;

    if (!adapter->is_initialized)
    {
        return ERR_NOT_INITIALIZED;
    }

    dwin_error_t res = dwin_write_vp_u32(&adapter->dwin, vp, value);

    return (res == DWIN_OK) ? ERR_OK : ERR_HARDWARE_FAULT;
}

/**
 * @brief Write float (converted to DWIN format: int16 × 100)
 */
static Result_t display_write_float_impl(
    void *ctx,
    LgcDisplayVP_t vp,
    float value)
{
    LGC_VALIDATE_PTR(ctx);

    LgcDisplayAdapter_t *adapter = (LgcDisplayAdapter_t *)ctx;

    if (!adapter->is_initialized)
    {
        return ERR_NOT_INITIALIZED;
    }

    /* Convert float to display format: value × 100 (2 decimal places) */
    int32_t display_value = (int32_t)(value * 100.0f);

    dwin_error_t res = dwin_write_vp_u32(&adapter->dwin, vp, (uint32_t)display_value);

    return (res == DWIN_OK) ? ERR_OK : ERR_HARDWARE_FAULT;
}

/**
 * @brief Write text (ASCII string)
 */
static Result_t display_write_text_impl(
    void *ctx,
    LgcDisplayVP_t vp,
    const char *text)
{
    LGC_VALIDATE_PTR(ctx);
    LGC_VALIDATE_PTR(text);

    LgcDisplayAdapter_t *adapter = (LgcDisplayAdapter_t *)ctx;

    if (!adapter->is_initialized)
    {
        return ERR_NOT_INITIALIZED;
    }

    uint16_t len = strlen(text);
    if (len > 32)
    {
        len = 32; /* Truncate if too long */
    }

    dwin_error_t res = dwin_write_vp_raw(&adapter->dwin, vp, (uint8_t *)text, len);

    return (res == DWIN_OK) ? ERR_OK : ERR_HARDWARE_FAULT;
}

/**
 * @brief Read variable
 */
static Result_t display_read_variable_impl(
    void *ctx,
    LgcDisplayVP_t vp,
    void *out_data,
    uint16_t max_len)
{
    LGC_VALIDATE_PTR(ctx);
    LGC_VALIDATE_PTR(out_data);

    LgcDisplayAdapter_t *adapter = (LgcDisplayAdapter_t *)ctx;

    if (!adapter->is_initialized)
    {
        return ERR_NOT_INITIALIZED;
    }

    uint8_t len_words = max_len / 2; /* Convert bytes to words */
    dwin_error_t res = dwin_read_vp(
        &adapter->dwin,
        vp,
        len_words,
        (uint8_t *)out_data,
        adapter->config.timeout_ms);

    return (res == DWIN_OK) ? ERR_OK : ERR_TIMEOUT;
}

/**
 * @brief Change page
 */
static Result_t display_change_page_impl(void *ctx, uint8_t page_id)
{
    LGC_VALIDATE_PTR(ctx);

    LgcDisplayAdapter_t *adapter = (LgcDisplayAdapter_t *)ctx;

    if (!adapter->is_initialized)
    {
        return ERR_NOT_INITIALIZED;
    }

    dwin_error_t res = dwin_page_jump(&adapter->dwin, page_id);

    return (res == DWIN_OK) ? ERR_OK : ERR_HARDWARE_FAULT;
}

/**
 * @brief Attach button callback
 */
static Result_t display_attach_callback_impl(
    void *ctx,
    LgcDisplayCallback_t callback,
    void *user_ctx)
{
    LGC_VALIDATE_PTR(ctx);

    LgcDisplayAdapter_t *adapter = (LgcDisplayAdapter_t *)ctx;

    adapter->event_callback = callback;
    adapter->event_user_ctx = user_ctx;

    return ERR_OK;
}

/**
 * @brief Detach button callback
 */
static Result_t display_detach_callback_impl(void *ctx)
{
    LGC_VALIDATE_PTR(ctx);

    LgcDisplayAdapter_t *adapter = (LgcDisplayAdapter_t *)ctx;

    adapter->event_callback = NULL;
    adapter->event_user_ctx = NULL;

    return ERR_OK;
}

/**
 * @brief Process display events
 */
static Result_t display_process_impl(void *ctx)
{
    LGC_VALIDATE_PTR(ctx);

    LgcDisplayAdapter_t *adapter = (LgcDisplayAdapter_t *)ctx;

    if (!adapter->is_initialized)
    {
        return ERR_NOT_INITIALIZED;
    }

    /* Process DWIN events (non-blocking) */
    dwin_process(&adapter->dwin);

    return ERR_OK;
}

/**
 * @brief Deinitialize display
 */
static Result_t display_deinit_impl(void *ctx)
{
    LGC_VALIDATE_PTR(ctx);

    LgcDisplayAdapter_t *adapter = (LgcDisplayAdapter_t *)ctx;

    if (!adapter->is_initialized)
    {
        return ERR_NOT_INITIALIZED;
    }

    /* Detach callback */
    adapter->event_callback = NULL;
    adapter->event_user_ctx = NULL;

    adapter->is_initialized = false;
    return ERR_OK;
}

/* ============================= Public API =========================== */

/** Global UART handle for DWIN (set by Init function) */
static UART_HandleTypeDef *g_display_uart = NULL;

Result_t LgcDisplayAdapter_Init(
    LgcDisplayAdapter_t *adapter,
    UART_HandleTypeDef *huart)
{
    LGC_VALIDATE_PTR(adapter);
    LGC_VALIDATE_PTR(huart);

    /* Clear adapter structure */
    memset(adapter, 0, sizeof(LgcDisplayAdapter_t));

    /* Store UART handle globally for DWIN HAL */
    adapter->huart = huart;
    g_display_uart = huart;

    /* Adapter initialized (DWIN protocol init called via display->init()) */
    return ERR_OK;
}

ILgcDisplay_t *LgcDisplayAdapter_GetInterface(LgcDisplayAdapter_t *adapter)
{
    if (adapter == NULL)
    {
        return NULL;
    }

    /* Static V-Table (interface) */
    static ILgcDisplay_t iface = {
        .context = NULL, /* Will be set below */
        .init = display_init_impl,
        .write_variable = display_write_variable_impl,
        .write_u16 = display_write_u16_impl,
        .write_u32 = display_write_u32_impl,
        .write_float = display_write_float_impl,
        .write_text = display_write_text_impl,
        .read_variable = display_read_variable_impl,
        .change_page = display_change_page_impl,
        .attach_callback = display_attach_callback_impl,
        .detach_callback = display_detach_callback_impl,
        .process = display_process_impl,
        .deinit = display_deinit_impl};

    /* Update context pointer */
    iface.context = adapter;

    return &iface;
}

Result_t LgcDisplayAdapter_Deinit(LgcDisplayAdapter_t *adapter)
{
    return display_deinit_impl(adapter);
}

void LgcDisplayAdapter_RxISRCallback(
    LgcDisplayAdapter_t *adapter,
    uint8_t *data,
    uint16_t len)
{
    if (adapter == NULL || data == NULL || len == 0)
    {
        return;
    }

    /* Push data to DWIN ring buffer (ISR-safe) */
    dwin_rx_push_ex(&adapter->dwin, data, len);

    /* Notify DWIN task (optional, for immediate processing) */
    dwin_rx_notify(&adapter->dwin);
}
