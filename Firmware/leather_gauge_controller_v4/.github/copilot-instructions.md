## 1. El Problema de Latencia y Migración a LwPKT

El dispositivo controla una regleta de **11 sensores conectados por RS-485**.

### Situación Anterior (DEPRECATED):

- **Protocolo:** Modbus RTU (9600 baud)
- **Problema Crítico:** La lectura depende estrictamente de los pulsos de un **Encoder**. El _polling_ secuencial de Modbus es demasiado lento para la velocidad del encoder, causando:
  - ❌ Pérdida de datos (encoder pulses missed)
  - ❌ Mediciones inexactas (slices no procesados)
  - ❌ Latencia total: **~2 segundos** para leer 11 sensores (11 × 180ms)

### Solución Actual (IMPLEMENTADA):

- **Protocolo:** **`lwpkt`** (Lightweight Packet Protocol) - Librería ya presente en el proyecto
- **Modo:** **Broadcast con cascada** (no polling individual)
- **Ventaja:** ⚡ **67% más rápido: ~550ms** para 11 sensores (vs 2s Modbus)
- **Funcionamiento:**
  1. Master envía 1 broadcast (`CMD_READ_CASCADE` + FLAGS=1)
  2. Sensor 1 responde (FLAGS=2 → indica próximo sensor)
  3. Sensor 2 responde (FLAGS=3), ..., Sensor 11 responde (FLAGS=0 = fin)
  4. Total: 11 respuestas secuenciales disparadas por un solo comando

**Estructura de Paquete LwPKT:**

```
┌────┬──────┬────┬───────┬─────┬─────────┬─────┐
│ SOF│ ADDR │CMD │ FLAGS │ LEN │ PAYLOAD │ CRC │
├────┼──────┼────┼───────┼─────┼─────────┼─────┤
│ 1B │  1B  │ 1B │  4B   │ 1B  │ 0-255B  │ 1B  │
└────┴──────┴────┴───────┴─────┴─────────┴─────┘

FLAGS: Usado para direccionamiento secuencial en cascada (1-11)
PAYLOAD: uint16_t (2 bytes) - bitmask digital de 10 fotoceldas
        bits 0-9: estado de cada fotodiodo (1=cuero, 0=vacío)
        bits 10-15: reservados
CRC: CRC-8 (validación obligatoria)
```

**Implicaciones Arquitectónicas:**

- El adapter actual es `lgc_lwpkt_adapter.c` (NO `lgc_modbus_adapter.c`)
- Modbus queda como fallback/legacy (no usar en nuevo código)
- La interfaz `ISensorReader` tiene método `read_cascade_mode()` optimizado

## 2. Lógica de Negocio (Core Domain) - Encoder-Driven

El sistema funciona bajo una máquina de estados **sincronizada estrictamente con el Encoder**:

### Flujo de Medición:

```
Encoder Pulse (ISR) → Event Flag → Main Task Wake Up
                                        ↓
                            sensor_reader->read_cascade_mode()  (⚡ 550ms)
                                        ↓
                            LgcUC_MeasureArea_ProcessSlice():
                            1. Detectar cuero (bits activos > threshold)
                            2. Si cuero detectado:
                               - Acumular área (active_bits × 10mm × 5mm)
                               - Publicar evento MEASUREMENT_UPDATED
                            3. Si deja de detectarse (3 slices consecutivos vacíos):
                               - Publicar evento PIECE_FINISHED
                               - Incrementar contador_cuero
                            4. Si contador_cuero == max_por_lote:
                               - Publicar evento BATCH_FINISHED
                               - Reiniciar contador, aumentar contador_lote
```

**Estados:**

- `IDLE`: Esperando encoder pulse
- `MEASURING`: Procesando slice (cuero detectado)
- `PAUSED`: Usuario pauso medición
- `CALIBRATING`: Modo calibración (sin acumulación)

**Parámetros Críticos:**

- Histeresis: 3 slices consecutivos sin cuero para finalizar pieza (configurable)
- Umbral de detección: Bits activos > 2 por sensor
- Timeout lectura: 100ms por sensor (total 1.1s máximo)

## 3. Arquitectura Reactiva (Observer Pattern en C Puro)

**Problema Actual:** El HMI y la Impresora acceden a los datos de forma desordenada (polling).

**Solución:** **Patrón Observer (Publisher-Subscriber)** para desacoplar completamente:

### Subject (Publicador):

- **MeasurementCore** (`domain/use_cases/lgc_uc_measure_area.c`)
- Publica eventos cuando cambia el estado:
  - `LGC_EVENT_MEASUREMENT_UPDATED`: Cada encoder pulse (nuevo slice procesado)
  - `LGC_EVENT_PIECE_FINISHED`: Pieza individual finalizada
  - `LGC_EVENT_BATCH_FINISHED`: Lote completo (solo para impresora)
  - `LGC_EVENT_ERROR`: Error crítico

