# Domain Layer - Business Logic Core

## 📋 Overview

The **Domain Layer** contains the **pure business logic** of the Leather Gauge Controller. This layer is:

- ✅ **Hardware-independent** (can run on PC for testing)
- ✅ **Framework-independent** (no RTOS, no HAL)
- ✅ **Testable** (100% unit test coverage target)
- ✅ **Stable** (rarely changes - only when business rules change)

---

## 🏗️ Structure

```
domain/
├── entities/              Pure data structures
├── use_cases/            Business rules (algorithms)
└── interfaces/           Ports (abstractions for adapters)
```

---

## 📦 Entities (Data Structures)

**Purpose:** Define business objects and value types.

### Files:

#### [lgc_common_types.h](entities/lgc_common_types.h)

- `Result_t` - Error codes
- `LgcSystemState_t` - System states
- `LgcUnit_t` - Measurement units
- `LgcDateTime_t` - Timestamp structure
- Constants: `LGC_SENSOR_COUNT`, `LGC_MAX_PIECES_PER_BATCH`, etc.

#### [lgc_sensor_array_entity.h](entities/lgc_sensor_array_entity.h)

- `LgcSensorReading_t` - Single sensor (10 photocells as bitmask)
- `LgcSensorArray_t` - Complete array (11 sensors)
- `LgcCalibrationData_t` - Zero offset calibration
- Helper functions: `CountActiveBits()`, `HasLeather()`

#### [lgc_measurement_entity.h](entities/lgc_measurement_entity.h)

- `LgcLeatherPiece_t` - Single leather measurement
- `LgcBatch_t` - Batch of pieces (up to 100)
- `LgcActiveMeasurement_t` - Current measurement in progress
- Helper functions: `CalculateTotalArea()`, `CalculateAverageArea()`

#### [lgc_configuration_entity.h](entities/lgc_configuration_entity.h)

- `LgcSystemConfig_t` - System configuration (persisted to EEPROM)
- Default values, validation functions

**Rules for Entities:**

- ❌ NO business logic (only data)
- ❌ NO dependencies (except other entities)
- ✅ Pure C structures with inline helpers

---

## 🎯 Use Cases (Business Rules)

**Purpose:** Implement business algorithms and workflows.

### Structure:

```
use_cases/
├── measure/              Core measurement algorithms
├── batch/                Batch management
├── calibration/          Sensor calibration
├── reporting/            Report generation
└── configuration/        Config management
```

### Implemented Use Cases:

#### [measure/lgc_uc_process_slice.c](use_cases/measure/lgc_uc_process_slice.c)

**Purpose:** Process single measurement slice (core algorithm)

**Algorithm:**

1. Count active bits across 11 sensors (110 photocells)
2. Calculate area: `active_bits × 10mm × 5mm = area_dm²`
3. Detect leather presence (threshold check)
4. Apply hysteresis (3 consecutive empty slices = piece end)

**API:**

```c
Result_t LgcUC_ProcessSlice(
    const LgcSensorArray_t *sensor_data,  // Input: Sensor readings
    const LgcSystemConfig_t *config,      // Input: Threshold, hysteresis
    LgcActiveMeasurement_t *active,       // In/Out: Current measurement state
    LgcSliceResult_t *result              // Output: Slice result
);
```

**Performance:** <500µs typical

**Threading:** Thread-safe if external mutex protects `active`

---

### Pending Use Cases:

#### measure/lgc_uc_measure_area.c (TODO)

**Purpose:** Complete measurement cycle orchestration

- Initialize measurement
- Attach encoder callback
- Process each slice (calls `ProcessSlice`)
- Finalize piece when boundary detected

#### batch/lgc_uc_manage_batch.c (TODO)

**Purpose:** Batch lifecycle management

- Create new batch
- Add piece to batch
- Finalize batch (calculate statistics)
- Export batch data

#### calibration/lgc_uc_calibrate_sensors.c (TODO)

**Purpose:** Zero offset calibration

- Capture baseline readings (no leather)
- Validate calibration
- Store calibration data

#### reporting/lgc_uc_generate_report.c (TODO)

**Purpose:** Format batch data for printing

- Generate text report
- Format for ESC/POS printer
- Calculate statistics (total, average, min, max)

#### configuration/lgc_uc_manage_config.c (TODO)

**Purpose:** Configuration management

- Load config from storage
- Validate config parameters
- Save config with CRC32
- Apply default values on corruption

