# Análisis y Plan: Refactorización del Almacenamiento de Mediciones

**Fecha:** 14 de marzo de 2026  
**Autor:** Análisis técnico del codebase  
**Alcance:** `lgc_typedefs.h`, `lgc_measurements_t`, `LgcBatchReport_t`, `lgc_report_manager.c/h`, `lgc_main_task.c`, `lgc_hmi_task.c`

---

## 1. Estado Actual — Diagnóstico

### 1.1 Estructuras de datos relevantes

#### `lgc_measurements_t` (en `lgc_typedefs.h`)

```c
typedef struct {
    uint16_t current_batch_index;                              // Índice del lote actual
    uint16_t current_leather_index;                            // Piezas en el lote actual
    uint16_t total_leathers_measured;                          // Total histórico
    float current_leather_area;                                // Acumulador pieza en curso
    float leather_measurement[LGC_LEATHER_COUNT_MAX];          // ✅ Medidas lote actual  (150)
    float leather_measurement_last[LGC_LEATHER_COUNT_MAX];     // ⚠️  Declarado pero NUNCA SE POPULA
    float batch_measurement[LGC_LEATHER_BATCH_COUNT_MAX];      // Suma total por lote (200)
    uint8_t is_measuring;
    uint8_t no_detection_count;
    OsMutex mutex;
} lgc_measurements_t;
```

#### `LgcBatchReport_t` (en `lgc_typedefs.h`)

```c
typedef struct {
    uint32_t batch_id;
    uint32_t batch_index;
    uint16_t year; uint8_t month; uint8_t day;
    uint8_t hours; uint8_t minutes; uint8_t seconds;
    char client_name[16]; char color[16]; char leather_id[16];
    float pieces_area[LGC_LEATHER_COUNT_MAX];   // Array de áreas (150 floats = 600 bytes)
    uint16_t total_pieces;
    float total_area;
    uint8_t units; uint8_t conversion; uint8_t is_valid;
} LgcBatchReport_t;
```

#### `LgcLiveStatus_t` — scalar, para HMI en tiempo real

```c
typedef struct {
    uint16_t batch_count; uint16_t leather_count;
    float current_leather_area; float accumulated_batch_area;
    uint8_t system_state; uint8_t motor_feedback;
    uint8_t motor_speed; uint8_t guard_status; uint8_t is_measuring;
} LgcLiveStatus_t;
```

#### `LGC_LEATHER_COUNT_MAX`

```c
#define LGC_LEATHER_COUNT_MAX 150   // ✅ Ya está en 150
```

---

### 1.2 Flujo actual de medición y snapshot

```
Encoder pulse → lgc_process_measurement()
    ├─ acumula en measurements.current_leather_area
    └─ cuando fin de pieza:
         ├─ measurements.leather_measurement[current_leather_index] = area
         ├─ measurements.batch_measurement[current_batch_index] += area
         └─ current_leather_index++

Cierre de lote → lgc_finalize_batch_snapshot()
    ├─ Copia leather_measurement[] → finalized_batch.pieces_area[]
    ├─ Resetea current_leather_index = 0
    ├─ Resetea leather_measurement[] a 0
    ├─ current_batch_index++
    └─ señaliza LGC_EVENT_SNAPSHOT_READY → Report Task imprime

Report Manager almacena:
    static LgcBatchReport_t last_finalized_batch;   // ✅ Único snapshot
    static LgcLiveStatus_t  current_live_status;    // ✅ Escalares vivos
```

---

### 1.3 Visualización HMI actual

- **Páginas 12–17**: muestran `lgc_report_get_last_snapshot()` → el último lote CERRADO (50 piezas por página)
- **No hay páginas** que muestren el lote **en curso** (current)
- **LgcLiveStatus_t** solo tiene `leather_count` y `current_leather_area`, no el array completo

---

### 1.4 Función de borrado actual

```c
// lgc_main_task.c — solo borra la ÚLTIMA pieza (undo)
void lgc_clear_measurement_last_leather(void) {
    // Solo opera sobre current_leather_index - 1
    // No permite elegir índice
    // No tiene flag "deleted", simplemente decrementa el índice
}
```

