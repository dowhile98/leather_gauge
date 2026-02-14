# Sistema de Medición de Cuero - Documentación de Arquitectura de Sistema

**Documento Técnico Senior** | Arquitectura Embebida STM32F4 + RTOS  
**Fecha:** 13 de Febrero de 2026  
**Versión:** 2.0 (Actualización: Daisy Chain & Snapshot Architecture)

---

## Tabla de Contenidos

1. [Resumen Ejecutivo](#resumen-ejecutivo)
2. [Arquitectura de Tareas (RTOS)](#arquitectura-de-tareas-rtos)
3. [Estrategia de Adquisición: Daisy Chain](#estrategia-de-adquisición-daisy-chain)
4. [Arquitectura de Reportes: Snapshot-Consumer](#arquitectura-de-reportes-snapshot-consumer)
5. [Interfaz de Usuario (HMI)](#interfaz-de-usuario-hmi)
6. [Mapa de Hardware](#mapa-de-hardware)

---

## Resumen Ejecutivo

El sistema **Leather Gauge Controller** es un controlador de grado industrial diseñado para la medición de alta precisión del área de cueros en cintas transportadoras. La Versión 2.0 introduce una arquitectura de alta velocidad y un sistema de reportes desacoplado:

- **Adquisición Daisy Chain:** Lectura de 11 sensores en ráfaga sincronizada (< 5ms total).
- **Arquitectura Snapshot-Consumer:** Separación total entre el motor de medición (tiempo real) y los consumidores lentos (HMI, Impresora).
- **Report Manager:** Tarea dedicada para la gestión de snapshots y protocolos ESC/POS.
- **Sincronización por Lotes:** Capacidad de hasta 300 piezas por lote con cierre manual o automático.

---

## Arquitectura de Tareas (RTOS)

El sistema utiliza **Azure ThreadX** bajo una capa de abstracción (OSAL). La arquitectura se basa en el desacoplamiento mediante **Snapshots**.

### 1. **Tarea de Medición (Main Task)** - `PRI: 10`
Es el motor de tiempo real. Su prioridad es la más alta entre las tareas de aplicación para garantizar que no se pierdan pulsos del encoder.
- **Trigger Daisy Chain:** Genera el pulso físico de inicio de ráfaga.
- **Parsing de Ráfaga:** Procesa 33 bytes de datos entrantes (DMA) y valida la integridad de los sensores.
- **Publicación de Live Status:** Cada ciclo de encoder publica un snapshot ligero (`LgcLiveStatus_t`) para el HMI.

### 2. **Report Manager (Consumidor de Snapshots)** - `PRI: 15`
Maneja las operaciones de E/S lentas y pesadas.
- **Impresión Térmica:** Formatea reportes ESC/POS con datos corporativos de **CURPISCO S.A.C.**
- **Batch Finalization:** Al cerrar un lote, recibe un `LgcBatchReport_t` completo y lo procesa sin bloquear la medición.

### 3. **HMI Update Task** - `PRI: 10`
Tarea encargada de la cosmética de la pantalla DWIN.
- **Consumo de Live Status:** Lee el snapshot más reciente del manager para actualizar contadores y áreas en tiempo real.
- **Zero-Allocation:** Se eliminó el uso de memoria dinámica para mejorar la estabilidad a largo plazo.

---

## Estrategia de Adquisición: Daisy Chain

A diferencia del modo Modbus tradicional (polling secuencial), el sistema ahora utiliza un modo de **Ráfaga Sincronizada**.

### Flujo de Datos
1. **Trigger:** La MCU genera un pulso de ~500µs en el pin `MASTER_TRIGGER`.
2. **Respuesta en Cascada:** El Sensor 1 envía sus datos y habilita al Sensor 2, y así sucesivamente.
3. **Burst UART:** La MCU recibe una ráfaga continua de 33 bytes:
   - `[ID_Sensor (1 byte)] [Data_High (1 byte)] [Data_Low (1 byte)]`
4. **Validación:** El firmware verifica que los IDs lleguen en secuencia (1 → 11). Cualquier error de sincronía marca el sensor como fallido en `sensor_status`.

### Ventajas Técnicas
- **Latencia:** Reducción del ciclo de lectura de 250ms a **3ms**.
- **Determinismo:** Los 110 fotoreceptores se capturan en el mismo instante físico del cuero.

---

## Arquitectura de Reportes: Snapshot-Consumer

Para evitar que la lentitud de una impresora térmica o los retardos de la UART del HMI afecten la precisión de la medición, se implementó un sistema de **Snapshots**.

### LgcLiveStatus_t (Snapshots en Tiempo Real)
Estructura pequeña enviada al HMI en cada slice:
- Estado del sistema (STOP/RUNNING/FAIL)
- Área del cuero actual
- Área acumulada del lote
- Contadores de piezas

### LgcBatchReport_t (Snapshots de Lote)
Estructura robusta generada al finalizar un lote:
- Metadata: Cliente, Color, ID, Fecha/Hora (RTC).
- Datos: Tabla completa de hasta 300 mediciones individuales.
- Totales: Área total y conversión de unidades (ft² / m²).

---

## Interfaz de Usuario (HMI)

### Direccionamiento Estándar (VP Addresses)
Se ha centralizado el control de la pantalla DWIN eliminando números mágicos. Todas las direcciones se manejan mediante el enum `LGC_HMI_VAR_ADDR_TypeDef_t`.

| Elemento | Dirección VP | Uso |
| :--- | :--- | :--- |
| **Batch Counter** | `0x1000` | Muestra el ID del lote actual |
| **Leather Counter** | `0x1001` | Muestra piezas en el lote actual |
| **Total Area** | `0x1004` | Área acumulada (Lote) |
| **Current Area** | `0x1006` | Área de la pieza en tránsito |
| **Status Icon** | `0x100A` | Icono dinámico de estado del sistema |

### Lógica de Borrado (Undo)
El usuario puede presionar "Borrar" en el HMI. El sistema:
1. Identifica la última pieza en el buffer del lote actual.
2. Resta su área del acumulador total.
3. Decrementa el índice y publica un nuevo **Live Status**.
4. La pantalla se actualiza instantáneamente reflejando el cambio.

---

## Mapa de Hardware (Actualizado)

| Recurso | Pin / Interface | Función |
| :--- | :--- | :--- |
| **Master Trigger** | `GPIOB Pin 12` | Disparo de ráfaga Daisy Chain |
| **Sensor Bus** | `USART3 (RS485)` | Entrada de ráfaga 33 bytes |
| **Encoder** | `TIM (EXTI)` | Sincronización de slices |
| **HMI** | `USART6` | Comunicación con DWIN |
| **EEPROM** | `I2C1` | Configuración persistente |
| **RTC** | `Internal` | Timestamping de reportes |

---

**Documentación de Arquitectura v2.0** | Leather Gauge Project  
**Especialista:** Firmware Lead Developer