---

## 🔌 Interfaces (Ports)

**Purpose:** Define contracts for external dependencies (adapters).

### Key Concept: **Dependency Inversion**

```
 Domain (High-level)  ──────defines─────►  Interface (Abstract)
                                                   ▲
                                                   │
                                             implements
                                                   │
 Adapter (Low-level)  ─────────────────────────────┘
```

Domain rules:

- ✅ Define WHAT needs to be done (interface)
- ❌ NEVER know HOW it's done (implementation details)

---

### Implemented Interfaces:

#### [lgc_i_sensor_reader.h](interfaces/lgc_i_sensor_reader.h)

**Purpose:** Read sensor array data

**Methods:**

- `init(config)` - Initialize sensor reader
- `read_all_sensors(out_data)` - Sequential polling (legacy, 2s)
- `read_cascade_mode(out_data)` - Optimized cascade (550ms)
- `deinit()` - Cleanup

**Implementations:** (in adapters/)

- `ModbusAdapter` - Modbus RTU (9600 baud, polling, DEPRECATED)
- `LwPktAdapter` - LwPKT cascade (DMA, ring buffer, ACTIVE)

---

#### [lgc_i_encoder.h](interfaces/lgc_i_encoder.h)

**Purpose:** Rotary encoder interaction

**Methods:**

- `init(config)` - Initialize encoder
- `get_position(out_position)` - Read current position
- `reset_position()` - Zero encoder
- `attach_callback(callback, ctx)` - Register ISR callback
- `detach_callback()` - Remove callback
- `deinit()` - Cleanup

**Implementation:** (in adapters/)

- `EncoderAdapter` - GPIO EXTI ISR

**Critical:** Encoder callback called from ISR - MUST be fast (<1ms)

---

#### [lgc_i_storage.h](interfaces/lgc_i_storage.h)

**Purpose:** Persistent storage operations

**Methods:**

- `init(config)` - Initialize storage
- `save_config(config)` - Save config with CRC32
- `load_config(out_config)` - Load & validate config
- `save_batch(batch)` - Persist batch data
- `load_batch(batch_number, out_batch)` - Retrieve batch
- `erase_all()` - Factory reset
- `deinit()` - Cleanup

**Implementation:** (in adapters/)

- `EepromAdapter` - AT24Cxx I2C + CRC32

---

#### [lgc_i_display.h](interfaces/lgc_i_display.h)

**Purpose:** HMI display communication

**Methods:**

- `init(config)` - Initialize display
- `write_variable(vp, data, len)` - Write display variable
- `read_variable(vp, out_data, max_len)` - Read display variable
- `change_page(page_id)` - Change screen
- `deinit()` - Cleanup

**Implementation:** (in adapters/)

- `DisplayAdapter` - DWIN UART protocol

---

#### [lgc_i_printer.h](interfaces/lgc_i_printer.h)

**Purpose:** Thermal printer control

**Methods:**

- `init(config)` - Initialize printer
- `print_text(text)` - Print line
- `set_alignment(align)` - LEFT/CENTER/RIGHT
- `set_font_size(size)` - NORMAL/LARGE/SMALL
- `feed_paper(lines)` - Advance paper
- `cut_paper()` - Cut (if supported)
- `print_barcode(data)` - Print barcode (optional)
- `deinit()` - Cleanup

**Implementation:** (in adapters/)

- `PrinterAdapter` - ESC/POS over USB

---

## 🚨 Critical Rules

### 1. NO Hardware Dependencies

```c
// ❌ FORBIDDEN in domain/
#include "stm32f4xx_hal.h"
#include "tx_api.h"
#include "usart.h"

// ✅ ALLOWED
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "../interfaces/lgc_i_sensor_reader.h"
```

**Validation:**

```bash
# This MUST return empty
grep -r "stm32f4xx_hal.h" domain/
```

---

### 2. Always Use Interfaces

```c
// ❌ WRONG: Direct dependency on concrete adapter
#include "../../adapters/communication/lwpkt_adapter/lgc_lwpkt_adapter.h"
LwPktAdapter_t adapter;
LwPktAdapter_ReadSensors(&adapter, &data);  // Tight coupling!

// ✅ CORRECT: Dependency on interface
#include "../interfaces/lgc_i_sensor_reader.h"
ILgcSensorReader_t *sensor;  // Injected
sensor->read_cascade_mode(sensor->context, &data);  // Loose coupling!
```

---

