/**
 * @file    lg_i_lwpkt.h
 * @brief   LwPKT Codec Interface (DIP abstraction)
 * @author  TDD Agent (c-pro mode)
 * @date    2026-02-09
 *
 * @note    This interface enforces Dependency Inversion Principle.
 *          Core domain NEVER includes lwpkt.h directly.
 */

#ifndef LG_I_LWPKT_H
#define LG_I_LWPKT_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h> // For NULL
#include "lg_domain_types.h"

/* ============================================================================
 * Type Definitions
 * ========================================================================= */

/**
 * @brief LwPKT packet encoder/decoder interface.
 * @note  Must be reentrant (no internal state in interface methods).
 */
typedef struct ILwPktCodec
{
    /**
     * @brief Encode a command packet into a buffer.
     *
     * @param[in]  from      Source address (1-11 for sensors, 0xFF for master).
     * @param[in]  to        Destination address (0=broadcast, 1-11=specific).
     * @param[in]  cmd       Command ID (see lg_cmd_t).
     * @param[in]  flags     FLAGS field (0 if not used, or sensor# for cascade).
     * @param[in]  payload   Payload buffer (may be NULL if len=0).
     * @param[in]  len       Payload length in bytes (max 255).
     * @param[out] out_buf   Output buffer for encoded frame.
     * @param[in]  out_size  Size of output buffer (must be >= len+overhead).
     * @param[out] out_len   Actual encoded frame length.
     *
     * @return LG_OK on success.
     * @return LG_INVALID_PARAM if arguments are NULL or invalid.
     * @return LG_ERROR if buffer is too small.
     *
     * @pre out_buf must not be NULL, out_size >= (len + 10).
     * @post If LG_OK, out_buf contains valid LwPKT frame with CRC.
     */
    lg_result_t (*Encode)(
        uint8_t from,
        uint8_t to,
        uint8_t cmd,
        uint32_t flags,
        const uint8_t *payload,
        uint16_t len,
        uint8_t *out_buf,
        uint16_t out_size,
        uint16_t *out_len);

    /**
     * @brief Decode a received LwPKT frame.
     *
     * @param[in]  frame       Raw frame buffer (must not be NULL).
     * @param[in]  frame_len   Frame length in bytes.
     * @param[out] from        Parsed source address.
     * @param[out] to          Parsed destination address.
     * @param[out] cmd         Parsed command ID.
     * @param[out] flags       Parsed FLAGS field (0 if not present).
     * @param[out] payload     Pointer to payload within frame (NOT copied).
     * @param[out] len         Payload length.
     *
     * @return LG_OK on success.
     * @return LG_INVALID_PARAM if frame or output pointers are NULL.
     * @return LG_ERROR if CRC validation fails or frame is malformed.
     *
     * @pre frame must not be NULL.
     * @post If LG_OK, *payload points to data within frame buffer.
     *
     * @warning Do NOT modify *payload; it references internal buffer.
     */
    lg_result_t (*Decode)(
        const uint8_t *frame,
        uint16_t frame_len,
        uint8_t *from,
        uint8_t *to,
        uint8_t *cmd,
        uint32_t *flags,
        const uint8_t **payload,
        uint16_t *len);
} ILwPktCodec_t;

/* ============================================================================
 * Inline Wrapper Functions (Enforce interface usage, prevent direct calls)
 * ========================================================================= */

/**
 * @brief Encode packet (wrapper with validation).
 * @note  Prevents direct function pointer calls, enforces NULL checks.
 */
static inline lg_result_t LgLwPkt_Encode(
    const ILwPktCodec_t *codec,
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
    if (codec == NULL || codec->Encode == NULL)
    {
        return LG_ERROR;
    }
    return codec->Encode(from, to, cmd, flags, payload, len, out_buf, out_size, out_len);
}

/**
 * @brief Decode packet (wrapper with validation).
 */
static inline lg_result_t LgLwPkt_Decode(
    const ILwPktCodec_t *codec,
    const uint8_t *frame,
    uint16_t frame_len,
    uint8_t *from,
    uint8_t *to,
    uint8_t *cmd,
    uint32_t *flags,
    const uint8_t **payload,
    uint16_t *len)
{
    if (codec == NULL || codec->Decode == NULL)
    {
        return LG_ERROR;
    }
    return codec->Decode(frame, frame_len, from, to, cmd, flags, payload, len);
}

#endif // LG_I_LWPKT_H
