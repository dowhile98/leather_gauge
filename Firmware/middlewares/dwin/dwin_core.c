/**
 * @file dwin_core.c
 * @brief Implementation of the DWIN T5L/DGUS II Driver.
 * @details Includes robust FSM parsing, Ring Buffer management, and optional ACK support.
 */

#include "dwin_core.h"
#include <string.h>

/* Utility Macros for Endianness (Big Endian) */
#define DWIN_U16_TO_BE(v) (uint16_t)(((v) << 8) | ((v) >> 8))

/*---------------------------------------------------------------------------*/
/* Private Helper Functions                                                  */
/*---------------------------------------------------------------------------*/

static void _dwin_lock(dwin_t *hdl)
{
    if (hdl->iface.lock)
        hdl->iface.lock();
}

static void _dwin_unlock(dwin_t *hdl)
{
    if (hdl->iface.unlock)
        hdl->iface.unlock();
}

/**
 * @brief Generic wait for response (ACK or Data).
 * Uses Semaphore if available, otherwise falls back to polling with timeout.
 */
static dwin_error_t _dwin_wait_response(dwin_t *hdl, volatile bool *flag, uint32_t timeout_ms)
{
    dwin_error_t ret = DWIN_ERROR_TIMEOUT;

    // Use RTOS Semaphore if provided
    if (hdl->iface.sem_wait)
    {
        if (hdl->iface.sem_wait(timeout_ms))
        {
            // Signal received, check if the flag was cleared (indicating success)
            if (*flag == false)
                ret = DWIN_OK;
        }
    }
    // Fallback to polling (Bare-metal or simple ticks)
    else
    {
        uint32_t start = hdl->iface.get_tick_ms();
        while ((hdl->iface.get_tick_ms() - start) < timeout_ms)
        {
            /*process for non RTOS*/
            dwin_process(hdl);

            if (*flag == false)
            {
                ret = DWIN_OK;
                break;
            }
        }
    }

    // Cleanup on timeout
    if (ret != DWIN_OK)
    {
        *flag = false;
    }
    return ret;
}

static void _dwin_handle_frame(dwin_t *hdl, uint8_t *payload, uint8_t len)
{
    if (len < 1)
        return;

    uint8_t cmd = payload[0];

    /* ---------------------------------------------------------------------- */
    /* Special Case: Write Confirmation "OK" (0x82 0x4F 0x4B)                 */
    /* The screen replies with 0x82 0x4F('O') 0x4B('K') if write was success. */
    /* ---------------------------------------------------------------------- */
    if (cmd == DWIN_CMD_WRITE_VP && len == 3 && payload[1] == 0x4F && payload[2] == 0x4B)
    {
#if DWIN_WAIT_FOR_WRITE_RESPONSE
        if (hdl->waiting_ack)
        {
            hdl->waiting_ack = false;
            if (hdl->iface.sem_signal)
                hdl->iface.sem_signal();
        }
#endif
        // Do NOT propagate this as a user event; it's internal protocol signaling.
        return;
    }

    /* ---------------------------------------------------------------------- */
    /* Case: Variable Read Response (0x83)                                    */
    /* ---------------------------------------------------------------------- */
    if (cmd == DWIN_CMD_READ_VP)
    {
        if (len < 3)
            return;

        uint16_t vp = (payload[1] << 8) | payload[2];
        uint8_t len_data = payload[3]; // Length in WORDS
        uint8_t *raw_data = &payload[4];
        uint8_t raw_len_bytes = len - 4;

        /* 1. Check for synchronous Read Transaction */
        if (hdl->waiting_response && hdl->pending_read_addr == vp)
        {
            size_t copy_len = (raw_len_bytes < hdl->response_dest_len) ? raw_len_bytes : hdl->response_dest_len;
            if (hdl->response_dest_buf)
            {
                memcpy(hdl->response_dest_buf, raw_data, copy_len);
            }
            hdl->waiting_response = false;
            if (hdl->iface.sem_signal)
                hdl->iface.sem_signal();
        }
        /* 2. Dispatch Async Callback (e.g. unsolicited events) */
        else if (hdl->event_callback)
        {
            dwin_evt_t evt;
            evt.cmd = cmd;
            evt.addr = vp;
            evt.len = len_data;
            evt.data = raw_data;
            evt.data_len = raw_len_bytes;
            hdl->event_callback(&evt, hdl->user_ctx);
        }
    }
    /* ---------------------------------------------------------------------- */
    /* Case: Spontaneous Write Notification (0x82)                            */
    /* ---------------------------------------------------------------------- */
    else if (cmd == DWIN_CMD_WRITE_VP)
    {
        if (len < 3)
            return;
        uint16_t vp = (payload[1] << 8) | payload[2];

        if (hdl->event_callback)
        {
            dwin_evt_t evt;
            evt.cmd = cmd;
            evt.addr = vp;
            evt.len = (len - 3) / 2;
            evt.data = &payload[3];
            evt.data_len = len - 3;
            hdl->event_callback(&evt, hdl->user_ctx);
        }
    }
}