---

### 1.5 Problemas identificados

| #   | Problema                                                                                                              | Impacto                                           |
| --- | --------------------------------------------------------------------------------------------------------------------- | ------------------------------------------------- |
| P1  | `leather_measurement_last[]` declarado pero nunca se llena                                                            | Campo muerto, confusión                           |
| P2  | No existe snapshot del lote **en curso** con el array completo                                                        | HMI no puede mostrar la lista current             |
| P3  | Al cerrar un lote, `leather_measurement[]` se borra → se pierde antes de que HMI pueda mostrarlo en las páginas 12-17 | Race condition potencial                          |
| P4  | Borrado solo del último (undo), sin selección por índice                                                              | Limitación funcional                              |
| P5  | Sin flag de borrado → no hay trazabilidad de qué se borró                                                             | Si se borra y luego se imprime, no queda registro |

---

## 2. Requerimientos del Cambio

### REQ-1: Almacenamiento dual (current + last)

- Al completar un lote, las medidas individuales deben pasar a `last` y seguir disponibles
- El lote `current` inicia vacío para el siguiente ciclo
- Ambos (`current` y `last`) deben ser accesibles desde:
  - HMI (páginas de visualización separadas)
  - Report Manager (impresión)
- **Máximo 150 piezas por lote** (`LGC_LEATHER_COUNT_MAX = 150`)

### REQ-2: Borrado por índice con soft-delete

- Se especifica el **índice visual** (1-based, tal como aparece en pantalla)
- La operación **no elimina físicamente** el dato: marca `deleted = true`
- El area de la pieza eliminada se **resta del total del lote**
- Display y print **recalculan el índice visual** ignorando los `deleted = true`
- Las piezas siguientes se renumeran visualmente: si borro el 3, el antiguo 4 pasa a ser el 3
- El borrado aplica **solo sobre el lote current** (el lote `last` es inmutable)

---

## 3. Diseño Propuesto

### 3.1 Nueva estructura `LgcBatchSlot_t` — unidad de medida con flag

```c
// En lgc_typedefs.h
typedef struct {
    float area;        // Área medida en unidades configuradas
    bool  deleted;     // Soft-delete: true = ignorar en display y print
} LgcBatchSlot_t;
```

> **Nota de RAM**: `float(4) + bool(1) + padding(3) = 8 bytes × 150 = 1200 bytes por lote`.  
> Dos instancias (current + last) = 2.4 KB. Manejable dentro de 128 KB RAM.

---

### 3.2 Nuevo `LgcBatchSnapshot_t` — reemplaza `LgcBatchReport_t`

```c
// En lgc_typedefs.h
typedef struct {
    /* Identificación */
    uint32_t batch_id;
    uint32_t batch_index;

    /* Timestamp */
    uint16_t year;
    uint8_t  month;   uint8_t day;
    uint8_t  hours;   uint8_t minutes;  uint8_t seconds;

    /* Config del cliente (copiada al cerrar) */
    char client_name[16];
    char color[16];
    char leather_id[16];

    /* Medidas individuales con soft-delete */
    LgcBatchSlot_t slots[LGC_LEATHER_COUNT_MAX];   // 150 slots

    /* Contadores */
    uint16_t total_slots;      // Slots usados (incluyendo deleted)
    uint16_t active_count;     // Slots con deleted=false (piezas válidas)
    float    total_area;       // Suma de areas con deleted=false

    /* Config de unidades */
    uint8_t units;
    uint8_t conversion;

    /* Estado */
    uint8_t is_valid;          // 0 = vacío, 1 = datos válidos
} LgcBatchSnapshot_t;
```

> **Compatibilidad**: `LgcBatchReport_t` puede mantenerse o ser reemplazado dependiendo del impacto. Ver sección 4.

---

### 3.3 Report Manager — dos snapshots

```c
// En lgc_report_manager.c — nuevas variables estáticas
static LgcBatchSnapshot_t s_current_batch;   // Lote en medición (se actualiza pieza a pieza)
static LgcBatchSnapshot_t s_last_batch;      // Último lote cerrado (inmutable hasta el próximo cierre)
static OsMutex s_current_mutex;
static OsMutex s_last_mutex;
```

