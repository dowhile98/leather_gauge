# LGC Controller - Clean Architecture Implementation

## 📁 Project Structure

This folder contains the refactored **Leather Gauge Controller** firmware following **Clean Architecture** and **SOLID** principles.

```
lgc_controller/
├── domain/           🧠 Business Logic (Pure C, NO HAL)
├── adapters/         ⚙️  Infrastructure (HAL, Drivers)
├── app/              📱 Composition Root (DI Container)
├── config/           ⚙️  Configuration files
└── Third_Party/      📦 External libraries
```

---

## 🏗️ Architecture Layers

### 1. Domain Layer (`domain/`)

**Purpose:** Pure business logic - TESTABLE on PC without hardware.

**Rules:**

- ❌ NEVER include `stm32f4xx_hal.h`
- ❌ NEVER include `tx_api.h` (ThreadX)
- ✅ Only `<stdint.h>`, `<stdbool.h>`, `<string.h>` allowed
- ✅ Define **Interfaces** (Ports), not implementations

**Structure:**

```
domain/
├── entities/              Pure data structures
│   ├── lgc_common_types.h       - Result_t, enums, constants
│   ├── lgc_sensor_array_entity.h - Sensor data structures
│   ├── lgc_measurement_entity.h  - Measurement, Batch, LeatherPiece
│   └── lgc_configuration_entity.h - System config
│
├── use_cases/            Business rules (testable)
│   ├── measure/
│   │   └── lgc_uc_process_slice.c/h - Core measurement algorithm
│   ├── batch/
│   ├── calibration/
│   ├── reporting/
│   └── configuration/
│
└── interfaces/           Ports (abstractions)
    ├── lgc_i_sensor_reader.h    - Read sensor array
    ├── lgc_i_encoder.h          - Encoder position & callbacks
    ├── lgc_i_storage.h          - EEPROM/Flash persistence
    ├── lgc_i_display.h          - HMI display (DWIN)
    └── lgc_i_printer.h          - Thermal printer
```

**Key Concept:** **Dependency Inversion Principle (DIP)**

- Domain defines **WHAT** (interfaces)
- Adapters implement **HOW** (concrete implementations)

---

### 2. Adapters Layer (`adapters/`)

**Purpose:** Concrete implementations of domain interfaces using hardware drivers.

**Rules:**

- ✅ CAN include HAL (`stm32f4xx_hal.h`)
- ✅ MUST implement domain interfaces
- ✅ ONE adapter per interface implementation
- ❌ NEVER called directly by domain (only via interfaces)

**Structure:**

```
adapters/
├── communication/
│   ├── modbus_adapter/       Legacy Modbus RTU (2s latency)
│   └── lwpkt_adapter/        LwPKT cascade (550ms)
│
├── peripherals/
│   ├── encoder_adapter/      GPIO EXTI ISR
│   ├── display_adapter/      DWIN UART
│   ├── printer_adapter/      ESC/POS USB
│   └── digital_inputs_adapter/ GPIO + lwbtn
│
└── storage/
    ├── eeprom_adapter/       AT24Cxx I2C + CRC32
    └── rtc_adapter/          STM32 RTC HAL
```

**Example Adapter:**

```c
// lgc_lwpkt_adapter.c
typedef struct {
    UART_HandleTypeDef *huart;
    lwpkt_t lwpkt_instance;
    // ... private fields
} LwPktAdapter_t;

static Result_t lwpkt_read_cascade(void *ctx, LgcSensorArray_t *out) {
    LwPktAdapter_t *adapter = (LwPktAdapter_t *)ctx;
    // Implementation with HAL (OK here)
    return ERR_OK;
}

ILgcSensorReader_t* LwPktAdapter_GetInterface(LwPktAdapter_t *adapter) {
    static ILgcSensorReader_t iface = {
        .context = adapter,
        .read_cascade_mode = lwpkt_read_cascade,
        // ...
    };
    return &iface;
}
```

---

### 3. Application Layer (`app/`)

**Purpose:** Composition Root - Wire dependencies & bootstrap system.

**Structure:**

```
app/
├── inc/
│   ├── lgc.h                  Public API
│   └── lgc_di_container.h     DI Container
│
└── src/
    ├── lgc_di_container.c     Dependency Injection
    ├── lgc_main_task.c        Main control task
    ├── hmi/
    │   └── lgc_hmi_task.c     HMI update task
    └── printer/
        └── lgc_printer_task.c Printer command task
```

**DI Container Pattern:**

```c
// lgc_di_container.c
void LgcDI_WireComponents(void) {
    // 1. Create adapter instances (static)
    static LwPktAdapter_t lwpkt_adapter;
    static EncoderAdapter_t encoder_adapter;

    // 2. Initialize adapters
    LwPktAdapter_Init(&lwpkt_adapter, &huart2);
    EncoderAdapter_Init(&encoder_adapter, EXTI_LINE_0);

    // 3. Get interfaces
    ILgcSensorReader_t *sensor = LwPktAdapter_GetInterface(&lwpkt_adapter);
    ILgcEncoder_t *encoder = EncoderAdapter_GetInterface(&encoder_adapter);

    // 4. Inject into Use Cases
    LgcUC_MeasureArea_Init(&measure_uc, sensor, encoder);

    // 5. Create tasks
    tx_thread_create(&main_task, ..., lgc_main_task_entry, &measure_uc, ...);
}
```

