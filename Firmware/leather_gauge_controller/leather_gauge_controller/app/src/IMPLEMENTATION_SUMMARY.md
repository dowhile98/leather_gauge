# Implementación: Lógica de Medición de Cuero - Leather Gauge

## Resumen de Cambios

Se ha implementado completamente la lógica de procesamiento de medición para la máquina medidora de cuero basada en sistemas embebidos (STM32/RTOS) en el archivo `lgc_main_task.c`.

---

## 1. Macros Configurables Agregadas

```c
#define LGC_PIXEL_WIDTH_MM        10.0f   // Ancho de cada fotoreceptor
#define LGC_ENCODER_STEP_MM        5.0f   // Distancia por pulso de encoder
#define LGC_PHOTORECEPTORS_PER_SENSOR 10  // Bits por sensor (máximo)
#define LGC_LEATHER_END_HYSTERESIS  3     // Pasos sin detección para confirmar fin
```

**Nota**: Estos valores pueden ser ajustados según las medidas reales del hardware. El usuario puede sobrescribirlos en tiempo de compilación usando defines condicionales.

---

## 2. Estructura de Datos Mejorada: `lgc_measurements_t`

Reemplaza la estructura simple anterior con una más funcional:

```c
typedef struct {
    uint16_t current_batch_index;           // Índice del lote actual (0-199)
    uint16_t current_leather_index;         // Índice dentro del lote (0-299)
    float current_leather_area;             // Acumulador de área (mm²)
    float leather_measurement[300];         // Histórico de áreas individuales
    float batch_measurement[200];           // Histórico de sumas por lote
    uint8_t is_measuring;                   // Flag: ¿midiendo pieza?
    uint8_t no_detection_count;             // Contador de histéresis
} lgc_measurements_t;
```

Variable global: `static lgc_measurements_t measurements;`

---

## 3. Lógica de Lectura Modbus (case LGC_RUNNING)

El bucle principal ahora:

1. **Espera pulso del encoder**: `osWaitForSemaphore(&encoder_flag, 50)`
2. **Lee 11 sensores con reintentos**: Hasta 4 intentos con delay de 20ms entre intentos
3. **Actualiza flags de estado**: Bit por sensor en `data.sensor_status`
4. **Valida salud de sensores**: Si todos están bien, procesa medición
5. **Llama función de procesamiento**: `lgc_process_measurement(&config)`

```c
if (data.sensor_status == NO_ERROR) {
    lgc_process_measurement(&config);
}
```

---

## 4. Funciones Auxiliares Implementadas

### A. `lgc_count_active_bits()` → `uint16_t`

Cuenta fotoreceptores activos (bits en 1) en todos los sensores:

- Itera 11 sensores × 10 bits = 110 puntos máximos
- Detecta bits 0-9 en cada `data.sensor[i]`
- **Retorna**: 0 a 110 bits activos

```c
for (uint8_t i = 0; i < LGC_SENSOR_NUMBER; i++) {
    for (uint8_t bit = 0; bit < LGC_PHOTORECEPTORS_PER_SENSOR; bit++) {
        if (sensor_data & (1 << bit)) active_bits++;
    }
}
```

### B. `lgc_calculate_slice_area(uint16_t active_bits)` → `float`

Convierte bits activos a área en mm²:

**Fórmula**: `area = active_bits × ancho_pixel × paso_encoder`

Ejemplo:
```
50 bits activos × 10mm = 500mm ancho
500mm × 5mm = 2500 mm² por paso
```

### C. `lgc_process_measurement(LGC_CONF_TypeDef_t *config)` → `void`

**Función principal** que implementa la máquina de estados de detección.

---

## 5. Máquina de Estados de Detección (Proceso Principal)

### Estado 1: LEATHER PRESENT (bits activos > 0)

```
Si bits_activos > 0:
  ├─ Si NO estamos midiendo:
  │   └─ INICIAR PIEZA
  │       ├─ is_measuring = 1
  │       ├─ current_leather_area = 0
  │       └─ no_detection_count = 0
  │
  └─ ACUMULAR ÁREA
      ├─ current_leather_area += slice_area
      └─ no_detection_count = 0  (reset histéresis)
```

### Estado 2: NO LEATHER (bits activos = 0)

```
Si bits_activos == 0:
  └─ Si estamos midiendo:
      ├─ no_detection_count++
      │
      └─ Si no_detection_count >= HYSTERESIS:
          ├─ FINALIZAR PIEZA
          │   ├─ is_measuring = 0
          │   └─ no_detection_count = 0
          │
          ├─ GUARDAR MEDICIÓN
          │   └─ leather_measurement[leather_index] = current_leather_area
          │
          ├─ ACUMULAR A LOTE
          │   └─ batch_measurement[batch_index] += current_leather_area
          │
          ├─ INCREMENTAR ÍNDICES
          │   ├─ leather_index++
          │   │
          │   └─ Si leather_index >= batch_limit:
          │       ├─ leather_index = 0 (reset lote)
          │       ├─ batch_index++     (siguiente lote)
          │       │
          │       └─ Si batch_index >= BATCH_COUNT_MAX:
          │           └─ ERROR: Desbordamiento de lotes
          │
          └─ current_leather_area = 0 (limpiar acumulador)
```

---

## 6. Gestión de Lotes

La configuración `LGC_CONF_TypeDef_t` contiene:
- `batch_limit`: Cantidad de cueros por lote (típicamente 10-200)

**Ejemplo con batch_limit = 10**:

