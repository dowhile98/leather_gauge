# Leather Gauge Controller

[![STM32](https://img.shields.io/badge/STM32-F446RC-blue.svg)](https://www.st.com/en/microcontrollers-microprocessors/stm32f446rc.html)
[![RTOS](https://img.shields.io/badge/RTOS-Azure_ThreadX-green.svg)](https://github.com/eclipse-threadx/threadx)
[![License](https://img.shields.io/badge/license-Proprietary-red.svg)]()

Sistema embebido industrial para medición automática del área de piezas de cuero en movimiento continuo mediante sensores fotoeléctricos sincronizados con encoder rotativo.

---

## Descripción General

El **Leather Gauge Controller** es un firmware profesional basado en STM32F446RCTx que implementa un sistema de medición de alta precisión para cuero en procesos industriales. La Versión 2.0 introduce una arquitectura de alta velocidad con adquisición Daisy Chain y un sistema de reportes completamente desacoplado (Snapshot-Consumer Architecture).

### Características Principales

- **Adquisición Daisy Chain**: Lectura sincronizada de 11 sensores en ráfaga de 33 bytes por pulso de encoder (< 50ms).
- **Snapshot-Consumer Architecture**: Desacoplamiento total entre medición (Main Task) y salida (HMI / Impresora / P10).
- **Sistema multi-tarea**: Azure ThreadX RTOS con 6 tareas concurrentes.
- **Interfaz HMI**: Display DWIN UART optimizado con snapshots ligeros (`LgcLiveStatus_t`).
- **Impresión de reportes**: Report Manager asíncrono con ESC/POS para impresora térmica USB.
- **Salida Modbus P10**: Publicación en tiempo real del área medida hacia indicador externo.
- **Almacenamiento persistente**: EEPROM AT24C256 I2C con validación CRC32.
- **Identidad Corporativa**: Reportes térmicos para **CURPISCO S.A.C.**

---

## Estado Actual del Proyecto (Marzo 2026)

### Validación de Módulos

| Módulo             | Estado          | Observaciones                                            |
| ------------------ | --------------- | -------------------------------------------------------- |
| **Daisy Chain**    | ✅ Validado     | Ráfaga de 33 bytes (11 sensores × 3 bytes) operativa     |
| **Encoder**        | ✅ Validado     | ISR-safe, señal por semáforo, escalable por `PULSE_FLAG` |
| **EEPROM**         | ✅ Validado     | CRC32 con recuperación automática de defaults            |
| **Report Manager** | ✅ Implementado | Snapshot atómico + impresión asíncrona + estado en vivo  |
| **HMI (DWIN)**     | ✅ Funcional    | 3 tareas dedicadas, 20+ páginas, sensor test integrado   |
| **Impresora USB**  | ✅ Funcional    | ESC/POS vía USBX, auto-skip si no conectada              |
| **Main Task**      | ✅ Funcional    | Motor de medición de alta prioridad con state machine    |
| **P10 Task**       | ✅ Funcional    | Publicación Modbus RTU hacia indicador externo           |

### Cambios Recientes (v2.0 - 2026)

- **Daisy Chain Mode**: Sustitución del polling Modbus secuencial por lectura en ráfaga sincronizada.
- **Snapshot Architecture**: Implementación de `LgcBatchReport_t` (cierre de lote) y `LgcLiveStatus_t` (estado en vivo).
- **Report Manager**: Nueva tarea dedicada para impresión térmica y distribución de snapshots.
- **P10 Task**: Nueva tarea que publica el área actual hacia un indicador Modbus externo (UART5).
- **Manual Undo**: Botón de borrado del último cuero con retroalimentación inmediata al HMI.
- **HMI Clean-up**: Eliminación de números mágicos, centralización de VP addresses en `lgc_hmi.h`.
- **Memory Pool**: Gestión de memoria dinámica segura mediante `TX_BYTE_POOL` de ThreadX.

---

## Especificaciones Técnicas

### Hardware

| Componente         | Especificación                          |
| ------------------ | --------------------------------------- |
| **MCU**            | STM32F446RCTx (ARM Cortex-M4F)          |
| **Flash**          | 256 KB                                  |
| **RAM**            | 128 KB                                  |
| **Frecuencia**     | 180 MHz                                 |
| **FPU**            | FPv4-SP-D16 (hardware single-precision) |
| **Sensores**       | 11 módulos RS-485 (110 fotocélulas)     |
| **Display**        | DWIN LCD UART (USART6, 115200 bps)      |
| **Encoder**        | Rotativo incremental (EXTI, PA0)        |
| **Almacenamiento** | AT24C256 EEPROM I2C (I2C1, 100 kHz)     |
| **Impresora**      | Térmica ESC/POS (USB Host - USBX)       |
| **Indicador P10**  | RS-485 Modbus RTU (UART5)               |

### Software

- **RTOS**: Azure ThreadX (Eclipse ThreadX)
- **HAL**: STM32 HAL Driver
- **Toolchain**: GNU ARM Embedded 13.2.1
- **IDE**: STM32CubeIDE
- **C Standard**: C11
- **Middlewares**:
  - nanoMODBUS (Modbus RTU master/slave)
  - lwprintf (lightweight printf, sin malloc)
  - lwrb (ring buffers para UART DMA)
  - lwbtn (debounce de botones)
  - dwin (driver display DWIN)
  - at24cxx (driver EEPROM I2C)

---

## Estructura del Proyecto

```
leather_gauge_controller_v4/
├── Core/                                  # Código generado por STM32CubeMX
│   ├── Inc/                               # Cabeceras HAL (GPIO, USART, I2C, RTC...)
│   ├── Src/                               # main.c, interrupts, app_threadx.c
│   └── Startup/                           # Startup assembly y linker
│
├── leather_gauge_controller/              # Código de aplicación
│   ├── app/
│   │   ├── inc/
│   │   │   ├── lgc.h                      # API pública del sistema
│   │   │   ├── lgc_typedefs.h             # Tipos globales, enums, structs
│   │   │   └── lgc_report_manager.h       # API del Report Manager
│   │   └── src/
│   │       ├── lgc.c                      # lgc_system_init(): arranque de todas las tareas
│   │       ├── lgc_main_task.c            # Motor de medición (1014 líneas)
│   │       ├── lgc_mem_pool.c             # Gestión de memory pool ThreadX
│   │       ├── lgc_report_manager.c       # Report Manager + Report Task
│   │       ├── hmi/
│   │       │   ├── lgc_hmi.h              # VP addresses y páginas del display
│   │       │   └── lgc_hmi_task.c         # DWIN Process + HMI Task + HMI Update Task
│   │       └── p10/
│   │           └── lgc_p10_task.c         # Publicación Modbus hacia indicador P10
│   │
│   ├── modules/                           # Módulos de hardware (HAL abstraction)
│   │   ├── di/lgc_module_input.c          # Entradas digitales + debounce (lwbtn)
│   │   ├── encoder/lgc_module_encoder.c   # Encoder: wrapper EXTI → callback
│   │   ├── eeprom/lgc_module_eeprom.c     # Config EEPROM con CRC32 (AT24C256)
│   │   ├── rtc/lgc_module_rtc.c           # RTC con mutex thread-safe
│   │   ├── modbus/lgc_inteface_modbus.c   # UART3 DMA: modo Modbus RTU y Daisy Chain
│   │   └── printer/
│   │       ├── lgc_interface_printer.c    # Interfaz USB (USBX) hacia impresora
│   │       └── ESC_POS_Printer.c          # Protocolo ESC/POS
│   │
│   ├── osal/                              # OS Abstraction Layer (14 RTOS)
│   │   ├── include/os_port.h              # API unificada: tasks, mutex, semáforos, eventos
│   │   ├── common/                        # Utilidades comunes
│   │   └── portable/                      # Ports específicos por RTOS
│   │
│   ├── config/                            # Opciones de configuración
│   │   ├── os_port_config.h               # Selección de RTOS activo
│   │   └── ...
│   └── middlewares/                       # Librerías externas (links simbólicos)
│
├── Middlewares/ST/
│   ├── threadx/                           # Azure ThreadX
│   └── usbx/                              # Azure USBX (USB Host para impresora)
│
├── Drivers/                               # STM32 HAL + CMSIS
├── docs/SYSTEM_ARCHITECTURE.md            # Documentación de arquitectura
└── README.md
```

---

## Arquitectura del Sistema

### Diagrama de Tareas y Flujo de Datos

```
  ┌───────────────────────────────────────────────────────────────────────┐
  │                         STM32F446RC MCU                               │
  │                                                                       │
  │  Encoder ISR ──semáforo──▶  ┌─────────────────────────────────────┐  │
  │                             │         Main Task  (pri=10)          │  │
  │  Button ISR ──evento────▶   │  - State Machine (STOP/RUN/FAIL)     │  │
  │                             │  - Daisy Chain trigger + burst parse  │  │
  │                             │  - lgc_process_measurement()          │  │
  │                             │  - lgc_finalize_batch_snapshot()      │  │
  │                             └────────┬────────────────┬────────────┘  │
  │                                      │                │               │
  │                         LgcLiveStatus_t         LGC_EVENT_SNAPSHOT   │
  │                                      │                │               │
  │  ┌───────────────────────────────────▼──────┐  ┌─────▼────────────┐ │
  │  │        Report Manager (servicio)         │  │  Report Task     │ │
  │  │  - lgc_report_update_live_status()        │  │  (pri=15)        │ │
  │  │  - lgc_report_get_live_status()           │  │  - Espera evento │ │
  │  │  - lgc_report_update_snapshot()           │  │  - Imprime ESC   │ │
  │  │  - lgc_report_get_last_snapshot()         │  │    /POS via USB  │ │
  │  └───────────┬───────────────────────────────┘  └──────────────────┘ │
  │              │  LgcLiveStatus_t                                       │
  │  ┌───────────▼──────────────────────────────────────────────────────┐ │
  │  │  HMI Update Task (pri=10)       HMI Task (pri=10)                │ │
  │  │  - Espera LGC_HMI_UPDATE_REQ    - Recibe touch events de DWIN    │ │
  │  │  - Escribe VPs al display       - Gestiona navegación de páginas  │ │
  │  │  - Actualiza 20+ variables      - Envía comandos al Main Task     │ │
  │  │                                                                   │ │
  │  │  DWIN Process Task (pri=10)                                       │ │
  │  │  - Driver interno DWIN (RX/TX protocolo)                          │ │
  │  └──────────────────────────────────────────────────────────────────┘ │
  │                                                                       │
  │  ┌────────────────────────────────┐                                  │
  │  │  P10 Task (pri=10)             │                                  │
  │  │  - Lee current_leather_area    │                                  │
  │  │  - Escribe a reg Modbus 12     │                                  │
  │  │  - Refresh cada 300ms (UART5)  │                                  │
  │  └────────────────────────────────┘                                  │
  └───────────────────────────────────────────────────────────────────────┘
         │            │            │             │           │
         ▼            ▼            ▼             ▼           ▼
    Encoder      RS-485 DMA     DWIN         Printer      EEPROM
    (EXTI/PA0)  (UART3/huart3) (UART6)     (USBX/USB)   (I2C1)
                 Sensores 1-11                            AT24C256
                 + P10 (UART5)
```

---

## Tareas RTOS — Descripción Detallada

El sistema arranca desde `app_threadx.c → lgc_system_init()` que inicializa todos los módulos y crea las 6 tareas.

### 1. Main Task — `lgc_main_task_entry()`

**Archivo**: [leather_gauge_controller/app/src/lgc_main_task.c](leather_gauge_controller/app/src/lgc_main_task.c)  
**Prioridad**: 10 | **Stack**: 1024 words

Motor central del sistema. Implementa la máquina de estados y el algoritmo de medición completo.

**Responsabilidades:**

- **Máquina de estados** con tres estados: `LGC_STOP`, `LGC_RUNNING`, `LGC_FAIL`
- **En LGC_RUNNING**: aguarda semáforo del encoder (`encoder_flag`) con timeout 50ms
- **Por cada pulso de encoder:**
  1. Envía pulso trigger al bus RS-485 para iniciar ráfaga Daisy Chain
  2. Espera evento `LGC_EVENT_BURST_READY` (timeout 50ms) del módulo Modbus
  3. Parsea los 33 bytes de burst → 11 valores de sensor (`data.sensor[]`)
  4. Llama a `lgc_process_measurement()` (bajo mutex)
  5. Actualiza `LgcLiveStatus_t` via `lgc_update_live_status()`
  6. Si la pieza terminó (evento 1): señaliza `LGC_HMI_UPDATE_REQUIRED`
  7. Si el lote se completó (evento 2) o hay petición manual: llama `lgc_finalize_batch_snapshot()`
- **Gestión de cierre de lote**: detecta `LGC_EVENT_CLOSE_BATCH_REQ` del HMI y ejecuta el cierre atómico

**Algoritmo de medición** (`lgc_process_measurement`):

```
Por cada pulso de encoder:
  1. Contar bits activos en los 11 sensores (0-110 fotocélulas)
  2. slice_area = active_bits × 20mm (pixel) × 20.398mm (encoder)
  3. Convertir a m² y aplicar factor de conversión (ft² o m²)
  4. Si hay bits activos → is_measuring=1, acumular current_leather_area
  5. Si no hay bits activos → no_detection_count++
  6. Cuando no_detection_count >= 3 (histéresis):
     - Guardar medición en leather_measurement[index]
     - Acumular a batch_measurement[batch_index]
     - Incrementar leather_index
     - Si leather_index >= config.batch → evento = 2 (lote completo)
     - Si no → evento = 1 (pieza individual completa)
```

**Callbacks de botones** (`lgc_buttons_callback`):

- `START_STOP`: toggle `LGC_EVENT_START / LGC_EVENT_STOP`
- `GUARD`: activa `LGC_FAILURE_DETECTED` (fallo) / `LGC_FAILURE_CLEARED`
- `SPEEDS`: conmuta LED velocidad alta/baja
- `FEEDBACK`: flag de retroalimentación motor

---

### 2. DWIN Process Task — `lgc_dwin_process_task_entry()`

**Archivo**: [leather_gauge_controller/app/src/hmi/lgc_hmi_task.c](leather_gauge_controller/app/src/hmi/lgc_hmi_task.c)  
**Prioridad**: 10 | **Stack**: 512 words

Tarea interna del driver DWIN. Procesa el protocolo de comunicación con el display UART (USART6, 115200 bps, RS-485 via `DIR_DISPLAY`). Recibe datos via DMA en `uart_rx[128]`, los empuja al buffer interno del driver y despacha eventos de touch hacia la cola `hmi_msg`.

---

### 3. HMI Task — `lgc_hmi_task_entry()`

**Archivo**: [leather_gauge_controller/app/src/hmi/lgc_hmi_task.c](leather_gauge_controller/app/src/hmi/lgc_hmi_task.c)  
**Prioridad**: 10 | **Stack**: 1024 words

Procesa los **eventos táctiles del usuario** recibidos desde DWIN Process Task vía cola `hmi_msg`.

**Responsabilidades por VP de touch:**

| VP Touch                         | Acción                                                           |
| -------------------------------- | ---------------------------------------------------------------- |
| `LGC_HMI_TOUCH_PAGE_ADDR`        | Navega de página; emite `LGC_EVENT_STOP` al entrar config        |
| `LGC_HMI_VP_LIST_DELETE`         | Llama `lgc_clear_measurement_last_leather()` (undo último cuero) |
| `LGC_HMI_VP_PRINT`               | Emite `LGC_EVENT_CLOSE_BATCH_REQ` → Main Task cierra lote        |
| `LGC_HMI_VP_TEST_CHOICED_SENSOR` | Activa modo test de sensor (página 3/4)                          |
| `LGC_HMI_VP_CONFIG_SAVE_CMD`     | Guarda configuración en EEPROM (cliente, color, ID, unidades)    |
| `LGC_HMI_VP_CONFIG_TIME_CMD`     | Actualiza hora/fecha en RTC                                      |

---

### 4. HMI Update Task — `lgc_hmi_update_task_entry()`

**Archivo**: [leather_gauge_controller/app/src/hmi/lgc_hmi_task.c](leather_gauge_controller/app/src/hmi/lgc_hmi_task.c)  
**Prioridad**: 10 | **Stack**: 1024 words

Tarea de **actualización reactiva del display**. Se despierta por eventos o cada 300ms.

**Páginas soportadas:**

| Página          | Contenido mostrado                                                                  |
| --------------- | ----------------------------------------------------------------------------------- |
| `HMI_PAGE1`     | Estado, velocidad, fecha, batch count, leather count, área actual/acumulada, config |
| `HMI_PAGE3/4`   | Test de sensores: valor de bit del sensor seleccionado                              |
| `HMI_PAGE12-17` | Reporte de lote: lista paginada de 50 piezas por página (desde snapshot)            |

**Fuente de datos:**

- Páginas de medición → `lgc_report_get_live_status()` → `LgcLiveStatus_t` (liviano, sin copiar arrays)
- Páginas de reporte → `lgc_report_get_last_snapshot()` → `LgcBatchReport_t` (snapshot completo)

---

### 5. Report Task — `lgc_report_task_entry()`

**Archivo**: [leather_gauge_controller/app/src/lgc_report_manager.c](leather_gauge_controller/app/src/lgc_report_manager.c)  
**Prioridad**: 15 (la más baja) | **Stack**: 2048 words

Tarea de **impresión asíncrona**. Espera indefinidamente el evento `LGC_EVENT_SNAPSHOT_READY`.

**Flujo cuando recibe el evento:**

1. Verifica si la impresora USB está conectada (`lgc_interface_printer_connected()`)
2. Adquiere mutex del snapshot para garantizar coherencia
3. Imprime el reporte completo via `lgc_print_batch_report()`:
   - Cabecera corporativa (CURPISCO S.A.C., dirección, RUC)
   - Número de lote, fecha/hora del cierre
   - Datos del cliente (nombre, color, ID de cuero)
   - Tabla: ítem / área / unidad (ft² o m²) para cada pieza
   - Total del lote con unidades
   - Corte de papel
4. Si no hay impresora → log de error, sin bloqueo

**API del Report Manager (servicio compartido):**

```c
lgc_report_update_live_status()  // llamada por Main Task cada encoder pulse
lgc_report_get_live_status()     // llamada por HMI Update Task
lgc_report_update_snapshot()     // llamada por Main Task al cerrar lote
lgc_report_get_last_snapshot()   // llamada por HMI Update Task (páginas 12-17)
```

---

### 6. P10 Task — `lgc_p10_task_entry()`

**Archivo**: [leather_gauge_controller/app/src/p10/lgc_p10_task.c](leather_gauge_controller/app/src/p10/lgc_p10_task.c)  
**Prioridad**: 10 | **Stack**: 512 words

Publica el área de cuero actual hacia un **indicador externo tipo P10** (tablero de puntuación u otro dispositivo RS-485) a través de UART5.

**Funcionamiento:**

1. Lee `LgcLiveStatus_t.current_leather_area` desde el Report Manager
2. Escala el valor (×100 para conservar 2 decimales sin float)
3. Escribe el valor en registro Modbus 12 del servidor RTU (dirección 1) via UART5
4. Delay 300ms → repite

> **Nota**: Usa `DIR_DISPLAY_GPIO_Port` para control de dirección RS-485, comparte línea física con el display DWIN o usa un transceptor secundario.

---

## Módulos de Hardware

### `lgc_module_encoder` — Encoder Incremental

**Archivo**: [leather_gauge_controller/modules/encoder/lgc_module_encoder.c](leather_gauge_controller/modules/encoder/lgc_module_encoder.c)

Wrapper minimalista de EXTI. Registra el callback externo y lo invoca desde `HAL_GPIO_EXTI_Callback`. El callback de la aplicación (`lgc_encoder_callback`) cuenta pulsos y libera el semáforo `encoder_flag` cada `LGC_LEATHER_MAX_PULSE_FLAG` pulsos.

---

### `lgc_interface_modbus` — Bus RS-485 Dual Mode

**Archivo**: [leather_gauge_controller/modules/modbus/lgc_inteface_modbus.c](leather_gauge_controller/modules/modbus/lgc_inteface_modbus.c)

Driver UART3 con DMA que soporta dos modos de operación:

| Modo                       | Descripción                                                                                         |
| -------------------------- | --------------------------------------------------------------------------------------------------- |
| `LGC_BUS_MODE_MODBUS`      | nanoMODBUS RTU estándar con ring buffer + semáforo. Para config/diagnóstico.                        |
| `LGC_BUS_MODE_DAISY_CHAIN` | Acumulación de burst en `burst_buffer[]`. Espera 33 bytes (11×3). Señaliza `LGC_EVENT_BURST_READY`. |

En modo Daisy Chain, el módulo mantiene el transceptor RS-485 (`DIR_SENSORES`) permanentemente en RX. El pulso trigger se envía por GPIO (`MASTER_TRIGGER`), no por UART.

---

### `lgc_module_eeprom` — Configuración Persistente

**Archivo**: [leather_gauge_controller/modules/eeprom/lgc_module_eeprom.c](leather_gauge_controller/modules/eeprom/lgc_module_eeprom.c)

AT24C256 (32KB) via I2C1. Almacena `LGC_CONF_TypeDef_t` en dirección 0x0000 con CRC32 IEEE 802.3 al final. Si el CRC no coincide al cargar, restaura defaults automáticamente.

**Configuración `LGC_CONF_TypeDef_t`:**

| Campo         | Tipo     | Descripción                                            |
| ------------- | -------- | ------------------------------------------------------ |
| `client_name` | char[16] | Nombre del cliente                                     |
| `color`       | char[16] | Color del cuero                                        |
| `leather_id`  | char[16] | Identificador del cuero                                |
| `batch`       | uint32_t | Límite de piezas por lote                              |
| `units`       | uint8_t  | 0 = ft², 1 = m²                                        |
| `conversion`  | uint8_t  | Factor de conversión (0=28×28, 1=30×30, 2=30.48×30.48) |
| `crc`         | uint32_t | CRC32 del resto de la estructura                       |

---

### `lgc_module_input` — Entradas Digitales

**Archivo**: [leather_gauge_controller/modules/di/lgc_module_input.c](leather_gauge_controller/modules/di/lgc_module_input.c)

Debounce de botones con librería `lwbtn`. Mapeo de entradas:

| Entrada             | Pin | Función                         |
| ------------------- | --- | ------------------------------- |
| `LGC_DI_START_STOP` | PC0 | Toggle Start/Stop del sistema   |
| `LGC_DI_GUARD`      | PC1 | Parada de emergencia (guard)    |
| `LGC_DI_SPEEDS`     | PC2 | Conmutación velocidad alta/baja |
| `LGC_DI_FEEDBACK`   | PC3 | Feedback del motor              |

---

### `ESC_POS_Printer` + `lgc_interface_printer` — Impresora Térmica

**Archivos**: [leather_gauge_controller/modules/printer/](leather_gauge_controller/modules/printer/)

`lgc_interface_printer.c` expone la conexión al host USB USBX. `ESC_POS_Printer.c` implementa el protocolo ESC/POS (alineación, tamaño de fuente, separadores, tabla de columnas, corte).

---

## Flujo Completo del Sistema

```
BOOT
 └─▶ lgc_system_init()
      ├─ lgc_hmi_init()         → crea DWIN Process + HMI Task + HMI Update Task
      ├─ lgc_report_manager_init() → crea Report Task
      ├─ lgc_interface_modbus_init() → configura UART3 DMA
      ├─ lgc_module_input_init()   → registra callbacks de botones
      ├─ lgc_module_eeprom_init()  → carga configuración desde EEPROM
      ├─ lgc_p10_init()            → crea P10 Task
      └─ osCreateTask(lgc_main_task_entry) → crea Main Task

OPERACIÓN NORMAL en LGC_RUNNING:
  Encoder ISR ─(semáforo)─▶ Main Task se despierta
                              ├─ lgc_trigger_chain()       ← pulso GPIO
                              ├─ espera LGC_EVENT_BURST_READY (50ms)
                              ├─ lgc_parse_burst_data()    ← rellena data.sensor[]
                              ├─ lgc_process_measurement() ← algoritmo, devuelve 0/1/2
                              ├─ lgc_update_live_status()  ← publica LgcLiveStatus_t
                              └─ si evento==2: lgc_finalize_batch_snapshot()
                                               ├─ copia todo a LgcBatchReport_t
                                               ├─ resetea contadores del lote
                                               └─ señaliza LGC_EVENT_SNAPSHOT_READY

LGC_EVENT_SNAPSHOT_READY ─▶  Report Task se despierta
                              └─ imprime reporte ESC/POS via USB

LGC_HMI_UPDATE_REQUIRED  ─▶  HMI Update Task se despierta
                              └─ dwin_write_vp_u16() para cada variable en pantalla
```

---

## Configuración Hardware

### Pinout Principal

#### Entradas Digitales

| Pin | Función  | Descripción                              |
| --- | -------- | ---------------------------------------- |
| PA0 | DI_0_INT | Encoder pulse (EXTI interrupt)           |
| PC0 | DI_2     | Botón START/STOP                         |
| PC1 | DI_3     | Botón GUARD (parada de emergencia)       |
| PC2 | DI_4     | Botón SPEEDS (conmutación velocidad)     |
| PC3 | DI_5     | Botón FEEDBACK (retroalimentación motor) |

#### Salidas Digitales

| Pin  | Función        | Descripción                        |
| ---- | -------------- | ---------------------------------- |
| PB0  | DO_0           | Output control (Running)           |
| PB1  | DO_1           | LED Running                        |
| PB3  | DO_2           | LED Running (invertido)            |
| PB9  | DO_6           | LED Speed Low                      |
| PB15 | DO_7           | LED Speed High                     |
| PC13 | DIR_DISPLAY    | RS-485 dirección DWIN (USART6)     |
| PB14 | DIR_SENSORES   | RS-485 dirección sensores (USART3) |
| PA2  | MASTER_TRIGGER | Pulso trigger Daisy Chain          |

#### Comunicación

| Periférico | Pines     | Función                       | Velocidad    |
| ---------- | --------- | ----------------------------- | ------------ |
| USART3     | PB10/PB11 | Sensores RS-485 (Daisy Chain) | Configurable |
| USART6     | PC6/PC7   | Display DWIN (RS-485)         | 115200 bps   |
| UART5      | -         | Indicador P10 (Modbus RTU)    | Configurable |
| USB OTG FS | -         | Impresora térmica (USB Host)  | -            |
| I2C1       | PB6/PB7   | EEPROM AT24C256               | 100 kHz      |

---

## Uso y Operación

### Máquina de Estados del Sistema

```
                   START Button
┌──────────┐      (guard == 0)       ┌─────────────┐
│ LGC_STOP │ ─────────────────────▶  │ LGC_RUNNING │
└──────────┘                         └──────┬──────┘
     ▲                                      │
     │                           STOP btn / │ GUARD pressed
     │                           guard==1   │
     │                                      ▼
     │                              ┌──────────────┐
     └──────────────────────────────│  LGC_FAIL    │
              GUARD released        └──────────────┘
```

### Operación Normal

1. **Encendido**: Sistema inicia en `LGC_STOP`. Se carga configuración de EEPROM.
2. **Iniciar**: Presionar START → `LGC_RUNNING`, activa Daisy Chain mode, enciende LEDs.
3. **Por cada pulso de encoder**:
   - Adquiere ráfaga de 33 bytes de los 11 sensores
   - Calcula área del slice y lo acumula en la pieza actual
   - Detecta fin de pieza (3 slices vacíos consecutivos)
   - Actualiza display en tiempo real
4. **Cierre de lote** (automático cuando se llena, o manual desde HMI):
   - Crea snapshot `LgcBatchReport_t` con todos los datos
   - Incrementa índice de lote, resetea contadores
   - Report Task imprime el reporte en la impresora USB
5. **Undo**: Botón "borrar último" elimina la última pieza del lote actual
6. **Detener**: Presionar STOP → `LGC_STOP`, modo Modbus normal restaurado.

### Capacidades

| Parámetro               | Valor                               |
| ----------------------- | ----------------------------------- |
| Piezas máximas por lote | 300 (`LGC_LEATHER_COUNT_MAX`)       |
| Lotes máximos           | 200 (`LGC_LEATHER_BATCH_COUNT_MAX`) |
| Tamaño de pixel         | 20mm × 20.398mm (ancho × encoder)   |
| Resolución de área      | 0.25 ft² (redondeo configurable)    |
| Ancho máximo medición   | 2200mm (110 fotocélulas × 20mm)     |
| Tiempo máximo de ráfaga | 50ms timeout por encoder pulse      |

---

## Parámetros Configurables

Editar constantes en los archivos fuente:

```c
// leather_gauge_controller/app/src/lgc_main_task.c
#define LGC_PIXEL_WIDTH_MM        20.0f    // Ancho de cada fotocélula en mm
#define LGC_ENCODER_STEP_MM       20.398f  // Distancia por pulso de encoder en mm
#define LGC_LEATHER_END_HYSTERESIS 3       // Slices vacíos para detectar fin de pieza
#define LGC_LEATHER_MAX_PULSE_FLAG 1       // Pulsos de encoder por trigger de lectura

// leather_gauge_controller/app/inc/lgc_typedefs.h
#define LGC_SENSOR_NUMBER         11       // Número de sensores
#define LGC_LEATHER_COUNT_MAX     300      // Piezas máximas por lote
#define LGC_LEATHER_BATCH_COUNT_MAX 200    // Lotes máximos

// leather_gauge_controller/modules/modbus/lgc_inteface_modbus.c
#define NMBS_READ_TIMEOUT         50       // Timeout lectura Modbus (ms)
#define MODBUS_RX_BUFFER_SIZE     128      // Tamaño buffer DMA
```

---

## Direcciones VP del Display DWIN

Las direcciones VP están centralizadas en [leather_gauge_controller/app/src/hmi/lgc_hmi.h](leather_gauge_controller/app/src/hmi/lgc_hmi.h):

| Variable                               | Dirección VP | Descripción                     |
| -------------------------------------- | ------------ | ------------------------------- |
| `LGC_HMI_VP_STATE`                     | 0x1110       | Estado del sistema (guard icon) |
| `LGC_HMI_VP_ICON_SPEEP`                | 0x1111       | Indicador de velocidad          |
| `LGC_HMI_VP_FEEDBACK_MOTOR`            | 0x1112       | Feedback motor ON/OFF           |
| `LGC_HMI_VP_BATCH_COUNT`               | 0x1050       | Contador de lotes               |
| `LGC_HMI_VP_LEATHER_COUNT`             | 0x1051       | Contador de cueros              |
| `LGC_HMI_VP_CURRENT_LEATHER_AREA`      | 0x1060       | Área pieza actual (×100)        |
| `LGC_HMI_VP_ACUMULATED_LEATHER_AREA`   | 0x1080       | Área acumulada del lote (×100)  |
| `LGC_HMI_VP_CONFIG_DAY`                | 0x1341       | Configuración día RTC           |
| `LGC_HMI_VP_CONFIG_MONTH`              | 0x1342       | Configuración mes RTC           |
| `LGC_HMI_VP_CONFIG_YEAR`               | 0x1343       | Configuración año RTC           |
| `LGC_HMI_VP_CONFIG_HOUR`               | 0x1346       | Configuración hora RTC          |
| `LGC_HMI_VP_CONFIG_MINUTE`             | 0x1347       | Configuración minuto RTC        |
| `LGC_HMI_VP_CONFIG_SECOND`             | 0x1348       | Configuración segundo RTC       |
| `LGC_HMI_VP_LIST_DELETE`               | (touch)      | Borrar última medición          |
| `LGC_HMI_VP_PRINT`                     | (touch)      | Cerrar lote e imprimir          |
| `LGC_HMI_VP_LIST_ADDRESS_LEATHER_BASE` | 0x2000+      | Base lista individual (50/pág)  |

---

## Cambio de RTOS

El proyecto soporta 14 diferentes RTOS mediante OSAL. Para cambiar, editar [leather_gauge_controller/config/os_port_config.h](leather_gauge_controller/config/os_port_config.h):

```c
// Descomentar el RTOS deseado
#define USE_THREADX           // ThreadX (actual)
// #define USE_FREERTOS       // FreeRTOS
// #define USE_CMSIS_RTOS     // CMSIS-RTOS v1
// #define USE_CMSIS_RTOS2    // CMSIS-RTOS v2
// ... etc
```

RTOS soportados: ThreadX, FreeRTOS, µC/OS-II, µC/OS-III, CMSIS-RTOS, CMSIS-RTOS2, RTX, SafeRTOS, Zephyr, ChibiOS, embOS, PX5, Windows, POSIX, None.

---

## Compilación

### Requisitos

- **STM32CubeIDE** 1.x o superior
- **GNU ARM Embedded Toolchain** 13.2.1 o compatible

### Opción 1: STM32CubeIDE (Recomendado)

1. `File` → `Import` → `Existing Projects into Workspace`
2. Seleccionar el directorio raíz del proyecto
3. Build: `Project` → `Build All` (Ctrl+B)

### Opción 2: Línea de comandos

```bash
make -C Debug clean
make -C Debug all -j$(nproc)
# Binario generado en Debug/leather_gauge_controller_v4.elf
```

### Configuraciones de Build

| Configuración | Optimización | Tamaño  | Uso                    |
| ------------- | ------------ | ------- | ---------------------- |
| **Debug**     | -Og          | ~150 KB | Desarrollo, depuración |
| **Release**   | -O2 / -Os    | ~100 KB | Producción             |

---

## Flasheo y Programación

```bash
# ST-LINK CLI
st-flash write Debug/leather_gauge_controller_v4.bin 0x08000000

# OpenOCD
openocd -f interface/stlink.cfg -f target/stm32f4x.cfg \
  -c "program Debug/leather_gauge_controller_v4.elf verify reset exit"
```

---

## Depuración

### Mensajes de Log

El sistema utiliza `stm32_log` configurado en [leather_gauge_controller/config/stm32_log_config.h](leather_gauge_controller/config/stm32_log_config.h):

```c
#define STM32_LOG_ENABLE   1
#define STM32_LOG_LEVEL    LOG_DEBUG
```

### Breakpoints Clave

| Función                         | Archivo              | Propósito                             |
| ------------------------------- | -------------------- | ------------------------------------- |
| `lgc_encoder_callback`          | lgc_main_task.c      | Verificar interrupciones encoder      |
| `lgc_parse_burst_data`          | lgc_main_task.c      | Depurar datos de sensores             |
| `lgc_process_measurement`       | lgc_main_task.c      | Algoritmo de cálculo de área          |
| `lgc_finalize_batch_snapshot`   | lgc_main_task.c      | Cierre atómico de lote                |
| `lgc_print_batch_report`        | lgc_report_manager.c | Depurar impresión ESC/POS             |
| `lgc_hmi_update_task_entry`     | lgc_hmi_task.c       | Actualización de variables en display |
| `lgc_report_update_live_status` | lgc_report_manager.c | Verificar publicación de live status  |

### Variables en Tiempo Real (Live Watch)

```c
data.sensor[0..10]           // Valores raw de los 11 sensores
measurements.current_leather_area  // Área en acumulación
measurements.current_leather_index // Pieza actual en el lote
sensor_read_time_diff_ms     // Latencia de la ráfaga Daisy Chain
```

---

## Solución de Problemas

### Burst Daisy Chain no llega (`LGC_EVENT_BURST_READY` timeout)

```
Causa: Los sensores no responden al pulso trigger o llegan < 33 bytes
Solución:
- Verificar conexión RS-485 (DIR_SENSORES en GPIO_PIN_RESET para RX)
- Medir pulso en MASTER_TRIGGER_Pin (PA2) con osciloscopio
- Verificar que burst_rx_index alcanza 33 en lgc_modbus_rx_callback()
- Revisar baudrate de los sensores
```

### Display DWIN no actualiza

```
Causa: Fallo en comunicación UART6 o cola hmi_msg llena
Solución:
- Verificar USART6 a 115200 bps (pines PC6/PC7)
- Revisar DIR_DISPLAY (PC13) en modo TX/RX correcto
- Poner breakpoint en lgc_dwin_uart_RxEventCallback()
```

### EEPROM no carga configuración correcta

```
Causa: CRC inválido → se cargan defaults automáticamente (batch=10, units=ft²)
Solución:
- Verificar I2C1 (PB6/PB7, SDA/SCL)
- Confirmar chip AT24C256 con dirección A000
- Revisar lgc_module_conf_load() → comparar CRC calculado vs almacenado
```

### Impresora no imprime

```
Causa: USB no conectado o USBX no enumera el dispositivo
Solución:
- Verificar lgc_interface_printer_connected() retorna true
- El Report Task hace skip si no hay impresora (no bloquea el sistema)
- Revisar configuración USBX en USBX/App/app_usbx_host.c
```

---

## Checklist de Validación HMI

- [ ] Display enciende y muestra página 1 (HMI_PAGE1)
- [ ] Contador de cueros actualiza en tiempo real durante medición
- [ ] Contador de lotes incrementa al completar batch
- [ ] Área actual muestra valor correcto (dividir VP entre 100)
- [ ] Botón "borrar" resta correctamente la última pieza
- [ ] Botón "imprimir" cierra lote y dispara Report Task
- [ ] Fecha/hora se muestra desde RTC
- [ ] Configuración del cliente se guarda y carga desde EEPROM
- [ ] Páginas 12-17 muestran lista de piezas del último lote
- [ ] Modo test de sensores (páginas 3/4) muestra bit pattern del sensor

---

## Documentación Adicional

- **[docs/SYSTEM_ARCHITECTURE.md](docs/SYSTEM_ARCHITECTURE.md)**: Arquitectura detallada del sistema
- **[action-daisy-chain-implementation.md](action-daisy-chain-implementation.md)**: Documentación del modo Daisy Chain
- **[action-batch-reporting-architecture.md](action-batch-reporting-architecture.md)**: Diseño de la arquitectura de reportes

### Referencias Externas

- [STM32F446 Reference Manual](https://www.st.com/resource/en/reference_manual/dm00135183-stm32f446xx-advanced-arm-based-32-bit-mcus-stmicroelectronics.pdf)
- [Azure ThreadX Documentation](https://github.com/eclipse-threadx/rtos-docs)
- [nanoMODBUS Library](https://github.com/debevv/nanoMODBUS)
- [DWIN Display Protocol](https://www.dwin-global.com/)

---

## Contribución

```bash
# Crear branch para feature
git checkout -b feature/nueva-funcionalidad

# Commits con mensajes descriptivos
git commit -m "feat: descripción clara del cambio"
git commit -m "fix: descripción del bug corregido"

# Push y crear pull request
git push origin feature/nueva-funcionalidad
```

**Convenciones de código:**

- Prefijo `lgc_` para funciones públicas
- `snake_case` para funciones y variables
- `PascalCase_t` para tipos `typedef struct`
- `LGC_UPPER_CASE` para constantes y enums

---

## Roadmap

- [ ] Tests unitarios (Unity + CMock para algoritmo de medición)
- [ ] CI/CD pipeline (GitHub Actions, build en Linux)
- [ ] Watchdog timer (IWDG) para auto-recuperación
- [ ] Calibración configurable desde HMI (pixel size, encoder step)
- [ ] Logging persistente de errores en EEPROM
- [ ] Modo diagnóstico de sensores (test individual por Modbus)
- [ ] Comunicación Ethernet/WiFi para monitoreo remoto

---

## Licencia

Propietario - Todos los derechos reservados.  
Este código es propiedad privada y no debe ser distribuido sin autorización.

---

## Historial de Versiones

| Versión | Fecha      | Cambios                                                           |
| ------- | ---------- | ----------------------------------------------------------------- |
| 2.0.0   | 2026-03-14 | Daisy Chain Mode, Snapshot Architecture, Report Manager, P10 Task |
| 1.1.0   | 2026-01-20 | Módulo RTC thread-safe, centralización VP addresses, Manual Undo  |
| 1.0.0   | 2026-01-16 | Versión inicial: medición Modbus + DWIN + Impresora               |

---

**Desarrollado con STM32CubeIDE y Azure ThreadX**
