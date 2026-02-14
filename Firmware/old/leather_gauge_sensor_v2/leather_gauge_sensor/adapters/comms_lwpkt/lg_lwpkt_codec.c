/**
 * @file    lg_lwpkt_codec.c
 * @brief   LwPKT Codec Implementation (GREEN phase of TDD)
 * @author  TDD Agent (c-pro mode)
 * @date    2026-02-09
 *
 * @note    Implements ILwPktCodec_t interface using lwpkt library.
 *          This is the ONLY file that includes lwpkt.h in the comm adapter.
 */

#include "lg_lwpkt_codec.h"
#include "lwpkt/lwpkt.h"
#include "lwrb/lwrb.h"
#include <string.h>

/* ============================================================================
 * Private Macros
 * ========================================================================= */
#define CODEC_TX_BUFFER_SIZE 512 /**< Static TX buffer for encoding */
#define CODEC_RX_BUFFER_SIZE 512 /**< Static RX buffer for decoding */

/* ============================================================================
 * Private Variables (Static Allocation Only)
 * ========================================================================= */
static lwpkt_t s_pkt_instance;
static lwrb_t s_tx_ringbuf;
static lwrb_t s_rx_ringbuf;
static uint8_t s_tx_buf_data[CODEC_TX_BUFFER_SIZE];
static uint8_t s_rx_buf_data[CODEC_RX_BUFFER_SIZE];
static bool s_is_initialized = false;

/* ============================================================================
 * Private Function Prototypes
 * ========================================================================= */
static lg_result_t codec_encode(
    uint8_t from, uint8_t to, uint8_t cmd, uint32_t flags,
    const uint8_t *payload, uint16_t len,
    uint8_t *out_buf, uint16_t out_size, uint16_t *out_len);

static lg_result_t codec_decode(
    const uint8_t *frame, uint16_t frame_len,
    uint8_t *from, uint8_t *to, uint8_t *cmd, uint32_t *flags,
    const uint8_t **payload, uint16_t *len);

static void codec_init_once(void);

/* ============================================================================
 * Interface Definition (Static Singleton)
 * ========================================================================= */
static ILwPktCodec_t s_codec_interface = {
    .Encode = codec_encode,
    .Decode = codec_decode};

/* ============================================================================
 * Public API
 * ========================================================================= */

ILwPktCodec_t *LgLwPktCodec_GetInterface(void)
{
    if (!s_is_initialized)
    {
        codec_init_once();
    }
    return &s_codec_interface;
}

/* ============================================================================
 * Private Functions
 * ========================================================================= */

/**
 * @brief One-time initialization of static buffers and LwPKT instance.
 * @note  Called automatically on first GetInterface() call.
 */
static void codec_init_once(void)
{
    // Initialize ring buffers
    lwrb_init(&s_tx_ringbuf, s_tx_buf_data, sizeof(s_tx_buf_data));
    lwrb_init(&s_rx_ringbuf, s_rx_buf_data, sizeof(s_rx_buf_data));

    // Initialize LwPKT instance (no callbacks needed for codec-only mode)
    lwpkt_init(&s_pkt_instance, &s_tx_ringbuf, &s_rx_ringbuf);

    s_is_initialized = true;
}

/**
 * @brief Encode a packet using LwPKT library.
 */
static lg_result_t codec_encode(
    uint8_t from,
    uint8_t to,
    uint8_t cmd,
    uint32_t flags,
    const uint8_t *payload,
    uint16_t len,
    uint8_t *out_buf,
    uint16_t out_size,
    uint16_t *out_len)
{
    // Validate inputs
    if (out_buf == NULL || out_len == NULL)
    {
        return LG_INVALID_PARAM;
    }

    if (len > 0 && payload == NULL)
    {
        return LG_INVALID_PARAM;
    }

    // Check buffer size (estimate: len + 12 bytes overhead with flags)
    if (out_size < (len + 12))
    {
        return LG_ERROR;
    }

    // Clear TX ring buffer
    lwrb_reset(&s_tx_ringbuf);

    // Set source address (lwpkt uses this internally)
    lwpkt_set_addr(&s_pkt_instance, from);

    // Prepare LwPKT packet
    // Parameters (with FLAGS enabled): pkt, to, flags, cmd, data, len
    lwpktr_t res = lwpkt_write(
        &s_pkt_instance,
        to,    // destination address
        flags, // 🆕 FLAGS field (for cascade control)
        cmd,   // command ID
        payload,
        len);

    if (res != lwpktOK)
    {
        return LG_ERROR;
    }

    // Read encoded data from ring buffer
    size_t available = lwrb_get_full(&s_tx_ringbuf);
    if (available == 0 || available > out_size)
    {
        return LG_ERROR;
    }

    *out_len = (uint16_t)lwrb_read(&s_tx_ringbuf, out_buf, available);

    return LG_OK;
}

/**
 * @brief Decode a received frame using LwPKT library.
 */
static lg_result_t codec_decode(
    const uint8_t *frame,
    uint16_t frame_len,
    uint8_t *from,
    uint8_t *to,
    uint8_t *cmd,
    uint32_t *flags,
    const uint8_t **payload,
    uint16_t *len)
{
    // Validate inputs
    if (frame == NULL || from == NULL || to == NULL ||
        cmd == NULL || flags == NULL || payload == NULL || len == NULL)
    {
        return LG_INVALID_PARAM;
    }

    if (frame_len == 0)
    {
        return LG_INVALID_PARAM;
    }

    // Clear RX ring buffer
    lwrb_reset(&s_rx_ringbuf);

    // Write raw frame to RX buffer
    size_t written = lwrb_write(&s_rx_ringbuf, frame, frame_len);
    if (written != frame_len)
    {
        return LG_ERROR;
    }

    // Process the packet (validate CRC, parse fields)
    lwpktr_t res = lwpkt_process(&s_pkt_instance, 0); // timestamp not critical for decode
    if (res != lwpktOK && res != lwpktINPROG)
    {
        return LG_ERROR;
    }

    // Read decoded packet
    lwpktr_t read_res = lwpkt_read(&s_pkt_instance);
    if (read_res != lwpktVALID)
    {
        return LG_ERROR; // CRC fail or malformed
    }

    // Extract fields using LwPKT API macros
    *from = (uint8_t)lwpkt_get_from_addr(&s_pkt_instance);
    *to = (uint8_t)lwpkt_get_to_addr(&s_pkt_instance);
    *cmd = (uint8_t)lwpkt_get_cmd(&s_pkt_instance);
    *flags = lwpkt_get_flags(&s_pkt_instance); // 🆕 Extract FLAGS field
    *payload = (const uint8_t *)lwpkt_get_data(&s_pkt_instance);
    *len = (uint16_t)lwpkt_get_data_len(&s_pkt_instance);

    return LG_OK;
}