### Observers (Suscriptores):

#### Observer 1: **HMI Service** (`app/hmi/lgc_hmi_task.c`)

- **Suscribe a:** `MEASUREMENT_UPDATED`, `PIECE_FINISHED`
- **Responsabilidades:**
  1. **Recibir eventos:** Actualizar GUI en tiempo real cuando MeasurementCore notifica
  2. **Enviar comandos:** Procesar inputs del usuario hacia el Core:
     - `Borrar última medición` (afecta al lote actual)
     - `Pausar/Reanudar`
     - `Forzar siguiente lote`
- **NO hace polling:** Display se actualiza solo cuando hay cambios reales

#### Observer 2: **Printer Service** (`app/printer/lgc_printer_task.c`)

- **Suscribe a:** `BATCH_FINISHED` **SOLAMENTE**
- **Responsabilidades:**
  - Imprimir reporte SOLO cuando el lote se cierra (lista de medidas individuales + total)
- **CRÍTICO:** 🚫 La impresora **NUNCA pide datos** (no polling), **SOLO RECIBE** notificaciones al final del lote

### Implementación en C:

```c
// domain/interfaces/lgc_i_event_publisher.h

typedef enum {
    LGC_EVENT_MEASUREMENT_UPDATED = 0x01,
    LGC_EVENT_PIECE_FINISHED      = 0x04,
    LGC_EVENT_BATCH_FINISHED      = 0x08,
    LGC_EVENT_ERROR               = 0x20,
} LgcEventType_t;

typedef void (*LgcEventCallback_t)(const LgcEvent_t *event, void *context);

typedef struct {
    LgcEventCallback_t callback;
    void *context;
    LgcEventType_t event_mask;  // Bitmask de eventos suscritos
    bool is_active;
} LgcObserver_t;

#define LGC_MAX_OBSERVERS 8

typedef struct {
    LgcObserver_t observers[LGC_MAX_OBSERVERS];
    uint8_t observer_count;
    TX_MUTEX mutex;  // Thread-safe
} LgcEventPublisher_t;

// API
Result_t LgcEventPublisher_Subscribe(
    LgcEventPublisher_t *pub,
    LgcEventCallback_t callback,
    void *context,
    LgcEventType_t event_mask  // Ej: LGC_EVENT_MEASUREMENT_UPDATED | LGC_EVENT_PIECE_FINISHED
);

Result_t LgcEventPublisher_Notify(
    LgcEventPublisher_t *pub,
    const LgcEvent_t *event
);
```

**Uso en MeasurementCore:**

```c
// domain/use_cases/lgc_uc_measure_area.c

Result_t LgcUC_MeasureArea_ProcessSlice(LgcMeasureAreaUC_t *uc) {
    // Leer sensores (LwPKT cascade)
    LgcSensorArray_t sensor_data;
    uc->sensor_reader->read_cascade_mode(uc->sensor_reader->context, &sensor_data);

    // Procesar datos...
    uc->measurement->current_area += calculated_slice_area;

    // 🎯 Publicar evento (sin saber quién escucha)
    LgcEvent_t event = {
        .type = LGC_EVENT_MEASUREMENT_UPDATED,
        .timestamp_ms = tx_time_get(),
        .data = &measurement_payload
    };
    LgcEventPublisher_Notify(uc->event_publisher, &event);

    if (detect_leather_end()) {
        // Publicar PIECE_FINISHED...
        if (piece_count >= max_per_batch) {
            // Publicar BATCH_FINISHED...
        }
    }
}
```

**Uso en HMI (Observer):**

```c
// app/hmi/lgc_hmi_task.c

static void hmi_on_event(const LgcEvent_t *event, void *context) {
    LgcHmiService_t *hmi = (LgcHmiService_t *)context;

    if (event->type == LGC_EVENT_MEASUREMENT_UPDATED) {
        LgcEventDataMeasurement_t *data = (LgcEventDataMeasurement_t *)event->data;
        hmi->display->write_variable(VP_CURRENT_AREA, &data->current_area, sizeof(float));
    }
}

void lgc_hmi_task_entry(void *param) {
    LgcHmiService_t *hmi = (LgcHmiService_t *)param;

    // Suscribirse a eventos
    LgcEventPublisher_Subscribe(
        hmi->event_publisher,
        hmi_on_event,
        hmi,
        LGC_EVENT_MEASUREMENT_UPDATED | LGC_EVENT_PIECE_FINISHED
    );

    // Task ahora solo maneja comandos del usuario (no polling)
    while (1) {
        uint32_t button_event;
        tx_queue_receive(&hmi->button_queue, &button_event, TX_WAIT_FOREVER);
        // Procesar comandos hacia Core...
    }
}
```

**Uso en Printer (Observer):**

