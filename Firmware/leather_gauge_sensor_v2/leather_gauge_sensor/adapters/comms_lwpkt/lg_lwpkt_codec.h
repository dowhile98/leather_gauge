/**
 * @file    lg_lwpkt_codec.h
 * @brief   LwPKT Codec Implementation (Adapter Layer)
 * @author  TDD Agent (c-pro mode)
 * @date    2026-02-09
 *
 * @note    This file bridges lg_i_lwpkt.h interface with lwpkt.h library.
 *          It enforces DIP: Core domain uses lg_i_lwpkt.h, NOT lwpkt.h.
 */

#ifndef LG_LWPKT_CODEC_H
#define LG_LWPKT_CODEC_H

#include "lg_i_lwpkt.h"

/**
 * @brief Get the LwPKT codec interface implementation.
 *
 * @return ILwPktCodec_t* Pointer to interface (static singleton).
 *
 * @note  This is the ONLY public function. Core domain calls this
 *        to get the codec, never includes lwpkt.h directly.
 */
ILwPktCodec_t *LgLwPktCodec_GetInterface(void);

#endif // LG_LWPKT_CODEC_H
