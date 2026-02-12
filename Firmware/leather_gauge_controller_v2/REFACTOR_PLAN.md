# 📋 Refactor Plan: Leather Gauge Controller V2

**Documento Maestro de Refactorización - Clean Architecture + SOLID en C**

---

## 🎯 Resumen Ejecutivo

Este documento establece el plan completo para refactorizar el firmware del **Leather Gauge Controller** desde una arquitectura monolítica a una arquitectura limpia basada en **Clean Architecture** y principios **SOLID**. El objetivo es lograr un sistema modular, testable, y mantenible que permita futuras extensiones sin modificar el código existente.

### Objetivos Estratégicos

| Objetivo                            | Métrica de Éxito                                     | Estado Actual | Estado Objetivo |
| ----------------------------------- | ---------------------------------------------------- | ------------- | --------------- |
| **Desacoplamiento**                 | % de módulos sin dependencias directas a HAL         | ~30%          | 100%            |
| **Testabilidad**                    | % de lógica de negocio testable en PC (sin hardware) | 0%            | 90%             |
| **Inversión de Dependencias**       | Interfaces definidas por Core vs Adapters            | 0/15          | 15/15           |
| **Responsabilidad Única**           | Promedio de razones para cambiar por módulo          | ~3-4          | 1               |
| **Migración sin Downtime**          | Sistema funcional en cada commit                     | ❌            | ✅              |
| **Cobertura de Documentación**      | % de interfaces documentadas (Doxygen)               | ~40%          | 100%            |
| **Performance (latencia medición)** | Tiempo máximo de procesamiento por slice             | ~5ms          | <3ms            |

---

## 🏗️ Arquitectura Propuesta: Clean Architecture en C

### 1. Capas Arquitectónicas

```
┌───────────────────────────────────────────────────────────────────────────┐
│                        📱 PRESENTATION LAYER                               │
│                   (leather_gauge_controller/app)                           │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │ Composition Root (DI Container)                                      │  │
│  │  - lgc_di_container.c: Dependency Injection & Wiring                 │  │
│  │  - lgc_main_task.c: Main control task (ThreadX)                      │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │ Presentation Tasks (UI Controllers)                                  │  │
│  │  - lgc_hmi_task.c: DWIN display state machine                        │  │
│  │  - lgc_printer_task.c: Thermal printer command dispatcher            │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
└────────────────────────────────┬──────────────────────────────────────────┘
                                 │ Dependency Injection (Interfaces)
┌────────────────────────────────▼──────────────────────────────────────────┐
│                          🧠 DOMAIN LAYER (CORE)                            │
│                    (leather_gauge_controller/domain)                       │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │ Entities (Pure data structures)                                      │  │
│  │  - lgc_measurement_entity.h: Measurement, Batch, LeatherPiece       │  │
│  │  - lgc_sensor_array_entity.h: SensorArray, SensorReading            │  │
│  │  - lgc_configuration_entity.h: SystemConfig, CalibrationData        │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │ Use Cases (Business Rules)                                           │  │
│  │  - lgc_uc_measure_area.c: Area integration algorithm                │  │
│  │  - lgc_uc_process_slice.c: Single slice processing logic            │  │
│  │  - lgc_uc_manage_batch.c: Batch creation & finalization             │  │
│  │  - lgc_uc_calibrate_sensors.c: Zero offset calibration              │  │
│  │  - lgc_uc_print_report.c: Generate & format report data             │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
└────────────────────────────────┬──────────────────────────────────────────┘
                                 │ Interfaces (Ports)
┌────────────────────────────────▼──────────────────────────────────────────┐
│                        🔌 INTERFACE LAYER (PORTS)                          │
│                   (leather_gauge_controller/interfaces)                    │
│  - lgc_i_sensor_reader.h: ISensorReader (read array, cascade mode)       │
│  - lgc_i_encoder.h: IEncoder (get_position, attach_callback)             │
│  - lgc_i_storage.h: IStorage (save/load config, batch data)              │
│  - lgc_i_display.h: IDisplay (write_variable, get_variable)              │
│  - lgc_i_printer.h: IPrinter (print_text, print_barcode, cut_paper)      │
│  - lgc_i_digital_inputs.h: IDigitalInputs (get_state, register_callback) │
│  - lgc_i_real_time_clock.h: IRealTimeClock (get/set datetime)            │
│  - lgc_i_communication.h: ICommunication (read_sensors via Modbus/LwPKT) │
└────────────────────────────────┬──────────────────────────────────────────┘
                                 │ Implementations
┌────────────────────────────────▼──────────────────────────────────────────┐
│                     ⚙️ INFRASTRUCTURE LAYER (ADAPTERS)                     │
│                   (leather_gauge_controller/adapters)                      │
│  ┌──────────────────┬───────────────────┬──────────────────────────────┐  │
│  │ Communication    │ Peripherals       │ Storage & Persistence        │  │
│  │ ───────────────  │ ─────────────     │ ──────────────────────       │  │
│  │ lwpkt_adapter/   │ encoder_adapter/  │ eeprom_adapter/              │  │
│  │ - Implements     │ - Implements      │ - Implements IStorage        │  │
│  │   ISensorReader  │   IEncoder        │ - AT24Cxx I2C driver         │  │
│  │ - LwPKT cascade  │ - EXTI GPIO ISR   │ - CRC32 validation           │  │
│  │ - DMA + lwrb RB  │ - Event-driven    │                              │  │
│  │ - 550ms (11 SNS) │ - Critical sync   │ rtc_adapter/                 │  │
│  │                  │                   │ - Implements IRealTimeClock  │  │
│  │ (Legacy/Backup)  │ display_adapter/  │ - STM32 RTC HAL              │  │
│  │ modbus_adapter/  │ - Implements      │ - Mutex protection           │  │
│  │ - Deprecated     │   IDisplay        │                              │  │
│  │ - 2s latency     │ - DWIN UART       │ digital_inputs_adapter/      │  │
│  │                  │                   │ - Implements IDigitalInputs  │  │
│  │                  │ printer_adapter/  │ - GPIO + lwbtn debounce      │  │
│  │                  │ - Implements      │ - Event callbacks            │  │
│  │                  │   IPrinter        │                              │  │
│  │                  │ - ESC/POS USB     │                              │  │
│  └──────────────────┴───────────────────┴──────────────────────────────┘  │
└────────────────────────────────┬──────────────────────────────────────────┘
                                 │
┌────────────────────────────────▼──────────────────────────────────────────┐
│                        🔩 HARDWARE ABSTRACTION LAYER                       │
│  - STM32F4 HAL: UART, I2C, GPIO, DMA, RTC, USB                            │
│  - ThreadX RTOS: Tasks, Mutexes, Semaphores, Events                       │
│  - Third-Party: nanoMODBUS, lwrb, lwprintf, lwbtn, dwin, at24cxx          │
└───────────────────────────────────────────────────────────────────────────┘
```

### 2. Regla de Dependencia (Dependency Rule) 📏

**Ley Fundamental:** Las dependencias SIEMPRE apuntan HACIA ADENTRO (hacia el Core).