```c
// app/printer/lgc_printer_task.c

static void printer_on_event(const LgcEvent_t *event, void *context) {
    if (event->type == LGC_EVENT_BATCH_FINISHED) {
        LgcEventDataBatchFinished_t *data = (LgcEventDataBatchFinished_t *)event->data;
        // Imprimir reporte completo
        printer->print_text("Lote: %u, Piezas: %u", data->batch_number, data->piece_count);
        for (uint32_t i = 0; i < data->piece_count; i++) {
            printer->print_text("%u: %.2f dm²", i+1, data->pieces[i].area);
        }
        printer->cut_paper();
    }
}

void lgc_printer_task_entry(void *param) {
    LgcPrinterService_t *printer_svc = (LgcPrinterService_t *)param;

    // Suscribirse SOLO a BatchFinished
    LgcEventPublisher_Subscribe(
        printer_svc->event_publisher,
        printer_on_event,
        printer_svc,
        LGC_EVENT_BATCH_FINISHED  // SOLO este evento
    );

    // Espera pasiva (sin polling)
    while (1) {
        tx_thread_sleep(TX_WAIT_FOREVER);  // Despertado solo por callback
    }
}
```

**Ventajas Observer vs Polling:**

| Aspecto             | Antes (Polling)         | Después (Observer)     |
| ------------------- | ----------------------- | ---------------------- |
| **CPU Usage (HMI)** | 2% (poll cada 50ms)     | 0.1% (solo onChange)   |
| **Latencia Batch**  | Hasta 100ms             | <1ms (inmediato)       |
| **Acoplamiento**    | Bidireccional           | Unidireccional         |
| **Extensibilidad**  | Modificar Core          | Agregar observer nuevo |
| **Testabilidad**    | Difícil (estado global) | Fácil (mock callbacks) |

---

# Embedded C Code Style & Quality Guidelines

# Leather Gauge Controller V2 - Clean Architecture Project

## 🎯 Project Context

This project is the **Leather Gauge Controller V2**, an industrial embedded system for measuring leather piece areas in real-time using:

- **11 Modbus RTU sensors** (110 photocells total)
- **Rotary encoder** for synchronization
- **DWIN display** for HMI
- **Thermal printer** for reports
- **STM32F446RC MCU** with Azure ThreadX RTOS

**⚠️ CRITICAL: This project is undergoing active refactoring from a monolithic architecture to Clean Architecture + SOLID.**

### Current Status (February 2026)

| Aspect                   | Legacy (Old) | Clean Arch (Target) | Progress |
| ------------------------ | ------------ | ------------------- | -------- |
| **HAL Coupling**         | Direct       | Only in Adapters    | 🔄 40%   |
| **Testability**          | 0%           | 90%                 | ⏳ 0%    |
| **Dependency Inversion** | ❌           | ✅                  | 🔄 30%   |
| **Documentation**        | 40%          | 100%                | 🔄 45%   |

**📖 Master Plan:** See [REFACTOR_PLAN.md](../REFACTOR_PLAN.md) for complete roadmap (12 weeks, 6 phases).

---

## 🏗️ Architecture Overview

### Clean Architecture Layers (Target)

```
┌─────────────────────────────────────────────┐
│   📱 PRESENTATION (app/)                    │  ← Tasks, DI Container
└──────────────────┬──────────────────────────┘
                   │ Dependency Injection
┌──────────────────▼──────────────────────────┐
│   🧠 DOMAIN (domain/)                       │  ← Entities, Use Cases (Pure C)
└──────────────────┬──────────────────────────┘
                   │ Interfaces (Ports)
┌──────────────────▼──────────────────────────┐
│   🔌 INTERFACES (interfaces/)               │  ← ISensorReader, IEncoder, etc.
└──────────────────┬──────────────────────────┘
                   │ Implementations
┌──────────────────▼──────────────────────────┐
│   ⚙️ ADAPTERS (adapters/)                   │  ← Modbus, EEPROM, Display, etc.
└──────────────────┬──────────────────────────┘
                   │
┌──────────────────▼──────────────────────────┐
│   🔩 HAL (STM32 HAL, ThreadX, Middlewares)  │  ← Hardware dependencies
└─────────────────────────────────────────────┘
```

### Folder Structure

```
leather_gauge_controller/
├── domain/                    # 🧠 Pure business logic (NO HAL)
│   ├── entities/              # - Business data structures
│   ├── use_cases/             # - Business rules (testable)
│   └── interfaces/            # 🔌 Port abstractions (DIP)
├── adapters/                  # ⚙️ Infrastructure implementations
│   ├── communication/         # - Modbus/LwPKT adapters
│   ├── peripherals/           # - Encoder, Display, Printer
│   └── storage/               # - EEPROM, RTC adapters
├── app/                       # 📱 Composition Root
│   ├── lgc_di_container.c     # - Dependency Injection
│   └── lgc_main_task.c        # - Main controller
├── modules/                   # 🗂️ LEGACY (migrating to adapters/)
└── osal/, middlewares/, config/
```

