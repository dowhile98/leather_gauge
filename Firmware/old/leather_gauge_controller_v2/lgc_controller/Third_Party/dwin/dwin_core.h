/**
 * @file dwin_core.h
 * @brief Professional driver for DWIN HMI Displays (T5L/DGUS II Protocol).
 * @details This library implements a decoupled, event-driven architecture optimized
 * for embedded systems (RTOS/Bare-metal). It supports ring-buffer ingestion,
 * state-machine parsing, and async/sync command modes.
 * * @author AI Assistant (based on tecna-smart-lab)
 * @date 2025
 * @version 2.0.0
 */

#ifndef DWIN_CORE_H_
#define DWIN_CORE_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/*---------------------------------------------------------------------------*/
/* Feature Configuration                                                     */
/*---------------------------------------------------------------------------*/

/**
 * @brief Enable synchronous waiting for Write Confirmation ("OK").
 * - 0: Asynchronous write ("Fire and forget"). Faster, non-blocking.
 * - 1: Synchronous write. Waits for 0x4F 0x4B response from the screen. Safer.
 */
#ifndef DWIN_WAIT_FOR_WRITE_RESPONSE
#define DWIN_WAIT_FOR_WRITE_RESPONSE 1
#endif

/*---------------------------------------------------------------------------*/
/* T5L/DGUS II System Addresses                                              */
/*---------------------------------------------------------------------------*/
#define DWIN_SYS_RESET 0x0004   // Software Reset
#define DWIN_SYS_VERSION 0x000F // Hardware/Firmware Version
#define DWIN_SYS_RTC_NOW 0x0010 // Read Current RTC
#define DWIN_SYS_PIC_NOW 0x0014 // Current Page ID (Read Only)
#define DWIN_SYS_LED_CFG 0x0082 // Backlight Configuration (Standby + Active)
#define DWIN_SYS_PIC_SET 0x0084 // Page Switch
#define DWIN_SYS_RTC_SET 0x009C // RTC Setting
#define DWIN_SYS_BUZZER 0x00A0  // Buzzer Control