```c
// ✅ CORRECTO: Core define la interfaz, Adapter la implementa
// En domain/interfaces/lgc_i_sensor_reader.h
typedef struct ILgcSensorReader_t {
    Result_t (*init)(void *ctx, const LgcSensorConfig_t *config);
    Result_t (*read_all_sensors)(void *ctx, LgcSensorArray_t *out_data);
    Result_t (*read_cascade_mode)(void *ctx, LgcSensorArray_t *out_data);
} ILgcSensorReader_t;

// En adapters/modbus_adapter/lgc_modbus_adapter.c (implementación)
static Result_t modbus_read_all_sensors(void *ctx, LgcSensorArray_t *out_data) {
    // Implementación específica con nanoMODBUS
    // ESTE MÓDULO PUEDE incluir "usart.h" y HAL
}

// ❌ INCORRECTO: Core depende de implementación concreta
// En domain/use_cases/lgc_uc_measure_area.c
#include "lgc_modbus_adapter.h"  // ❌ NUNCA hacer esto
#include "stm32f4xx_hal.h"       // ❌ Prohibido en domain/

// ✅ CORRECTO: Core depende de abstracción
#include "lgc_i_sensor_reader.h"  // ✅ Solo interfaz
Result_t LgcUC_ProcessSlice(ILgcSensorReader_t *sensor, /* ... */);
```

---

## 🎭 Patrón Observer (Publisher-Subscriber Reactivo)

### Problema que Resuelve

**Acoplamiento Actual:** El HMI y la Impresora acceden directamente a estructuras compartidas:

```c
// ❌ ANTES: Acoplamiento directo y polling
// En lgc_hmi_task.c
void lgc_hmi_task_entry(void *param) {
    while (1) {
        tx_mutex_get(&lgc.measurements.mutex, TX_WAIT_FOREVER);
        display_update(lgc.measurements.current_area);  // Polling constante
        tx_mutex_put(&lgc.measurements.mutex);
        tx_thread_sleep(50);  // ❌ Latencia + overhead
    }
}

// En lgc_printer_task.c
void lgc_printer_task_entry(void *param) {
    while (1) {
        if (lgc.measurements.batch_ready) {  // ❌ Polling
            print_batch();
            lgc.measurements.batch_ready = false;
        }
        tx_thread_sleep(100);
    }
}
```

**Problemas:**

- ❌ Polling desperdicia CPU (HMI revisa cada 50ms aunque no haya cambios)
- ❌ Latencia variable (hasta 100ms para detección de batch)
- ❌ Acoplamiento bidireccional (Core y Observers se conocen)
- ❌ No escalable (agregar nuevo observer requiere modificar Core)

### Arquitectura Reactiva (Event-Driven)

```c
// ✅ DESPUÉS: Publisher-Subscriber
//
//          ┌───────────────────────┐
//          │   MEASUREMENT CORE    │ ◄─── Encoder ISR
//          │    (Publisher)        │
//          └───────────┬───────────┘
//                      │ Notify(Event)
//          ┌───────────▼───────────┐
//          │  EVENT DISPATCHER     │
//          └───┬───────────────┬───┘
//              │               │
//      Notify  │               │ Notify
//  ┌───────────▼───┐     ┌─────▼────────┐
//  │  Observer 1:  │     │  Observer 2: │
//  │  HMI Service  │     │  Printer Svc │
//  └───────────────┘     └──────────────┘
//  • Update display      • Print ONLY on
//    on every change       BatchFinished
//  • Send commands:      • Never polls
//    - Delete last
//    - Pause/Resume
//    - Force next batch
```

### Implementación en C Puro

#### 1. Tipos de Evento

```c
// domain/interfaces/lgc_i_event_publisher.h

/**
 * @brief Tipos de eventos publicados por MeasurementCore
 */
typedef enum {
    LGC_EVENT_MEASUREMENT_UPDATED = 0x01,  ///< Nueva medición (cada encoder pulse)
    LGC_EVENT_PIECE_STARTED       = 0x02,  ///< Inicio de pieza detectado
    LGC_EVENT_PIECE_FINISHED      = 0x04,  ///< Pieza finalizada (contador++)
    LGC_EVENT_BATCH_FINISHED      = 0x08,  ///< Lote completo (solo impresora)
    LGC_EVENT_CALIBRATION_DONE    = 0x10,  ///< Calibración completada
    LGC_EVENT_ERROR               = 0x20,  ///< Error crítico
} LgcEventType_t;

/**
 * @brief Estructura de evento (payload polimórfico)
 */
typedef struct {
    LgcEventType_t type;           ///< Tipo de evento
    uint32_t timestamp_ms;         ///< ThreadX tick
    void *data;                    ///< Payload específico (cast según type)
} LgcEvent_t;

/**
 * @brief Payloads específicos por tipo de evento
 */
typedef struct {
    float current_area;            ///< Área acumulada actual (dm²)
    uint16_t active_sensors;       ///< Bitmask de sensores activos
    uint32_t slice_count;          ///< Número de slice procesado
} LgcEventDataMeasurement_t;

typedef struct {
    uint32_t piece_count;          ///< Número de pieza en batch
    float final_area;              ///< Área final de la pieza
} LgcEventDataPieceFinished_t;

typedef struct {
    uint32_t batch_number;         ///< Número de lote
    uint32_t piece_count;          ///< Total de piezas en lote
    float total_area;              ///< Área total del lote
    LgcMeasurement_t pieces[100];  ///< Array de mediciones individuales
} LgcEventDataBatchFinished_t;
```

#### 2. Interfaz Observer

```c
/**
 * @brief Callback signature for observers
 * @param[in] event  Pointer to event structure (read-only)
 * @param[in] context  User-defined context (e.g., observer instance)
 */
typedef void (*LgcEventCallback_t)(const LgcEvent_t *event, void *context);

/**
 * @brief Observer registry entry
 */
typedef struct {
    LgcEventCallback_t callback;   ///< Function to call on event
    void *context;                 ///< User context (passed to callback)
    LgcEventType_t event_mask;     ///< Bitmask of subscribed events
    bool is_active;                ///< Subscription active flag
} LgcObserver_t;

/**
 * @brief Event publisher (Subject)
 */
#define LGC_MAX_OBSERVERS 8

typedef struct {
    LgcObserver_t observers[LGC_MAX_OBSERVERS];
    uint8_t observer_count;
    TX_MUTEX mutex;                ///< Thread-safe registration
} LgcEventPublisher_t;
```

#### 3. API del Publisher

```c
// domain/interfaces/lgc_i_event_publisher.h

/**
 * @brief Initialize event publisher
 */
Result_t LgcEventPublisher_Init(LgcEventPublisher_t *pub);

/**
 * @brief Subscribe observer to events
 * @param[in] pub         Publisher instance
 * @param[in] callback    Observer callback function
 * @param[in] context     User context (e.g., HMI state)
 * @param[in] event_mask  Bitmask of events to subscribe to
 * @return ERR_OK on success, ERR_BUSY if max observers reached
 */
Result_t LgcEventPublisher_Subscribe(
    LgcEventPublisher_t *pub,
    LgcEventCallback_t callback,
    void *context,
    LgcEventType_t event_mask
);

/**
 * @brief Unsubscribe observer
 */
Result_t LgcEventPublisher_Unsubscribe(
    LgcEventPublisher_t *pub,
    LgcEventCallback_t callback
);

/**
 * @brief Notify all subscribers (called by MeasurementCore)
 * @param[in] pub    Publisher instance
 * @param[in] event  Event to dispatch
 * @note Thread-safe, iterates subscribers and calls matching callbacks
 */
Result_t LgcEventPublisher_Notify(
    LgcEventPublisher_t *pub,
    const LgcEvent_t *event
);
```

#### 4. Implementación del Publisher

