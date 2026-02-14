/**
 * @file lgc_modbus_fsm_types.h
 * @brief Type definitions for Modbus Asynchronous FSM
 *
 * Defines the state machine states, events, and configuration for
 * the asynchronous Modbus communication task.
 *
 * @note Part of Clean Architecture Domain Layer - NO HAL dependencies allowed
 *
 * @date Created: Feb 13, 2026
 * @author GitHub Copilot
 */

#ifndef DOMAIN_INTERFACES_LGC_MODBUS_FSM_TYPES_H_
#define DOMAIN_INTERFACES_LGC_MODBUS_FSM_TYPES_H_

/* ============================================================================
 * INCLUDES
 * ============================================================================ */
#include <stdint.h>
#include <stdbool.h>

/* ============================================================================
 * CONFIGURATION DEFAULTS
 * ============================================================================ */

/** @brief Default timeout waiting for sensor response (ms) */
#ifndef LGC_MODBUS_RX_TIMEOUT_MS
#define LGC_MODBUS_RX_TIMEOUT_MS 15
#endif

/** @brief Default time between read cycles (ms) */
#ifndef LGC_MODBUS_CYCLE_PERIOD_MS
#define LGC_MODBUS_CYCLE_PERIOD_MS 10
#endif

/** @brief Maximum consecutive failures before marking sensor offline */
#ifndef LGC_MODBUS_MAX_FAILURES
#define LGC_MODBUS_MAX_FAILURES 3
#endif

/** @brief Inter-frame silence time T3.5 at 38400 bps (~1ms) */
#ifndef LGC_MODBUS_T35_DELAY_MS
#define LGC_MODBUS_T35_DELAY_MS 2
#endif

/** @brief Number of sensors to poll */
#ifndef LGC_MODBUS_SENSOR_COUNT
#define LGC_MODBUS_SENSOR_COUNT 11
#endif

/* ============================================================================
 * FSM STATE DEFINITIONS
 * ============================================================================ */

/**
 * @brief Modbus FSM States
 *
 * State machine for non-blocking sequential sensor polling.
 */
typedef enum
{
    MB_STATE_IDLE = 0x00,     /**< Waiting for trigger (encoder/timer) */
    MB_STATE_SEND_REQ = 0x01, /**< Sending Modbus request via DMA */
    MB_STATE_WAIT_RX = 0x02,  /**< Waiting for response (semaphore pend) */
    MB_STATE_PARSE = 0x03,    /**< Parsing received PDU */
    MB_STATE_NEXT = 0x04,     /**< Advance to next sensor or complete */
    MB_STATE_ERROR = 0x05,    /**< Error recovery state */
    MB_STATE_COUNT            /**< Number of states (for validation) */
} LgcModbusFsmState_t;

/**
 * @brief FSM Events
 */
typedef enum
{
    MB_EVENT_NONE = 0x00,        /**< No event pending */
    MB_EVENT_START_CYCLE = 0x01, /**< Begin new read cycle */
    MB_EVENT_TX_COMPLETE = 0x02, /**< DMA TX finished */
    MB_EVENT_RX_COMPLETE = 0x04, /**< UART IDLE line detected */
    MB_EVENT_TIMEOUT = 0x08,     /**< Response timeout */
    MB_EVENT_ERROR = 0x10,       /**< Communication error */
    MB_EVENT_ABORT = 0x20,       /**< Abort current cycle */
} LgcModbusFsmEvent_t;

/**
 * @brief Sensor error tracking per device
 */
typedef struct
{
    uint8_t consecutive_failures; /**< Failure counter (reset on success) */
    uint8_t is_offline;           /**< Marked offline after MAX_FAILURES */
    uint32_t last_success_ms;     /**< Timestamp of last successful read */
} LgcSensorErrorTracker_t;

/**
 * @brief FSM Runtime Context
 */
typedef struct
{
    LgcModbusFsmState_t state;  /**< Current state */
    uint8_t current_sensor_idx; /**< Sensor being polled (0-10) */
    uint8_t sensors_completed;  /**< Count of completed reads */
    uint32_t cycle_start_ms;    /**< Cycle start timestamp */
    uint32_t state_entry_ms;    /**< State entry timestamp */
    LgcSensorErrorTracker_t
        error_tracker[LGC_MODBUS_SENSOR_COUNT]; /**< Per-sensor error tracking */
} LgcModbusFsmContext_t;

/**
 * @brief FSM Configuration
 */
typedef struct
{
    uint16_t rx_timeout_ms;        /**< Per-sensor response timeout */
    uint16_t cycle_period_ms;      /**< Minimum time between cycles */
    uint8_t max_failures;          /**< Failures before offline marking */
    uint8_t t35_delay_ms;          /**< Inter-frame silence */
    uint16_t modbus_address_start; /**< First sensor Modbus address */
    uint16_t modbus_register;      /**< Register to read (default: 45) */
} LgcModbusFsmConfig_t;

/**
 * @brief FSM Statistics
 */
typedef struct
{
    uint32_t total_cycles;       /**< Total completed read cycles */
    uint32_t cycles_skipped;     /**< Cycles skipped (HMI sensor test active) */
    uint32_t total_timeouts;     /**< Total timeout events */
    uint32_t total_crc_errors;   /**< Total CRC failures */
    uint32_t last_cycle_time_ms; /**< Duration of last complete cycle */
    uint32_t min_cycle_time_ms;  /**< Minimum cycle time recorded */
    uint32_t max_cycle_time_ms;  /**< Maximum cycle time recorded */
} LgcModbusFsmStats_t;

/**
 * @brief Default configuration initializer
 */
#define LGC_MODBUS_FSM_CONFIG_DEFAULT {            \
    .rx_timeout_ms = LGC_MODBUS_RX_TIMEOUT_MS,     \
    .cycle_period_ms = LGC_MODBUS_CYCLE_PERIOD_MS, \
    .max_failures = LGC_MODBUS_MAX_FAILURES,       \
    .t35_delay_ms = LGC_MODBUS_T35_DELAY_MS,       \
    .modbus_address_start = 1,                     \
    .modbus_register = 45}

#endif /* DOMAIN_INTERFACES_LGC_MODBUS_FSM_TYPES_H_ */
