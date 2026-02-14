#ifndef LGC_RTC_ADAPTER_H
#define LGC_RTC_ADAPTER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../domain/interfaces/lgc_i_real_time_clock.h"
#include "stm32f4xx_hal.h" // Required for RTC_HandleTypeDef

/**
 * @brief RTC Adapter Context structure.
 *        This structure holds the internal state and HAL handle for the RTC adapter.
 */
typedef struct {
    RTC_HandleTypeDef *hrtc;  // Pointer to the HAL RTC handle
    bool is_initialized;
    // Add other internal state variables as needed
} LgcRtcAdapter_t;

/**
 * @brief Initialize the RTC Adapter.
 *
 * @param[in,out] adapter Pointer to the RTC adapter instance.
 * @param[in] hrtc Pointer to the HAL RTC handle (e.g., &hrtc).
 * @return ERR_OK if initialization is successful, an error code otherwise.
 */
Result_t LgcRtcAdapter_Init(LgcRtcAdapter_t *adapter, RTC_HandleTypeDef *hrtc);

/**
 * @brief Deinitialize the RTC Adapter.
 *
 * @param[in,out] adapter Pointer to the RTC adapter instance.
 * @return ERR_OK if deinitialization is successful, an error code otherwise.
 */
Result_t LgcRtcAdapter_Deinit(LgcRtcAdapter_t *adapter);

/**
 * @brief Get the IRealTimeClock interface from the RTC Adapter instance.
 *
 * @param[in,out] adapter Pointer to the RTC adapter instance.
 * @return Pointer to the ILgcRealTimeClock_t interface, or NULL if the adapter is not initialized.
 */
const ILgcRealTimeClock_t *LgcRtcAdapter_GetInterface(LgcRtcAdapter_t *adapter);

#ifdef __cplusplus
}
#endif

#endif // LGC_RTC_ADAPTER_H