```c
// domain/event_publisher/lgc_event_publisher.c

Result_t LgcEventPublisher_Notify(
    LgcEventPublisher_t *pub,
    const LgcEvent_t *event
) {
    if (pub == NULL || event == NULL) return ERR_NULL_POINTER;

    tx_mutex_get(&pub->mutex, TX_WAIT_FOREVER);

    for (uint8_t i = 0; i < pub->observer_count; i++) {
        LgcObserver_t *obs = &pub->observers[i];

        // Solo notificar si está suscrito a este tipo de evento
        if (obs->is_active && (obs->event_mask & event->type)) {
            obs->callback(event, obs->context);  // ✅ Llamada polimórfica
        }
    }

    tx_mutex_put(&pub->mutex);
    return ERR_OK;
}
```

#### 5. Uso en MeasurementCore (Publisher)

```c
// domain/use_cases/lgc_uc_measure_area.c

typedef struct {
    ILgcSensorReader_t *sensor_reader;
    ILgcEncoder_t *encoder;
    LgcEventPublisher_t *event_publisher;  // ✅ DI del publisher
    LgcMeasurement_t *measurement;
} LgcMeasureAreaUC_t;

Result_t LgcUC_MeasureArea_ProcessSlice(LgcMeasureAreaUC_t *uc) {
    LgcSensorArray_t sensor_data;
    Result_t res = uc->sensor_reader->read_cascade_mode(uc->sensor_reader->context, &sensor_data);
    if (res != ERR_OK) return res;

    // Procesar datos...
    uc->measurement->current_area += calculated_slice_area;

    // 🎯 Publicar evento (sin saber quién escucha)
    LgcEventDataMeasurement_t payload = {
        .current_area = uc->measurement->current_area,
        .active_sensors = sensor_data.active_bitmask,
        .slice_count = uc->measurement->slice_count
    };
    LgcEvent_t event = {
        .type = LGC_EVENT_MEASUREMENT_UPDATED,
        .timestamp_ms = tx_time_get(),
        .data = &payload
    };
    LgcEventPublisher_Notify(uc->event_publisher, &event);

    // Detectar fin de pieza
    if (detect_leather_end()) {
        LgcEventDataPieceFinished_t piece_payload = {
            .piece_count = uc->measurement->piece_count,
            .final_area = uc->measurement->current_area
        };
        LgcEvent_t piece_event = {
            .type = LGC_EVENT_PIECE_FINISHED,
            .timestamp_ms = tx_time_get(),
            .data = &piece_payload
        };
        LgcEventPublisher_Notify(uc->event_publisher, &piece_event);

        // Si alcanzó max_por_lote -> BatchFinished
        if (uc->measurement->piece_count >= uc->measurement->config.max_per_batch) {
            LgcEventDataBatchFinished_t batch_payload = { /* ... */ };
            LgcEvent_t batch_event = {
                .type = LGC_EVENT_BATCH_FINISHED,
                .timestamp_ms = tx_time_get(),
                .data = &batch_payload
            };
            LgcEventPublisher_Notify(uc->event_publisher, &batch_event);
        }
    }

    return ERR_OK;
}
```

#### 6. Observer 1: HMI Service (Subscriber)

```c
// app/hmi/lgc_hmi_task.c

typedef struct {
    ILgcDisplay_t *display;
    LgcEventPublisher_t *event_publisher;
    // Estado interno HMI
} LgcHmiService_t;

/**
 * @brief Callback HMI (observer)
 */
static void hmi_on_event(const LgcEvent_t *event, void *context) {
    LgcHmiService_t *hmi = (LgcHmiService_t *)context;

    switch (event->type) {
        case LGC_EVENT_MEASUREMENT_UPDATED: {
            LgcEventDataMeasurement_t *data = (LgcEventDataMeasurement_t *)event->data;
            // ✅ Update display SOLO cuando hay cambio real
            hmi->display->write_variable(hmi->display->context,
                                          VP_CURRENT_AREA,
                                          &data->current_area,
                                          sizeof(float));
            break;
        }
        case LGC_EVENT_PIECE_FINISHED: {
            LgcEventDataPieceFinished_t *data = (LgcEventDataPieceFinished_t *)event->data;
            display_show_notification(hmi->display, "Pieza finalizada");
            break;
        }
        default:
            break;  // Ignora eventos no suscritos
    }
}

void lgc_hmi_task_entry(void *param) {
    LgcHmiService_t *hmi = (LgcHmiService_t *)param;

    // ✅ Suscribirse a eventos relevantes
    LgcEventPublisher_Subscribe(
        hmi->event_publisher,
        hmi_on_event,
        hmi,  // Context
        LGC_EVENT_MEASUREMENT_UPDATED | LGC_EVENT_PIECE_FINISHED  // Bitmask
    );

    // Task ahora solo maneja comandos del usuario (buttons, touch)
    while (1) {
        uint32_t button_event;
        if (tx_queue_receive(&hmi->button_queue, &button_event, TX_WAIT_FOREVER) == TX_SUCCESS) {
            // Procesar comandos del usuario:
            if (button_event == BTN_DELETE_LAST) {
                LgcUC_DeleteLastMeasurement();  // ✅ Comando hacia Core
            } else if (button_event == BTN_PAUSE) {
                LgcUC_PauseMeasurement();
            }
        }
    }
}
```

#### 7. Observer 2: Printer Service (Subscriber)

```c
// app/printer/lgc_printer_task.c

typedef struct {
    ILgcPrinter_t *printer;
    LgcEventPublisher_t *event_publisher;
} LgcPrinterService_t;

/**
 * @brief Callback Printer (observer)
 */
static void printer_on_event(const LgcEvent_t *event, void *context) {
    LgcPrinterService_t *printer_svc = (LgcPrinterService_t *)context;

    // ✅ Solo responde a BatchFinished (no polling)
    if (event->type == LGC_EVENT_BATCH_FINISHED) {
        LgcEventDataBatchFinished_t *data = (LgcEventDataBatchFinished_t *)event->data;

        // Imprimir reporte
        printer_svc->printer->print_text("===== Lote Finalizado =====");
        printer_svc->printer->print_text("Número: %u", data->batch_number);
        printer_svc->printer->print_text("Piezas: %u", data->piece_count);
        printer_svc->printer->print_text("Total: %.2f dm²", data->total_area);

        for (uint32_t i = 0; i < data->piece_count; i++) {
            printer_svc->printer->print_text("  %u: %.2f dm²", i+1, data->pieces[i].area);
        }

        printer_svc->printer->cut_paper();
    }
}

void lgc_printer_task_entry(void *param) {
    LgcPrinterService_t *printer_svc = (LgcPrinterService_t *)param;

    // ✅ Suscribirse SOLO a BatchFinished
    LgcEventPublisher_Subscribe(
        printer_svc->event_publisher,
        printer_on_event,
        printer_svc,
        LGC_EVENT_BATCH_FINISHED  // Solo este evento
    );

    // Task en espera pasiva (sin polling)
    while (1) {
        tx_thread_sleep(TX_WAIT_FOREVER);  // Despertado solo por callback
    }
}
```

#### 8. Wiring en DI Container

