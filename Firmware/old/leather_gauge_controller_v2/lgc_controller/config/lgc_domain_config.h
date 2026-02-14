/**
 * @file    lgc_domain_config.h
 * @brief   Domain layer configuration constants
 * @author  Clean Architecture Refactor Team
 * @date    2026-02-12
 * @version 1.0.0
 *
 * @details Constants for business logic configuration.
 *          Hardware-independent values only.
 *
 * @note    This file is part of DOMAIN layer - NO hardware specifics here.
 *          For hardware config, see lgc_hardware_config.h in app layer.
 *
 * Copyright (c) 2026 Leather Gauge Controller Team
 */

#ifndef LGC_DOMAIN_CONFIG_H
#define LGC_DOMAIN_CONFIG_H

#ifdef __cplusplus
extern "C"
{
#endif

/* ============================= Measurement Constants ================ */
/**
 * @defgroup LGC_Domain_Config Domain Configuration
 * @{
 */

/** Encoder displacement per pulse (mm) */
#ifndef LGC_ENCODER_STEP_MM
#define LGC_ENCODER_STEP_MM 5U
#endif

/** Photocell spacing (mm) */
#ifndef LGC_PHOTOCELL_SPACING_MM
#define LGC_PHOTOCELL_SPACING_MM 10U
#endif

/** Minimum active bits per sensor to detect leather */
#ifndef LGC_DEFAULT_LEATHER_THRESHOLD
#define LGC_DEFAULT_LEATHER_THRESHOLD 2U
#endif

/** Consecutive empty slices to finalize piece */
#ifndef LGC_DEFAULT_HYSTERESIS
#define LGC_DEFAULT_HYSTERESIS 3U
#endif

/** Maximum pieces per batch before auto-finalization */
#ifndef LGC_DEFAULT_MAX_PIECES_PER_BATCH
#define LGC_DEFAULT_MAX_PIECES_PER_BATCH 100U
#endif

/** @} */

/* ============================= Timeout Configuration ================ */
/**
 * @defgroup LGC_Timeouts Operation Timeouts
 * @{
 */

/** Sensor read timeout per sensor (ms) */
#ifndef LGC_SENSOR_READ_TIMEOUT_MS
#define LGC_SENSOR_READ_TIMEOUT_MS 200U
#endif

/** Storage write timeout (ms) */
#ifndef LGC_STORAGE_WRITE_TIMEOUT_MS
#define LGC_STORAGE_WRITE_TIMEOUT_MS 100U
#endif

/** Display communication timeout (ms) */
#ifndef LGC_DISPLAY_TIMEOUT_MS
#define LGC_DISPLAY_TIMEOUT_MS 50U
#endif

/** Printer operation timeout (ms) */
#ifndef LGC_PRINTER_TIMEOUT_MS
#define LGC_PRINTER_TIMEOUT_MS 1000U
#endif

/** @} */

/* ============================= Unit Conversions ===================== */
/**
 * @defgroup LGC_Conversions Unit Conversion Factors
 * @{
 */

/** Convert dm² to m² */
#define LGC_DM2_TO_M2 0.01f

/** Convert dm² to cm² */
#define LGC_DM2_TO_CM2 100.0f

/** Convert dm² to ft² (approximate) */
#define LGC_DM2_TO_FT2 0.01076391f

/** @} */

/* ============================= Performance Targets ================== */
/**
 * @defgroup LGC_Performance Performance Targets
 * @{
 */

/** Maximum slice processing time (µs) */
#define LGC_MAX_SLICE_PROCESSING_TIME_US 500U

/** Maximum sensor read time for cascade mode (ms) */
#define LGC_MAX_CASCADE_READ_TIME_MS 600U

/** Maximum sensor read time for polling mode (ms) */
#define LGC_MAX_POLLING_READ_TIME_MS 2200U

    /** @} */

#ifdef __cplusplus
}
#endif

#endif /* LGC_DOMAIN_CONFIG_H */
