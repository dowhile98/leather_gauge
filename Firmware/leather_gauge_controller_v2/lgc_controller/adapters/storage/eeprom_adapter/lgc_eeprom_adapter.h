/**
 * @file    lgc_eeprom_adapter.h
 * @brief   EEPROM Storage Adapter - Interface
 * @author  Clean Architecture Refactor Team (TDD)
 * @date    2026-02-12
 * @version 1.0.0
 *
 * @details Implements ILgcStorage_t using AT24Cxx EEPROM via I2C.
 *          Uses CRC32 (IEEE 802.3) for configuration validation.
 *
 * @note    ADAPTER LAYER - Can include HAL and middleware headers
 *
 * Copyright (c) 2026 Leather Gauge Controller Team
 */

#ifndef LGC_EEPROM_ADAPTER_H
#define LGC_EEPROM_ADAPTER_H

#ifdef __cplusplus
extern "C"
{
#endif

/* ============================= Includes ============================= */
#include "../../../domain/interfaces/lgc_i_storage.h"

    /* ============================= Public API =========================== */
    /**
     * @brief Get EEPROM Storage Adapter Interface
     *
     * @return const ILgcStorage_t* Pointer to singleton interface instance
     *
     * @note  Uses AT24C256 EEPROM (32KB) via I2C2
     * @note  Address: 0xA0 (A2=A1=A0=0)
     *
     * @usage
     * @code
     * // In DI Container:
     * ILgcStorage_t *storage = LgcEepromAdapter_GetInterface();
     * Result_t res = storage->init(storage->context, &config);
     * @endcode
     */
    const ILgcStorage_t *LgcEepromAdapter_GetInterface(void);

#ifdef __cplusplus
}
#endif

#endif /* LGC_EEPROM_ADAPTER_H */