### 3. Dependency Injection (Always)

```c
// ❌ WRONG: Hidden dependency (global/singleton lookup)
Result_t LgcUC_MeasureArea_Init(void) {
    ILgcSensorReader_t *sensor = GlobalSensorRegistry_Get();  // ❌
}

// ✅ CORRECT: Explicit injection
Result_t LgcUC_MeasureArea_Init(
    LgcMeasureAreaUC_t *uc,
    ILgcSensorReader_t *sensor,      // ✅ Injected
    ILgcEncoder_t *encoder            // ✅ Injected
) {
    uc->sensor = sensor;
    uc->encoder = encoder;
}
```

---

### 4. Pure Functions Preferred

```c
// ✅ Pure function: Same inputs → same outputs, no side effects
float LgcUC_CalculateSliceArea(const LgcSensorArray_t *data) {
    // No global state access
    // No I/O operations
    // Thread-safe by nature
}
```

---

### 5. Validation Macros

```c
Result_t LgcUC_ProcessSlice(/* ... */) {
    LGC_VALIDATE_PTR(sensor_data);   // Returns ERR_NULL_POINTER if NULL
    LGC_VALIDATE_PTR(config);
    LGC_VALIDATE_RANGE(config->threshold, 1, 10);  // Range check

    // Business logic...
}
```

---

## 🧪 Testing Strategy

### Unit Tests (Unity + CMock)

**Target:** 90% coverage for use cases

**Example:**

```c
// test/test_lgc_uc_process_slice.c
#include "unity.h"
#include "mock_lgc_i_sensor_reader.h"
#include "lgc_uc_process_slice.h"

void test_ProcessSlice_AllSensorsActive_CalculatesCorrectArea(void) {
    // Arrange
    LgcSensorArray_t sensor_data = {
        .sensors = {
            [0 ... 10] = { .status = 0x3FF, .is_valid = true }  // All active
        }
    };
    LgcSystemConfig_t config;
    LgcSystemConfig_InitDefaults(&config);
    LgcActiveMeasurement_t active;
    LgcActiveMeasurement_Init(&active);
    LgcSliceResult_t result;

    // Act
    Result_t res = LgcUC_ProcessSlice(&sensor_data, &config, &active, &result);

    // Assert
    TEST_ASSERT_EQUAL(ERR_OK, res);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 5.5, result.slice_area_dm2);  // 110 bits × 0.05 dm²
    TEST_ASSERT_TRUE(result.leather_detected);
}
```

**Build & Run:**

```bash
cd tests
mkdir build && cd build
cmake ..
make
ctest --output-on-failure
```

---

## 📈 Performance Requirements

| Operation                  | Target Time | Actual |
| -------------------------- | ----------- | ------ |
| `LgcUC_ProcessSlice`       | <500µs      | ~200µs |
| `LgcUC_CalculateSliceArea` | <50µs       | ~20µs  |
| `LgcUC_DetectLeather`      | <100µs      | ~50µs  |

**Measurement:** Cycle counter on STM32F446 @ 180MHz

---

## 📚 Documentation Standards

All public functions documented with Doxygen:

```c
/**
 * @brief  Brief description (one line)
 *
 * @details Detailed explanation (optional)
 *
 * @param[in]     input_param  Description
 * @param[out]    output_param Description
 * @param[in,out] inout_param  Description
 * @return Error code
 * @retval ERR_OK           Success case
 * @retval ERR_NULL_POINTER Failure case
 *
 * @pre  Preconditions (must be true before call)
 * @post Postconditions (guaranteed after successful call)
 *
 * @note  Special notes
 * @warning Important warnings
 * @see   Related functions
 */
```

---

## 🔄 Migration Checklist

When creating a new use case:

- [ ] Define interface (if new external dependency)
- [ ] Write entity structures (if new data types)
- [ ] Write header (.h) with Doxygen
- [ ] Write implementation (.c)
- [ ] Write unit tests (TDD)
- [ ] Validate NO HAL includes (`grep -r "stm32" domain/`)
- [ ] Add to DI Container wiring
- [ ] Update this README

---

## 📞 Contact

Questions about domain layer design?
See [REFACTOR_PLAN.md](../../REFACTOR_PLAN.md) or contact architecture team.

---

**Last Updated:** 2026-02-12  
**Status:** ✅ Foundation Complete (Entities, Interfaces, Core Use Case)  
**Next:** Implement remaining use cases (batch, calibration, reporting)
