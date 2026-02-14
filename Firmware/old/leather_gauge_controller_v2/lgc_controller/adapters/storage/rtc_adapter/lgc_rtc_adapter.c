#include "lgc_rtc_adapter.h"
#include "rtc.h" // For MX_RTC_Init()
#include "string.h" // For memset

// Forward declarations for the interface functions
static Result_t rtc_init(void *ctx);
static Result_t rtc_get_datetime(void *ctx, LgcDateTime_t *out_datetime);
static Result_t rtc_set_datetime(void *ctx, const LgcDateTime_t *datetime);
static Result_t rtc_deinit_interface(void *ctx); // Renamed to avoid conflict with LgcRtcAdapter_Deinit

// Interface definition
static const ILgcRealTimeClock_t s_rtc_interface = {
    .context = NULL, // Will be set during LgcRtcAdapter_Init
    .init = rtc_init,
    .get_datetime = rtc_get_datetime,
    .set_datetime = rtc_set_datetime,
    .deinit = rtc_deinit_interface,
};

Result_t LgcRtcAdapter_Init(LgcRtcAdapter_t *adapter, RTC_HandleTypeDef *hrtc_handle) {
    if (adapter == NULL || hrtc_handle == NULL) {
        return ERR_NULL_POINTER;
    }

    memset(adapter, 0, sizeof(LgcRtcAdapter_t));
    adapter->hrtc = hrtc_handle;
    adapter->is_initialized = true;

    // Set the context for the interface
    ((ILgcRealTimeClock_t *)&s_rtc_interface)->context = adapter;

    return ERR_OK;
}

Result_t LgcRtcAdapter_Deinit(LgcRtcAdapter_t *adapter) {
    if (adapter == NULL) {
        return ERR_NULL_POINTER;
    }
    if (!adapter->is_initialized) {
        return ERR_NOT_INITIALIZED;
    }

    HAL_RTC_DeInit(adapter->hrtc); // Deinitialize the HAL RTC
    adapter->is_initialized = false;
    return ERR_OK;
}

const ILgcRealTimeClock_t *LgcRtcAdapter_GetInterface(LgcRtcAdapter_t *adapter) {
    if (adapter == NULL || !adapter->is_initialized) {
        return NULL;
    }
    // Ensure the context is correctly set for this instance
    ((ILgcRealTimeClock_t *)&s_rtc_interface)->context = adapter;
    return &s_rtc_interface;
}

static Result_t rtc_init(void *ctx) {
    LgcRtcAdapter_t *adapter = (LgcRtcAdapter_t *)ctx;
    if (adapter == NULL || adapter->hrtc == NULL) {
        return ERR_NULL_POINTER;
    }

    // Call the CubeMX-generated initialization function
    // This will configure the RTC and set a default time if the backup register is empty
    MX_RTC_Init();

    return ERR_OK;
}

static Result_t rtc_get_datetime(void *ctx, LgcDateTime_t *out_datetime) {
    LgcRtcAdapter_t *adapter = (LgcRtcAdapter_t *)ctx;
    if (adapter == NULL || adapter->hrtc == NULL || out_datetime == NULL) {
        return ERR_NULL_POINTER;
    }

    if (!adapter->is_initialized) {
        return ERR_NOT_INITIALIZED;
    }

    RTC_TimeTypeDef sTime = {0};
    RTC_DateTypeDef sDate = {0};

    // Get the current time and date
    // Note: HAL_RTC_GetTime must be called before HAL_RTC_GetDate to ensure coherency
    if (HAL_RTC_GetTime(adapter->hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK) {
        return ERR_HARDWARE_FAULT;
    }
    if (HAL_RTC_GetDate(adapter->hrtc, &sDate, RTC_FORMAT_BIN) != HAL_OK) {
        return ERR_HARDWARE_FAULT;
    }

    out_datetime->year = sDate.Year + 2000; // Assuming year 0 is 2000
    out_datetime->month = sDate.Month;
    out_datetime->day = sDate.Date;
    out_datetime->hour = sTime.Hours;
    out_datetime->minute = sTime.Minutes;
    out_datetime->second = sTime.Seconds;

    return ERR_OK;
}

static Result_t rtc_set_datetime(void *ctx, const LgcDateTime_t *datetime) {
    LgcRtcAdapter_t *adapter = (LgcRtcAdapter_t *)ctx;
    if (adapter == NULL || adapter->hrtc == NULL || datetime == NULL) {
        return ERR_NULL_POINTER;
    }

    if (!adapter->is_initialized) {
        return ERR_NOT_INITIALIZED;
    }

    RTC_TimeTypeDef sTime = {0};
    RTC_DateTypeDef sDate = {0};

    sTime.Hours = datetime->hour;
    sTime.Minutes = datetime->minute;
    sTime.Seconds = datetime->second;
    sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    sTime.StoreOperation = RTC_STOREOPERATION_RESET;

    sDate.Year = datetime->year - 2000; // Convert to 0-99 format
    sDate.Month = datetime->month;
    sDate.Date = datetime->day;
    // WeekDay is not part of LgcDateTime_t, so use a default or calculate if needed
    sDate.WeekDay = RTC_WEEKDAY_MONDAY; // Default to Monday for now.
                                         // In a real application, this might be calculated
                                         // or passed as part of an extended LgcDateTime_t.

    if (HAL_RTC_SetTime(adapter->hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK) {
        return ERR_HARDWARE_FAULT;
    }
    if (HAL_RTC_SetDate(adapter->hrtc, &sDate, RTC_FORMAT_BIN) != HAL_OK) {
        return ERR_HARDWARE_FAULT;
    }

    // Write a magic number to backup register to indicate RTC has been set
    HAL_RTCEx_BKUPWrite(adapter->hrtc, RTC_BKP_DR1, 0x32F2);

    return ERR_OK;
}

static Result_t rtc_deinit_interface(void *ctx) {
    LgcRtcAdapter_t *adapter = (LgcRtcAdapter_t *)ctx;
    if (adapter == NULL) {
        return ERR_NULL_POINTER;
    }
    if (!adapter->is_initialized) {
        return ERR_NOT_INITIALIZED;
    }

    return LgcRtcAdapter_Deinit(adapter);
}
