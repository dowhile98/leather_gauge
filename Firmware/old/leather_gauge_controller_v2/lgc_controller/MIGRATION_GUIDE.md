# Leather Gauge Controller - Migration Guide

## 📋 Middleware Migration Status

### ✅ Already Migrated to Third_Party:

- **lwpkt** - Lightweight Packet Protocol (LwPKT v1.5)
- **lwrb** - Lightweight Ring Buffer

### ⚠️ Pending Migration to Third_Party:

Copy the following from `leather_gauge_controller/middlewares/` to `lgc_controller/Third_Party/`:

#### 1. AT24Cxx EEPROM Driver (REQUIRED)

- **Source:** `leather_gauge_controller/middlewares/at24cxx/`
- **Destination:** `lgc_controller/Third_Party/at24cxx/`
- **Files:** `driver_at24cxx.c`, `driver_at24cxx.h`
- **Used by:** EEPROM Storage Adapter
- **Priority:** 🔴 CRITICAL (storage adapter depends on this)

#### 2. DWIN Display Driver (REQUIRED)

- **Source:** `leather_gauge_controller/middlewares/dwin/`
- **Destination:** `lgc_controller/Third_Party/dwin/`
- **Files:** `dwin_core.c`, `dwin_core.h`
- **Used by:** Display Adapter (HMI)
- **Priority:** 🔴 CRITICAL (HMI adapter depends on this)

#### 3. lwprintf - Lightweight printf (OPTIONAL)

- **Source:** `leather_gauge_controller/middlewares/lwprintf/`
- **Destination:** `lgc_controller/Third_Party/lwprintf/`
- **Used by:** Logging, debugging
- **Priority:** 🟡 MEDIUM (useful for development)

#### 4. stm32_log - Logging Utility (OPTIONAL)

- **Source:** `leather_gauge_controller/middlewares/stm32_log/`
- **Destination:** `lgc_controller/Third_Party/stm32_log/`
- **Used by:** Debug logging
- **Priority:** 🟡 MEDIUM

#### 5. nanoMODBUS (LEGACY FALLBACK)

- **Source:** `leather_gauge_controller/middlewares/nanoMODBUS/`
- **Destination:** `lgc_controller/Third_Party/nanomodbus/`
- **Files:** `nanomodbus.c`, `nanomodbus.h`
- **Used by:** Legacy Modbus adapter (fallback if LwPKT fails)
- **Priority:** 🟢 LOW (LwPKT is primary protocol)

#### 6. lwbtn - Lightweight Button (OPTIONAL)

- **Source:** `leather_gauge_controller/middlewares/lwbtn/`
- **Destination:** `lgc_controller/Third_Party/lwbtn/`
- **Used by:** DI adapter (if physical buttons present)
- **Priority:** 🟢 LOW

### ❌ Not Needed:

- **DSP_Biquad** - Only used in sensor firmware, not controller
- **lwrb** (duplicate) - Already migrated

---

## 🔧 Migration Steps

### Step 1: Copy Critical Middlewares

```bash
cd Firmware/leather_gauge_controller_v2

# AT24Cxx (EEPROM)
cp -r leather_gauge_controller/middlewares/at24cxx \
      lgc_controller/Third_Party/

# DWIN (Display)
cp -r leather_gauge_controller/middlewares/dwin \
      lgc_controller/Third_Party/
```

### Step 2: Update Include Paths

After copying, update `#include` paths in adapters:

**Before (legacy):**

```c
#include "driver_at24cxx.h"  // Searches in middlewares/
```

**After (clean):**

```c
#include "Third_Party/at24cxx/driver_at24cxx.h"  // Explicit path
```

### Step 3: Update Build System (Makefile/CMake)

Add Third_Party directories to include paths:

```makefile
C_INCLUDES += \
    -Ilgc_controller/Third_Party/at24cxx \
    -Ilgc_controller/Third_Party/dwin \
    -Ilgc_controller/Third_Party/lwpkt/src/include \
    -Ilgc_controller/Third_Party/lwrb/src/include
```