---

## 🎯 Design Principles Applied

### SOLID in C

#### 1. Single Responsibility Principle (SRP)

- **lgc_uc_process_slice.c**: Only processes ONE slice
- **lgc_lwpkt_adapter.c**: Only handles LwPKT protocol
- **lgc_eeprom_adapter.c**: Only manages EEPROM I2C

#### 2. Open/Closed Principle (OCP)

**Add new protocol without modifying domain:**

```c
// Add LwPKT (new) without changing domain/use_cases/
ILgcSensorReader_t *sensor = LwPktAdapter_GetInterface(...);
// vs
ILgcSensorReader_t *sensor = ModbusAdapter_GetInterface(...);
```

#### 3. Liskov Substitution Principle (LSP)

All `ILgcSensorReader_t` implementations honor the contract:

- Return `ERR_OK` if data valid
- Populate `out_data` on success
- Mark invalid sensors with `is_valid = false`

#### 4. Interface Segregation Principle (ISP)

Split interfaces by responsibility:

- `ILgcSensorReader` (read sensors)
- `ILgcEncoder` (position & callbacks)
- `ILgcStorage` (persist data)

NOT: `IPeripherals` with 10+ methods

#### 5. Dependency Inversion Principle (DIP)

**Domain depends on abstractions (interfaces), not concrete adapters.**

```c
// ✅ CORRECT
#include "lgc_i_sensor_reader.h"  // Interface

// ❌ WRONG
#include "lgc_lwpkt_adapter.h"    // Concrete implementation
```

---

## 🔧 Development Workflow

### TDD Cycle

1. **Red:** Write failing test

```c
// test_lgc_uc_process_slice.c
void test_ProcessSlice_AllSensorsActive_CalculatesCorrectArea(void) {
    // Arrange
    LgcSensorArray_t mock_data = { /* ... */ };

    // Act
    Result_t res = LgcUC_ProcessSlice(&mock_data, &config, &active, &result);

    // Assert
    TEST_ASSERT_EQUAL(ERR_OK, res);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 5.5f, result.slice_area_dm2);
}
```

2. **Green:** Write minimal code to pass

3. **Refactor:** Improve structure

### Build Integration

```bash
# Validate NO HAL in domain/
grep -r "stm32f4xx_hal.h" lgc_controller/domain/
# Must return empty!

# Static analysis
cppcheck --enable=all lgc_controller/domain/

# Unit tests (PC, no hardware)
cd tests && make run
```

---

## 📝 Coding Standards

### Naming Conventions

| Element        | Format             | Example                  |
| -------------- | ------------------ | ------------------------ |
| **Files**      | `snake_case`       | `lgc_uc_process_slice.c` |
| **Types**      | `PascalCase_t`     | `LgcMeasurement_t`       |
| **Interfaces** | `ILgc*_t`          | `ILgcSensorReader_t`     |
| **Functions**  | `Module_Action`    | `LgcUC_ProcessSlice`     |
| **Variables**  | `snake_case`       | `sensor_data`            |
| **Constants**  | `UPPER_SNAKE_CASE` | `LGC_SENSOR_COUNT`       |

### Memory Management

- ❌ **PROHIBITED:** `malloc`, `free`, `calloc`
- ✅ **ALLOWED:** Static allocation, ThreadX memory pools

### Error Handling

```c
// Always check return values
Result_t res = LgcUC_ProcessSlice(/* ... */);
if (res != ERR_OK) {
    // Handle error
}
```

---

## 🚀 Migration Status

| Component           | Status      | Progress |
| ------------------- | ----------- | -------- |
| **Entities**        | ✅ Complete | 100%     |
| **Interfaces**      | ✅ Complete | 100%     |
| **Use Cases**       | 🔄 Started  | 10%      |
| **LwPKT Adapter**   | ⏳ Pending  | 0%       |
| **Encoder Adapter** | ⏳ Pending  | 0%       |
| **DI Container**    | ⏳ Pending  | 0%       |
| **Tests**           | ⏳ Pending  | 0%       |

---

## 📚 References

- [REFACTOR_PLAN.md](../../REFACTOR_PLAN.md) - Complete refactoring roadmap
- [docs/SYSTEM_ARCHITECTURE.md](../../docs/SYSTEM_ARCHITECTURE.md) - System design
- **Clean Architecture** by Robert C. Martin (Uncle Bob)
- **Clean Code** by Robert C. Martin

---

**Next Steps:**

1. Implement remaining use cases (batch management, calibration)
2. Create LwPKT adapter (priority: 67% faster than Modbus)
3. Implement encoder adapter (critical for sync)
4. Build DI Container
5. Write unit tests (Unity + CMock)
6. Integration with existing firmware

**Questions?** Review [REFACTOR_PLAN.md](../../REFACTOR_PLAN.md) Phase 1 tasks.