/*---------------------------------------------------------------------------*/
/* Core API Implementation                                                   */
/*---------------------------------------------------------------------------*/

dwin_error_t dwin_init(dwin_t *hdl, const dwin_interface_t *iface, uint8_t *buffer, size_t buffer_size)
{
    if (!hdl || !iface || !buffer || buffer_size == 0)
        return DWIN_ERROR_PARAM;

    memset(hdl, 0, sizeof(dwin_t));
    hdl->iface = *iface;
    hdl->rx_fifo_buf = buffer;
    hdl->rx_fifo_size = buffer_size;
    hdl->parse_state = DWIN_STATE_WAIT_H;

    return DWIN_OK;
}

void dwin_register_callback(dwin_t *hdl, dwin_event_cb_t cb, void *user_ctx)
{
    if (hdl)
    {
        hdl->event_callback = cb;
        hdl->user_ctx = user_ctx;
    }
}

void dwin_rx_push_ex(dwin_t *hdl, uint8_t *data, size_t len)
{
    for (size_t i = 0; i < len; i++)
    {
        dwin_rx_push(hdl, data[i]);
    }
}

void dwin_rx_notify(dwin_t *hdl)
{
    if (hdl->iface.sem_new_data_signal)
    {
        hdl->iface.sem_new_data_signal();
    }
}

void dwin_rx_push(dwin_t *hdl, uint8_t data)
{
    if (!hdl)
        return;

    size_t next_head = (hdl->rx_head + 1) % hdl->rx_fifo_size;
    if (next_head != hdl->rx_tail)
    {
        hdl->rx_fifo_buf[hdl->rx_head] = data;
        hdl->rx_head = next_head;
    }
}

void dwin_process(dwin_t *hdl)
{
    if (!hdl)
        return;

    // Block waiting for new data if callback is provided
    if (hdl->rx_tail == hdl->rx_head && hdl->iface.sem_new_data_wait)
    {
        hdl->iface.sem_new_data_wait();
    }

    while (hdl->rx_tail != hdl->rx_head)
    {
        uint8_t byte = hdl->rx_fifo_buf[hdl->rx_tail];
        hdl->rx_tail = (hdl->rx_tail + 1) % hdl->rx_fifo_size;

        switch (hdl->parse_state)
        {
        case DWIN_STATE_WAIT_H:
            if (byte == DWIN_FRAME_HEADER_H)
                hdl->parse_state = DWIN_STATE_WAIT_L;
            break;
        case DWIN_STATE_WAIT_L:
            if (byte == DWIN_FRAME_HEADER_L)
                hdl->parse_state = DWIN_STATE_LEN;
            else
                hdl->parse_state = (byte == DWIN_FRAME_HEADER_H) ? DWIN_STATE_WAIT_L : DWIN_STATE_WAIT_H;
            break;
        case DWIN_STATE_LEN:
            hdl->rx_expected_len = byte;
            hdl->rx_frame_idx = 0;
            if (hdl->rx_expected_len > DWIN_MAX_PAYLOAD_LEN)
                hdl->parse_state = DWIN_STATE_WAIT_H;
            else
                hdl->parse_state = DWIN_STATE_PAYLOAD;
            break;
        case DWIN_STATE_PAYLOAD:
            hdl->rx_frame_buf[hdl->rx_frame_idx++] = byte;
            if (hdl->rx_frame_idx >= hdl->rx_expected_len)
            {
                _dwin_handle_frame(hdl, hdl->rx_frame_buf, hdl->rx_expected_len);
                hdl->parse_state = DWIN_STATE_WAIT_H;
            }
            break;
        }
    }
}

dwin_error_t dwin_write_vp_u16(dwin_t *hdl, uint16_t vp_addr, uint16_t value)
{
    uint8_t frame[8];
    frame[0] = DWIN_FRAME_HEADER_H;
    frame[1] = DWIN_FRAME_HEADER_L;
    frame[2] = 0x05;
    frame[3] = DWIN_CMD_WRITE_VP;
    frame[4] = (vp_addr >> 8) & 0xFF;
    frame[5] = vp_addr & 0xFF;
    frame[6] = (value >> 8) & 0xFF;
    frame[7] = value & 0xFF;

    dwin_error_t ret = DWIN_OK;
    _dwin_lock(hdl);
#if DWIN_WAIT_FOR_WRITE_RESPONSE
    hdl->waiting_ack = true;
#endif
    hdl->iface.uart_transmit(frame, 8);
#if DWIN_WAIT_FOR_WRITE_RESPONSE
    ret = _dwin_wait_response(hdl, &hdl->waiting_ack, 100); // 100ms timeout for ACK
#endif
    _dwin_unlock(hdl);
    return ret;
}

