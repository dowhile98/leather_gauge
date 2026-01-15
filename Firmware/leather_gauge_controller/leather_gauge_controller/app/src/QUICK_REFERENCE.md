Actúa como un experto en sistemas embebidos y C (STM32/RTOS). Necesito implementar la lógica de procesamiento para una máquina medidora de cuero basada en el archivo `lgc_main_task.c` proporcionado.

## Contexto del Hardware
- **Eje X (Ancho):** 11 sensores esclavos Modbus. Cada sensor lee 10 fotoreceptores.
  - La data llega en un `uint16_t` por sensor.
  - Los bits 0-9 representan los 10 fotoreceptores (1 = cuero detectado, 0 = vacío).
  - En total hay 110 puntos de medición de ancho.
- **Eje Y (Largo):** Un encoder rotatorio dispara una interrupción o semáforo (`encoder_flag`) cada vez que la banda transportadora avanza una distancia unitaria (step).

## Estructuras de Datos
Usa estrictamente estas definiciones:

```c
// Configuración del lote (ingresada por usuario)
typedef struct __attribute__((__packed__)) LGC_CONF_TypeDef {
    char client_name[12];
    char color[10];
    char leather_id[20];
    uint32_t batch_limit;      // Cantidad de cueros por lote
    uint8_t units;             // dm2, ft2, m2
    uint8_t conversion;
    uint32_t crc;
} LGC_CONF_TypeDef_t;

// Resultados de medición
typedef struct {
    uint16_t current_batch_index;   // Índice del lote actual
    uint16_t current_leather_index; // Índice del cuero dentro del lote actual
    float current_leather_area;     // Acumulador de área del cuero pasando actualmente
    float leather_measurement[LGC_LEATHER_COUNT_MAX]; // Historial de áreas individuales
    float batch_measurement[LGC_LEATHER_BATCH_COUNT_MAX]; // Historial de sumas por lote
    uint8_t is_measuring;           // Flag de estado: ¿Hay un cuero pasando?
} lgc_measurements_t;
Requerimientos Funcionales
Dentro de lgc_main_task_entry, en el estado LGC_RUNNING, implementa la lógica siguiente cada vez que se obtiene el semáforo del encoder:

Lectura Modbus: Itera los 11 sensores. Si la lectura es exitosa, procesa los bits. Si falla, maneja el error (ya existe código base para esto, intégralo).

Cálculo de "Rebanada" (Slice):

Por cada pulso de encoder, calcula el ancho instantáneo del cuero sumando los bits activos de todos los sensores (11 sensores * 10 bits).

Multiplica bits_activos * ANCHO_PIXEL * LARGO_STEP para obtener el área diferencial.

Máquina de Estados de Detección (Start/Stop de Cuero):

Inicio de Cuero: Si bits_activos > 0 y is_measuring == 0, inicia un nuevo cuero. Limpia el acumulador current_leather_area.

Fin de Cuero: Si bits_activos == 0 por un número consecutivo de pasos (histéresis) y is_measuring == 1, finaliza el cuero.

Gestión de Lotes (Batch Logic):

Al finalizar un cuero:

Guarda el área en leather_measurement[current_leather_index].

Acumula el área en batch_measurement[current_batch_index].

Incrementa current_leather_index.

Si current_leather_index alcanza config.batch_limit:

Reinicia current_leather_index a 0.

Incrementa current_batch_index.

Valida no exceder los límites de los arrays (LGC_LEATHER_COUNT_MAX, etc.).

Tarea
Genera el código C necesario para completar el case LGC_RUNNING dentro de lgc_main_task.c.

Crea una función auxiliar static void lgc_process_measurement(...) para mantener el main limpio.

Define macros para LGC_PIXEL_WIDTH_MM y LGC_ENCODER_STEP_MM (asume valores por defecto, yo los ajustaré).

Asegúrate de iterar solo los bits 0 a 9 de cada registro del sensor (data.sensor[i]).# Quick Reference Card - Leather Gauge

## Constants (Ajustables)

```c
LGC_PIXEL_WIDTH_MM = 10.0f      // Cambiar si ancho real ≠ 10mm
LGC_ENCODER_STEP_MM = 5.0f      // Cambiar si paso real ≠ 5mm
LGC_LEATHER_END_HYSTERESIS = 3  // Aumentar si hay ruido
```

## Global Variables

```c
data                    // Sensor raw data + status
measurements            // All measurement results
encoder_flag           // Semaphore for encoder pulses
mutex                  // Mutex for thread safety
```

## Main Functions

| Función | Retorna | Descripción |
|---------|---------|------------|
| `lgc_count_active_bits()` | uint16_t (0-110) | Fotoreceptores activos |
| `lgc_calculate_slice_area(bits)` | float | Area mm² de bits dados |
| `lgc_process_measurement(config)` | void | Procesa pulso del encoder |

## Measurement Structure

```c
measurements.current_batch_index       // 0-199
measurements.current_leather_index     // 0-299
measurements.current_leather_area      // Running total mm²
measurements.is_measuring              // 0 o 1
measurements.no_detection_count        // Hysteresis counter
measurements.leather_measurement[]     // Array de áreas
measurements.batch_measurement[]       // Array de lotes
```

## Sensor Data Structure

```c
data.state                             // LGC_STOP, RUNNING, FAIL
data.sensor[11]                        // 11 sensores uint16_t
data.sensor_status                     // Bitmap de errores
```

## Error Handling

```c
if (data.sensor_status != NO_ERROR)
    // Algún sensor falló - skipped medición