#### API nueva:

```c
// Actualizar current batch (llamada desde Main Task al finalizar cada pieza)
void lgc_report_update_current_batch(const LgcBatchSnapshot_t *snap);

// Obtener current batch (HMI páginas current, print parcial)
error_t lgc_report_get_current_batch(LgcBatchSnapshot_t *out);

// Al cierre de lote: copiar current → last, luego resetear current
void lgc_report_finalize_batch(void);

// Obtener last batch (ya existente como lgc_report_get_last_snapshot)
error_t lgc_report_get_last_batch(LgcBatchSnapshot_t *out);

// Soft-delete por índice visual (solo sobre current)
// visual_index: 1-based, contando solo los no-deleted
error_t lgc_report_delete_current_slot(uint16_t visual_index);
```

---

### 3.4 Lógica de soft-delete

```
Visual index 1-based:               Array interno:
  1 → slot[0]  deleted=false  ✅
  2 → slot[1]  deleted=false  ✅
  3 → slot[2]  deleted=true   ❌  ← skipped
  4 → slot[3]  deleted=false  ✅  → visual index 3
  5 → slot[4]  deleted=false  ✅  → visual index 4

"Eliminar visual[3]" → busca el 3er slot con deleted=false → slot[3]
→ slot[3].deleted = true
→ total_area -= slot[3].area
→ active_count--
→ measurements.current_leather_index--   ◄ CLAVE: el contador de cierre de lote también baja
→ Notificar HMI: LGC_HMI_UPDATE_REQUIRED
```

**Función helper de conversión visual → slot:**

```c
// Retorna el índice real en slots[] dado un índice visual (1-based)
// Retorna LGC_LEATHER_COUNT_MAX si no se encuentra
static uint16_t find_slot_by_visual_index(LgcBatchSnapshot_t *snap, uint16_t visual_index) {
    uint16_t visual = 0;
    for (uint16_t i = 0; i < snap->total_slots; i++) {
        if (!snap->slots[i].deleted) {
            visual++;
            if (visual == visual_index) return i;
        }
    }
    return LGC_LEATHER_COUNT_MAX; // not found
}
```

---

### 3.5 Cambios en `lgc_main_task.c`

#### Agregar pieza al slot (al finalizar una pieza):

```c
// En lgc_process_measurement(), cuando event_status = 1:
// Después de calcular measurements.current_leather_area:

// 1. Guardar en measurements (existente, para no romper lógica interna)
measurements.leather_measurement[measurements.current_leather_index] = area;
measurements.batch_measurement[measurements.current_batch_index] += area;
measurements.current_leather_index++;

// 2. NUEVO: Sincronizar con current_batch snapshot
LgcBatchSlot_t new_slot = { .area = area, .deleted = false };
lgc_report_append_current_slot(&new_slot);   // nueva función en report manager
```

#### Cierre de lote (`lgc_finalize_batch_snapshot`):

```c
// NUEVO: Antes de resetear measurements:
lgc_report_finalize_batch();   // copia current → last, resetea current con nuevo batch_id/timestamp

// Luego continúa el reset existente de measurements (sin cambios)
measurements.current_leather_index = 0;
measurements.current_leather_area = 0.0f;
memset(measurements.leather_measurement, 0, ...);
measurements.current_batch_index++;
```

#### Borrado por índice (nueva función pública):

```c
// Reemplaza lgc_clear_measurement_last_leather()  ← DEPRECATED
// Nueva función:
error_t lgc_delete_leather_by_visual_index(uint16_t visual_index);

// Internamente:
// 1. Busca el slot real usando find_slot_by_visual_index()
// 2. Marca deleted=true
// 3. Resta del batch total en measurements.batch_measurement[]
// 4. Decrementa active_count en el snapshot
// 5. Decrementa measurements.current_leather_index  ◄ para que la condición de cierre
//    de lote sea coherente: si el config.batch=10, y había 5 válidos → borrar uno
//    deja 4 válidos, y el lote cerrará cuando lleguen 6 más (total 10 válidos)
// 6. Notifica HMI
```