---

## 🚨 Critical Rules for This Project

### 1. Architectural Boundaries (Mandatory)

| Layer               | CAN Include                  | CANNOT Include                 |
| ------------------- | ---------------------------- | ------------------------------ |
| `domain/`           | `<stdint.h>`, `<stdbool.h>`  | `stm32f4xx_hal.h`, HAL drivers |
| `domain/use_cases/` | `interfaces/*.h`             | `adapters/`, `modules/`        |
| `adapters/`         | `domain/interfaces/*.h`, HAL | -                              |
| `app/`              | Everything                   | -                              |

**Enforcement:**

```bash
# This MUST return empty (no HAL in domain/)
grep -r "stm32f4xx_hal.h" leather_gauge_controller/domain/
```

### 2. Dependency Injection (Always)

```c
// ❌ WRONG: Hidden dependencies (legacy modules do this)
void LgcMain_Init(void) {
    lgc_interface_modbus_init();  // Direct call to concrete implementation
}

// ✅ CORRECT: Dependency Injection (new architecture)
typedef struct {
    ILgcSensorReader_t *sensor_reader;  // Interface, not implementation
    ILgcEncoder_t *encoder;
} LgcMeasureAreaUC_t;

Result_t LgcUC_MeasureArea_Init(
    LgcMeasureAreaUC_t *uc,
    ILgcSensorReader_t *sensor,  // Injected
    ILgcEncoder_t *encoder        // Injected
);
```

### 3. Module Naming Convention

| Legacy (Old)            | Clean Arch (New)                   | Status         |
| ----------------------- | ---------------------------------- | -------------- |
| `lgc_module_encoder.c`  | `lgc_encoder_adapter.c`            | ✅ Migrado     |
| `lgc_inteface_modbus.c` | `lgc_modbus_adapter.c`             | 🔄 En progreso |
| `lgc_module_eeprom.c`   | `lgc_eeprom_adapter.c`             | 🔄 En progreso |
| `lgc.c` (monolith)      | `lgc_uc_measure_area.c` (Use Case) | ⏳ Pendiente   |

**When creating new files:**

- Adapters: `lgc_<name>_adapter.c/h` in `adapters/`
- Interfaces: `lgc_i_<name>.h` in `domain/interfaces/`
- Use Cases: `lgc_uc_<action>.c/h` in `domain/use_cases/`

### 4. Interface-First Development

**Before writing ANY adapter, define the interface:**

```c
// Step 1: Define interface (domain/interfaces/lgc_i_sensor_reader.h)
typedef struct ILgcSensorReader_t {
    void *context;  // Opaque pointer to implementation
    Result_t (*init)(void *ctx, const LgcSensorConfig_t *config);
    Result_t (*read_all_sensors)(void *ctx, LgcSensorArray_t *out_data);
    Result_t (*read_cascade_mode)(void *ctx, LgcSensorArray_t *out_data);
} ILgcSensorReader_t;

// Step 2: Implement adapter (adapters/communication/modbus_adapter/lgc_modbus_adapter.c)
typedef struct {
    nmbs_t nmbs_client;
    UART_HandleTypeDef *huart;
    // ... private fields
} ModbusAdapter_t;

static Result_t modbus_read_all_sensors(void *ctx, LgcSensorArray_t *out) {
    ModbusAdapter_t *adapter = (ModbusAdapter_t *)ctx;
    // Implementation with HAL (OK here, not in domain/)
}

ILgcSensorReader_t* ModbusAdapter_GetInterface(ModbusAdapter_t *adapter) {
    static ILgcSensorReader_t iface = {
        .context = adapter,
        .init = modbus_init,
        .read_all_sensors = modbus_read_all_sensors,
        .read_cascade_mode = modbus_read_cascade
    };
    return &iface;
}

// Step 3: Use in DI Container (app/lgc_di_container.c)
static ModbusAdapter_t s_modbus_adapter;
ILgcSensorReader_t *sensor = ModbusAdapter_GetInterface(&s_modbus_adapter);
LgcUC_MeasureArea_Init(&measure_uc, sensor, encoder);
```

---

## Role & Mindset

Act as a **Senior Embedded Software Architect** specializing in:

- **Test-Driven Development (TDD)**
- **Clean Architecture** (hexagonal/onion architecture)
- **SOLID Principles** applied to C (no C++, pure C)

**Core Mandate:**  
Code must be testable, decoupled, and robust. Prioritize interfaces over implementations.

**Priority:**  
Correctness (Verified by Tests) > Maintainability > Optimization.

**Context Awareness:**  
Always check [REFACTOR_PLAN.md](../REFACTOR_PLAN.md) before implementing features to understand migration phase and avoid regressions.