dwin_error_t dwin_write_vp_u32(dwin_t *hdl, uint16_t vp_addr, uint32_t value)
{
    uint8_t frame[10];
    frame[0] = DWIN_FRAME_HEADER_H;
    frame[1] = DWIN_FRAME_HEADER_L;
    frame[2] = 0x07;
    frame[3] = DWIN_CMD_WRITE_VP;
    frame[4] = (vp_addr >> 8) & 0xFF;
    frame[5] = vp_addr & 0xFF;
    frame[6] = (value >> 24) & 0xFF;
    frame[7] = (value >> 16) & 0xFF;
    frame[8] = (value >> 8) & 0xFF;
    frame[9] = value & 0xFF;

    dwin_error_t ret = DWIN_OK;
    _dwin_lock(hdl);
#if DWIN_WAIT_FOR_WRITE_RESPONSE
    hdl->waiting_ack = true;
#endif
    hdl->iface.uart_transmit(frame, 10);
#if DWIN_WAIT_FOR_WRITE_RESPONSE
    ret = _dwin_wait_response(hdl, &hdl->waiting_ack, 100);
#endif
    _dwin_unlock(hdl);
    return ret;
}

dwin_error_t dwin_write_vp_raw(dwin_t *hdl, uint16_t vp_addr, uint8_t *data, uint16_t len)
{
    if (len > (DWIN_MAX_PAYLOAD_LEN - 3))
        return DWIN_ERROR_PARAM;

    uint8_t header[6];
    header[0] = DWIN_FRAME_HEADER_H;
    header[1] = DWIN_FRAME_HEADER_L;
    header[2] = 3 + len;
    header[3] = DWIN_CMD_WRITE_VP;
    header[4] = (vp_addr >> 8) & 0xFF;
    header[5] = vp_addr & 0xFF;

    dwin_error_t ret = DWIN_OK;
    _dwin_lock(hdl);
#if DWIN_WAIT_FOR_WRITE_RESPONSE
    hdl->waiting_ack = true;
#endif
    hdl->iface.uart_transmit(header, 6);
    hdl->iface.uart_transmit(data, len);
#if DWIN_WAIT_FOR_WRITE_RESPONSE
    ret = _dwin_wait_response(hdl, &hdl->waiting_ack, 100);
#endif
    _dwin_unlock(hdl);
    return ret;
}

dwin_error_t dwin_read_vp(dwin_t *hdl, uint16_t vp_addr, uint8_t len_words, uint8_t *dest_buf, uint32_t timeout_ms)
{
    if (!hdl || !dest_buf)
        return DWIN_ERROR_PARAM;

    uint8_t frame[7];
    frame[0] = DWIN_FRAME_HEADER_H;
    frame[1] = DWIN_FRAME_HEADER_L;
    frame[2] = 0x04;
    frame[3] = DWIN_CMD_READ_VP;
    frame[4] = (vp_addr >> 8) & 0xFF;
    frame[5] = vp_addr & 0xFF;
    frame[6] = len_words;

    _dwin_lock(hdl);
    hdl->pending_read_addr = vp_addr;
    hdl->response_dest_buf = dest_buf;
    hdl->response_dest_len = len_words * 2;
    hdl->waiting_response = true;

    hdl->iface.uart_transmit(frame, 7);

    // Reuse the generic wait function
    dwin_error_t ret = _dwin_wait_response(hdl, &hdl->waiting_response, timeout_ms);

    if (ret != DWIN_OK)
    {
        hdl->response_dest_buf = NULL;
    }
    _dwin_unlock(hdl);
    return ret;
}

/* System Utils wrappers... */
dwin_error_t dwin_page_jump(dwin_t *hdl, uint16_t page_id)
{
    uint32_t payload = (0x5A << 24) | (0x01 << 16) | page_id;
    return dwin_write_vp_u32(hdl, DWIN_SYS_PIC_SET, payload);
}

dwin_error_t dwin_write_text(dwin_t *hdl, uint16_t vp_addr, const char *text)
{
    if (!text)
        return DWIN_ERROR_PARAM;
    size_t len = strlen(text);
    return dwin_write_vp_raw(hdl, vp_addr, (uint8_t *)text, (uint16_t)len);
}

