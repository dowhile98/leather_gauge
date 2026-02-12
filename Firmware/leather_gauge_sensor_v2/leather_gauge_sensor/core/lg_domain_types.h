#ifndef LG_DOMAIN_TYPES_H
#define LG_DOMAIN_TYPES_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Common result codes for the domain
 */
typedef enum
{
    LG_OK = 0,
    LG_ERROR,
    LG_BUSY,
    LG_TIMEOUT,
    LG_INVALID_PARAM
} lg_result_t;

/**
 * @brief Configuration Structure (Entity)
 */
typedef struct
{
    uint8_t address;    /* Modbus/LwPKT Address */
    uint32_t baudrate;  /* UART Baudrate */
    float fc;           /* Filter Cutoff Frequency */
    uint16_t threshold; /* Digital Threshold */
    float offset[10];   /* Per-channel offset (assuming up to 10 channels based on old code) */
} lg_config_t;

/**
 * @brief Sensor Data Structure (Entity)
 */
typedef struct
{
    uint16_t raw[10];       /* Raw ADC values */
    float filtered[10];     /* Filtered values */
    float calibrated[10];   /* Values with offset applied */
    uint16_t digital_state; /* Bitmask of threshold states */
} lg_sensor_data_t;

/**
 * @brief Communication Packet (Entity)
 */
typedef struct
{
    uint8_t cmd;        /* Command ID */
    uint8_t data[256];  /* Payload */
    uint16_t len;       /* Payload Length */
    uint32_t from_addr; /* Sender Address */
    uint32_t flags;     /* 🆕 LwPKT flags (for cascade control) */
} lg_comm_packet_t;

/**
 * @brief LwPKT Commands (aligned with protocol spec)
 */
typedef enum
{
    /* Read Commands */
    CMD_READ_SENSOR = 0x10,      /**< Request sensor value (no payload) - Single read */
    CMD_READ_SENSOR_RESP = 0x90, /**< Response with sensor data */
    CMD_READ_RAW = 0x11,         /**< Read raw ADC value - Single read */
    CMD_READ_RAW_RESP = 0x91,    /**< Raw ADC response */

    /* 🆕 Cascade Read Commands (Broadcast with FLAGS) */
    CMD_READ_CASCADE = 0x12,      /**< Cascade read: FLAGS indicates which sensor responds */
    CMD_READ_CASCADE_RESP = 0x92, /**< Response + FLAGS for next sensor */

    /* Write/Config Commands */
    CMD_WRITE_CONFIG = 0x20,      /**< Write configuration */
    CMD_WRITE_CONFIG_RESP = 0xA0, /**< Config write ACK/NACK */
    CMD_SET_OFFSET = 0x21,        /**< Set calibration offset */
    CMD_SET_FILTER = 0x22,        /**< Set filter parameters */

    /* Control Commands */
    CMD_CALIBRATE = 0x30,       /**< Trigger calibration sequence */
    CMD_CALIBRATE_RESP = 0xB0,  /**< Calibration result */
    CMD_GET_STATUS = 0x31,      /**< Request device status */
    CMD_GET_STATUS_RESP = 0xB1, /**< Status response */

    /* Error/Special */
    CMD_ERROR = 0xFF, /**< Error response (1B error code) */
} lg_cmd_t;

#endif // LG_DOMAIN_TYPES_H