> **Invariante crítica:** `measurements.current_leather_index` siempre debe ser igual a
> `s_current_batch.active_count`. Son la misma magnitud vista desde dos capas distintas.
> El cierre de lote se dispara cuando `current_leather_index >= config.batch`.

#### Condición de cierre de lote tras el cambio:

```c
// En lgc_process_measurement() — ANTES (lógica por total_slots):
if (measurements.current_leather_index >= config->batch)  // incluye deleted ← INCORRECTO

// DESPUÉS (lógica por piezas válidas = active_count):
// current_leather_index ya refleja solo piezas activas (se decrementa al borrar)
// por lo tanto la misma condición es correcta:
if (measurements.current_leather_index >= config->batch)  // solo piezas válidas ← CORRECTO
```

Ejemplo con `config.batch = 10`:

```
Estado        │ total_slots │ active_count = current_leather_index │ ¿Cierra lote?
──────────────┼─────────────┼──────────────────────────────────────┼──────────────
Mide 5        │      5      │          5                           │    No
Borra visual 3│      5      │          4     ← decrementado        │    No
Mide 5 más    │     10      │          9                           │    No
Mide 1 más    │     11      │         10     ← llega al límite     │    SÍ
```

---

### 3.6 Cambios en HMI — páginas

#### Páginas current (NUEVO)

| Páginas HMI                      | Contenido                                                       |
| -------------------------------- | --------------------------------------------------------------- |
| `HMI_PAGE12..17` (**actual**)    | Último lote (`s_last_batch`) — sin cambios en número de páginas |
| `HMI_PAGE_CURR_1..N` (**nuevo**) | Lote actual (`s_current_batch`) — misma lógica, páginas nuevas  |

> Coordinar con equipo de diseño DWIN la asignación de páginas nuevas.

#### Renderizado con soft-delete:

```c
// En lgc_hmi_update_task_entry(), páginas de lista:

LgcBatchSnapshot_t snap;
lgc_report_get_current_batch(&snap);   // o get_last_batch según la página

uint16_t visual_index = 0;
for (uint16_t i = 0; i < snap.total_slots && visual_index < 50; i++) {
    if (!snap.slots[i].deleted) {
        visual_index++;
        uint16_t vp = vp_addr + (visual_index - 1);
        dwin_write_vp_u16(&dwin_hmi, vp, (uint16_t)(snap.slots[i].area * 100));
    }
}
// Limpiar VPs sobrantes (visual_index hasta 50)
for (; visual_index < 50; visual_index++) {
    dwin_write_vp_u16(&dwin_hmi, vp_addr + visual_index, 0);
}
```

#### Borrado desde HMI:

```c
// VP touch de delete ahora envía el índice visual seleccionado (no borra el último)
case LGC_HMI_VP_LIST_DELETE:
{
    uint16_t visual_index = (msg.data[0] << 8) | msg.data[1]; // el display envía qué índice
    lgc_delete_leather_by_visual_index(visual_index);
    osSetEventBits(&events, LGC_HMI_UPDATE_REQUIRED);
    break;
}
```

---

### 3.7 Cambios en impresión (`lgc_report_manager.c`)

```c
// lgc_print_batch_report() — reescribir loop de piezas:

static void lgc_print_batch_report(LgcBatchSnapshot_t *report) {
    // ...cabecera sin cambios...

    uint16_t visual_index = 0;
    for (uint16_t i = 0; i < report->total_slots; i++) {
        if (!report->slots[i].deleted) {
            visual_index++;
            char item[8], area_str[16];
            lwprintf_snprintf(item, sizeof(item), "%d", visual_index);
            lwprintf_snprintf(area_str, sizeof(area_str), "%.2f", report->slots[i].area);
            esc_pos_print_table_row(&printer_dev, item, area_str,
                                   report->units == 0 ? "ft2" : "m2");
        }
    }

    // Total usa report->total_area (ya descontados los deleted)
    lwprintf_snprintf(buffer, sizeof(buffer), "TOTAL: %.2f", report->total_area);
    esc_pos_print_line(&printer_dev, buffer);
    // ...
}
```