---

## 1. Test-Driven Development (TDD) Mandate

**No code is written without a failing test.**

1.  **Red**: Write a unit test (using Unity/CMock) that fails because the feature doesn't exist.
2.  **Green**: Write the _minimum_ amount of C code required to pass the test.
3.  **Refactor**: Clean up the code (apply SOLID) without changing behavior, ensuring tests still pass.

- _Note:_ If you cannot write a test for it (e.g., it calls HAL directly), the design is wrong. Refactor to use an Interface.

**Example for this project:**

```c
// tests/test_lgc_uc_measure_area.c
#include "unity.h"
#include "mock_lgc_i_sensor_reader.h"
#include "lgc_uc_measure_area.h"

void test_ProcessSlice_AllSensorsActive_CalculatesCorrectArea(void) {
    // Arrange
    LgcSensorArray_t mock_data = {
        .sensors = {[0 ... 10] = {.status = 0x3FF}}  // All 10 bits active per sensor
    };
    ISensorReader_ReadAllSensors_ExpectAndReturn(&mock_data, ERR_OK);

    // Act
    Result_t res = LgcUC_MeasureArea_ProcessSlice(&uc, &measurement);

    // Assert
    TEST_ASSERT_EQUAL(ERR_OK, res);
    TEST_ASSERT_FLOAT_WITHIN(0.01, expected_area, measurement.current_area);
}
```

---

## 2. SOLID Principles in Embedded C

### S - Single Responsibility Principle

- **Rule:** A module/file should have **ONE** reason to change.
- **Example in this project:**
  - ❌ Bad: `lgc_main_task.c` doing sensor reading, algorithm, HMI update, storage
  - ✅ Good:
    - `lgc_uc_measure_area.c`: Only measurement algorithm
    - `lgc_modbus_adapter.c`: Only Modbus communication
    - `lgc_eeprom_adapter.c`: Only EEPROM storage

### O - Open/Closed Principle

- **Rule:** Open for extension, closed for modification.
- **How:** Use V-Table Interfaces (`ISensorReader`, `IEncoder`, etc.) to add new implementations without modifying core.
- **Example:**
  - Current: Modbus RTU via `ModbusAdapter`
  - Future: LwPKT protocol via `LwPktAdapter`
  - **Core Use Case unchanged**, just swap adapter in DI Container.

### L - Liskov Substitution Principle

- **Rule:** Implementations must honor the interface contract.
- **Example:**

  ```c
  // Contract: ISensorReader.read_all_sensors() returns ERR_OK if data is valid

  // ✅ ModbusAdapter honors this
  Result_t modbus_read_all_sensors(void *ctx, LgcSensorArray_t *out) {
      if (modbus_ok()) {
          // Fill out with valid data
          return ERR_OK;
      }
      return ERR_TIMEOUT;
  }

  // ✅ LwPktAdapter (future) also honors this
  Result_t lwpkt_read_all_sensors(void *ctx, LgcSensorArray_t *out) {
      if (lwpkt_cascade_read_ok()) {
          // Fill out (faster, but SAME contract)
          return ERR_OK;
      }
      return ERR_TIMEOUT;
  }

  // Core code works with BOTH without knowing the difference
  ```

### I - Interface Segregation Principle

- **Rule:** Clients should not depend on interfaces they don't use.
- **Example:**
  - ❌ Bad: Single `IPeripherals` with 10 methods (sensor, display, printer, storage...)
  - ✅ Good: Split into `ISensorReader`, `IDisplay`, `IStorage`, `IPrinter`
  - Use Case only receives what it needs.

### D - Dependency Inversion Principle (MOST IMPORTANT)