/* General Configuration */
#define DWIN_FRAME_HEADER_H 0x5A
#define DWIN_FRAME_HEADER_L 0xA5
#ifndef DWIN_MAX_PAYLOAD_LEN
#define DWIN_MAX_PAYLOAD_LEN 64 // Adjust based on RAM availability
#endif
#define DWIN_CMD_WRITE_VP 0x82
#define DWIN_CMD_READ_VP 0x83

    /*---------------------------------------------------------------------------*/
    /* Data Types & Structures                                                   */
    /*---------------------------------------------------------------------------*/

    /**
     * @brief DWIN Operation Results
     */
    typedef enum
    {
        DWIN_OK = 0,
        DWIN_ERROR_TIMEOUT,
        DWIN_ERROR_FULL,
        DWIN_ERROR_PARAM,
        DWIN_ERROR_COMM,
        DWIN_ERROR_CRC,
        DWIN_ERROR_ACK // Error if no "OK" confirmation received
    } dwin_error_t;

    /**
     * @brief Event structure for received data.
     */
    typedef struct
    {
        uint8_t cmd;       // Command (e.g., 0x83 Read)
        uint16_t addr;     // Variable Address (VP)
        uint8_t len;       // Data length (in Words/Bytes depending on context)
        uint8_t *data;     // Pointer to raw data
        uint16_t data_len; // Length in bytes of 'data'
    } dwin_evt_t;

    /**
     * @brief Callback prototype for asynchronous events.
     */
    typedef void (*dwin_event_cb_t)(dwin_evt_t *evt, void *user_ctx);

    /**
     * @brief Hardware Abstraction Layer (HAL) Interface.
     * Pass this structure during initialization to decouple the driver from the hardware.
     */
    typedef struct
    {
        /**
         * @brief Transmit bytes via UART.
         * @return Number of bytes written.
         */
        uint32_t (*uart_transmit)(uint8_t *data, uint16_t len);

        /**
         * @brief Get system time in milliseconds (Required for timeouts).
         */
        uint32_t (*get_tick_ms)(void);

        /**
         * @brief (Optional) Mutex Lock for thread-safe transmission.
         */
        void (*lock)(void);

        /**
         * @brief (Optional) Mutex Unlock.
         */
        void (*unlock)(void);

        /**
         * @brief (Optional) Wait for a signal/semaphore.
         * @param timeout_ms Maximum wait time.
         * @return true if signaled, false on timeout.
         */
        bool (*sem_wait)(uint32_t timeout_ms);

        /**
         * @brief (Optional) Signal/Release the semaphore.
         */
        void (*sem_signal)(void);

        /**
         * @brief (Optional) Block task until new data arrives in RX buffer (Optimization).
         */
        void (*sem_new_data_wait)(void);

        /**
         * @brief (Optional) Signal new data arrived (Called from ISR).
         */
        void (*sem_new_data_signal)(void);

    } dwin_interface_t;

    /**
     * @brief Main DWIN Instance Handler.
     * @note Internal members should not be accessed directly.
     */
    typedef struct
    {
        /* Public Configuration */
        dwin_interface_t iface;
        dwin_event_cb_t event_callback;
        void *user_ctx;

        /* Internal Ring Buffer */
        uint8_t *rx_fifo_buf;
        size_t rx_fifo_size;
        volatile size_t rx_head;
        volatile size_t rx_tail;

        /* Frame Parser State Machine */
        enum
        {
            DWIN_STATE_WAIT_H,
            DWIN_STATE_WAIT_L,
            DWIN_STATE_LEN,
            DWIN_STATE_PAYLOAD
        } parse_state;

        uint8_t rx_frame_buf[DWIN_MAX_PAYLOAD_LEN];
        uint16_t rx_frame_idx;
        uint8_t rx_expected_len;

        /* Transaction Synchronization */
        volatile bool waiting_response; // For Read commands (0x83)
        volatile bool waiting_ack;      // For Write commands (0x82 -> 0x4F4B)
        uint16_t pending_read_addr;
        uint8_t *response_dest_buf;
        uint8_t response_dest_len;
    } dwin_t;

    /**
     * @brief RTC Date/Time Structure
     */
    typedef struct
    {
        uint8_t year;   // 0-99 (20xx)
        uint8_t month;  // 1-12
        uint8_t day;    // 1-31
        uint8_t week;   // 0-6 (Sunday-Saturday, optional)
        uint8_t hour;   // 0-23
        uint8_t minute; // 0-59
        uint8_t second; // 0-59
    } dwin_rtc_t;

    /*---------------------------------------------------------------------------*/
    /* Core API                                                                  */
    /*---------------------------------------------------------------------------*/

    /**
     * @brief Initialize the DWIN driver instance.
     * @param hdl Pointer to the DWIN handler.
     * @param iface Pointer to the interface configuration (HAL).
     * @param buffer Pointer to a memory block for the internal Ring Buffer.
     * @param buffer_size Size of the buffer.
     * @return DWIN_OK on success.
     */
    dwin_error_t dwin_init(dwin_t *hdl, const dwin_interface_t *iface, uint8_t *buffer, size_t buffer_size);

    /**
     * @brief Register a global callback for asynchronous events (e.g., Touch notifications).
     */
    void dwin_register_callback(dwin_t *hdl, dwin_event_cb_t cb, void *user_ctx);

    /**
     * @brief Main processing function. Call this from a dedicated Task or Main Loop.
     * @details This function parses the Ring Buffer and dispatches events.
     */
    void dwin_process(dwin_t *hdl);

    /**
     * @brief Push a single byte into the RX Ring Buffer.
     * @note Safe to call from ISR.
     */
    void dwin_rx_push(dwin_t *hdl, uint8_t data);

    /**
     * @brief Push an array of bytes into the RX Ring Buffer (e.g., DMA buffer).
     * @note Safe to call from ISR.
     */
    void dwin_rx_push_ex(dwin_t *hdl, uint8_t *data, size_t len);

    /**
     * @brief Notify the processing task that new data is available.
     * @note Call this from ISR after pushing data to wake up the task immediately.
     */
    void dwin_rx_notify(dwin_t *hdl);

    /* Basic Commands */
    dwin_error_t dwin_write_vp_u16(dwin_t *hdl, uint16_t vp_addr, uint16_t value);
    dwin_error_t dwin_write_vp_u32(dwin_t *hdl, uint16_t vp_addr, uint32_t value);
    dwin_error_t dwin_write_vp_raw(dwin_t *hdl, uint16_t vp_addr, uint8_t *data, uint16_t len);
    dwin_error_t dwin_read_vp(dwin_t *hdl, uint16_t vp_addr, uint8_t len_words, uint8_t *dest_buf, uint32_t timeout_ms);
    dwin_error_t dwin_page_jump(dwin_t *hdl, uint16_t page_id);

    /*---------------------------------------------------------------------------*/
    /* System Utilities API                                                      */
    /*---------------------------------------------------------------------------*/

    dwin_error_t dwin_get_page(dwin_t *hdl, uint16_t *page_id);
    dwin_error_t dwin_beep(dwin_t *hdl, uint16_t duration_ms);
    dwin_error_t dwin_set_brightness(dwin_t *hdl, uint8_t percent);
    dwin_error_t dwin_get_brightness(dwin_t *hdl, uint8_t *percent);
    dwin_error_t dwin_get_version(dwin_t *hdl, uint16_t *version);
    dwin_error_t dwin_get_rtc(dwin_t *hdl, dwin_rtc_t *rtc);
    dwin_error_t dwin_set_rtc(dwin_t *hdl, const dwin_rtc_t *rtc);
    dwin_error_t dwin_soft_reset(dwin_t *hdl);

    /*---------------------------------------------------------------------------*/
    /* Data Utilities API                                                        */
    /*---------------------------------------------------------------------------*/

    /**
     * @brief Write a text string to a VP Address.
     * Automatically handles padding and termination required by DWIN.
     */
    dwin_error_t dwin_write_text(dwin_t *hdl, uint16_t vp_addr, const char *text);

    /**
     * @brief Read a text string from a VP Address.
     * Automatically strips DWIN padding (0xFF).
     */
    dwin_error_t dwin_read_text(dwin_t *hdl, uint16_t vp_addr, char *buf, uint16_t max_len, uint32_t timeout_ms);

    dwin_error_t dwin_read_u16(dwin_t *hdl, uint16_t vp_addr, uint16_t *value, uint32_t timeout_ms);
    dwin_error_t dwin_read_u32(dwin_t *hdl, uint16_t vp_addr, uint32_t *value, uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif

#endif /* DWIN_CORE_H_ */