```c
// app/lgc_di_container.c

void LgcDI_WireComponents(void) {
    static LgcEventPublisher_t event_publisher;
    LgcEventPublisher_Init(&event_publisher);

    // Crear MeasurementCore con publisher
    static LgcMeasureAreaUC_t measure_uc;
    LgcUC_MeasureArea_Init(&measure_uc, sensor, encoder, &event_publisher);

    // Crear HMI Service (observer)
    static LgcHmiService_t hmi_svc = {
        .display = display_adapter,
        .event_publisher = &event_publisher
    };
    tx_thread_create(&hmi_task, ..., lgc_hmi_task_entry, &hmi_svc, ...);

    // Crear Printer Service (observer)
    static LgcPrinterService_t printer_svc = {
        .printer = printer_adapter,
        .event_publisher = &event_publisher
    };
    tx_thread_create(&printer_task, ..., lgc_printer_task_entry, &printer_svc, ...);
}
```

### Ventajas del Observer Pattern

| Aspecto             | Antes (Polling)         | Después (Observer)     |
| ------------------- | ----------------------- | ---------------------- |
| **CPU Usage (HMI)** | 2% (poll cada 50ms)     | 0.1% (solo onChange)   |
| **Latencia Batch**  | Hasta 100ms             | <1ms (inmediato)       |
| **Acoplamiento**    | Bidireccional           | Unidireccional         |
| **Extensibilidad**  | Modificar Core          | Agregar observer nuevo |
| **Testabilidad**    | Difícil (estado global) | Fácil (mock callbacks) |

---

## 🧩 Principios SOLID Aplicados en Embedded C

### S - Single Responsibility Principle (SRP)

**Definición:** Un módulo debe tener una sola razón para cambiar.

**Implementación:**

```c
// ❌ ANTES: lgc_main_task.c (Múltiples responsabilidades)
// - Lee sensores Modbus
// - Procesa algoritmo de medición
// - Actualiza HMI
// - Imprime reportes
// - Gestiona almacenamiento
// Razones para cambiar: 5+

// ✅ DESPUÉS: Cada módulo tiene UNA responsabilidad

// domain/use_cases/lgc_uc_measure_area.c
// Razón para cambiar: Cambio en el algoritmo de integración de área
Result_t LgcUC_MeasureArea_ProcessSlice(
    const LgcSensorArray_t *sensor_data,
    LgcMeasurement_t *measurement
);

// adapters/modbus_adapter/lgc_modbus_adapter.c
// Razón para cambiar: Cambio en protocolo Modbus o hardware UART
Result_t ModbusAdapter_ReadSensors(/* ... */);

// adapters/display_adapter/lgc_display_adapter.c
// Razón para cambiar: Cambio en display DWIN o protocolo
Result_t DisplayAdapter_UpdateMeasurement(/* ... */);
```

### O - Open/Closed Principle (OCP)

**Definición:** Abierto para extensión, cerrado para modificación.

**Implementación:**

```c
// ✅ Agregar nuevo protocolo sin modificar Core
// Paso 1: Core ya define la interfaz ISensorReader
typedef struct ILgcSensorReader_t { /* ... */ } ILgcSensorReader_t;

// Paso 2: Crear nuevo adapter (lgc_lwpkt_adapter.c)
static Result_t lwpkt_read_all_sensors(void *ctx, LgcSensorArray_t *out) {
    // Implementación LwPKT con cascada optimizada
}

ILgcSensorReader_t* LwPktAdapter_GetInterface(LwPktAdapter_t *adapter) {
    static ILgcSensorReader_t iface = {
        .init = lwpkt_init,
        .read_all_sensors = lwpkt_read_all_sensors,
        .read_cascade_mode = lwpkt_read_cascade
    };
    return &iface;
}

// Paso 3: En DI Container, cambiar implementación (1 línea)
// app/lgc_di_container.c
ILgcSensorReader_t *sensor_reader = LwPktAdapter_GetInterface(&lwpkt_adapter);
// vs
ILgcSensorReader_t *sensor_reader = ModbusAdapter_GetInterface(&modbus_adapter);

// ✅ Core NO SE MODIFICA
```

### L - Liskov Substitution Principle (LSP)

**Definición:** Las implementaciones deben cumplir el contrato de la interfaz.

**Implementación:**

```c
// Contrato: ISensorReader debe devolver ERR_OK si los datos son válidos
typedef struct ILgcSensorReader_t {
    /**
     * @brief Read all sensors (11 sensors x 10 channels)
     * @pre  Sensors must be initialized
     * @post out_data contains valid readings OR error is returned
     * @return ERR_OK on success, ERR_TIMEOUT if no response
     */
    Result_t (*read_all_sensors)(void *ctx, LgcSensorArray_t *out_data);
} ILgcSensorReader_t;

// ✅ CORRECTO: ModbusAdapter respeta el contrato
Result_t modbus_read_all_sensors(void *ctx, LgcSensorArray_t *out) {
    if (modbus_transaction_ok()) {
        // Llenar out_data con valores válidos
        return ERR_OK;
    }
    return ERR_TIMEOUT;  // Error explícito
}

// ✅ CORRECTO: LwPktAdapter también respeta el contrato
Result_t lwpkt_read_all_sensors(void *ctx, LgcSensorArray_t *out) {
    if (lwpkt_cascade_read_ok()) {
        // Llenar out_data (67% más rápido pero MISMO contrato)
        return ERR_OK;
    }
    return ERR_TIMEOUT;
}

// ✅ Core puede intercambiarlos sin saber la diferencia
```

### I - Interface Segregation Principle (ISP)

**Definición:** Los clientes no deben depender de interfaces que no usan.

**Implementación:**

```c
// ❌ MAL: Interfaz "god object" con todo
typedef struct ILgcPeripherals_t {
    Result_t (*read_sensors)(/* ... */);
    Result_t (*update_display)(/* ... */);
    Result_t (*print_report)(/* ... */);
    Result_t (*save_config)(/* ... */);
    Result_t (*get_time)(/* ... */);
} ILgcPeripherals_t;  // ❌ UseCase solo necesita sensores pero recibe TODO

// ✅ BIEN: Interfaces cohesivas y específicas
typedef struct ILgcSensorReader_t {
    Result_t (*read_all_sensors)(/* ... */);
    Result_t (*read_cascade_mode)(/* ... */);
} ILgcSensorReader_t;

typedef struct ILgcDisplay_t {
    Result_t (*write_variable)(/* ... */);
    Result_t (*read_variable)(/* ... */);
} ILgcDisplay_t;

typedef struct ILgcStorage_t {
    Result_t (*save_config)(/* ... */);
    Result_t (*load_config)(/* ... */);
} ILgcStorage_t;

// ✅ UseCase solo recibe lo que necesita
Result_t LgcUC_ProcessSlice(
    ILgcSensorReader_t *sensor,  // Solo sensores
    LgcMeasurement_t *measurement
);

Result_t LgcUC_SaveConfiguration(
    ILgcStorage_t *storage,  // Solo almacenamiento
    const LgcSystemConfig_t *config
);
```

### D - Dependency Inversion Principle (DIP)

**Definición:** Módulos de alto nivel NO deben depender de módulos de bajo nivel. Ambos deben depender de abstracciones.

**Implementación:**