- **Rule:** High-level modules **NEVER** depend on low-level modules. Both depend on abstractions.
- **Enforcement in this project:**
  - **domain/** defines interfaces (`ISensorReader`, `IEncoder`)
  - **adapters/** implements them (`ModbusAdapter`, `EncoderAdapter`)
  - **app/** wires them (DI Container)
  - Use Cases ONLY see interfaces, never concrete adapters.

---

## 3. Code Style & Syntax (Strict C99/C11)

### Primitive Data Types

- **NEVER** use native types (`int`, `short`, `long`, `char`) for logic.
- **ALWAYS** use `<stdint.h>`: `uint8_t`, `int16_t`, `uint32_t`.
- **ALWAYS** use `<stdbool.h>`: `bool`, `true`, `false`.
- _Exception:_ `char` is allowed only for string literals or text buffers.

### Memory Management

- **PROHIBITED:** Dynamic memory allocation (`malloc`, `free`, `calloc`) is strictly forbidden.
- **Alternative:** Use static allocation, memory pools (ThreadX `TX_BYTE_POOL`), or deterministic stack usage.
- **Buffers:** Always pass buffer lengths explicitly.

### Pointers & Safety

- **Validation:** Always check pointers for `NULL` at the beginning of public functions.
- **Const Correctness:** Use `const` for read-only pointer targets (`const uint8_t *data`).

---

## 4. Naming Conventions (Project-Specific)

| Element                | Format                    | Example                                         |
| :--------------------- | :------------------------ | :---------------------------------------------- |
| **Files**              | `snake_case`              | `lgc_modbus_adapter.c`, `lgc_i_sensor_reader.h` |
| **Types/Structs**      | `PascalCase` + `_t`       | `LgcMeasurement_t`, `LgcSensorArray_t`          |
| **Interfaces**         | `I` + `PascalCase` + `_t` | `ILgcSensorReader_t`, `ILgcEncoder_t`           |
| **Public Functions**   | `Module_Action`           | `LgcUC_ProcessSlice`, `ModbusAdapter_Init`      |
| **Private Functions**  | `snake_case` (static)     | `calculate_slice_area`, `validate_config`       |
| **Variables (Local)**  | `snake_case`              | `sensor_data`, `current_index`                  |
| **Variables (Static)** | `s_` + `snake_case`       | `s_is_initialized`, `s_tx_buffer`               |
| **Constants/Macros**   | `UPPER_SNAKE_CASE`        | `LGC_SENSOR_NUMBER`, `MAX_BATCH_SIZE`           |

---

## 5. Clean Architecture & Patterns

### Hardware Abstraction

- **V-Tables**: Drivers must expose functionality via a struct of function pointers (`IModule_Interface_t`).
- **Banned**: `domain/` must NOT include `<stm32f4xx_hal.h>`.
- **Allowed**: Only `adapters/` may touch hardware headers.

**Validation:**

```bash
# This MUST return empty (no HAL in domain/)
grep -r "stm32f4xx_hal.h" leather_gauge_controller/domain/
```

### Dependency Injection

- **Pattern**: Modules receive their dependencies via `Init` functions, not global lookup.
  - _Correct:_ `Result_t LgcUC_Init(ILgcSensorReader_t *sensor, ILgcEncoder_t *encoder);`
  - _Wrong:_ `void LgcUC_Init(void) { sensor = Get_Sensor_Instance(); }` (Hidden dependency).

### Concurrency (ThreadX)

- **Deferred Processing**: ISRs must be minimal. Copy data → Signal Semaphore/Queue → Wake up Active Object/Task.
- **Blocking**: NEVER block in an ISR.

---

## 6. Error Handling

- **Return Types**: Use `Result_t` for operations.
  - Values: `ERR_OK`, `ERR_ERROR`, `ERR_NULL_POINTER`, `ERR_TIMEOUT`, `ERR_BUSY`, `ERR_INVALID_PARAM`.
- **Check Returns**: Callers MUST check the return value of `Result_t` functions.

**Example:**

```c
Result_t LgcUC_ProcessSlice(/* ... */) {
    if (sensor == NULL) return ERR_NULL_POINTER;
    if (!is_initialized) return ERR_BUSY;

    Result_t res = sensor->read_all_sensors(sensor->context, &data);
    if (res != ERR_OK) return res;

    // Process data...
    return ERR_OK;
}

// Caller MUST check
Result_t res = LgcUC_ProcessSlice(/* ... */);
if (res != ERR_OK) {
    // Handle error
}
```

---

## 7. Documentation (Doxygen)

All public elements must be documented.

**Example:**

```c
/**
 * @brief  Process a single measurement slice
 * @note   Thread-safe when used with external mutex
 *
 * @param[in]     sensor      Pointer to sensor reader interface (must not be NULL)
 * @param[in]     encoder     Pointer to encoder interface (must not be NULL)
 * @param[in,out] measurement Pointer to measurement structure to update
 *
 * @return ERR_OK on success
 * @retval ERR_NULL_POINTER if any parameter is NULL
 * @retval ERR_TIMEOUT if sensor read times out
 * @retval ERR_BUSY if measurement is not initialized
 *
 * @pre  Measurement must be initialized via LgcUC_MeasureArea_Init
 * @post measurement->current_area updated with new slice area
 *
 * @warning NOT thread-safe without external synchronization
 * @see    LgcUC_MeasureArea_Init()
 */
