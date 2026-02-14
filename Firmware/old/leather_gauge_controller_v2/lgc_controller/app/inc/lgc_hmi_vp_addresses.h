/**
 * @file    lgc_hmi_vp_addresses.h
 * @brief   DWIN Display VP (Variable Pointer) Address Definitions
 * @author  Clean Architecture Refactor Team (TDD)
 * @date    2026-02-12
 * @version 1.0.0
 *
 * @details Centralized definition of all DWIN display variable addresses.
 *          Eliminates magic numbers and provides documentation for each variable.
 *
 *          **VP Address Mapping:**
 *          - Page 1: Main measurement screen
 *          - Page 2: Settings/configuration screen
 *          - Page 3-4: Sensor testing screens
 *          - Page 10: Configuration editor
 *
 * @note    Based on "Lista de variables.xlsx" from DWIN project
 *
 * Copyright (c) 2026 Leather Gauge Controller Team
 */

#ifndef LGC_HMI_VP_ADDRESSES_H
#define LGC_HMI_VP_ADDRESSES_H

#ifdef __cplusplus
extern "C"
{
#endif

/* ============================= Includes ============================= */
#include <stdint.h>

    /* ============================= Page Definitions ===================== */
    /**
     * @brief DWIN Screen Page Numbers
     */
    typedef enum
    {
        LGC_HMI_PAGE_SPLASH = 0,       /**< Splash screen (startup) */
        LGC_HMI_PAGE_MAIN = 1,         /**< Main measurement screen */
        LGC_HMI_PAGE_SETTINGS = 2,     /**< Settings menu */
        LGC_HMI_PAGE_SENSOR_TEST = 3,  /**< Sensor test screen (part 1) */
        LGC_HMI_PAGE_SENSOR_TEST2 = 4, /**< Sensor test screen (part 2) */
        LGC_HMI_PAGE_CONFIG = 10,      /**< Configuration editor */
        LGC_HMI_PAGE_MAX = 24          /**< Maximum page number */
    } LgcHmiPage_t;

    /* ============================= System State (Page 1) ================= */
    /**
     * @brief System state indicator
     * @page  1 (Main screen)
     * @type  Icon Display (Data Variable)
     * @values 0=Stopped, 1=Running, 2=Paused
     */
#define VP_STATE 0x1110U

    /**
     * @brief Speed indicator icon
     * @page  1 (Main screen)
     * @type  Icon Display (Data Variable)
     * @values 0=Slow, 1=Medium, 2=Fast
     */
#define VP_ICON_SPEED 0x1111U

    /**
     * @brief Motor feedback status
     * @page  1 (Main screen)
     * @type  Icon Display (Data Variable)
     * @values 0=OFF, 1=ON
     */
#define VP_FEEDBACK_MOTOR 0x1112U

    /* ============================= Measurement Data (Page 1) ============ */
    /**
     * @brief Current batch number
     * @page  1 (Main screen)
     * @type  Data Variable Display (uint16_t)
     * @values 0-9999
     * @unit  None (counter)
     */
#define VP_BATCH_COUNT 0x1050U

    /**
     * @brief Current leather piece count in batch
     * @page  1 (Main screen)
     * @type  Data Variable Display (uint16_t)
     * @values 0-9999
     * @unit  pieces
     * @note  Resets when batch finished
     */
#define VP_LEATHER_COUNT 0x1051U

    /**
     * @brief Current leather area (piece being measured)
     * @page  1 (Main screen)
     * @type  Data Variable Display (uint16_t)
     * @values 0-65535 (area × 100)
     * @unit  dm² (decimeters squared, displayed as XX.XX)
     * @note  Value sent as integer: actual_area × 100
     *        Example: 1.25 dm² → send 125
     */
#define VP_CURRENT_AREA 0x1060U

    /**
     * @brief Accumulated area for current batch
     * @page  1 (Main screen)
     * @type  Data Variable Display (uint16_t)
     * @values 0-65535 (area × 100)
     * @unit  dm² (sum of all pieces in batch)
     * @note  Value sent as integer: accumulated × 100
     */
#define VP_ACCUMULATED_AREA 0x1080U

    /* ============================= Sensor Test (Pages 3-4) ============== */
    /**
     * @brief Selected sensor ID for testing
     * @page  3, 4 (Sensor test screens)
     * @type  Icon Display (Data Variable)
     * @values 1-11 (sensor number)
     */
#define VP_TEST_CHOSEN_SENSOR 0x1101U

    /**
     * @brief Sensor photodiode bit states (test mode)
     * @page  3, 4 (Sensor test screens)
     * @type  Data Variable Display (uint16_t bitmap)
     * @values Bits 0-9: photodiode states (1=active, 0=inactive)
     * @note  Display shows 10 indicators for 10 photocells
     */
#define VP_TEST_BIT_SENSOR 0x1104U

    /**
     * @brief Sensor threshold slider (test mode)
     * @page  3, 4 (Sensor test screens)
     * @type  Slider (Data Variable)
     * @values 0-1023 (ADC threshold)
     * @note  Adjusts detection sensitivity
     */
#define VP_TEST_SLIDER_THRESHOLD 0x1108U

    /**
     * @brief Sensor threshold numeric display (test mode)
     * @page  3, 4 (Sensor test screens)
     * @type  Data Variable Display (uint16_t)
     * @values 0-1023 (mirrors slider value)
     */
#define VP_TEST_NUMBER_THRESHOLD 0x1109U

    /* ============================= Pattern Selection (Page 2) =========== */
    /**
     * @brief Pattern 3048 selection icon
     * @page  2 (Settings)
     * @type  Icon Display (Data Variable)
     * @values 0=Not selected, 1=Selected
     */
#define VP_PATTERN_ICON_3048 0x121AU

    /**
     * @brief Pattern 3048 selection button
     * @page  2 (Settings)
     * @type  Return Key Code Button
     * @values Button press returns key code
     */
#define VP_PATTERN_BTN_3048 0x121BU

    /**
     * @brief Pattern 3000 selection icon
     * @page  2 (Settings)
     * @type  Icon Display (Data Variable)
     * @values 0=Not selected, 1=Selected
     */
#define VP_PATTERN_ICON_3000 0x122AU

    /**
     * @brief Pattern 3000 selection button
     * @page  2 (Settings)
     * @type  Return Key Code Button
     * @values Button press returns key code
     */
#define VP_PATTERN_BTN_3000 0x122BU

    /**
     * @brief Pattern 2800 selection icon
     * @page  2 (Settings)
     * @type  Icon Display (Data Variable)
     * @values 0=Not selected, 1=Selected
     */
#define VP_PATTERN_ICON_2800 0x123AU

    /**
     * @brief Pattern 2800 selection button
     * @page  2 (Settings)
     * @type  Return Key Code Button
     * @values Button press returns key code
     */
#define VP_PATTERN_BTN_2800 0x123BU

    /* ============================= Configuration (Page 10) ============== */
    /**
     * @brief Client name text field
     * @page  10 (Config editor)
     * @type  Text Display (ASCII)
     * @values Max 12 characters (0x0C length)
     * @note  Null-terminated string
     */
#define VP_CONFIG_NAME_CLIENT 0x1310U

    /**
     * @brief Leather color text field
     * @page  10 (Config editor)
     * @type  Text Display (ASCII)
     * @values Max 12 characters (0x0C length)
     */
#define VP_CONFIG_NAME_COLOR 0x1320U

    /**
     * @brief Leather ID text field
     * @page  10 (Config editor)
     * @type  Text Display (ASCII)
     * @values Max 12 characters (0x0C length)
     */
#define VP_CONFIG_NAME_LEATHER 0x1330U

    /**
     * @brief Maximum pieces per batch
     * @page  10 (Config editor)
     * @type  Data Variable Display (uint16_t)
     * @values 1-9999
     * @note  Triggers batch finished event when reached
     */
#define VP_CONFIG_BATCH_SIZE 0x1340U

    /* ============================= Date Configuration (Page 10) ========= */
    /**
     * @brief Day configuration
     * @page  10 (Config editor)
     * @type  Data Variable Display (uint16_t)
     * @values 1-31
     */
#define VP_CONFIG_DAY 0x1341U

    /**
     * @brief Month configuration
     * @page  10 (Config editor)
     * @type  Data Variable Display (uint16_t)
     * @values 1-12
     */
#define VP_CONFIG_MONTH 0x1342U

    /**
     * @brief Year configuration
     * @page  10 (Config editor)
     * @type  Data Variable Display (uint16_t)
     * @values 2000-2099
     */
#define VP_CONFIG_YEAR 0x1343U

    /* ============================= Time Configuration (Page 10) ========= */
    /**
     * @brief Hour configuration
     * @page  10 (Config editor)
     * @type  Data Variable Display (uint16_t)
     * @values 0-23
     */
#define VP_CONFIG_HOUR 0x1346U

    /**
     * @brief Minute configuration
     * @page  10 (Config editor)
     * @type  Data Variable Display (uint16_t)
     * @values 0-59
     */
#define VP_CONFIG_MINUTE 0x1347U

    /**
     * @brief Second configuration
     * @page  10 (Config editor)
     * @type  Data Variable Display (uint16_t)
     * @values 0-59
     */
#define VP_CONFIG_SECOND 0x1348U

    /* ============================= Units Configuration (Page 10) ======== */
    /**
     * @brief Area units selection
     * @page  10 (Config editor)
     * @type  Data Variable Display (uint16_t)
     * @values 0=dm² (metric), 1=ft² (imperial)
     */
#define VP_CONFIG_UNITS 0x1350U

    /* ============================= Commands (All Pages) ================= */
    /**
     * @brief Save configuration command
     * @page  10 (Config editor)
     * @type  Return Key Code Button
     * @values Button press triggers save
     * @note  Write any value to trigger save operation
     */
#define VP_CONFIG_SAVE_CMD 0x1002U

    /**
     * @brief Save result status
     * @page  10 (Config editor)
     * @type  Data Variable Display (uint16_t)
     * @values 0=Idle, 1=Success, 2=Failed
     * @note  Firmware writes result after save operation
     */
#define VP_CONFIG_SAVE_RESULT 0x1003U

    /**
     * @brief Print batch button
     * @page  1 (Main screen)
     * @type  Return Key Code Button
     * @values Button press triggers print
     * @note  Closes current batch and sends to printer
     */
#define VP_PRINT 0x1400U

    /**
     * @brief Delete last measurement button
     * @page  1 (Main screen)
     * @type  Return Key Code Button
     * @values Button press triggers delete
     * @note  Removes last leather piece from current batch
     */
#define VP_LIST_DELETE 0x1501U

    /* ============================= Helper Macros ======================== */
    /**
     * @brief Convert float area to DWIN uint16_t (×100)
     * @param area_dm2 Area in dm² (float)
     * @return uint16_t value to send to display (area × 100)
     */
#define VP_AREA_TO_UINT16(area_dm2) ((uint16_t)((area_dm2) * 100.0f))

    /**
     * @brief Convert DWIN uint16_t to float area (÷100)
     * @param value uint16_t from display
     * @return Area in dm² (float)
     */
#define VP_UINT16_TO_AREA(value) ((float)(value) / 100.0f)

#ifdef __cplusplus
}
#endif

#endif /* LGC_HMI_VP_ADDRESSES_H */