```c
// ❌ ANTES: Acoplamiento directo (Violación de DIP)
// En lgc_main_task.c
#include "lgc_inteface_modbus.c"  // Dependencia concreta
void lgc_main_task_entry(void *param) {
    lgc_interface_modbus_init();  // Llamada directa
    while(1) {
        lgc_modbus_read_holding_regs(/* ... */);  // Low-level
    }
}

// ✅ DESPUÉS: Inversión de Dependencias (Cumple DIP)
// En domain/use_cases/lgc_uc_measure_area.c
#include "lgc_i_sensor_reader.h"  // Abstracción

typedef struct {
    ILgcSensorReader_t *sensor_reader;  // Dependencia inyectada
    ILgcEncoder_t *encoder;
    LgcMeasurement_t *measurement;
} LgcMeasureAreaUC_t;

Result_t LgcUC_MeasureArea_Init(
    LgcMeasureAreaUC_t *uc,
    ILgcSensorReader_t *sensor,  // DI: Recibe abstracción
    ILgcEncoder_t *encoder
) {
    uc->sensor_reader = sensor;
    uc->encoder = encoder;
    return ERR_OK;
}

Result_t LgcUC_MeasureArea_ProcessSlice(LgcMeasureAreaUC_t *uc) {
    LgcSensorArray_t data;
    Result_t res = uc->sensor_reader->read_all_sensors(
        uc->sensor_reader->context,
        &data
    );
    // Procesar datos...
}

// En app/lgc_di_container.c (Composition Root)
void LgcDI_WireComponents(void) {
    // Crear adapters concretos
    static ModbusAdapter_t modbus_adapter;
    ModbusAdapter_Init(&modbus_adapter, &huart3);

    // Obtener interfaces
    ILgcSensorReader_t *sensor = ModbusAdapter_GetInterface(&modbus_adapter);
    ILgcEncoder_t *encoder = EncoderAdapter_GetInterface(&encoder_adapter);

    // Inyectar en UseCase (DI)
    static LgcMeasureAreaUC_t measure_uc;
    LgcUC_MeasureArea_Init(&measure_uc, sensor, encoder);
}
```

---

## 📂 Nueva Estructura de Archivos

### Árbol de Directorios Completo

```
lgc_controller/
│
├── domain/                                    🧠 CAPA DE DOMINIO (Pure C)
│   ├── entities/                              Entidades del negocio
│   │   ├── lgc_measurement_entity.h           - LeatherPiece, Batch, Measurement
│   │   ├── lgc_sensor_array_entity.h          - SensorArray (11x10), SensorReading
│   │   ├── lgc_configuration_entity.h         - SystemConfig, CalibrationData
│   │   └── lgc_common_types.h                 - Result_t, DateTime_t, enums
│   │
│   ├── use_cases/                             Reglas de negocio
│   │   ├── measure/
│   │   │   ├── lgc_uc_measure_area.c/h        - Algoritmo integración por slices
│   │   │   ├── lgc_uc_process_slice.c/h       - Procesar single slice
│   │   │   └── lgc_uc_detect_leather.c/h      - Detección inicio/fin pieza
│   │   ├── batch/
│   │   │   ├── lgc_uc_manage_batch.c/h        - Crear/Finalizar batch
│   │   │   └── lgc_uc_calculate_statistics.c/h - Stats (promedio, total)
│   │   ├── calibration/
│   │   │   └── lgc_uc_calibrate_sensors.c/h   - Zero offset, validación
│   │   ├── reporting/
│   │   │   ├── lgc_uc_generate_report.c/h     - Formatear datos para impresión
│   │   │   └── lgc_uc_export_batch_data.c/h   - Serializar para almacenamiento
│   │   └── configuration/
│   │       └── lgc_uc_manage_config.c/h       - Validar, persistir config
│   │
│   └── interfaces/                            🔌 PORTS (Abstracciones)
│       ├── lgc_i_sensor_reader.h              - ISensorReader (read array)
│       ├── lgc_i_encoder.h                    - IEncoder (position, callbacks)
│       ├── lgc_i_storage.h                    - IStorage (save/load)
│       ├── lgc_i_display.h                    - IDisplay (read/write VP)
│       ├── lgc_i_printer.h                    - IPrinter (print, cut)
│       ├── lgc_i_digital_inputs.h             - IDigitalInputs (buttons)
│       ├── lgc_i_real_time_clock.h            - IRealTimeClock (get/set time)
│       └── lgc_i_communication.h              - ICommunication (generic comms)
│
├── adapters/                                  ⚙️ CAPA DE INFRAESTRUCTURA (HAL + Libs)
│   ├── communication/
│   │   ├── modbus_adapter/                    Adapter Modbus RTU
│   │   │   ├── lgc_modbus_adapter.c/h         - Implementa ISensorReader
│   │   │   └── lgc_modbus_config.h            - Timeouts, baudrate
│   │   └── lwpkt_adapter/                     Adapter LwPKT (futuro)
│   │       ├── lgc_lwpkt_adapter.c/h          - Implementa ISensorReader
│   │       └── lgc_lwpkt_cascade.c/h          - Modo cascada optimizado
│   │
│   ├── peripherals/
│   │   ├── encoder_adapter/
│   │   │   └── lgc_encoder_adapter.c/h        - Implementa IEncoder (EXTI GPIO)
│   │   ├── display_adapter/
│   │   │   └── lgc_display_adapter.c/h        - Implementa IDisplay (DWIN UART)
│   │   ├── printer_adapter/
│   │   │   └── lgc_printer_adapter.c/h        - Implementa IPrinter (ESC/POS USB)
│   │   └── digital_inputs_adapter/
│   │       └── lgc_digital_inputs_adapter.c/h - Implementa IDigitalInputs (lwbtn)
│   │
│   └── storage/
│       ├── eeprom_adapter/
│       │   ├── lgc_eeprom_adapter.c/h         - Implementa IStorage (AT24Cxx I2C)
│       │   └── lgc_eeprom_crc.c/h             - CRC32 para validación
│       └── rtc_adapter/
│           └── lgc_rtc_adapter.c/h            - Implementa IRealTimeClock (STM32 RTC)
│
├── app/                                       📱 CAPA DE APLICACIÓN (Composition Root)
│   ├── inc/
│   │   ├── lgc.h                              - API pública del sistema
│   │   ├── lgc_typedefs.h                     - Tipos legacy (deprecar gradualmente)
│   │   └── lgc_di_container.h                 - DI Container público
│   │
│   └── src/
│       ├── lgc.c                              - Inicialización sistema
│       ├── lgc_di_container.c                 - Dependency Injection & Wiring
│       ├── lgc_main_task.c                    - Main control task (ThreadX)
│       ├── lgc_mem_pool.c                     - Memory pool management
│       ├── hmi/
│       │   ├── lgc_hmi_task.c                 - HMI update task
│       │   └── lgc_hmi.h                      - VP address definitions
│       └── printer/
│           └── lgc_printer_task.c             - Printer command task
│
├── middlewares/                               📦 THIRD-PARTY (Sin cambios)
│   ├── nanoMODBUS/
│   ├── lwrb/
│   ├── lwprintf/
│   ├── lwbtn/
│   ├── dwin/
│   └── at24cxx/
│
├── osal/                                      🔄 OS ABSTRACTION (Sin cambios)
│   ├── common/
│   ├── include/
│   └── portable/threadx/
│
└── config/                                    ⚙️ CONFIGURACIÓN
    ├── lgc_domain_config.h                    - Constants del dominio
    ├── lgc_hardware_config.h                  - Hardware-specific (pins, clocks)
    └── lwbtn_opts.h, lwprintf_opts.h, etc.
```

### Cambios de Nomenclatura