---

## 📦 Module to Adapter Migration Map

| Legacy Module      | New Adapter                             | Status         |
| ------------------ | --------------------------------------- | -------------- |
| `modules/encoder/` | `adapters/peripherals/encoder_adapter/` | ✅ **DONE**    |
| `modules/eeprom/`  | `adapters/storage/eeprom_adapter/`      | ✅ **DONE**    |
| `modules/modbus/`  | `adapters/communication/lwpkt_adapter/` | 🔄 In Progress |
| `modules/printer/` | `adapters/peripherals/printer_adapter/` | ⏳ TODO        |
| `modules/rtc/`     | `adapters/peripherals/rtc_adapter/`     | ⏳ TODO        |
| `modules/di/`      | `adapters/peripherals/di_adapter/`      | ⏳ TODO        |

---

## 🎯 Next Implementation Priorities

### Phase 1: Core Measurement (Week 3-4)

1. ✅ **Encoder Adapter** - COMPLETED
2. ✅ **EEPROM Adapter** - COMPLETED
3. 🔄 **LwPKT Adapter** - IN PROGRESS (stub exists)
   - Complete cascade read implementation
   - Add TX/RX handlers
   - Test with real sensors

### Phase 2: HMI (Week 5-6)

4. ⏳ **Display Adapter** (DWIN) - TODO
   - Read variable values (VP addresses)
   - Write updates (area, piece count)
   - Handle button events

### Phase 3: Reporting (Week 7-8)

5. ⏳ **Printer Adapter** - TODO
   - USB thermal printer commands
   - Batch report formatting

### Phase 4: Supporting (Week 9-10)

6. ⏳ **RTC Adapter** - TODO (timestamp for batches)
7. ⏳ **Digital Input Adapter** - TODO (if physical buttons)

---

## ⚠️ Breaking Changes from Legacy

### 1. Dependency Injection Required

**Legacy (direct call):**

```c
lgc_module_encoder_init(callback);  // Global function
```

**New (dependency injection):**

```c
ILgcEncoder_t *encoder = LgcEncoderAdapter_GetInterface();
encoder->init(encoder->context, &config);
encoder->attach_callback(encoder->context, callback, user_ctx);
```

### 2. No Global State

**Legacy:**

```c
extern LGC_CONF_TypeDef_t lgc_conf;  // Global mutable
```

**New:**

```c
// State encapsulated in adapters, accessed via interfaces
ILgcStorage_t *storage = /* ... */;
storage->load_config(storage->context, &local_config);
```

### 3. Result Codes Standardized

**Legacy:** `error_t` (custom values)  
**New:** `Result_t` (ERR_OK, ERR_ERROR, ERR_NULL_POINTER, etc.)

### 4. ThreadX API Wrappers

**Legacy:** Direct `TX_MUTEX`, `tx_mutex_get()`  
**New:** Still direct ThreadX in adapters (allowed), but domain uses abstract interfaces

---

## ✅ Validation Checklist

After migration, verify:

- [ ] No HAL headers in `domain/` layer (run grep check)
- [ ] All middlewares in `Third_Party/`, not `leather_gauge_controller/middlewares/`
- [ ] Adapters compile without warnings
- [ ] DI Container wires all adapters correctly
- [ ] Legacy `modules/` marked as DEPRECATED
- [ ] Build system includes correct paths
- [ ] No global mutable state in adapters (use context pointers)

---

## 📚 Reference

- **REFACTOR_PLAN.md** - Complete 12-week roadmap
- **STATUS.md** - Current implementation status
- **copilot-instructions.md** - Coding standards and architecture rules
- **SYSTEM_ARCHITECTURE.md** - Technical deep dive

---

**Last Updated:** 2026-02-12  
**Phase:** 2.2 (LwPKT Adapter Implementation)