```
Lote 0 [0-9]:        10 cueros
Lote 1 [10-19]:      10 cueros
...
Lote 19 [190-199]:   10 cueros
```

**Validaciones**:
- Si `current_leather_index >= 300` → No guardar (desbordamiento de array)
- Si `current_batch_index >= 200` → Detener (error crítico)

---

## 7. Control de Histéresis

Previene falsos positivos en detección de fin:

```
Pasos sin detección necesarios: LGC_LEATHER_END_HYSTERESIS = 3

Secuencia:
Pulso 1: 80 bits → no_detect_count = 0
Pulso 2: 75 bits → no_detect_count = 0
Pulso 3: 0 bits  → no_detect_count = 1
Pulso 4: 0 bits  → no_detect_count = 2
Pulso 5: 0 bits  → no_detect_count = 3 ← THRESHOLD ALCANZADO
         └─ FINALIZAR CUERO (no es ruido)
```

---

## 8. Ejemplo de Ejecución Completa

```
Configuración:
  - batch_limit = 5 cueros por lote
  - pixel_width = 10mm
  - encoder_step = 5mm

Secuencia de pulsos:
┌──────┬────────────────┬────────────┬─────────────────────────────┐
│Pulso │ Bits Activos   │ Área Slice │ Acción                      │
├──────┼────────────────┼────────────┼─────────────────────────────┤
│  1   │ 60 bits        │ 3000 mm²   │ INICIO pieza[0], área=3000  │
│  2   │ 65 bits        │ 3250 mm²   │ Acumular, área=6250         │
│  3   │ 70 bits        │ 3500 mm²   │ Acumular, área=9750         │
│  4   │ 0 bits         │ 0 mm²      │ no_detect_count=1           │
│  5   │ 0 bits         │ 0 mm²      │ no_detect_count=2           │
│  6   │ 0 bits         │ 0 mm²      │ no_detect_count=3 ← FIN     │
│      │                │            │ leather[0]=9750             │
│      │                │            │ batch[0]+=9750              │
│      │                │            │ leather_idx=1               │
├──────┼────────────────┼────────────┼─────────────────────────────┤
│  7   │ 55 bits        │ 2750 mm²   │ INICIO pieza[1], área=2750  │
│  8   │ 60 bits        │ 3000 mm²   │ Acumular, área=5750         │
│  9   │ 0 bits         │ 0 mm²      │ no_detect_count=1           │
│ 10   │ 0 bits         │ 0 mm²      │ no_detect_count=2           │
│ 11   │ 0 bits         │ 0 mm²      │ no_detect_count=3 ← FIN     │
│      │                │            │ leather[1]=5750             │
│      │                │            │ batch[0]+=5750 (total=15500)│
│      │                │            │ leather_idx=2               │
│ ...  │   ...          │   ...      │ ... (3 piezas más)          │
│      │                │            │ leather_idx=5               │
│      │                │            │ leather_idx >= batch_limit  │
│      │                │            │ ├─ leather_idx = 0 (reset)  │
│      │                │            │ └─ batch_idx = 1 ← LOTE 2   │
└──────┴────────────────┴────────────┴─────────────────────────────┘

Resultados finales:
  leather_measurement[0..4] = [ 9750, 5750, ..., ... ] mm²
  batch_measurement[0]       = suma de 5 cueros
  batch_measurement[1]       = inicia nuevo lote
```

---

## 9. Integración con RTOS

- **Mutex**: Protege estado de máquina (`osAcquireMutex/osReleaseMutex`)
- **Semáforo**: Sincronización con encoder (`osWaitForSemaphore`)
- **Delays**: Reintentos de Modbus (`osDelayTask(20)`)
- **Callbacks**: Encoder → libera semáforo (`osReleaseSemaphore`)

---

## 10. Archivos Modificados

| Archivo | Cambios |
|---------|---------|
| `lgc_main_task.c` | Estructura mejorada + 3 funciones nuevas + lógica completa |

## 11. Archivos Nuevos Creados

| Archivo | Descripción |
|---------|-------------|
| `MEASUREMENT_ALGORITHM.md` | Documentación detallada con diagramas |
| `IMPLEMENTATION_SUMMARY.md` | Este archivo |

---

## 12. Mejoras Futuras (TODO)

1. **Condición de Parada**: Implementar `case LGC_STOP` con validación de condición de parada
2. **Manejo de Errores**: Señalizar cuando `batch_index` alcance el máximo
3. **Interfaz de Usuario**: Mostrar mediciones en panel P10
4. **Logging**: Registrar eventos de inicio/fin de medición
5. **Validación de Límites**: Considerar `units` y `conversion` de configuración

---

## 13. Testing Recomendado

```bash
# Compilar con macros personalizadas
gcc -D LGC_PIXEL_WIDTH_MM=12.5 -D LGC_ENCODER_STEP_MM=3.0 ...

# Verificar:
□ Lectura correcta de bits 0-9 de cada sensor
□ Detección de inicio/fin de cuero
□ Acumulación de área correcta
□ Transiciones de lote en límite
□ Manejo de desbordamiento de arrays
□ Histéresis previene falsos positivos
```

---

## 14. Contacto y Cambios

**Usuario**: tecna-smart-lab  
**Fecha**: 15 de Enero, 2026  
**Rama**: main  
**Estado**: Implementación Completa ✓

Para ajustar valores:
```c
// En compilación:
#define LGC_PIXEL_WIDTH_MM 12.5f
#define LGC_ENCODER_STEP_MM 3.0f
#define LGC_LEATHER_END_HYSTERESIS 5
```