else
    lgc_process_measurement(&config)   // OK - procesar
```

## Array Limits

```c
LGC_SENSOR_NUMBER = 11                 // Sensores
LGC_PHOTORECEPTORS_PER_SENSOR = 10     // Bits por sensor
LGC_LEATHER_COUNT_MAX = 300            // Max cueros individuales
LGC_LEATHER_BATCH_COUNT_MAX = 200      // Max lotes
```

## Area Calculation Formula

```
Área = (bits_activos × ancho_pixel_mm) × paso_encoder_mm
     = (bits × 10) × 5
     = bits × 50 mm²
```

**Ejemplos**:
- 1 bit:   50 mm²
- 10 bits: 500 mm²
- 50 bits: 2500 mm²
- 110 bits (max): 5500 mm²

## Detection State Machine

```
No bits      → Idle (is_measuring=0)
↓
Bits detected → Start (is_measuring=1, area=0)
↓
More bits    → Accumulate (area+=slice)
↓
No bits (3×) → End (save, batch++, idx++)
↓
Back to Idle
```

## Batch Logic

```
batch_limit = 10 (example)

Cuero 0-9   → batch[0]
Cuero 10-19 → batch[1]
...
Cuero 190-199 → batch[19]
```

When `leather_idx >= batch_limit`:
```
leather_idx = 0  (reset para nuevo lote)
batch_idx++      (ir al siguiente lote)
```

## Access Patterns

### Read Current Measurement
```c
osAcquireMutex(&mutex);
float area = measurements.current_leather_area;
uint16_t idx = measurements.current_leather_index;
osReleaseMutex(&mutex);
```

### Iterate All Leathers in Batch
```c
for (int i = batch*10; i < (batch+1)*10; i++) {
    float area = measurements.leather_measurement[i];
}
```

### Sum All Batch
```c
float total = measurements.batch_measurement[batch_idx];
```

## Debugging Commands

```c
// Ver bits activos
printf("Bits: %u\n", lgc_count_active_bits());

// Ver área actual
printf("Area: %.2f mm²\n", measurements.current_leather_area);

// Ver estado
printf("Measuring: %u, idx: %u/%u\n",
       measurements.is_measuring,
       measurements.current_leather_index,
       10);  // batch_limit

// Ver sensores
for (int i = 0; i < 11; i++) {
    printf("S%u: 0x%04X\n", i, data.sensor[i]);
}
```

## Common Issues & Fixes

| Problema | Causa | Solución |
|----------|-------|----------|
| Sensores siempre fallan | Modbus timeout | Aumentar NMBS_READ_TIMEOUT |
| Cuero se divide en 2 | Ruido | Aumentar LGC_LEATHER_END_HYSTERESIS |
| Áreas incorrectas | Macros mal | Verificar LGC_PIXEL_WIDTH_MM |
| Datos se pierden | Batch overflow | Reducir batch_limit |

## Module Dependencies

```
lgc_main_task.c
    ├─ lgc_interface_modbus.h       (lectura sensores)
    ├─ lgc_module_encoder.h          (encoder interrupt)
    ├─ lgc_module_eeprom.h           (config)
    ├─ os_port.h                     (RTOS)
    └─ lgc.h                         (tipos)
```

## Compile Macros (Override Defaults)

```bash
# Cambiar en tiempo de compilación
gcc -D LGC_PIXEL_WIDTH_MM=8.5f ...
gcc -D LGC_ENCODER_STEP_MM=4.0f ...
gcc -D LGC_LEATHER_END_HYSTERESIS=5 ...
```

## Configuration from EEPROM

```c
LGC_CONF_TypeDef_t config;
lgc_module_conf_get(&config);

// Usar:
// config.batch_limit - cueros por lote
// config.units - unidades (dm², ft², etc)
// config.conversion - factor de escala
```

## Typical Values

```
Hardware           Value
─────────────────────────
pixel_width        10 mm
encoder_step       5 mm
hysteresis         3 steps
batch_limit        10 leathers
total_leathers_max 300
total_batches_max  200
```

## File Organization

```
lgc_main_task.c
├─ Includes
├─ Defines (Macros)
├─ Typedefs
├─ Global Variables
├─ Function Prototypes
├─ Main Task Entry
├─ State Machine (STOP/RUNNING/FAIL)
├─ Callbacks
└─ Helper Functions
    ├─ lgc_count_active_bits()
    ├─ lgc_calculate_slice_area()
    └─ lgc_process_measurement()
```

## Modbus Register Details

```c
// Lectura
lgc_modbus_read_holding_regs(
    slave_addr,     // i+1 (1-11)
    register,       // 45 (verify!)
    &data.sensor[i],// storage
    count           // 1
)

// Sensor data format
// Bits 0-9: Photoreceptor status (1=detected, 0=empty)
// Bits 10-15: Reserved/unused
```

## Thread Safety Checklist

- [x] Access `data.sensor[]` in same thread (main task)
- [x] Access `measurements` with mutex if from other tasks
- [x] `encoder_flag` semaphore syncs with interrupt
- [x] `mutex` protects state changes

## Testing Sequence

1. Compile: `make clean && make`
2. Flash: STM32 programmer
3. Monitor UART output
4. Place leather on conveyor
5. Watch measurement progress
6. Verify saved values in EEPROM
7. Export and validate results

---

**Keep this card visible for quick reference!**
*Last updated: 2026-01-15*