---

## 4. Análisis de Impacto en RAM

| Estructura                                         | Antes  | Después               | Delta          |
| -------------------------------------------------- | ------ | --------------------- | -------------- |
| `lgc_measurements_t.leather_measurement[150]`      | 600 B  | 600 B                 | 0              |
| `lgc_measurements_t.leather_measurement_last[150]` | 600 B  | **0 B** (eliminado)   | -600 B         |
| `s_current_batch` (LgcBatchSnapshot_t)             | —      | ~1.3 KB               | +1.3 KB        |
| `s_last_batch` (LgcBatchSnapshot_t)                | —      | ~1.3 KB               | +1.3 KB        |
| `last_finalized_batch` (LgcBatchReport_t antiguo)  | ~750 B | **0 B** (reemplazado) | -750 B         |
| **Delta total**                                    |        |                       | **≈ +1.25 KB** |

> ✅ Impacto en RAM: ~1.25 KB adicionales sobre 128 KB disponibles. Perfectamente viable.

**Detalle de `LgcBatchSnapshot_t`:**

```
LgcBatchSlot_t slots[150]:
    float area  (4B) + bool deleted (1B) + 3B padding = 8B × 150 = 1200 B
Metadata (batch_id, fechas, strings, contadores, flags): ~80 B
────────────────────────────────────────────────────────────────
Total por instancia: ≈ 1280 B ≈ 1.25 KB
Dos instancias (current + last): ≈ 2.5 KB
```

---

## 5. Plan de Implementación — Fases

### FASE 1 — Estructuras de datos (sin romper código existente)

**Archivos:** `lgc_typedefs.h`

| Tarea | Detalle                                                                                |
| ----- | -------------------------------------------------------------------------------------- |
| F1.1  | Agregar `LgcBatchSlot_t { float area; bool deleted; }`                                 |
| F1.2  | Agregar `LgcBatchSnapshot_t` con `slots[LGC_LEATHER_COUNT_MAX]`                        |
| F1.3  | Mantener `LgcBatchReport_t` como alias/compatibilidad hasta que la capa HMI esté lista |
| F1.4  | Eliminar `leather_measurement_last[]` de `lgc_measurements_t`                          |

---

### FASE 2 — Report Manager: almacenamiento dual + API

**Archivos:** `lgc_report_manager.h`, `lgc_report_manager.c`

| Tarea | Detalle                                                                                 |
| ----- | --------------------------------------------------------------------------------------- |
| F2.1  | Agregar `s_current_batch` y `s_last_batch` como estáticos                               |
| F2.2  | Agregar `s_current_mutex` y `s_last_mutex`                                              |
| F2.3  | Implementar `lgc_report_append_current_slot()`                                          |
| F2.4  | Implementar `lgc_report_finalize_batch()` → copia current → last, resetea current       |
| F2.5  | Implementar `lgc_report_get_current_batch()`                                            |
| F2.6  | Implementar `lgc_report_get_last_batch()` (reemplaza `lgc_report_get_last_snapshot`)    |
| F2.7  | Implementar `lgc_report_delete_current_slot(visual_index)` con lógica de soft-delete    |
| F2.8  | Actualizar `lgc_report_update_live_status()` para usar `active_count` del current batch |
| F2.9  | Actualizar `lgc_print_batch_report()` para iterar con soft-delete                       |

---

### FASE 3 — Main Task: conectar con nuevas APIs

**Archivos:** `lgc_main_task.c`

| Tarea | Detalle                                                                                                                                                                                    |
| ----- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| F3.1  | En `lgc_process_measurement()`, al guardar una pieza: llamar `lgc_report_append_current_slot()`                                                                                            |
| F3.2  | En `lgc_finalize_batch_snapshot()`: llamar `lgc_report_finalize_batch()` antes del reset                                                                                                   |
| F3.3  | Reemplazar `lgc_clear_measurement_last_leather()` con `lgc_delete_leather_by_visual_index()`                                                                                               |
| F3.4  | `lgc_delete_leather_by_visual_index()`: delegar a `lgc_report_delete_current_slot()` + restar de `measurements.batch_measurement[]` + **decrementar `measurements.current_leather_index`** |
| F3.5  | Verificar que la condición de cierre `current_leather_index >= config->batch` sigue correcta al usar solo piezas activas (debería funcionar sin cambio adicional)                          |