| Antes (Legacy)            | Después (Clean Arch)                        | Razón                                    |
| ------------------------- | ------------------------------------------- | ---------------------------------------- |
| `lgc_module_modbus.c`     | `lgc_modbus_adapter.c`                      | Claridad (Adapter pattern explícito)     |
| `lgc_inteface_printer.h`  | `lgc_i_printer.h` + `lgc_printer_adapter.c` | Separar Port (I) de Adapter (Impl)       |
| `lgc_module_eeprom.c`     | `lgc_eeprom_adapter.c`                      | Consistencia (todos los adapters)        |
| `lgc.c` (monolito)        | `lgc_uc_measure_area.c`, `lgc_uc_*.c`       | SRP (un UseCase por responsabilidad)     |
| `lgc_typedefs.h` (mezcla) | `entities/lgc_*_entity.h`                   | Separar Entities de DTOs                 |
| `modules/`                | `adapters/`                                 | Nomenclatura Clean Architecture estándar |
| `lgc_system_init()`       | `LgcDI_WireComponents()`                    | Explicitar DI Container                  |

---

## 🔄 Estrategia de Migración Incremental

### Principios de Migración

1. ✅ **No Big Bang:** Refactorizar módulo por módulo
2. ✅ **Funcionalidad intacta:** Sistema operativo en cada commit
3. ✅ **Branch por Feature:** `feat/clean-arch-sensor-reader`, `feat/clean-arch-encoder`, etc.
4. ✅ **Tests primero:** TDD donde sea posible (mocks de interfaces)
5. ✅ **Documentación paralela:** Actualizar docs con cada cambio

### Fases de Implementación (12 semanas)

#### 📅 **Fase 1: Fundaciones (Semanas 1-2)**

**Objetivo:** Establecer estructura base de Clean Architecture

- [ ] **T1.1:** Crear estructura de carpetas `domain/`, `adapters/`, `app/`
- [ ] **T1.2:** Definir `Result_t`, tipos comunes en `lgc_common_types.h`
- [ ] **T1.3:** Crear entidades puras:
  - `lgc_measurement_entity.h`
  - `lgc_sensor_array_entity.h`
  - `lgc_configuration_entity.h`
- [ ] **T1.4:** Setup DI Container básico (`lgc_di_container.c`)
- [ ] **T1.5:** Documentar convenciones de código en `.github/copilot-instructions.md`

**Validación:** Compilación exitosa, sistema legacy sigue funcionando.

#### 📅 **Fase 2: Adapters Críticos (Semanas 3-5)**

**Objetivo:** Migrar módulos más críticos con DIP + Implementar LwPKT

- [ ] **T2.1: Encoder Adapter** (Semana 3)
  - Definir `lgc_i_encoder.h` (IEncoder)
  - Implementar `lgc_encoder_adapter.c`
  - Integrar en DI Container
  - **CRÍTICO:** Encoder ISR dispara lectura de sensores (sincronización absoluta)
  - **Test:** Verificar callbacks de pulsos (latencia <500µs desde ISR hasta lectura)
- [ ] **T2.2: Sensor Reader Adapter - LwPKT** (Semana 4) 🔥 **PRIORIDAD MÁXIMA**
  - Definir `lgc_i_sensor_reader.h` (ISensorReader con método `read_cascade_mode`)
  - **Crear nuevo:** `lgc_lwpkt_adapter.c` (NO migrar Modbus legacy)
  - Implementar modo cascada (FLAGS field 1-11):
    - `lwpkt_init()` con DMA + lwrb ring buffers (UART3 RS-485)
    - `lwpkt_read_cascade()`: 1 broadcast → 11 respuestas secuenciales
    - Parser: Extraer payload (uint16_t bitmask × 11 sensores, 2 bytes cada uno)
    - Callback UART RX (DMA): `HAL_UART_RxCpltCallback()` → lwpkt_read() → Semaphore signal
  - **Test:**
    - Lectura cascada completa en <600ms (target: 550ms)
    - Validar CRC-8 de cada paquete
    - Simular pérdida de 1 sensor (timeout 100ms/sensor)
  - **Documentar:** Estructura de paquete LwPKT en `docs/sensor/README.md`
- [ ] **T2.3: Storage Adapter - EEPROM** (Semana 5)
  - Definir `lgc_i_storage.h` (IStorage)
  - Refactorizar `lgc_eeprom_adapter.c`
  - Agregar CRC32 validation (`lgc_eeprom_crc.c`)
  - **Test:** Save/Load config 100 veces sin errores

**Validación:** Sistema mide áreas correctamente con LwPKT (67% más rápido que Modbus legacy).

**Detalle Técnico T2.2 - Implementación LwPKT:**

```c
// adapters/communication/lwpkt_adapter/lgc_lwpkt_adapter.c

typedef struct {
    lwpkt_t lwpkt;                  ///< LwPKT instance
    lwrb_t tx_rb, rx_rb;            ///< Ring buffers (DMA)
    UART_HandleTypeDef *huart;      ///< UART3 (RS-485)
    uint8_t tx_buffer[256];         ///< TX ring buffer memory
    uint8_t rx_buffer[512];         ///< RX ring buffer memory (11 × 44 bytes)
    TX_SEMAPHORE rx_sem;            ///< Semaphore para espera de respuesta
    LgcSensorArray_t last_reading;  ///< Cache última lectura
} LwPktAdapter_t;

static LwPktAdapter_t s_lwpkt_adapter;

/**
 * @brief Implementa ISensorReader->read_cascade_mode()
 * @note Latencia objetivo: <550ms para 11 sensores
 */
static Result_t lwpkt_read_cascade_mode(void *ctx, LgcSensorArray_t *out_data) {
    LwPktAdapter_t *adapter = (LwPktAdapter_t *)ctx;

    // 1. Enviar comando broadcast: CMD_READ_CASCADE + FLAGS=1
    uint8_t cmd_data[] = {0x12};  // CMD_READ_CASCADE
    lwpkt_write(&adapter->lwpkt,
                 0xFF,             // Broadcast address
                 0x12,             // Command
                 1,                // FLAGS (inicia en sensor 1)
                 cmd_data, 0);     // Sin payload

    // 2. Esperar 11 respuestas secuenciales
    for (uint8_t sensor_idx = 0; sensor_idx < 11; sensor_idx++) {
        // Esperar respuesta con timeout de 100ms
        if (tx_semaphore_get(&adapter->rx_sem, TX_MS_TO_TICKS(100)) != TX_SUCCESS) {
            // Timeout: marcar sensor como inactivo
            adapter->last_reading.sensors[sensor_idx].status = 0x0000;
            continue;
        }

        // 3. Parsear paquete LwPKT recibido
        lwpkt_t *pkt = &adapter->lwpkt;
        if (pkt->data_len == 2) {  // uint16_t = 2 bytes
            // Extraer bitmask directo (el sensor ya hizo la conversión analógica→digital)
            uint16_t bitmask = *((uint16_t *)pkt->data);
            adapter->last_reading.sensors[sensor_idx].status = bitmask;
            // bits 0-9: estado de 10 fotodiodos (1=cuero detectado, 0=vacío)
            // El sensor aplicó threshold internamente antes de enviar
        }
    }

    // 4. Copiar resultado
    memcpy(out_data, &adapter->last_reading, sizeof(LgcSensorArray_t));
    return ERR_OK;
}

/**
 * @brief Callback UART RX (DMA, ISR)
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart == s_lwpkt_adapter.huart) {
        size_t len;
        uint8_t *data = lwrb_get_linear_block_read_address(&s_lwpkt_adapter.rx_rb, &len);

        // Procesar bytes recibidos
        for (size_t i = 0; i < len; i++) {
            lwpktr_t res = lwpkt_read(&s_lwpkt_adapter.lwpkt, data[i]);
            if (res == lwpktVALID) {
                // Paquete completo y válido (CRC OK)
                tx_semaphore_put(&s_lwpkt_adapter.rx_sem);  // Signal
                break;
            }
        }
        lwrb_skip(&s_lwpkt_adapter.rx_rb, len);
    }
}

// Vtable: Exponer interfaz ISensorReader
ILgcSensorReader_t* LwPktAdapter_GetInterface(LwPktAdapter_t *adapter) {
    static ILgcSensorReader_t iface = {
        .context = adapter,
        .init = lwpkt_init,
        .read_all_sensors = lwpkt_read_individual,  // Modo legacy (no usar)
        .read_cascade_mode = lwpkt_read_cascade_mode  // ✅ Modo rápido
    };
    return &iface;
}
```

