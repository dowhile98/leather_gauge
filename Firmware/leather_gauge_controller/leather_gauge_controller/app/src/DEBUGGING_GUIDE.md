# Guía de Uso y Depuración - Leather Gauge Measurement

## Índice
1. [Inicialización](#inicialización)
2. [Ajuste de Configuración](#ajuste-de-configuración)
3. [Monitoreo y Depuración](#monitoreo-y-depuración)
4. [Casos de Prueba](#casos-de-prueba)
5. [Solución de Problemas](#solución-de-problemas)

---

## Inicialización

### Requisitos Previos

Asegúrate de que las siguientes funciones estén implementadas:

```c
// Debe existir en lgc_module_input.c o similar
lgc_module_encoder_init(callback_t);

// Debe existir en lgc_interface_modbus.c
error_t lgc_interface_modbus_init(void);

// Debe existir en lgc_module_eeprom.c o similar
void lgc_module_conf_get(LGC_CONF_TypeDef_t *config);

// Debe existir en os_port.c
osCreateSemaphore(semaphore, initial_value);
osCreateMutex(mutex);
osWaitForSemaphore(semaphore, timeout);
osReleaseSemaphore(semaphore);
osAcquireMutex(mutex);
osReleaseMutex(mutex);
osDelayTask(milliseconds);
```

### Flujo de Inicialización

```c
void lgc_main_task_entry(void *param) {
    // 1. Se crean semáforo y mutex
    osCreateSemaphore(&encoder_flag, 0);
    osCreateMutex(&mutex);

    // 2. Se inicializa el encoder (conexión con hardware)
    lgc_module_encoder_init(lgc_encoder_callback);

    // 3. En el estado LGC_STOP se carga configuración
    lgc_module_conf_get(&config);

    // 4. Transición a LGC_RUNNING (implementar)
    // 5. Medición inicia automáticamente con pulsos del encoder
}
```

---

## Ajuste de Configuración

### Macros en Compilación

Editar valores en tiempo de compilación si es necesario:

```c
// En makefile o CMakeLists.txt:
CFLAGS += -D LGC_PIXEL_WIDTH_MM=10.0f
CFLAGS += -D LGC_ENCODER_STEP_MM=5.0f
CFLAGS += -D LGC_LEATHER_END_HYSTERESIS=3
```

### Ejemplo: Cambiar Ancho de Pixel

Si la máquina tiene fotoreceptores de 8mm en lugar de 10mm:

```c
/* En lgc_main_task.c, antes de compilar */
#ifndef LGC_PIXEL_WIDTH_MM
#define LGC_PIXEL_WIDTH_MM 8.0f   /* Cambiar de 10.0f a 8.0f */
#endif
```

### Ejemplo: Cambiar Paso del Encoder

Si el encoder emite 10 pulsos cada 2cm en lugar de 5mm:

```c
#ifndef LGC_ENCODER_STEP_MM
#define LGC_ENCODER_STEP_MM 2.0f   /* Cambiar de 5.0f a 2.0f */
#endif
```

### Ejemplo: Ajustar Histéresis

Si hay mucho ruido en sensores, aumentar histéresis:

```c
#define LGC_LEATHER_END_HYSTERESIS 5   /* De 3 a 5 pasos */
```

---

## Monitoreo y Depuración

### 1. Verificar Lectura de Sensores

Añadir código de depuración en `lgc_process_measurement()`:

```c
static void lgc_process_measurement(LGC_CONF_TypeDef_t *config) {
    uint16_t active_bits = lgc_count_active_bits();
    float slice_area = lgc_calculate_slice_area(active_bits);

    #ifdef DEBUG_MEASUREMENTS
    printf("[MEAS] Bits: %u, Area: %.2f mm²\n", active_bits, slice_area);
    printf("[MEAS] Measuring: %u, no_detect_count: %u\n", 
           measurements.is_measuring, 
           measurements.no_detection_count);
    #endif

    // ... resto del código
}
```

### 2. Verificar Estado de Sensores

```c
// En lgc_main_task_entry() después de leer sensores
for (uint8_t i = 0; i < LGC_SENSOR_NUMBER; i++) {
    uint8_t has_error = (data.sensor_status & (1 << i)) ? 1 : 0;
    printf("Sensor %u: 0x%04X [%s]\n", 
           i, 
           data.sensor[i], 
           has_error ? "ERROR" : "OK");
}
```

### 3. Monitorear Mediciones Guardadas

```c
// Mostrar últimas 5 mediciones cuando se finalice un cuero
if (measurements.current_leather_index > 0) {
    for (int i = measurements.current_leather_index - 5; 
         i < measurements.current_leather_index; i++) {
        if (i >= 0) {
            printf("Leather[%d]: %.2f mm²\n", 
                   i, 
                   measurements.leather_measurement[i]);
        }
    }
}
```

### 4. Verificar Acumulación de Lotes

```c
// Al cambiar de lote
if (measurements.current_leather_index >= config->batch_limit) {
    printf("Batch[%u] complete: %.2f mm² (%u items)\n",
           measurements.current_batch_index - 1,
           measurements.batch_measurement[measurements.current_batch_index - 1],
           config->batch_limit);
}
```

---

## Casos de Prueba

### Test 1: Detección de Inicio de Cuero

**Objetivo**: Verificar que `is_measuring` se activa cuando aparecen bits activos.

```
Evento                  Expected                Actual
─────────────────────────────────────────────────────────
Pulso 1: 0 bits        is_measuring=0          [ ]
Pulso 2: 50 bits       is_measuring=1          [ ]
                       area=2500 mm²           [ ]
Pulso 3: 60 bits       area+=3000 (total 5500)[ ]
```

**Verificar en código**:
```c
if (measurements.is_measuring != 1) {
    printf("ERROR: No detectado inicio de cuero\n");
}
```

---

### Test 2: Acumulación Correcta de Área

**Objetivo**: Verificar que el área se suma correctamente.

```
Bits   Fórmula                        Área esperada
─────  ─────────────────────────────  ───────────────
50     (50 × 10) × 5 = 500 × 5        2500 mm²
55     (55 × 10) × 5 = 550 × 5        2750 mm²
Total                                  5250 mm²
```

**Test en código**:
```c
uint16_t bits1 = 50;
uint16_t bits2 = 55;
float area1 = lgc_calculate_slice_area(bits1);
float area2 = lgc_calculate_slice_area(bits2);

if (area1 != 2500.0f || area2 != 2750.0f) {
    printf("ERROR: Cálculo de área incorrecto\n");
    printf("Expected: 2500, 2750 | Got: %.0f, %.0f\n", area1, area2);
}
```

---

### Test 3: Histéresis de Fin de Cuero

**Objetivo**: Verificar que se requieren N pasos sin detección para finalizar.

```
Pulso  Bits   no_detect  is_measuring  Acción
─────  ─────  ─────────  ────────────  ─────────────────
1      60     0          1             Midiendo
2      65     0          1             Continúa
3      0      1          1             Cuenta...
4      0      2          1             Cuenta...
5      0      3          0             ← FIN (threshold)
6      0      0          0             Inactivo
7      50     0          1             Nuevo cuero
```

**Verificar**:
```c
if (measurements.no_detection_count >= LGC_LEATHER_END_HYSTERESIS &&
    measurements.is_measuring == 0) {
    printf("Histéresis OK: Cuero finalizado en paso %u\n", 
           measurements.no_detection_count);
}
```

---

### Test 4: Gestión de Lotes

**Objetivo**: Verificar transición correcta entre lotes.

```
Configuración: batch_limit = 3

Cuero   leather_idx  batch_idx  Acción
─────   ───────────  ─────────  ──────────────────────
0       0            0          Lote 0
1       1            0          Lote 0
2       2            0          Lote 0
3       0            1          ← Lote 1 comienza
4       1            1          Lote 1
5       2            1          Lote 1
6       0            2          ← Lote 2 comienza
```

**Verificar**:
```c
if (measurements.current_leather_index == 0 && 
    measurements.current_batch_index > 0) {
    printf("Lote completado: batch_idx=%u\n", 
           measurements.current_batch_index - 1);
}
```

---

### Test 5: Validación de Límites de Array

**Objetivo**: Verificar que no se escriba fuera de los límites.

```c
// Test de desbordamiento
measurements.current_leather_index = LGC_LEATHER_COUNT_MAX;
measurements.current_batch_index = 5;

// Simular finalización de cuero
measurements.is_measuring = 0;
measurements.no_detection_count = LGC_LEATHER_END_HYSTERESIS;

// Verificar que no se sobrescribe
if (measurements.current_leather_index < LGC_LEATHER_COUNT_MAX) {
    printf("ALERTA: Intento de escribir fuera de bounds\n");
}
```

---

## Solución de Problemas

### Problema 1: Los sensores siempre fallan (sensor_status != 0)

**Síntomas**: 
- `data.sensor_status` siempre con bits SET
- Mensajes de reintentos constantemente

**Causas posibles**:
1. Comunicación Modbus incorrecta
2. Dirección de registro incorrecto (actualmente: 45)
3. Timeout de Modbus demasiado corto

**Solución**:
```c
// Aumentar timeout de Modbus
#define NMBS_READ_TIMEOUT 200   /* De 100 a 200 ms */
#define NMBS_WRITE_TIMEOUT 1500 /* De 1000 a 1500 ms */

// Verificar dirección de registro
// Cambiar 45 por la dirección correcta:
err = lgc_modbus_read_holding_regs(i + 1, 100, &data.sensor[i], 1);
```

---

### Problema 2: El cuero se detecta múltiples veces (falsos positivos)

**Síntomas**:
- Un cuero se guarda como 2-3 cueros separados
- `no_detection_count` alcanza el threshold muy rápido

**Causa**: Histéresis demasiado baja o ruido en sensores

**Solución**:
```c
// Aumentar histéresis
#define LGC_LEATHER_END_HYSTERESIS 5  /* De 3 a 5 */

// O filtrar bits muy bajos (ruido)
static uint16_t lgc_count_active_bits(void) {
    uint16_t active_bits = 0;
    const uint16_t MIN_VALID_WIDTH = 10;  /* Al menos 10 bits = 100mm */

    // ... contar bits ...

    // Rechazar si ancho es muy pequeño (ruido)
    if (active_bits < MIN_VALID_WIDTH) {
        return 0;
    }
    
    return active_bits;
}
```

---

### Problema 3: Las áreas medidas son incorrectas

**Síntomas**:
- Valores en `leather_measurement[]` inconsistentes
- Área no coincide con ancho visual de cuero

**Causa**: Macros `LGC_PIXEL_WIDTH_MM` o `LGC_ENCODER_STEP_MM` incorrectos

**Solución**:
```c
// Verificar cálculo:
// Para 60 bits detectados:
// Expected = (60 × LGC_PIXEL_WIDTH_MM) × LGC_ENCODER_STEP_MM

float expected_area = (60 * 10.0f) * 5.0f;  // = 3000 mm²

// Si resultado es diferente, ajustar macros
// Ejemplo: si área es el doble, el encoder_step está mal
#define LGC_ENCODER_STEP_MM 2.5f  // Cambiar a la mitad
```

---

### Problema 4: Se pierde datos (desbordamiento de batch)

**Síntomas**:
- `batch_measurement[]` deja de actualizarse
- `current_batch_index` se queda en 199

**Causa**: Se alcanzó `LGC_LEATHER_BATCH_COUNT_MAX`

**Solución**:
```c
// En lgc_process_measurement(), detectar condición:
if (measurements.current_batch_index >= LGC_LEATHER_BATCH_COUNT_MAX - 1) {
    printf("ALERTA: Se alcanzó el límite de lotes\n");
    printf("Último lote completado: batch[%u] = %.2f mm²\n",
           measurements.current_batch_index,
           measurements.batch_measurement[measurements.current_batch_index]);
    
    // TODO: Implementar acción (parar medición, guardar en EEPROM, etc.)
}
```

---

### Problema 5: El encoder no genera pulsos

**Síntomas**:
- `osWaitForSemaphore()` siempre timeout
- Ninguna medición se realiza

**Causa**: Encoder no conectado o interrupción no configurada

**Solución**:
```c
// Verificar inicialización del encoder
lgc_module_encoder_init(lgc_encoder_callback);

// Verificar callback:
static void lgc_encoder_callback(void) {
    osReleaseSemaphore(&encoder_flag);
    
    #ifdef DEBUG_ENCODER
    printf("Encoder pulse!\n");
    #endif
}

// Test manual: liberar semáforo en otro lugar para simular pulsos
osReleaseSemaphore(&encoder_flag);
```

---

## Verificación Final (Checklist)

- [ ] Todos los sensores leen correctamente (sensor_status == 0)
- [ ] Primera pieza detecta inicio cuando aparecen bits
- [ ] Área se acumula en `current_leather_area`
- [ ] Histéresis funciona (3+ pasos sin detección finaliza)
- [ ] `leather_measurement[]` se guarda correctamente
- [ ] `batch_measurement[]` suma múltiples cueros
- [ ] Transición de lote ocurre en `batch_limit`
- [ ] No hay desbordamiento de arrays
- [ ] Valores de área son realistas (ajustar macros si es necesario)
- [ ] Encoder emite pulsos regularmente

---

## Contacto y Soporte

Para más información, consultar:
- `IMPLEMENTATION_SUMMARY.md`: Resumen de cambios
- `MEASUREMENT_ALGORITHM.md`: Diagramas de flujo detallados

