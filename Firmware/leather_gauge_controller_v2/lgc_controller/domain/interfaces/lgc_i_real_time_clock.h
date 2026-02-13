#ifndef LGC_I_REAL_TIME_CLOCK_H
#define LGC_I_REAL_TIME_CLOCK_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lgc_common_types.h" // For Result_t, LgcDateTime_t

/**
 * @brief Real-Time Clock Interface (Port)
 */
typedef struct {
    void *context; // Opaque context pointer for implementation

    /**
     * @brief Initialize the RTC module.
     * @param[in] ctx Opaque context pointer.
     * @return ERR_OK on success, error code otherwise.
     */
    Result_t (*init)(void *ctx);

    /**
     * @brief Get current date and time.
     * @param[in] ctx Opaque context pointer.
     * @param[out] out_datetime Pointer to store the date and time.
     * @return ERR_OK on success, error code otherwise.
     */
    Result_t (*get_datetime)(void *ctx, LgcDateTime_t *out_datetime);

    /**
     * @brief Set current date and time.
     * @param[in] ctx Opaque context pointer.
     * @param[in] datetime Pointer to the date and time to set.
     * @return ERR_OK on success, error code otherwise.
     */
    Result_t (*set_datetime)(void *ctx, const LgcDateTime_t *datetime);

    /**
     * @brief Deinitialize the RTC module.
     * @param[in] ctx Opaque context pointer.
     * @return ERR_OK on success, error code otherwise.
     */
    Result_t (*deinit)(void *ctx);

} ILgcRealTimeClock_t;


#ifdef __cplusplus
}
#endif

#endif // LGC_I_REAL_TIME_CLOCK_H