**Flujo Temporal Crítico (Encoder-Driven):**

```
T=0ms:   Encoder ISR → Set Event Flag → Despierta Main Task
T=0.5ms: Main Task ejecuta LgcUC_MeasureArea_ProcessSlice()
         ↓
         sensor_reader->read_cascade_mode()  ← LwPKT broadcast
T=1ms:   Sensor 1 responde (FLAGS=2)
T=50ms:  Sensor 2 responde (FLAGS=3)
...      (cada sensor ~50ms)
T=550ms: Sensor 11 responde (FLAGS=0, fin cascada)
T=551ms: Main Task: Procesar datos → Publicar evento MEASUREMENT_UPDATED
T=552ms: HMI Observer actualiza display (no polling)

TOTAL: ~552ms desde encoder pulse hasta UI actualizada
```

**Comparación Latencias:**

| Fase                    | Modbus RTU (Legacy) | LwPKT Cascada (Actual) | Mejora  |
| ----------------------- | ------------------- | ---------------------- | ------- |
| Read 11 Sensors         | 1800-2000ms         | 500-550ms              | **73%** |
| Process Slice           | 5ms                 | 3ms (optimizado)       | 40%     |
| Update HMI              | Polling 50ms avg    | Event <1ms             | **98%** |
| **TOTAL Critical Path** | ~2050ms             | **~553ms**             | **73%** |

#### 📅 **Fase 3: Use Cases (Semanas 6-8)**

**Objetivo:** Extraer lógica de negocio a Use Cases

- [ ] **T3.1: Measure Area UseCase** (Semana 6)
  - Crear `lgc_uc_measure_area.c`
  - Extraer algoritmo de integración de `lgc_main_task.c`
  - Inyectar `ISensorReader` + `IEncoder`
  - **Test:** Comparar áreas medidas vs sistema legacy (delta <0.1%)
- [ ] **T3.2: Batch Management UseCase** (Semana 7)
  - Crear `lgc_uc_manage_batch.c`
  - Extraer lógica de batches
  - **Test:** Crear 200 batches + persistir
- [ ] **T3.3: Calibration UseCase** (Semana 8)
  - Crear `lgc_uc_calibrate_sensors.c`
  - Validación de offsets
  - **Test:** Calibración manual + verificación

**Validación:** Lógica de negocio desacoplada, testable en PC.

#### 📅 **Fase 4: Peripherals Externos (Semanas 9-10)**

**Objetivo:** Refactorizar HMI, Printer, Inputs

- [ ] **T4.1: Display Adapter** (Semana 9)
  - Definir `lgc_i_display.h`
  - Implementar `lgc_display_adapter.c` (DWIN)
  - Refactorizar `lgc_hmi_task.c` para usar IDisplay
  - **Test:** Write/Read 100 variables VP sin errores
- [ ] **T4.2: Printer Adapter** (Semana 9)
  - Definir `lgc_i_printer.h`
  - Implementar `lgc_printer_adapter.c` (ESC/POS)
  - **Test:** Imprimir reporte completo
- [ ] **T4.3: Digital Inputs Adapter** (Semana 10)
  - Definir `lgc_i_digital_inputs.h`
  - Implementar `lgc_digital_inputs_adapter.c` (lwbtn)
  - **Test:** Simular presión de botones (50 eventos)

**Validación:** UI funcional con arquitectura limpia.

#### 📅 **Fase 5: RTC & Reporting (Semana 11)**

**Objetivo:** Completar adapters restantes

- [ ] **T5.1: RTC Adapter**
  - Definir `lgc_i_real_time_clock.h`
  - Refactorizar `lgc_rtc_adapter.c` (era `lgc_module_rtc.c`)
  - **Test:** Set/Get datetime 1000 veces (thread-safe)
- [ ] **T5.2: Reporting UseCase**
  - Crear `lgc_uc_generate_report.c`
  - Formatear datos para impresora
  - **Test:** Generar reporte de 200 piezas

**Validación:** Reportes impresos correctos.

#### 📅 **Fase 6: Testing & Documentation (Semana 12)**

**Objetivo:** Validación final y documentación completa

- [ ] **T6.1:** Unit tests (mocks) para Use Cases críticos
- [ ] **T6.2:** Integration tests en hardware real (24h stress test)
- [ ] **T6.3:** Profiling de performance (latencia <3ms por slice)
- [ ] **T6.4:** Documentación Doxygen completa (100% interfaces)
- [ ] **T6.5:** Actualizar README.md con nueva arquitectura
- [ ] **T6.6:** Crear ARCHITECTURE.md (diagramas C4)

**Validación:** Sistema production-ready.

---

## 🧪 Estrategia de Testing

### 1. Unit Testing (PC sin Hardware)

**Framework:** Unity + CMock

```c
// tests/test_lgc_uc_measure_area.c
#include "unity.h"
#include "mock_lgc_i_sensor_reader.h"
#include "mock_lgc_i_encoder.h"
#include "lgc_uc_measure_area.h"

void test_ProcessSlice_AllSensorsActive_CalculatesCorrectArea(void) {
    // Arrange
    LgcSensorArray_t mock_data = { /* 11 sensores activos */ };
    LgcMeasurement_t measurement = {0};

    ISensorReader_ReadAllSensors_ExpectAndReturn(&mock_data, ERR_OK);

    // Act
    Result_t res = LgcUC_MeasureArea_ProcessSlice(&uc, &measurement);

    // Assert
    TEST_ASSERT_EQUAL(ERR_OK, res);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 15.75, measurement.current_area);
}
```

### 2. Integration Testing (Hardware)

**Escenarios:**

1. **Test de Latencia:** Medir 1000 slices → Promedio <3ms
2. **Test de Stress:** 24h continuas midiendo → 0 memory leaks
3. **Test de Precisión:** Comparar vs réglete manual → Delta <0.5%
4. **Test de Modbus:** Timeout rate <0.1% en 10,000 lecturas

### 3. Regression Testing

**Baseline:** Sistema legacy (commit `abc123`)  
**Método:** Comparar mediciones de misma pieza antes/después refactor  
**Criterio:** Delta promedio <0.1%, máximo 0.3%

---

## 📊 Métricas de Calidad

### Cobertura de Código

| Capa              | Target | Método                       |
| ----------------- | ------ | ---------------------------- |
| **Domain (Core)** | >90%   | Unity + Mocks (PC)           |
| **Use Cases**     | >85%   | Unity + Mocks                |
| **Adapters**      | >70%   | Integration tests (Hardware) |
| **App Layer**     | >60%   | Integration tests            |

### Performance

| Métrica                        | Target | Medición                 |
| ------------------------------ | ------ | ------------------------ |
| Latencia ProcessSlice          | <3ms   | Timer + Logic Analyzer   |
| Read Sensors (Modo Individual) | <2s    | Timestamp inicio/fin     |
| Read Sensors (Modo Cascada)    | <550ms | Protocolo LwPKT (futuro) |
| Memory Footprint (RAM)         | <50KB  | Map file analysis        |
| Flash Usage                    | <200KB | .elf size                |
| Stack Usage (Main Task)        | <4KB   | Stack painting           |