dwin_error_t dwin_read_text(dwin_t *hdl, uint16_t vp_addr, char *buf, uint16_t max_len, uint32_t timeout_ms)
{
    if (!buf || max_len == 0)
        return DWIN_ERROR_PARAM;
    uint8_t words_to_read = max_len / 2;
    if (words_to_read == 0)
        words_to_read = 1;
    if (words_to_read > 0x7F)
        words_to_read = 0x7F;

    dwin_error_t err = dwin_read_vp(hdl, vp_addr, words_to_read, (uint8_t *)buf, timeout_ms);
    if (err != DWIN_OK)
        return err;

    buf[max_len - 1] = '\0';
    for (int i = 0; i < (words_to_read * 2); i++)
    {
        if ((unsigned char)buf[i] == 0xFF)
        {
            buf[i] = '\0';
            break;
        }
    }
    return DWIN_OK;
}

dwin_error_t dwin_read_u16(dwin_t *hdl, uint16_t vp_addr, uint16_t *value, uint32_t timeout_ms)
{
    uint8_t raw[2];
    dwin_error_t err = dwin_read_vp(hdl, vp_addr, 1, raw, timeout_ms);
    if (err == DWIN_OK)
    {
        *value = (raw[0] << 8) | raw[1];
    }
    return err;
}

dwin_error_t dwin_read_u32(dwin_t *hdl, uint16_t vp_addr, uint32_t *value, uint32_t timeout_ms)
{
    uint8_t raw[4];
    dwin_error_t err = dwin_read_vp(hdl, vp_addr, 2, raw, timeout_ms);
    if (err == DWIN_OK)
    {
        *value = ((uint32_t)raw[0] << 24) | ((uint32_t)raw[1] << 16) | ((uint32_t)raw[2] << 8) | raw[3];
    }
    return err;
}

dwin_error_t dwin_get_page(dwin_t *hdl, uint16_t *page_id)
{
    return dwin_read_u16(hdl, DWIN_SYS_PIC_NOW, page_id, 200);
}

dwin_error_t dwin_beep(dwin_t *hdl, uint16_t duration_ms)
{
    uint16_t ticks = duration_ms / 10;
    if (ticks == 0 && duration_ms > 0)
        ticks = 1;
    return dwin_write_vp_u16(hdl, DWIN_SYS_BUZZER, ticks);
}

dwin_error_t dwin_set_brightness(dwin_t *hdl, uint8_t percent)
{
    if (percent > 100)
        percent = 100;
    // Set BOTH Standby and Active brightness to the same value to avoid dimming issues
    uint16_t val = (percent << 8) | percent;
    return dwin_write_vp_u16(hdl, DWIN_SYS_LED_CFG, val);
}

dwin_error_t dwin_get_brightness(dwin_t *hdl, uint8_t *percent)
{
    uint16_t raw;
    dwin_error_t err = dwin_read_u16(hdl, DWIN_SYS_LED_CFG, &raw, 200);
    if (err == DWIN_OK)
    {
        *percent = (uint8_t)(raw & 0xFF);
    }
    return err;
}

dwin_error_t dwin_get_version(dwin_t *hdl, uint16_t *version)
{
    return dwin_read_u16(hdl, DWIN_SYS_VERSION, version, 200);
}

dwin_error_t dwin_get_rtc(dwin_t *hdl, dwin_rtc_t *rtc)
{
    if (!rtc)
        return DWIN_ERROR_PARAM;
    uint8_t raw[8];
    dwin_error_t err = dwin_read_vp(hdl, DWIN_SYS_RTC_NOW, 4, raw, 300);

    if (err == DWIN_OK)
    {
        rtc->year = raw[0];
        rtc->month = raw[1];
        rtc->day = raw[2];
        rtc->week = raw[3];
        rtc->hour = raw[4];
        rtc->minute = raw[5];
        rtc->second = raw[6];
    }
    return err;
}

dwin_error_t dwin_set_rtc(dwin_t *hdl, const dwin_rtc_t *rtc)
{
    if (!rtc)
        return DWIN_ERROR_PARAM;
    uint8_t buffer[8];
    buffer[0] = rtc->year;
    buffer[1] = rtc->month;
    buffer[2] = rtc->day;
    buffer[3] = rtc->week;
    buffer[4] = rtc->hour;
    buffer[5] = rtc->minute;
    buffer[6] = rtc->second;
    buffer[7] = 0x00; // Padding
    return dwin_write_vp_raw(hdl, DWIN_SYS_RTC_SET, buffer, 8);
}

dwin_error_t dwin_soft_reset(dwin_t *hdl)
{
    return dwin_write_vp_u32(hdl, DWIN_SYS_RESET, 0x55AA5AA5);
}