---

### FASE 4 — HMI: visualización current + select-delete

**Archivos:** `lgc_hmi_task.c`, `lgc_hmi.h`

| Tarea | Detalle                                                                                      |
| ----- | -------------------------------------------------------------------------------------------- |
| F4.1  | Definir nuevas páginas HMI para lote current (coordinar con diseño DWIN)                     |
| F4.2  | Actualizar `lgc_hmi_update_task_entry()`: páginas current → `lgc_report_get_current_batch()` |
| F4.3  | Actualizar páginas last (12-17) → `lgc_report_get_last_batch()`                              |
| F4.4  | Actualizar renderizado de lista para iterar con soft-delete (loop con `deleted` check)       |
| F4.5  | Actualizar `LGC_HMI_VP_LIST_DELETE` touch handler para recibir índice visual del display     |
| F4.6  | Actualizar `LGC_HMI_VP_LEATHER_COUNT` para mostrar `active_count` (sin deleted)              |

---

### FASE 5 — Limpieza y validación

**Archivos:** todos

| Tarea | Detalle                                                                          |
| ----- | -------------------------------------------------------------------------------- |
| F5.1  | Eliminar `leather_measurement_last[]` de `lgc_measurements_t` definitivamente    |
| F5.2  | Eliminar `LgcBatchReport_t` si ya no se usa en ningún sitio                      |
| F5.3  | Actualizar `README.md` con nuevas capacidades                                    |
| F5.4  | Prueba end-to-end: medir 10 piezas, borrar índice 5, verificar display y reprint |

---

## 6. Diagrama de Flujo — Nuevo Almacenamiento

```
Encoder pulse → lgc_process_measurement()
    │
    ├─ Acumula current_leather_area (sin cambio)
    │
    └─ Pieza terminada (event=1):
         ├─ measurements.leather_measurement[i] = area  (interno, sin cambio)
         ├─ measurements.batch_measurement[bi] += area  (sin cambio)
         └─ lgc_report_append_current_slot({area, deleted=false})  ◄ NUEVO
              └─ s_current_batch.slots[total_slots++] = slot
              └─ s_current_batch.active_count++
              └─ s_current_batch.total_area += area

Cierre de lote → lgc_finalize_batch_snapshot()
    │
    ├─ lgc_report_finalize_batch()  ◄ NUEVO
    │    ├─ Copia s_current_batch → s_last_batch  (con RTC + config)
    │    └─ Resetea s_current_batch (total_slots=0, active_count=0, total_area=0)
    │
    ├─ Resetea measurements (sin cambio)
    └─ Señaliza LGC_EVENT_SNAPSHOT_READY → Report Task imprime s_last_batch

Borrado → lgc_delete_leather_by_visual_index(visual_idx)    ◄ NUEVO
    ├─ lgc_report_delete_current_slot(visual_idx)
    │    ├─ find_slot_by_visual_index() → slot_real_idx
    │    ├─ s_current_batch.slots[slot_real_idx].deleted = true
    │    ├─ s_current_batch.total_area -= area
    │    └─ s_current_batch.active_count--
    ├─ measurements.batch_measurement[bi] -= area  (mantener consistencia interna)
    ├─ measurements.current_leather_index--          ◄ CLAVE: coherencia con condición de cierre
    └─ Notifica LGC_HMI_UPDATE_REQUIRED

⚠️  INVARIANTE: measurements.current_leather_index == s_current_batch.active_count
    Ambos representan la misma magnitud: piezas válidas (no borradas) en el lote actual.
    La condición de cierre de lote (current_leather_index >= config.batch) es correcta
    porque refleja únicamente piezas activas.
```

---

## 7. Decisiones de Diseño — Puntos Abiertos