### Complejidad Ciclomática

| Función                          | Max CC | Actual | Estado |
| -------------------------------- | ------ | ------ | ------ |
| `LgcUC_MeasureArea_ProcessSlice` | 10     | TBD    | ⏳     |
| `ModbusAdapter_ReadSensors`      | 8      | TBD    | ⏳     |
| `LgcDI_WireComponents`           | 5      | TBD    | ⏳     |

---

## 🔒 Reglas de Código Estrictas

### 1. Prohibiciones Absolutas

```c
// ❌ NUNCA: malloc/free (embedded-unsafe)
uint8_t *buffer = malloc(256);  // PROHIBIDO

// ❌ NUNCA: Includes de HAL en domain/
#include "stm32f4xx_hal.h"  // Solo en adapters/

// ❌ NUNCA: Variables globales mutables en Core
static LgcMeasurement_t g_measurement;  // ❌ Usar estructuras pasadas por parámetro

// ❌ NUNCA: Dependencias circulares
// A.h incluye B.h, B.h incluye A.h
```

### 2. Tipos de Datos

```c
// ✅ SIEMPRE: stdint.h
uint8_t, int16_t, uint32_t, int64_t

// ✅ SIEMPRE: stdbool.h
bool, true, false

// ❌ NUNCA: Tipos nativos en lógica
int, short, long, char  // Solo permitidos en string literals
```

### 3. Gestión de Memoria

```c
// ✅ Allocation estática
static uint8_t s_tx_buffer[256];
static LgcMeasurement_t s_measurement_data[LGC_LEATHER_COUNT_MAX];

// ✅ ThreadX Byte Pools (si es necesario)
TX_BYTE_POOL *byte_pool;
tx_byte_allocate(byte_pool, (void **)&ptr, size, TX_NO_WAIT);

// ❌ Dynamic allocation
malloc(), calloc(), realloc(), free()  // PROHIBIDO
```

### 4. Manejo de Errores

```c
// ✅ SIEMPRE: Retornar Result_t
typedef enum {
    ERR_OK = 0,
    ERR_ERROR,
    ERR_NULL_POINTER,
    ERR_TIMEOUT,
    ERR_BUSY,
    ERR_INVALID_PARAM
} Result_t;

Result_t LgcUC_ProcessSlice(/* ... */) {
    if (sensor == NULL) return ERR_NULL_POINTER;
    if (!is_initialized) return ERR_BUSY;

    Result_t res = sensor->read_all_sensors(/* ... */);
    if (res != ERR_OK) return res;

    return ERR_OK;
}

// ✅ SIEMPRE: Verificar returns
Result_t res = LgcUC_ProcessSlice(/* ... */);
if (res != ERR_OK) {
    // Handle error
}
```

### 5. Documentación Doxygen

```c
/**
 * @brief  Process a single measurement slice
 * @note   Thread-safe cuando se usa con mutex externo
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

### 6. Nomenclatura

| Elemento                | Formato                   | Ejemplo                                          |
| :---------------------- | :------------------------ | :----------------------------------------------- |
| **Archivos**            | `snake_case`              | `lgc_uc_measure_area.c`, `lgc_i_sensor_reader.h` |
| **Tipos/Structs**       | `PascalCase` + `_t`       | `LgcMeasurement_t`, `LgcSensorArray_t`           |
| **Interfaces**          | `I` + `PascalCase` + `_t` | `ILgcSensorReader_t`, `ILgcEncoder_t`            |
| **Funciones Públicas**  | `Module_Action`           | `LgcUC_ProcessSlice`, `ModbusAdapter_Init`       |
| **Funciones Privadas**  | `snake_case` (static)     | `calculate_slice_area`, `validate_config`        |
| **Variables Locales**   | `snake_case`              | `sensor_data`, `current_index`                   |
| **Variables Estáticas** | `s_` + `snake_case`       | `s_is_initialized`, `s_tx_buffer`                |
| **Constantes/Macros**   | `UPPER_SNAKE_CASE`        | `LGC_SENSOR_NUMBER`, `MAX_BATCH_SIZE`            |

---

## 🛠️ Herramientas y Configuración

### Build System

```bash
# Compilar proyecto
make -C Debug all

# Limpiar
make -C Debug clean

# Flashear
openocd -f interface/stlink.cfg \
        -f target/stm32f4x.cfg \
        -c "program Debug/leather_gauge_controller.elf verify reset exit"
```

### Static Analysis

```bash
# Cppcheck
cppcheck --enable=all --inconclusive \
         --suppress=missingIncludeSystem \
         leather_gauge_controller/

# Clang-Tidy
clang-tidy leather_gauge_controller/domain/**/*.c \
           -checks='readability-*,modernize-*'
```

### Profiling & Debug

```bash
# Stack usage analysis
arm-none-eabi-readelf -s Debug/leather_gauge_controller.elf | grep -E '(FUNC|OBJECT)'

# Memory map
arm-none-eabi-nm --size-sort --print-size Debug/leather_gauge_controller.elf

# Disassembly (verificar optimizaciones)
arm-none-eabi-objdump -S Debug/leather_gauge_controller.elf > disassembly.txt
```

---

## 📝 Checklist de Aceptación (Definition of Done)

Para considerar la refactorización COMPLETA:

- [ ] **Arquitectura:**
  - [ ] Todas las interfaces (Ports) definidas (15/15)
  - [ ] Todos los adapters implementan interfaces (15/15)
  - [ ] DI Container funcional y documentado
  - [ ] Diagrama de componentes actualizado (C4)
- [ ] **Código:**
  - [ ] 0 includes de HAL en `domain/`
  - [ ] 0 variables globales mutables en Core
  - [ ] 0 `malloc` en todo el proyecto
  - [ ] Complejidad ciclomática <10 en funciones críticas
- [ ] **Testing:**
  - [ ] Cobertura Core >90%
  - [ ] Cobertura Use Cases >85%
  - [ ] 24h stress test sin errores
  - [ ] Regression tests pass (delta <0.1%)
- [ ] **Performance:**
  - [ ] Latencia ProcessSlice <3ms (promedio)
  - [ ] Memory footprint <50KB RAM
  - [ ] Flash usage <200KB
- [ ] **Documentación:**
  - [ ] 100% interfaces documentadas (Doxygen)
  - [ ] README.md actualizado
  - [ ] ARCHITECTURE.md creado
  - [ ] `.github/copilot-instructions.md` actualizado
- [ ] **Validación:**
  - [ ] Sistema legacy reemplazado 100%
  - [ ] HMI funcional
  - [ ] Impresora funcional
  - [ ] Almacenamiento EEPROM OK
  - [ ] Modbus 11 sensores OK

---

## 🚀 Siguientes Pasos Inmediatos

1. **Crear estructura de carpetas** (Fase 1.1)
2. **Definir entidades** (`lgc_measurement_entity.h`, etc.)
3. **Setup DI Container básico**
4. **Refactorizar Encoder Adapter** (módulo más simple, buen starting point)
5. **Documentar decisiones arquitectónicas** en commits

---

**Última actualización:** 11 de Febrero de 2026  
**Versión del Plan:** v2.0 (Clean Architecture)  
**Responsable:** Arquitecto Senior de Firmware