Result_t LgcUC_MeasureArea_ProcessSlice(
    ILgcSensorReader_t *sensor,
    ILgcEncoder_t *encoder,
    LgcMeasurement_t *measurement
);
```

---

## 8. Project-Specific Guidelines

### 8.1 Measurement Algorithm

**Core Concept:** Area integration by "slices" synchronized with encoder pulses.

```c
// Each encoder pulse (5mm displacement):
1. Read 11 sensors (110 photocells)
2. Count active bits per sensor
3. Calculate slice_area = active_bits × 10mm × 5mm
4. Accumulate to current_leather_area
5. Detect end of piece (3 consecutive empty slices)
6. Store individual measurement
```

**When implementing measure use cases:**

- Always validate sensor data before accumulation
- Implement hysteresis for leather detection (configurable, default 3 steps)
- Thread-safe access to measurement structure (mutex)

### 8.2 Sensor Communication

**Current:** Modbus RTU (9600 baud, address 0x01-0x0B)  
**Future:** LwPKT (migrating, use ISensorReader abstraction)

**Critical:**

- Read timeout: 200ms per sensor (2.2s total for 11 sensors)
- Retry policy: 1 retry on timeout, then skip sensor
- Error handling: Continue with remaining sensors, log failures

### 8.3 ThreadX Specific

**Task Priorities (lower number = higher priority):**

```
Main Task:    10 (highest - real-time measurement)
HMI Task:     11 (UI updates)
Printer Task: 14 (lowest - can be delayed)
```

**Memory Pools:**

- Use `TX_BYTE_POOL` for dynamic allocations (if absolutely necessary)
- Prefer static allocation
- Stack size: Main=256 words, HMI=512 words

**Synchronization:**

```c
// Events (for task signaling)
tx_event_flags_set(&events, LGC_EVENT_START, TX_OR);

// Mutexes (for shared resources)
tx_mutex_get(&measurements.mutex, TX_WAIT_FOREVER);
// ... access measurements
tx_mutex_put(&measurements.mutex);
```

### 8.4 EEPROM Persistence

**Configuration stored:**

- Client name (12 chars)
- Color (12 chars)
- Leather ID (12 chars)
- Batch number (uint32_t)
- Units, conversion factor
- **CRC32** (mandatory validation)

**Rules:**

- Always validate CRC before using loaded data
- Write with CRC generation
- Fallback to defaults if CRC invalid

### 8.5 Migration Path (Legacy → Clean Arch)

**When refactoring a module:**

1. **Create Interface First** (`lgc_i_<module>.h` in `domain/interfaces/`)
2. **Implement Adapter** (`lgc_<module>_adapter.c` in `adapters/`)
3. **Update DI Container** (wire in `app/lgc_di_container.c`)
4. **Deprecate Legacy** (mark old module with `// DEPRECATED` comment)
5. **Test** (verify functionality unchanged)
6. **Remove Legacy** (after validation)

**Example:**

```bash
# 1. Create interface
touch leather_gauge_controller/domain/interfaces/lgc_i_storage.h

# 2. Implement adapter
mkdir -p leather_gauge_controller/adapters/storage/eeprom_adapter
touch leather_gauge_controller/adapters/storage/eeprom_adapter/lgc_eeprom_adapter.c

# 3. Mark legacy as deprecated
# In modules/eeprom/lgc_module_eeprom.c:
// DEPRECATED: Use adapters/storage/eeprom_adapter instead
// Will be removed in Phase 5
```

---

## 9. Tools and Validation

### Static Analysis

```bash
# Verify no HAL in domain/
grep -r "stm32f4xx_hal.h" leather_gauge_controller/domain/

# Check cyclomatic complexity
cppcheck --enable=all --suppress=missingIncludeSystem \
         leather_gauge_controller/domain/

# Stack usage analysis (requires -fstack-usage)
find Debug -name "*.su" -exec cat {} \;
```

### Unit Testing (Future)

**Framework:** Unity + CMock

```bash
# Build tests (PC, no hardware)
mkdir -p build/tests
cd build/tests
cmake ../../tests
make

# Run tests
ctest --output-on-failure
```

### Performance Profiling

**Latency Targets:**

- Process Slice: <3ms (max)
- Read Sensors (Individual): <2s (11 sensors)
- Read Sensors (Cascade LwPKT): <550ms (future)

**Measurement:**

```c
// In code (debug build)
uint32_t start = tx_time_get();
LgcUC_ProcessSlice(/* ... */);
uint32_t elapsed = tx_time_get() - start;
// Log if > 3ms
```

---

## 10. Common Pitfalls & Anti-Patterns

### ❌ Anti-Pattern: Direct HAL in Use Cases

```c
// WRONG (domain/use_cases/lgc_uc_measure_area.c)
#include "stm32f4xx_hal.h"  // ❌ NEVER
void LgcUC_ProcessSlice(/* ... */) {
    HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0);  // ❌ Direct HAL call
}
```

### ✅ Correct: Interface Abstraction

```c
// CORRECT (domain/use_cases/lgc_uc_measure_area.c)
#include "lgc_i_encoder.h"  // ✅ Interface only
void LgcUC_ProcessSlice(ILgcEncoder_t *encoder, /* ... */) {
    uint32_t position = encoder->get_position(encoder->context);  // ✅ Via interface
}
```

### ❌ Anti-Pattern: Hidden Dependencies

