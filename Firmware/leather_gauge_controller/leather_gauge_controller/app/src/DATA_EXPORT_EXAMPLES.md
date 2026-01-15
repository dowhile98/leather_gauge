# Ejemplos de Uso - Exportación de Datos de Medición

## Índice
1. [Acceso a Mediciones](#acceso-a-mediciones)
2. [Exportación a EEPROM](#exportación-a-eeprom)
3. [Transmisión por UART](#transmisión-por-uart)
4. [Cálculo de Estadísticas](#cálculo-de-estadísticas)
5. [Integración con Panel P10](#integración-con-panel-p10)

---

## Acceso a Mediciones

### Variables Globales Disponibles

```c
extern lgc_measurements_t measurements;

// Contiene:
measurements.current_batch_index       // Lote actual (0-199)
measurements.current_leather_index     // Cuero actual en lote (0-299)
measurements.current_leather_area      // Acumulador (mientras se mide)
measurements.is_measuring              // Flag de estado
measurements.no_detection_count        // Contador de histéresis

// Arrays de datos:
measurements.leather_measurement[]     // Áreas individuales
measurements.batch_measurement[]       // Sumas por lote
```

### Acceso Thread-Safe

Aunque las mediciones se actualizan desde la tarea principal, acceso desde otras tareas requiere protección:

```c
/* Desde otra tarea (ej: tarea de impresión) */
static OsMutex measurements_mutex;

float get_current_leather_area(void) {
    float area;
    
    osAcquireMutex(&measurements_mutex);
    area = measurements.current_leather_area;
    osReleaseMutex(&measurements_mutex);
    
    return area;
}

uint16_t get_batch_index(void) {
    uint16_t idx;
    
    osAcquireMutex(&measurements_mutex);
    idx = measurements.current_batch_index;
    osReleaseMutex(&measurements_mutex);
    
    return idx;
}
```

---

## Exportación a EEPROM

### Estructura de Guardado

```c
typedef struct {
    uint16_t batch_count;                              // Número de lotes completados
    uint16_t total_leathers;                           // Total de cueros medidos
    float leather_area[300];                           // Copia de leather_measurement
    float batch_total[200];                            // Copia de batch_measurement
    uint32_t timestamp;                                // Marca de tiempo (ej: segundos desde boot)
    uint8_t crc;                                       // Checksum simple
} eeprom_measurement_record_t;
```

### Función para Guardar en EEPROM

```c
/**
 * @brief Save current measurements to EEPROM
 * @param batch_index Batch number to save (0-199)
 * @return error_t Status of save operation
 */
error_t save_measurements_to_eeprom(uint16_t batch_index) {
    eeprom_measurement_record_t record;
    error_t err = NO_ERROR;
    
    if (batch_index >= LGC_LEATHER_BATCH_COUNT_MAX) {
        return ERROR_FAILURE;
    }
    
    // Acquire mutex to prevent updates during save
    osAcquireMutex(&measurements_mutex);
    
    // Copy measurement data
    record.batch_count = measurements.current_batch_index;
    record.total_leathers = measurements.current_leather_index;
    record.timestamp = get_system_time();  // TODO: implementar
    
    // Copy arrays
    memcpy(record.leather_area, 
           measurements.leather_measurement, 
           sizeof(measurements.leather_measurement));
    
    memcpy(record.batch_total, 
           measurements.batch_measurement, 
           sizeof(measurements.batch_measurement));
    
    // Compute CRC
    record.crc = compute_crc8((uint8_t *)&record, 
                              sizeof(record) - 1);
    
    // Release mutex
    osReleaseMutex(&measurements_mutex);
    
    // Write to EEPROM (address: BATCH_BASE + (batch_index * sizeof(record)))
    uint32_t eeprom_addr = EEPROM_MEASUREMENTS_BASE + 
                           (batch_index * sizeof(eeprom_measurement_record_t));
    
    err = lgc_eeprom_write(eeprom_addr, 
                           (uint8_t *)&record, 
                           sizeof(eeprom_measurement_record_t));
    
    if (err == NO_ERROR) {
        printf("Batch[%u] saved to EEPROM at 0x%X\n", 
               batch_index, eeprom_addr);
    } else {
        printf("ERROR saving batch[%u]: %u\n", batch_index, err);
    }
    
    return err;
}
```

### Función para Leer de EEPROM

```c
/**
 * @brief Load measurements from EEPROM
 * @param batch_index Batch number to load
 * @param record Pointer to output structure
 * @return error_t Status of load operation
 */
error_t load_measurements_from_eeprom(uint16_t batch_index, 
                                      eeprom_measurement_record_t *record) {
    error_t err = NO_ERROR;
    uint8_t crc, crc_calc;
    
    if (batch_index >= LGC_LEATHER_BATCH_COUNT_MAX || record == NULL) {
        return ERROR_FAILURE;
    }
    
    // Calculate EEPROM address
    uint32_t eeprom_addr = EEPROM_MEASUREMENTS_BASE + 
                           (batch_index * sizeof(eeprom_measurement_record_t));
    
    // Read from EEPROM
    err = lgc_eeprom_read(eeprom_addr, 
                          (uint8_t *)record, 
                          sizeof(eeprom_measurement_record_t));
    
    if (err != NO_ERROR) {
        printf("ERROR reading batch[%u] from EEPROM\n", batch_index);
        return err;
    }
    
    // Verify CRC
    crc = record->crc;
    crc_calc = compute_crc8((uint8_t *)record, 
                            sizeof(*record) - 1);
    
    if (crc != crc_calc) {
        printf("ERROR: Batch[%u] CRC mismatch (got 0x%02X, expected 0x%02X)\n",
               batch_index, crc_calc, crc);
        return ERROR_FAILURE;
    }
    
    printf("Batch[%u] loaded from EEPROM: %u leathers, total=%.0f mm²\n",
           batch_index, 
           record->total_leathers,
           record->batch_total[batch_index]);
    
    return NO_ERROR;
}
```

---

## Transmisión por UART

### Formato CSV para Exportación

```c
/**
 * @brief Send measurements as CSV via UART
 * @param batch_index Batch to export
 */
void export_measurements_csv(uint16_t batch_index) {
    if (batch_index >= measurements.current_batch_index) {
        return;  // Batch not yet completed
    }
    
    uint16_t start_idx = batch_index * 10;     // Assuming 10 leathers per batch
    uint16_t end_idx = start_idx + 10;
    
    // CSV Header
    printf("Batch Measurement Export\n");
    printf("Batch Index, Leather Index, Area (mm²), Width (mm), Length (steps)\n");
    printf("============================================================\n");
    
    // Data rows
    for (uint16_t i = start_idx; i < end_idx && i < LGC_LEATHER_COUNT_MAX; i++) {
        float area = measurements.leather_measurement[i];
        float width = area / LGC_ENCODER_STEP_MM;  // Inverse calculation
        uint16_t steps = (uint16_t)(area / 
                                    (LGC_PIXEL_WIDTH_MM * LGC_ENCODER_STEP_MM));
        
        printf("%u, %u, %.2f, %.2f, %u\n",
               batch_index,
               i % 10,
               area,
               width,
               steps);
    }
    
    // Footer
    printf("============================================================\n");
    printf("Batch Total: %.2f mm²\n", 
           measurements.batch_measurement[batch_index]);
}
```

### Protocolo Binario para Telemetría

```c
/**
 * @brief Send real-time measurement data via UART
 * Called periodically from monitoring task
 */
#define TELEMETRY_PACKET_SIZE 16

typedef struct {
    uint8_t start;                    // 0xFF
    uint16_t active_bits;             // Current detection width
    float current_area;               // Running area accumulator
    uint8_t is_measuring;             // Measurement state
    uint16_t batch_idx;               // Current batch
    uint16_t leather_idx;             // Current leather
    uint8_t checksum;                 // Simple XOR checksum
    uint8_t end;                      // 0xFE
} telemetry_packet_t;

void send_telemetry(void) {
    telemetry_packet_t packet;
    uint8_t *p = (uint8_t *)&packet;
    uint8_t checksum = 0;
    
    packet.start = 0xFF;
    packet.active_bits = lgc_count_active_bits();
    packet.current_area = measurements.current_leather_area;
    packet.is_measuring = measurements.is_measuring;
    packet.batch_idx = measurements.current_batch_index;
    packet.leather_idx = measurements.current_leather_index;
    packet.end = 0xFE;
    
    // Calculate XOR checksum
    for (int i = 1; i < sizeof(telemetry_packet_t) - 2; i++) {
        checksum ^= p[i];
    }
    packet.checksum = checksum;
    
    // Send via UART
    HAL_UART_Transmit(&huart1, (uint8_t *)&packet, 
                      sizeof(telemetry_packet_t), 100);
}
```

---

## Cálculo de Estadísticas

### Estadísticas por Lote

```c
/**
 * @brief Calculate statistics for a completed batch
 */
typedef struct {
    float total_area;          // mm²
    float avg_area;            // mm²
    float min_area;            // mm²
    float max_area;            // mm²
    float std_deviation;       // mm²
    uint16_t leather_count;    // Number of items
} batch_statistics_t;

void calculate_batch_stats(uint16_t batch_index, 
                          batch_statistics_t *stats) {
    if (batch_index >= measurements.current_batch_index || 
        stats == NULL) {
        return;
    }
    
    uint16_t start = batch_index * 10;  // Assuming 10 per batch
    uint16_t end = start + 10;
    float sum = 0, sum_sq = 0;
    uint16_t count = 0;
    
    stats->min_area = FLT_MAX;
    stats->max_area = FLT_MIN;
    
    // First pass: compute min, max, sum
    for (uint16_t i = start; i < end && i < LGC_LEATHER_COUNT_MAX; i++) {
        float area = measurements.leather_measurement[i];
        
        if (area > 0) {
            sum += area;
            count++;
            
            if (area < stats->min_area) stats->min_area = area;
            if (area > stats->max_area) stats->max_area = area;
        }
    }
    
    stats->total_area = measurements.batch_measurement[batch_index];
    stats->leather_count = count;
    stats->avg_area = (count > 0) ? (sum / count) : 0;
    
    // Second pass: compute standard deviation
    sum_sq = 0;
    for (uint16_t i = start; i < end && i < LGC_LEATHER_COUNT_MAX; i++) {
        float area = measurements.leather_measurement[i];
        if (area > 0) {
            float diff = area - stats->avg_area;
            sum_sq += diff * diff;
        }
    }
    
    float variance = (count > 1) ? (sum_sq / (count - 1)) : 0;
    stats->std_deviation = sqrtf(variance);
}

void print_batch_stats(uint16_t batch_index) {
    batch_statistics_t stats;
    calculate_batch_stats(batch_index, &stats);
    
    printf("\n========== BATCH %u STATISTICS ==========\n", batch_index);
    printf("Total Items:       %u\n", stats.leather_count);
    printf("Total Area:        %.2f mm²\n", stats.total_area);
    printf("Average Area:      %.2f mm²\n", stats.avg_area);
    printf("Minimum Area:      %.2f mm²\n", stats.min_area);
    printf("Maximum Area:      %.2f mm²\n", stats.max_area);
    printf("Std Deviation:     %.2f mm²\n", stats.std_deviation);
    printf("=========================================\n\n");
}
```

### Tasa de Producción

```c
/**
 * @brief Calculate production rate
 */
typedef struct {
    float leathers_per_minute;
    float area_per_minute;
    float estimated_minutes_to_complete;
} production_rate_t;

void calculate_production_rate(uint32_t elapsed_seconds, 
                               production_rate_t *rate) {
    float elapsed_minutes = elapsed_seconds / 60.0f;
    uint16_t total_leathers = measurements.current_leather_index +
                              (measurements.current_batch_index * 10);
    float total_area = 0;
    
    // Sum all batch areas
    for (int i = 0; i < measurements.current_batch_index; i++) {
        total_area += measurements.batch_measurement[i];
    }
    
    // Add current batch in progress
    total_area += measurements.current_leather_area;
    
    rate->leathers_per_minute = (elapsed_minutes > 0) ? 
                                (total_leathers / elapsed_minutes) : 0;
    rate->area_per_minute = (elapsed_minutes > 0) ? 
                            (total_area / elapsed_minutes) : 0;
    
    // Estimate time to complete remaining (assuming 10 leathers per batch)
    uint16_t remaining = (20 * 10) - total_leathers;  // Total 200 batches
    rate->estimated_minutes_to_complete = (rate->leathers_per_minute > 0) ? 
                                          (remaining / rate->leathers_per_minute) : 0;
}
```

---

## Integración con Panel P10

### Actualización de Display

```c
/**
 * @brief Update P10 panel with current measurement status
 * Called periodically (e.g., every 100ms)
 */
void update_p10_display(void) {
    char line1[32];
    char line2[32];
    uint16_t active_bits = lgc_count_active_bits();
    
    if (measurements.is_measuring) {
        // Showing measurement in progress
        snprintf(line1, sizeof(line1), 
                "L:%u/%u A:%.0f", 
                measurements.current_leather_index,
                10,  // items per batch (config->batch_limit)
                measurements.current_leather_area);
        
        snprintf(line2, sizeof(line2),
                "B:%u W:%umm",
                measurements.current_batch_index,
                active_bits * 10);  // width in mm
    } else {
        // Ready state
        snprintf(line1, sizeof(line1),
                "Batch %u",
                measurements.current_batch_index);
        
        snprintf(line2, sizeof(line2),
                "Ready: %u/300",
                measurements.current_leather_index);
    }
    
    // Send to panel driver
    lgc_interface_printer_print_line(0, line1);  // Line 1
    lgc_interface_printer_print_line(1, line2);  // Line 2
}

/**
 * @brief Display final batch results on P10
 */
void display_batch_complete(void) {
    batch_statistics_t stats;
    calculate_batch_stats(measurements.current_batch_index - 1, &stats);
    
    // Screen 1: Summary
    char msg1[32], msg2[32];
    snprintf(msg1, sizeof(msg1), "Batch %u Complete", 
             measurements.current_batch_index - 1);
    snprintf(msg2, sizeof(msg2), "Items: %u", stats.leather_count);
    
    lgc_interface_printer_print_line(0, msg1);
    lgc_interface_printer_print_line(1, msg2);
    osDelayTask(2000);  // Show for 2 seconds
    
    // Screen 2: Area
    snprintf(msg1, sizeof(msg1), "Total: %.0f", stats.total_area);
    snprintf(msg2, sizeof(msg2), "Avg: %.0f", stats.avg_area);
    
    lgc_interface_printer_print_line(0, msg1);
    lgc_interface_printer_print_line(1, msg2);
    osDelayTask(2000);
}
```

---

## Resumen de Integración

```
Mediciones (RAM)
    ↓
    ├─→ EEPROM [save_measurements_to_eeprom()]
    │
    ├─→ UART CSV [export_measurements_csv()]
    │
    ├─→ Telemetría [send_telemetry()]
    │
    ├─→ Estadísticas [calculate_batch_stats()]
    │
    └─→ Panel P10 [update_p10_display()]
```

Cada módulo puede integrase de forma independiente según los requerimientos del proyecto.