| #   | Decisión                                                                     | Opciones                       | Recomendación                                                                                                                                             |
| --- | ---------------------------------------------------------------------------- | ------------------------------ | --------------------------------------------------------------------------------------------------------------------------------------------------------- |
| D1  | ¿Se puede borrar del `last` batch?                                           | Sí / No                        | **No**: `last` es inmutable (solo para referencia/impresión)                                                                                              |
| D2  | ¿Se puede reimprimir el `last` batch desde HMI?                              | Sí / No                        | **Sí**: el Report Manager ya lo tiene; agregar un botón de reprint                                                                                        |
| D3  | ¿El total mostrado en live status usa `active_count`?                        | `active_count` / `total_slots` | **`active_count`**: el operador ve piezas válidas                                                                                                         |
| D4  | ¿El display envía el índice visual en el VP delete, o envía índice de array? | Visual / Array                 | **Visual** (1-based): el MCU hace la traducción, el display no sabe de deleted                                                                            |
| D5  | ¿Se puede deshacer un borrado (undelete)?                                    | Sí / No                        | **Fuera de scope** por ahora                                                                                                                              |
| D6  | ¿`LgcBatchReport_t` se mantiene o se reemplaza por `LgcBatchSnapshot_t`?     | Mantener / Reemplazar          | **Reemplazar** gradualmente en FASE 5; mientras, ambas coexisten                                                                                          |
| D7  | ¿La condición de cierre de lote usa `active_count` o `total_slots`?          | `active_count` / `total_slots` | **`active_count`**: el lote cierra cuando hay N piezas válidas, no N intentos. `current_leather_index` ya refleja esto al decrementarse con cada borrado. |

---

## 8. Compatibilidad con código existente

| Función existente                                                  | Cambio                                                                       |
| ------------------------------------------------------------------ | ---------------------------------------------------------------------------- |
| `lgc_report_get_last_snapshot()`                                   | **Mantener** como wrapper de `lgc_report_get_last_batch()` durante migración |
| `lgc_report_update_snapshot()`                                     | **Deprecar** → reemplazado por `lgc_report_finalize_batch()`                 |
| `lgc_clear_measurement_last_leather()`                             | **Deprecar** → reemplazado por `lgc_delete_leather_by_visual_index()`        |
| `lgc_increment_batch_index()`                                      | **Sin cambio** (solo opera sobre `measurements`, lógica interna)             |
| `lgc_report_update_live_status()` / `lgc_report_get_live_status()` | **Sin cambio** (sigue usando `LgcLiveStatus_t` escalares)                    |
| `lgc_print_batch_report()`                                         | **Modificar** para iterar con soft-delete                                    |

---

## 9. Checklist de Validación

- [ ] Medir 5 piezas → verificar `s_current_batch.active_count == 5` y `measurements.current_leather_index == 5`
- [ ] Borrar visual índice 3 → `active_count == 4`, `current_leather_index == 4`, total_area descontado
- [ ] Verificar pantalla: pieza 4 renumerada como 3 en display
- [ ] Con `config.batch = 10` y 5 piezas medidas: borrar 2 → `active_count = 3`; medir 7 más → lote cierra exactamente al llegar a 10 activas
- [ ] Con `config.batch = 10` y 9 piezas: borrar 1 → `active_count = 8`; volver a medir 1 → no debe cerrar (9 activas); medir 1 más → cierra (10 activas)
- [ ] Borrar hasta 0 activas (`active_count == 0`, `current_leather_index == 0`) → el sistema no cierra lote; medir de nuevo desde 0
- [ ] Cerrar lote → `s_last_batch` tiene 4 piezas válidas, `s_current_batch` vaciado
- [ ] Iniciar nuevo lote → `s_current_batch` vacío, `s_last_batch` intacto
- [ ] Imprimir → lista solo muestra 4 piezas (la borrada no aparece)
- [ ] Páginas current (nuevo) → mostrar lote en medición
- [ ] Páginas last (12-17 existentes) → mostrar lote anterior sin cambios
- [ ] `LgcLiveStatus_t.leather_count` = `active_count` del current batch
- [ ] `batch_measurement[]` interno permanece consistente tras borrado