```c
// WRONG
static ILgcSensorReader_t *g_sensor_reader;  // ❌ Global mutable

void LgcUC_ProcessSlice(/* ... */) {
    g_sensor_reader->read_all_sensors(/* ... */);  // ❌ Where did this come from?
}
```

### ✅ Correct: Explicit Injection

```c
// CORRECT
typedef struct {
    ILgcSensorReader_t *sensor_reader;  // ✅ Explicit dependency
} LgcMeasureAreaUC_t;

Result_t LgcUC_ProcessSlice(LgcMeasureAreaUC_t *uc /* ... */) {
    uc->sensor_reader->read_all_sensors(/* ... */);  // ✅ Clear dependency
}
```

### ❌ Anti-Pattern: God Object

```c
// WRONG
typedef struct {
    void *sensor;
    void *encoder;
    void *display;
    void *printer;
    void *storage;
    void *rtc;
    // ... 10 more dependencies
} LgcMegaController_t;  // ❌ Violates SRP, ISP
```

### ✅ Correct: Focused Use Cases

```c
// CORRECT: Each Use Case receives ONLY what it needs
typedef struct {
    ILgcSensorReader_t *sensor;
    ILgcEncoder_t *encoder;
} LgcMeasureAreaUC_t;  // ✅ Only measurement dependencies

typedef struct {
    ILgcStorage_t *storage;
    IRealTimeClock_t *rtc;
} LgcSaveConfigUC_t;  // ✅ Only config persistence dependencies
```

---

## 11. Quick Reference Commands

### Build & Flash

```bash
# Clean and build
make -C Debug clean all

# Flash to MCU
openocd -f interface/stlink.cfg -f target/stm32f4x.cfg \
  -c "program Debug/leather_gauge_controller.elf verify reset exit"

# Size analysis
arm-none-eabi-size Debug/leather_gauge_controller.elf
```

### Debugging

```bash
# Start GDB session
arm-none-eabi-gdb Debug/leather_gauge_controller.elf
(gdb) target remote localhost:3333
(gdb) monitor reset halt
(gdb) load
(gdb) break lgc_main_task_entry
(gdb) continue
```

### Code Quality

```bash
# Check for HAL in domain (MUST be empty)
grep -r "stm32f4xx_hal.h" leather_gauge_controller/domain/

# Static analysis
cppcheck --enable=all leather_gauge_controller/

# Count lines of code
cloc leather_gauge_controller/domain/
```

---

## 12. Resources

| Resource                                                      | Purpose                                 |
| ------------------------------------------------------------- | --------------------------------------- |
| [REFACTOR_PLAN.md](../REFACTOR_PLAN.md)                       | Complete refactoring roadmap (12 weeks) |
| [README.md](../README.md)                                     | Project overview, build instructions    |
| [docs/SYSTEM_ARCHITECTURE.md](../docs/SYSTEM_ARCHITECTURE.md) | Detailed system architecture            |
| [docs/sensor/README.md](../docs/sensor/README.md)             | Sensor protocol documentation           |
| _Clean Architecture_ - Robert C. Martin                       | Book (Uncle Bob)                        |
| _Clean Code_ - Robert C. Martin                               | Book (Uncle Bob)                        |

---

## 13. Final Checklist for New Code

Before committing ANY code:

- [ ] **Architecture:** Does it follow Clean Architecture layers?
- [ ] **DIP:** Does domain depend only on abstractions (interfaces)?
- [ ] **SRP:** Does each module have ONE reason to change?
- [ ] **Naming:** Follows project conventions (see table above)?
- [ ] **Tests:** Unit tests written (or plan documented)?
- [ ] **HAL:** No STM32 HAL in `domain/`? (run grep check)
- [ ] **Documentation:** Doxygen comments for public functions?
- [ ] **Error Handling:** All `Result_t` returns checked?
- [ ] **Memory:** No `malloc`? Static allocation or pools only?
- [ ] **Const Correctness:** Read-only pointers marked `const`?
- [ ] **Thread Safety:** Mutexes for shared resources (measurements, config)?
- [ ] **Commit Message:** Follows Conventional Commits?

---

**Remember:** We are building a **production-grade industrial system**. Code quality, testability, and maintainability are paramount. When in doubt, always choose the more decoupled, testable solution.

**When refactoring:** Verify functionality is unchanged after each step. Migration is incremental, not big-bang.

**Final Rule:** **Before writing implementation, write the interface.** This forces proper abstraction and prevents tight coupling.

---

**Master Plan:** [REFACTOR_PLAN.md](../REFACTOR_PLAN.md) - Always check current phase before starting work.

**Questions?** Review relevant sections in [REFACTOR_PLAN.md](../REFACTOR_PLAN.md) or consult [docs/SYSTEM_ARCHITECTURE.md](../docs/SYSTEM_ARCHITECTURE.md).
