# Leather Gauge Controller V2 🚀

[![STM32](https://img.shields.io/badge/STM32-F446RC-blue.svg)](https://www.st.com/en/microcontrollers-microprocessors/stm32f446rc.html)
[![RTOS](https://img.shields.io/badge/RTOS-Azure_ThreadX-green.svg)](https://github.com/eclipse-threadx/threadx)
[![Architecture](https://img.shields.io/badge/Architecture-Clean_Architecture-green.svg)]()
[![SOLID](https://img.shields.io/badge/Principles-SOLID-orange.svg)]()
[![License](https://img.shields.io/badge/license-Proprietary-red.svg)]()

**Sistema embebido industrial de alta precisión para medición automática del área de piezas de cuero en movimiento continuo mediante sensores fotoeléctricos sincronizados con encoder rotativo.**

**⚠️ Proyecto en Refactorización hacia Clean Architecture - Ver [REFACTOR_PLAN.md](REFACTOR_PLAN.md)**

---

## 📋 Tabla de Contenidos

- [Descripción General](#-descripción-general)
- [🆕 Nueva Arquitectura (Clean Architecture)](#-nueva-arquitectura-clean-architecture)
- [Estado del Proyecto](#-estado-del-proyecto-febrero-2026)
- [Especificaciones Técnicas](#-especificaciones-técnicas)
- [Estructura del Proyecto](#-estructura-del-proyecto)
- [Compilación](#-compilación)
- [Recursos y Documentación](#-recursos-y-documentación)

---

## 🎯 Descripción General

El **Leather Gauge Controller** es un firmware profesional basado en **STM32F446RCTx** que implementa un sistema de medición de alta precisión para cuero en procesos industriales. El sistema mide automáticamente el área de cada pieza de cuero mientras se desplaza por una banda transportadora, utilizando un arreglo de **11 sensores con 110 fotocélulas totales**.

### Características Principales

✅ **Medición de precisión:** Algoritmo de integración por "slices" sincronizado con encoder (±0.5% accuracy)  
✅ **Sistema multi-tarea:** Azure ThreadX RTOS para operación concurrente y determinística  
✅ **Comunicación robusta:** LwPKT cascade (550ms para 11 sensores, 67% más rápido que Modbus legacy)  
✅ **Interfaz HMI:** Display DWIN táctil para visualización en tiempo real y configuración  
✅ **Impresión automática:** Impresora térmica ESC/POS para reportes de lote  
✅ **Almacenamiento persistente:** EEPROM I2C con CRC32 para configuración y batches  
✅ **Arquitectura limpia:** Refactorización hacia Clean Architecture + SOLID (en progreso 40%)  
✅ **Thread-safe:** Mutexes y semáforos para acceso concurrente seguro a recursos compartidos  
✅ **RTC integrado:** Módulo RTC con mutex para fecha/hora sincronizada

---

## 🏗️ Nueva Arquitectura (Clean Architecture)

### Visión Arquitectónica

Este proyecto está en proceso de **refactorización completa** desde una arquitectura monolítica a una arquitectura limpia basada en **Clean Architecture** y principios **SOLID**. El objetivo es lograr:

- **100% Desacoplamiento**: Core sin dependencias directas a HAL
- **90% Testabilidad**: Lógica de negocio testable en PC sin hardware
- **Inversión de Dependencias**: Interfaces definidas por Domain, implementadas por Adapters
- **Migración sin Downtime**: Sistema funcional en cada commit

### Capas Arquitectónicas (Target)

```
┌─────────────────────────────────────────────────────────────┐
│                    📱 PRESENTATION LAYER                     │
│  Composition Root (DI Container) + Tasks (HMI, Printer)     │
└──────────────────────────┬──────────────────────────────────┘
                           │ Dependency Injection
┌──────────────────────────▼──────────────────────────────────┐
│                  🧠 DOMAIN LAYER (CORE)                      │
│  Entities + Use Cases (Measure Area, Manage Batch, etc.)    │
└──────────────────────────┬──────────────────────────────────┘
                           │ Interfaces (Ports)
┌──────────────────────────▼──────────────────────────────────┐
│              🔌 INTERFACE LAYER (ABSTRACTIONS)               │
│  ISensorReader, IEncoder, IStorage, IDisplay, IPrinter...   │
└──────────────────────────┬──────────────────────────────────┘
                           │ Implementations
┌──────────────────────────▼──────────────────────────────────┐
│            ⚙️ INFRASTRUCTURE LAYER (ADAPTERS)                │
│  Modbus/LwPKT, Encoder, EEPROM, Display, Printer Adapters   │
└──────────────────────────┬──────────────────────────────────┘
                           │
┌──────────────────────────▼──────────────────────────────────┐
│         🔩 HARDWARE ABSTRACTION LAYER (HAL + RTOS)           │
│  STM32F4 HAL, ThreadX, nanoMODBUS, lwrb, lwbtn, dwin...     │
└─────────────────────────────────────────────────────────────┘
```

### Progreso de Refactorización

| Fase               | Estado | Progreso | Siguiente Hito                               |
| ------------------ | :----: | :------: | -------------------------------------------- |
| **Fundaciones**    |   ✅   |   100%   | Estructura de carpetas + DI Container básico |
| **Adapters**       |   🔄   |   40%    | Encoder ✅, Modbus 🔄, EEPROM 🔄             |
| **Use Cases**      |   ⏳   |    0%    | Extraer algoritmo de medición a UC           |
| **Peripherals**    |   ⏳   |    0%    | Display, Printer, Inputs adapters            |
| **Testing & Docs** |   ⏳   |    0%    | Unit tests (mocks) + Doxygen 100%            |

**📖 Ver plan detallado:** [REFACTOR_PLAN.md](REFACTOR_PLAN.md)

---

## 📊 Estado del Proyecto (Febrero 2026)

### Arquitectura Actual vs Target

| Aspecto                     | Legacy (Actual)             | Clean Architecture (Target) | Estado |
| --------------------------- | --------------------------- | --------------------------- | ------ |
| **Acoplamiento a HAL**      | Directo en módulos          | Solo en Adapters            | 🔄 40% |
| **Testabilidad**            | 0% (requiere hardware)      | 90% (mocks)                 | ⏳ 0%  |
| **Inversión Dependencias**  | ❌ No aplicado              | ✅ Interfaces + DI          | 🔄 30% |
| **Responsabilidad Única**   | Múltiples por módulo (~3-4) | Una por módulo              | 🔄 50% |
| **Documentación (Doxygen)** | ~40%                        | 100%                        | 🔄 45% |

### Validación de Módulos Funcionales

| Módulo            | Estado Legacy    | Estado Clean Arch | Observaciones                                   |
| ----------------- | ---------------- | ----------------- | ----------------------------------------------- |
| **Modbus RTU**    | 🗒️ Legacy/Backup | ❌ Deprecated     | Solo para fallback (latencia 2s vs 550ms LwPKT) |
| **LwPKT Cascade** | ✅ Actual        | ✅ Productión     | `lwpkt_adapter` con ISensorReader DMA + lwrb    |
| **Encoder**       | ✅ Funcional     | ✅ Refactorizado  | `encoder_adapter` implementa IEncoder           |
| **EEPROM**        | ✅ Funcional     | 🔄 Refactorando   | Migrar a `eeprom_adapter` + IStorage + CRC32    |
| **RTC**           | ✅ Funcional     | 🔄 Refactorando   | Migrar a `rtc_adapter` + IRealTimeClock         |
| **HMI (DWIN)**    | 🔄 En Pruebas    | ⏳ Pendiente      | Requiere `display_adapter` + IDisplay           |
| **Impresora**     | 🔄 Pendiente     | ⏳ Pendiente      | Requiere `printer_adapter` + IPrinter           |
| **Main Task**     | ✅ Funcional     | 🔄 Refactorando   | Extraer Use Cases (Measure Area, Batch Mgmt)    |

### Cambios Recientes (Refactorización)

- ✅ **Estructura Clean Architecture**: Carpetas `domain/`, `adapters/`, `app/` creadas
- ✅ **DI Container**: `lgc_di_container.c` implementado (básico)
- 🔄 **Encoder Adapter**: Refactorizado con IEncoder (90% completo)
- 🔄 **Entidades**: `lgc_measurement_entity.h`, `lgc_sensor_array_entity.h` definidas
- 📝 **Plan de Refactorización**: [REFACTOR_PLAN.md](REFACTOR_PLAN.md) actualizado (12 semanas)
- 📝 **Estándares de Código**: Guías SOLID + TDD en `.github/copilot-instructions.md`

---

## ⚙️ Especificaciones Técnicas

### Validación de Módulos

| Módulo         | Estado          | Observaciones                                    |
| -------------- | --------------- | ------------------------------------------------ |
| **Modbus RTU** | ✅ Validado     | Comunicación con 11 sensores probada y funcional |
| **Encoder**    | ✅ Validado     | Interrupciones EXTI funcionando correctamente    |
| **EEPROM**     | ✅ Validado     | Lectura/escritura de configuración operativa     |
| **RTC**        | ✅ Implementado | Módulo con mutex para acceso thread-safe         |
| **HMI (DWIN)** | 🔄 En Pruebas   | Requiere validación de escritura/lectura VP      |
| **Impresora**  | 🔄 Pendiente    | Pendiente integración con hardware               |
| **Main Task**  | ✅ Funcional    | Algoritmo de medición operativo                  |

### Cambios Recientes

- **lgc_module_rtc**: Nuevo módulo RTC con funciones `init/set/get/deinit` y mutex
- **lgc_hmi.h**: Centralización de direcciones VP en enumeración
- **lgc_hmi_task.c**: Corrección de índice `current_batch_index` (evita índice -1)
- **.gitignore**: Agregado para excluir carpeta Debug/

---

## Especificaciones Técnicas

### Hardware

| Componente         | Especificación                                     |
| ------------------ | -------------------------------------------------- |
| **MCU**            | STM32F446RCTx (ARM Cortex-M4F)                     |
| **Flash**          | 256 KB                                             |
| **RAM**            | 128 KB                                             |
| **Frecuencia**     | 180 MHz                                            |
| **FPU**            | FPv4-SP-D16 (hardware single-precision)            |
| **Sensores**       | 11 módulos LwPKT cascade (110 fotocélulas, RS-485) |
| **Display**        | DWIN LCD UART                                      |
| **Encoder**        | Rotativo incremental (EXTI)                        |
| **Almacenamiento** | AT24Cxx EEPROM I2C                                 |
| **Impresora**      | Térmica ESC/POS (UART)                             |

### Software

- **RTOS**: Azure ThreadX (Eclipse ThreadX)
- **HAL**: STM32 HAL Driver
- **Toolchain**: GNU ARM Embedded 13.2.1
- **IDE**: STM32CubeIDE
- **C Standard**: C11
- **Middlewares**:
  - nanoMODBUS (Modbus RTU)
  - lwprintf (lightweight printf)
  - lwrb (ring buffers)
  - lwbtn (button handling)
  - dwin (display driver)
  - at24cxx (EEPROM driver)

---

## 📂 Estructura del Proyecto

### Nueva Arquitectura (Clean Architecture - Target)

```
leather_gauge_controller_v2/
│
├── Core/                                      # STM32 HAL initialization (CubeMX)
│   ├── Inc/                                   # Hardware headers
│   ├── Src/                                   # main.c, interrupts, syscalls
│   └── Startup/                               # Startup assembly code
│
├── leather_gauge_controller/                  # 🚀 APPLICATION CODE (Clean Arch)
│   │
│   ├── domain/                                # 🧠 DOMAIN LAYER (Pure C, NO HAL)
│   │   ├── entities/                          # Business entities
│   │   │   ├── lgc_measurement_entity.h       # - LeatherPiece, Batch, Measurement
│   │   │   ├── lgc_sensor_array_entity.h      # - SensorArray (11x10), SensorReading
│   │   │   ├── lgc_configuration_entity.h     # - SystemConfig, CalibrationData
│   │   │   └── lgc_common_types.h             # - Result_t, DateTime_t, enums
│   │   │
│   │   ├── use_cases/                         # Business rules (testable)
│   │   │   ├── measure/
│   │   │   │   ├── lgc_uc_measure_area.c/h    # - Área integración por slices
│   │   │   │   ├── lgc_uc_process_slice.c/h   # - Procesar single slice
│   │   │   │   └── lgc_uc_detect_leather.c/h  # - Detección inicio/fin pieza
│   │   │   ├── batch/
│   │   │   │   └── lgc_uc_manage_batch.c/h    # - Crear/Finalizar batch
│   │   │   ├── calibration/
│   │   │   │   └── lgc_uc_calibrate_sensors.c/h # - Zero offset
│   │   │   └── reporting/
│   │   │       └── lgc_uc_generate_report.c/h # - Formatear reporte
│   │   │
│   │   └── interfaces/                        # 🔌 PORTS (Abstracciones DIP)
│   │       ├── lgc_i_sensor_reader.h          # - ISensorReader (read array)
│   │       ├── lgc_i_encoder.h                # - IEncoder (position, callback)
│   │       ├── lgc_i_storage.h                # - IStorage (save/load)
│   │       ├── lgc_i_display.h                # - IDisplay (read/write VP)
│   │       ├── lgc_i_printer.h                # - IPrinter (print, cut)
│   │       ├── lgc_i_digital_inputs.h         # - IDigitalInputs (buttons)
│   │       └── lgc_i_real_time_clock.h        # - IRealTimeClock (get/set time)
│   │
│   ├── adapters/                              # ⚙️ INFRASTRUCTURE (Implementations)
│   │   ├── communication/
│   │   │   ├── modbus_adapter/                # Modbus RTU adapter
│   │   │   │   ├── lgc_modbus_adapter.c/h     # - Implementa ISensorReader
│   │   │   │   └── lgc_modbus_config.h        # - Timeouts, baudrate
│   │   │   └── lwpkt_adapter/                 # LwPKT adapter (futuro)
│   │   │       └── lgc_lwpkt_adapter.c/h      # - Implementa ISensorReader
│   │   │
│   │   ├── peripherals/
│   │   │   ├── encoder_adapter/               # ✅ Refactorizado
│   │   │   │   └── lgc_encoder_adapter.c/h    # - Implementa IEncoder (EXTI)
│   │   │   ├── display_adapter/               # ⏳ Pendiente
│   │   │   │   └── lgc_display_adapter.c/h    # - Implementa IDisplay (DWIN)
│   │   │   ├── printer_adapter/               # ⏳ Pendiente
│   │   │   │   └── lgc_printer_adapter.c/h    # - Implementa IPrinter
│   │   │   └── digital_inputs_adapter/        # ⏳ Pendiente
│   │   │       └── lgc_digital_inputs_adapter.c/h # - IDigitalInputs
│   │   │
│   │   └── storage/
│   │       ├── eeprom_adapter/                # 🔄 Refactorando
│   │       │   ├── lgc_eeprom_adapter.c/h     # - Implementa IStorage
│   │       │   └── lgc_eeprom_crc.c/h         # - CRC32 validation
│   │       └── rtc_adapter/                   # 🔄 Refactorando
│   │           └── lgc_rtc_adapter.c/h        # - Implementa IRealTimeClock
│   │
│   ├── app/                                   # 📱 APPLICATION LAYER (Composition)
│   │   ├── inc/
│   │   │   ├── lgc.h                          # - API pública del sistema
│   │   │   ├── lgc_typedefs.h                 # - Tipos legacy (deprecar)
│   │   │   └── lgc_di_container.h             # - DI Container público
│   │   │
│   │   └── src/
│   │       ├── lgc.c                          # - Inicialización sistema
│   │       ├── lgc_di_container.c             # - Dependency Injection & Wiring
│   │       ├── lgc_main_task.c                # - Main control task (ThreadX)
│   │       ├── lgc_mem_pool.c                 # - Memory pool management
│   │       ├── hmi/
│   │       │   ├── lgc_hmi_task.c             # - HMI update task
│   │       │   └── lgc_hmi.h                  # - VP address definitions
│   │       └── printer/
│   │           └── lgc_printer_task.c         # - Printer command task
│   │
│   ├── modules/                               # 🗂️ LEGACY MODULES (migrar a adapters)
│   │   ├── di/                                # → digital_inputs_adapter
│   │   ├── encoder/                           # ✅ Migrado a encoder_adapter
│   │   ├── modbus/                            # 🔄 → modbus_adapter
│   │   ├── eeprom/                            # 🔄 → eeprom_adapter
│   │   ├── rtc/                               # 🔄 → rtc_adapter
│   │   └── printer/                           # ⏳ → printer_adapter
│   │
│   ├── osal/                                  # 🔄 OS ABSTRACTION LAYER
│   │   ├── include/                           # - os_port.h
│   │   ├── common/                            # - error.h, date_time.h
│   │   └── portable/threadx/                  # - ThreadX port
│   │
│   ├── middlewares/                           # 📦 THIRD-PARTY (sin cambios)
│   │   ├── nanoMODBUS/                        # - Modbus RTU client
│   │   ├── lwrb/                              # - Ring buffers
│   │   ├── lwprintf/                          # - Lightweight printf
│   │   ├── lwbtn/                             # - Button handling
│   │   ├── dwin/                              # - DWIN display driver
│   │   └── at24cxx/                           # - EEPROM driver
│   │
│   └── config/                                # ⚙️ CONFIGURATION
│       ├── lgc_domain_config.h                # - Domain constants
│       ├── lgc_hardware_config.h              # - Hardware pins, clocks
│       └── lwbtn_opts.h, lwprintf_opts.h, etc.
│
├── Middlewares/                               # ST middlewares
│   ├── ST/threadx/                            # Azure ThreadX RTOS
│   └── ST/usbx/                               # Azure USBX stack
│
├── Drivers/                                   # STM32 drivers
│   ├── CMSIS/                                 # ARM CMSIS
│   └── STM32F4xx_HAL_Driver/                  # STM32 HAL
│
├── docs/                                      # 📖 Documentation
│   ├── SYSTEM_ARCHITECTURE.md                 # Comprehensive architecture
│   ├── sensor/README.md                       # Sensor protocol docs
│   └── Lista de variables.xlsx                # Variable list
│
├── Debug/                                     # Build artifacts
├── REFACTOR_PLAN.md                           # 📋 Refactorización completa
├── README.md                                  # This file
├── STM32F446RCTX_FLASH.ld                     # Linker script
└── leather_gauge_controller.ioc               # STM32CubeMX project
```

### Leyenda de Estado

| Icono | Estado             | Descripción                                   |
| :---: | ------------------ | --------------------------------------------- |
|  ✅   | Refactorizado      | Cumple Clean Architecture + SOLID             |
|  🔄   | En Refactorización | Migración parcial (funcional pero no limpio)  |
|  ⏳   | Pendiente          | Legacy, a refactorizar según REFACTOR_PLAN.md |
|  🗂️   | Legacy             | Estructura vieja, será eliminada              |

---

## 🏛️ Arquitectura del Sistema

### Diagrama de Bloques

```
┌─────────────────────────────────────────────────────────────┐
│                    STM32F446RC MCU                          │
│                                                             │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐    │
│  │  Main Task   │  │   HMI Task   │  │ Printer Task │    │
│  │              │  │              │  │              │    │
│  │  - Encoder   │  │  - Display   │  │  - Reports   │    │
│  │  - Sensors   │  │  - User I/O  │  │  - ESC/POS   │    │
│  │  - Algorithm │  │  - Updates   │  │  - Events    │    │
│  └──────┬───────┘  └──────┬───────┘  └──────┬───────┘    │
│         │                 │                  │             │
│         └─────────────────┼──────────────────┘             │
│                           │                                │
│                   Azure ThreadX RTOS                       │
│                                                             │
│  ┌──────────────────────────────────────────────────────┐ │
│  │           Hardware Abstraction Layer (HAL)           │ │
│  └──────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────┘
         │          │          │          │          │
         ▼          ▼          ▼          ▼          ▼
    Encoder    Modbus     DWIN      Printer    EEPROM
    (EXTI)    (UART+DMA)  (UART)    (USB)     (I2C)
```

### Tareas RTOS

| Tarea            | Prioridad | Stack     | Función                            |
| ---------------- | --------- | --------- | ---------------------------------- |
| **Main Task**    | 10        | 256 words | Medición, encoder, sensores Modbus |
| **HMI Task**     | 11        | 512 words | Actualización display DWIN         |
| **HMI Update**   | 12        | 256 words | Procesamiento eventos UI           |
| **DWIN Process** | 13        | 512 words | Protocolo DWIN                     |
| **Printer Task** | 14        | 512 words | Impresión reportes                 |

### Algoritmo de Medición

El sistema implementa un **algoritmo de integración por "slices"** (rebanadas):

1. **Trigger**: Cada pulso del encoder (5mm de desplazamiento)
2. **Lectura**: 11 sensores LwPKT cascade (110 bits totales, ~550ms)
3. **Cálculo**: Área de slice = `bits_activos × 10mm × 5mm`
4. **Acumulación**: Área total de la pieza
5. **Detección de fin**: Histéresis de 3 pasos sin cuero
6. **Registro**: Almacenar medición y actualizar contadores

```
Leather piece:
┌─────────────────────────┐
│█████████████████████████│ ← Slice N   (encoder pulse)
│█████████████████████████│ ← Slice N-1
│██████████████░░░░░░░░░██│ ← Slice N-2
│████████░░░░░░░░░░░░░████│ ← ...
│███░░░░░░░░░░░░░░░░░░░███│
└─────────────────────────┘
   110 photocells (11×10)

Total Area = Σ(active_bits × pixel_area)
```

---

## Compilación

### Requisitos

- **STM32CubeIDE** 1.x o superior
- **GNU ARM Embedded Toolchain** 13.2.1 o compatible
- **STM32CubeMX** (incluido en STM32CubeIDE)

### Pasos

#### Opción 1: STM32CubeIDE (Recomendado)

1. Abrir STM32CubeIDE
2. `File` → `Import` → `Existing Projects into Workspace`
3. Seleccionar el directorio raíz del proyecto
4. Click en `Finish`
5. Build: `Project` → `Build All` (Ctrl+B)

#### Opción 2: Línea de comandos

```bash
# Navegar al directorio del proyecto
cd leather_gauge_controller

# Build Debug
make -C Debug clean
make -C Debug all -j$(nproc)

# Build Release (si está configurado)
make -C Release clean
make -C Release all -j$(nproc)

# El binario se generará en:
# Debug/leather_gauge_controller.elf
# Debug/leather_gauge_controller.hex
# Debug/leather_gauge_controller.bin
```

### Configuraciones de Build

| Configuración | Optimización | Tamaño  | Uso                    |
| ------------- | ------------ | ------- | ---------------------- |
| **Debug**     | -Og          | ~150 KB | Desarrollo, depuración |
| **Release**   | -O2 / -Os    | ~100 KB | Producción             |

---

## Flasheo y Programación

### ST-LINK

```bash
# Usando ST-LINK CLI
st-flash write Debug/leather_gauge_controller.bin 0x08000000

# O usando OpenOCD
openocd -f interface/stlink.cfg -f target/stm32f4x.cfg \
  -c "program Debug/leather_gauge_controller.elf verify reset exit"
```

### Desde STM32CubeIDE

1. Conectar ST-LINK al target
2. Click derecho en el proyecto → `Run As` → `STM32 C/C++ Application`
3. O usar `Run` → `Debug` (F11) para depuración

---

## Configuración Hardware

### Pinout Principal

#### Entradas Digitales

| Pin | Función  | Descripción                    |
| --- | -------- | ------------------------------ |
| PA0 | DI_0_INT | Encoder pulse (EXTI interrupt) |
| PC0 | DI_2     | Botón START/STOP               |
| PC1 | DI_3     | Botón GUARD (protección)       |
| PC2 | DI_4     | Botón SPEEDS (velocidades)     |
| PC3 | DI_5     | Botón FEEDBACK                 |

#### Salidas Digitales

| Pin  | Función      | Descripción                       |
| ---- | ------------ | --------------------------------- |
| PB0  | DO_0         | LED Running 1                     |
| PB1  | DO_1         | LED Running 2                     |
| PB3  | DO_2         | LED Running 3 (invertido)         |
| PB9  | DO_6         | LED Speed Low                     |
| PB15 | DO_7         | LED Speed High                    |
| PC13 | DIR_DISPLAY  | RS-485 direction control (DWIN)   |
| PB14 | DIR_SENSORES | RS-485 direction control (Modbus) |

#### Comunicación

| Periférico | Pines     | Función               | Baudrate   |
| ---------- | --------- | --------------------- | ---------- |
| USART3     | PB10/PB11 | Modbus RTU (sensores) | 9600 bps   |
| USART6     | PC6/PC7   | DWIN display          | 115200 bps |
| UART (TBD) | -         | Printer ESC/POS       | 9600 bps   |
| I2C1       | PB8/PB9   | EEPROM AT24Cxx        | 100 kHz    |

### Configuración Modbus

| Sensor ID | Dirección | Registro  | Función                                    |
| --------- | --------- | --------- | ------------------------------------------ |
| 1-11      | 0x01-0x0B | 0x2D (45) | Lectura 10 fotocélulas (holding registers) |

---

## Uso y Operación

### Estados del Sistema

```
┌──────────┐  START Button   ┌────────────┐  Encoder   ┌──────────┐
│ LGC_STOP │ ───────────────→ │ LGC_RUNNING│ ─────────→ │ Measuring│
└──────────┘                  └────────────┘            └──────────┘
     ↑                              │                          │
     │                              │ GUARD / FAIL             │
     │                              ↓                          │
     │                        ┌──────────┐                     │
     └────────────────────────│ LGC_FAIL │←────────────────────┘
                              └──────────┘
```

### Operación Normal

1. **Encendido**: Sistema inicia en estado `LGC_STOP`
2. **Iniciar**: Presionar botón START → estado `LGC_RUNNING`
3. **Medición Automática**:
   - Cada pulso del encoder lee sensores
   - Acumula área de la pieza actual
   - Detecta fin de pieza (3 pasos vacíos)
   - Registra medición individual
4. **Visualización**: Display muestra:
   - Cantidad de piezas (`leather_count`)
   - Área de pieza actual (`current_leather_area`)
   - Área total del lote (`batch_area`)
   - Número de lote (`batch_count`)
5. **Impresión**: Al completar lote, imprime reporte automáticamente
6. **Detener**: Presionar STOP o activar GUARD → estado `LGC_STOP`

### Capacidades

| Parámetro             | Límite                       |
| --------------------- | ---------------------------- |
| Piezas por lote       | 300                          |
| Lotes totales         | 200                          |
| Resolución espacial   | 10mm × 5mm (pixel × encoder) |
| Ancho máximo medición | 1100mm (110 fotocélulas)     |

---

## Configuración y Personalización

### Parámetros Configurables

Editar constantes en los archivos fuente:

```c
// lgc_main_task.c
#define ENCODER_DISTANCE_MM     5      // Distancia por pulso encoder
#define SENSOR_PIXEL_WIDTH_MM   10     // Ancho de cada fotocélula
#define HYSTERESIS_STEPS        3      // Pasos vacíos para fin de pieza
#define MAX_LEATHERS            300    // Piezas máximas por lote
#define MAX_BATCHES             200    // Lotes máximos

// lgc_inteface_modbus.c
#define MODBUS_TIMEOUT_MS       100    // Timeout lectura Modbus
#define MODBUS_RETRIES          4      // Reintentos en falla
```

### Cambio de RTOS

El proyecto soporta 14 diferentes RTOS mediante OSAL. Para cambiar:

1. Editar `os_port_config.h`:

   ```c
   // Descomentar el RTOS deseado
   #define USE_THREADX           // ThreadX (actual)
   // #define USE_FREERTOS       // FreeRTOS
   // #define USE_CMSIS_RTOS     // CMSIS-RTOS
   // ... etc
   ```

2. Recompilar proyecto

RTOS soportados: ThreadX, FreeRTOS, µC/OS-II, µC/OS-III, CMSIS-RTOS, CMSIS-RTOS2, RTX, SafeRTOS, Zephyr, ChibiOS, embOS, PX5, Windows, POSIX, None.

---

## Guía de Pruebas HMI (DWIN Display)

### Prerequisitos

1. Display DWIN conectado a USART6 (PC6/PC7)
2. Control de dirección RS-485 en PC13 (DIR_DISPLAY)
3. Firmware cargado en el STM32
4. ST-LINK conectado para depuración

### Direcciones VP Principales

Las direcciones VP están centralizadas en `lgc_hmi.h`:

| Variable                             | Dirección VP | Descripción            |
| ------------------------------------ | ------------ | ---------------------- |
| `LGC_HMI_VP_STATE`                   | 0x1110       | Estado del sistema     |
| `LGC_HMI_VP_ICON_SPEEP`              | 0x1111       | Indicador de velocidad |
| `LGC_HMI_VP_FEEDBACK_MOTOR`          | 0x1112       | Feedback motor ON/OFF  |
| `LGC_HMI_VP_BATCH_COUNT`             | 0x1050       | Contador de lotes      |
| `LGC_HMI_VP_LEATHER_COUNT`           | 0x1051       | Contador de cueros     |
| `LGC_HMI_VP_CURRENT_LEATHER_AREA`    | 0x1060       | Área actual (×100)     |
| `LGC_HMI_VP_ACUMULATED_LEATHER_AREA` | 0x1080       | Área acumulada lote    |
| `LGC_HMI_VP_CONFIG_DAY`              | 0x1341       | Configuración día      |
| `LGC_HMI_VP_CONFIG_MONTH`            | 0x1342       | Configuración mes      |
| `LGC_HMI_VP_CONFIG_YEAR`             | 0x1343       | Configuración año      |
| `LGC_HMI_VP_CONFIG_HOUR`             | 0x1346       | Configuración hora     |
| `LGC_HMI_VP_CONFIG_MINUTE`           | 0x1347       | Configuración minuto   |
| `LGC_HMI_VP_CONFIG_SECOND`           | 0x1348       | Configuración segundo  |

### Casos de Prueba

#### Prueba 1: Verificar Escritura de Variables

```c
// Breakpoint en lgc_hmi_update_task_entry()
// Verificar que dwin_var_write() se ejecute correctamente

// Pasos:
1. Colocar breakpoint en lgc_hmi_task.c línea de dwin_var_write()
2. Verificar que LGC_HMI_UPDATE_REQUIRED event se dispare
3. Confirmar que los valores escritos coincidan con measurements
4. Observar display para cambio visual
```

#### Prueba 2: Validar Contadores en Pantalla

```
1. Estado inicial: Verificar leather_count = 0, batch_count = 0
2. Simular medición:
   - Generar pulsos de encoder (manualmente o con generador)
   - Activar sensores simulados
3. Verificar incremento en:
   - LGC_HMI_VP_LEATHER_COUNT (0x1051)
   - LGC_HMI_VP_CURRENT_LEATHER_AREA (0x1060)
4. Completar pieza (histéresis de 3 pasos vacíos)
5. Verificar nuevo leather_count
```

#### Prueba 3: Validar Fecha/Hora (RTC → HMI)

```c
// Verificar lectura de RTC y escritura a display

1. Establecer fecha/hora con lgc_module_rtc_set()
2. Verificar escritura a VPs:
   - LGC_HMI_VP_CONFIG_DAY (0x1341)
   - LGC_HMI_VP_CONFIG_MONTH (0x1342)
   - LGC_HMI_VP_CONFIG_YEAR (0x1343)
   - LGC_HMI_VP_CONFIG_HOUR (0x1346)
   - LGC_HMI_VP_CONFIG_MINUTE (0x1347)
   - LGC_HMI_VP_CONFIG_SECOND (0x1348)
3. Confirmar visualización correcta en pantalla DWIN
```

#### Prueba 4: Respuesta a Botones de Usuario

```
1. Presionar START/STOP:
   - Verificar cambio de estado LGC_STOP ↔ LGC_RUNNING
   - Observar LEDs de estado
   - Confirmar actualización en display

2. Activar GUARD:
   - Verificar transición a LGC_FAIL
   - Confirmar indicador visual en HMI

3. Cambiar velocidad (SPEEDS):
   - Verificar LGC_HMI_VP_ICON_SPEEP actualiza
```

#### Prueba 5: Guardar Configuración

```
1. Modificar parámetros en pantalla de configuración
2. Presionar botón guardar (VP 0x1002)
3. Verificar:
   - LGC_HMI_VP_CONFIG_SAVE_CMD recibe comando
   - EEPROM escribe datos
   - LGC_HMI_VP_CONFIG_SAVE_RESULT muestra resultado (1=OK, 2=FAIL)
```

### Herramientas de Depuración

1. **STM32CubeIDE Debugger**: Breakpoints en funciones DWIN
2. **Logic Analyzer**: Capturar tráfico UART DWIN (115200 bps)
3. **DWIN Debugger Software**: Validar protocolo directamente
4. **Live Expressions**: Monitorear variables `measurements` y `state_data`

### Checklist de Validación HMI

- [ ] Display enciende y muestra página inicial
- [ ] Navegación entre páginas funciona
- [ ] Contador de cueros actualiza en tiempo real
- [ ] Contador de lotes incrementa al completar batch
- [ ] Área actual muestra valor correcto (×100 para resolución)
- [ ] Fecha/hora RTC sincronizada con display
- [ ] Botones físicos responden correctamente
- [ ] Estados visuales (LEDs, iconos) actualizan
- [ ] Configuración guarda en EEPROM y persiste tras reset

---

## 📚 Recursos y Documentación

### Documentación del Proyecto

| Documento                                                          | Descripción                                                      |
| ------------------------------------------------------------------ | ---------------------------------------------------------------- |
| [REFACTOR_PLAN.md](REFACTOR_PLAN.md)                               | **Plan Maestro de Refactorización** (Clean Architecture + SOLID) |
| [docs/SYSTEM_ARCHITECTURE.md](docs/SYSTEM_ARCHITECTURE.md)         | Arquitectura completa del sistema (legacy + nueva)               |
| [docs/sensor/README.md](docs/sensor/README.md)                     | Documentación sensores fotoeléctricos (protocolo Modbus/LwPKT)   |
| [.github/copilot-instructions.md](.github/copilot-instructions.md) | Estándares de código, TDD, SOLID (para agentes IA)               |
| `docs/Lista de variables.xlsx`                                     | Lista detallada de variables VP (DWIN)                           |

### Bibliotecas y Middlewares

| Biblioteca         | Versión | Propósito                                      | Repositorio                                                   |
| ------------------ | ------- | ---------------------------------------------- | ------------------------------------------------------------- |
| **nanoMODBUS**     | 0.2.0   | Cliente Modbus RTU                             | [GitHub](https://github.com/debevv/nanoMODBUS)                |
| **lwrb**           | 3.2.0   | Ring buffers (UART DMA)                        | [GitHub](https://github.com/MaJerle/lwrb)                     |
| **lwprintf**       | 2.0.0   | Lightweight printf                             | [GitHub](https://github.com/MaJerle/lwprintf)                 |
| **lwbtn**          | 1.3.0   | Button handling (debounce)                     | [GitHub](https://github.com/MaJerle/lwbtn)                    |
| **at24cxx**        | Custom  | EEPROM I2C driver                              | Local middleware                                              |
| **dwin**           | Custom  | DWIN display UART driver                       | Local middleware                                              |
| **Azure ThreadX**  | 6.x     | RTOS preemptivo de tiempo real                 | [Eclipse ThreadX](https://github.com/eclipse-threadx/threadx) |
| **LwPKT** (futuro) | 1.5.1   | Lightweight Packet Protocol (migración futura) | [GitHub](https://github.com/MaJerle/lwpkt)                    |

### Recursos STM32

- [STM32F446RC Datasheet](https://www.st.com/resource/en/datasheet/stm32f446rc.pdf)
- [STM32F4 Reference Manual (RM0390)](https://www.st.com/resource/en/reference_manual/rm0390-stm32f446xx-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)
- [STM32CubeIDE User Guide](https://www.st.com/resource/en/user_manual/um2609-stm32cubeide-user-guide-stmicroelectronics.pdf)
- [ThreadX User Guide](https://github.com/eclipse-threadx/rtos-docs/blob/main/rtos-docs/threadx/about-this-guide.md)

### Recursos de Clean Architecture

- **Libro:** _Clean Architecture_ - Robert C. Martin (Uncle Bob)
- **Libro:** _Clean Code_ - Robert C. Martin
- **Embedded:** [Applying Clean Architecture to Embedded Systems](https://blog.cleancoder.com/uncle-bob/2011/11/22/Clean-Architecture.html)
- **C en Embedded:** [SOLID principles for embedded C](https://interrupt.memfault.com/blog/unit-test-mocking)

### Herramientas de Desarrollo

| Herramienta         | Propósito                                       | Instalación                                                              |
| ------------------- | ----------------------------------------------- | ------------------------------------------------------------------------ |
| **STM32CubeIDE**    | IDE principal, compilación, debug               | [Descargar](https://www.st.com/en/development-tools/stm32cubeide.html)   |
| **STM32CubeMX**     | Configuración periféricos HAL (incluido en IDE) | Incluido en STM32CubeIDE                                                 |
| **ST-LINK Utility** | Programación y lectura de firmware              | [Descargar](https://www.st.com/en/development-tools/stsw-link004.html)   |
| **OpenOCD**         | Alternativa de programación/debug (open source) | `apt install openocd` / [GitHub](https://github.com/openocd-org/openocd) |
| **Logic Analyzer**  | Captura de señales UART, I2C (Saleae, etc.)     | Hardware externo                                                         |
| **Unity + CMock**   | Unit testing framework para C (futuro)          | [GitHub Unity](https://github.com/ThrowTheSwitch/Unity)                  |
| **Cppcheck**        | Static code analysis                            | `apt install cppcheck`                                                   |
| **Doxygen**         | Generación documentación API                    | `apt install doxygen graphviz`                                           |

---

## 🤝 Contribución y Desarrollo

### Flujo de Trabajo (Git Flow)

```
main (production)
  ↑
  └── develop (integration)
       ↑
       ├── feat/clean-arch-sensor-reader
       ├── feat/clean-arch-encoder-adapter
       ├── feat/clean-arch-measure-use-case
       └── fix/hmi-display-timeout
```

### Estándares de Código

**TODOS los desarrolladores y agentes IA DEBEN seguir:**

- ✅ **Guía principal:** [.github/copilot-instructions.md](.github/copilot-instructions.md)
- ✅ **Nomenclatura:** Ver tabla en REFACTOR_PLAN.md (section 6)
- ✅ **Documentación:** Doxygen obligatorio para funciones públicas
- ✅ **Testing:** TDD preferido (Red → Green → Refactor)
- ✅ **Commits:** Conventional Commits (`feat:`, `fix:`, `refactor:`, `docs:`)

#### Ejemplo de Commit

```bash
git commit -m "feat(adapter): implement ISensorReader for Modbus RTU

- Create lgc_modbus_adapter.c with ISensorReader vtable
- Add read_all_sensors() implementation
- Add cascade mode support (future LwPKT migration)
- Unit tests: mock UART HAL (coverage 85%)

Refs: #REFACTOR_PLAN.md Phase 2.2"
```

### Compilación para Testing

```bash
# Debug build (con symbols para GDB)
make -C Debug clean all VERBOSE=1

# Analizar tamaño de secciones
arm-none-eabi-size -A Debug/leather_gauge_controller.elf

# Verificar stack usage (requiere -fstack-usage)
find Debug -name "*.su" -exec cat {} \;

# Generar documentación Doxygen
doxygen Doxyfile  # (cuando esté creado)
```

---

## 📊 Progreso de Refactorización

### Roadmap (12 semanas)

| Fase                  | Semanas | Progreso | ETA        |
| --------------------- | ------- | :------: | ---------- |
| **Fundaciones**       | 1-2     | ✅ 100%  | Completado |
| **Adapters Críticos** | 3-5     |  🔄 40%  | Marzo 2026 |
| **Use Cases**         | 6-8     |  ⏳ 0%   | Abril 2026 |
| **Peripherals**       | 9-10    |  ⏳ 0%   | Mayo 2026  |
| **Testing & Docs**    | 11-12   |  ⏳ 0%   | Junio 2026 |

### Métricas Actuales vs Target

| Métrica                          | Actual | Target | Estado |
| -------------------------------- | :----: | :----: | :----: |
| Desacoplamiento HAL (%)          |  30%   |  100%  |   🔄   |
| Testabilidad Logic Negocio (%)   |   0%   |  90%   |   ⏳   |
| Inversión Dependencias (N/Total) |  5/15  | 15/15  |   🔄   |
| Cobertura Documentación (%)      |  45%   |  100%  |   🔄   |
| Complejidad Ciclomática (avg)    |   ~4   |  <10   |   ✅   |

---

## 📞 Contacto y Soporte

**Desarrollador Principal:** Tecna Smart Lab Engineering Team  
**Arquiteto Senior de Firmware:** [TBD]  
**Repositorio:** [GitHub - leather_gauge](https://github.com/dowhile98/leather_gauge)

Para reportar bugs, sugerencias o consultas técnicas:

📧 **Email:** soporte@tecnasmart.com  
🐛 **Issues:** [GitHub Issues](https://github.com/dowhile98/leather_gauge/issues)  
📝 **Documentación:** Ver carpeta `docs/` en el proyecto

---

## 📄 Licencia

**Propiedad de Tecna Smart Lab. Todos los derechos reservados.**

Este firmware es propietario y confidencial. No está permitida su distribución, copia, modificación o uso sin autorización expresa de Tecna Smart Lab.

---

## 🎯 Siguientes Pasos Inmediatos

1. [ ] **Completar Modbus Adapter** (ISensorReader implementation)
2. [ ] **Refactorizar EEPROM Adapter** (IStorage + CRC32)
3. [ ] **Extraer Use Case: Measure Area** (algoritmo de slices)
4. [ ] **Setup Unit Testing** (Unity + Mocks en PC)
5. [ ] **Documentar Interfaces** (Doxygen completo para `interfaces/`)

**Ver detalles completos en:** [REFACTOR_PLAN.md](REFACTOR_PLAN.md)

---

**Última actualización:** 11 de Febrero de 2026  
**Versión Firmware:** v2.0 (Clean Architecture en progreso - 40%)  
**Estado:** 🚧 Refactorización activa hacia Clean Architecture + SOLID

- [ ] Fecha/hora se muestra desde RTC
- [ ] Botones tactiles responden
- [ ] Configuración se guarda en EEPROM
- [ ] Indicadores de estado (velocidad, motor) actualizan

---

## Depuración

### Mensajes de Log

El sistema utiliza `stm32_log` para depuración:

```c
// Habilitar logs en stm32_log_config.h
#define STM32_LOG_ENABLE        1
#define STM32_LOG_LEVEL         LOG_DEBUG

// Ejemplo de logs en código
LOG_INFO("System started");
LOG_DEBUG("Encoder pulse: %d", count);
LOG_ERROR("Modbus timeout on sensor %d", sensor_id);
```

### Breakpoints Útiles

| Función                        | Archivo               | Propósito                        |
| ------------------------------ | --------------------- | -------------------------------- |
| `lgc_encoder_callback`         | lgc_main_task.c       | Verificar interrupciones encoder |
| `lgc_modbus_read_holding_regs` | lgc_inteface_modbus.c | Depurar comunicación Modbus      |
| `lgc_main_task_entry`          | lgc_main_task.c       | Lógica principal de medición     |
| `lgc_hmi_task`                 | lgc_hmi_task.c        | Depurar interfaz HMI             |

### Monitoreo en Tiempo Real

Usar **STM32CubeMonitor** o **SWV (Serial Wire Viewer)** para:

- Profiling de CPU
- Uso de stack por tarea
- Variables en tiempo real

---

## Documentación Adicional

### Documentos Técnicos

- **[SYSTEM_ARCHITECTURE.md](docs/SYSTEM_ARCHITECTURE.md)**: Documentación completa de arquitectura (989 líneas)
  - Tareas RTOS
  - Algoritmo de medición
  - Mapas de hardware
  - Protocolos de comunicación
  - Diagramas de flujo

- **Lista de variables.xlsx**: Configuración de variables DWIN

### Referencias Externas

- [STM32F446 Reference Manual](https://www.st.com/resource/en/reference_manual/dm00135183-stm32f446xx-advanced-arm-based-32-bit-mcus-stmicroelectronics.pdf)
- [Azure ThreadX Documentation](https://github.com/eclipse-threadx/rtos-docs)
- [nanoMODBUS Library](https://github.com/debevv/nanoMODBUS)
- [DWIN Display Protocol](https://www.dwin-global.com/)

---

## Solución de Problemas

### Errores Comunes

#### Error: "Modbus timeout"

```
Causa: Sensor Modbus no responde
Solución:
- Verificar conexión RS-485
- Comprobar dirección de sensor (1-11)
- Verificar baudrate (9600 bps)
- Revisar control de dirección (DIR_SENSORES)
```

#### Error: "No encoder pulses"

```
Causa: Encoder no está generando interrupciones
Solución:
- Verificar conexión PA0 (DI_0_INT)
- Comprobar EXTI configurado correctamente
- Verificar que el encoder esté alimentado
- Testear con osciloscopio
```

#### Error: "DWIN display no actualiza"

```
Causa: Comunicación UART con display fallando
Solución:
- Verificar baudrate (115200 bps)
- Comprobar pines PC6/PC7 (TX/RX)
- Revisar control de dirección (DIR_DISPLAY)
- Verificar protocolo DWIN
```

#### Error: "Build failed: undefined reference"

```
Causa: Bibliotecas o archivos faltantes
Solución:
- Verificar que middlewares estén enlazados correctamente
- Revisar .project para linkedResources
- Limpiar y reconstruir: make clean && make all
```

---

## Contribución

### Guidelines

1. **Estilo de código**: Seguir convenciones existentes
   - Prefijo `lgc_` para funciones públicas
   - Snake_case para funciones y variables
   - PascalCase para tipos `typedef struct`

2. **Commits**: Mensajes descriptivos

   ```bash
   git commit -m "fix: correct Modbus timeout handling"
   git commit -m "feat: add EEPROM configuration storage"
   ```

3. **Testing**: Verificar cambios en hardware real antes de commit

### Workflow

```bash
# Crear branch para feature
git checkout -b feature/nueva-funcionalidad

# Hacer cambios y commits
git add .
git commit -m "descripción clara"

# Push y crear pull request
git push origin feature/nueva-funcionalidad
```

---

## Roadmap

### Features Planificados

- [ ] Tests unitarios (Unity framework)
- [ ] CI/CD pipeline (GitHub Actions)
- [ ] Logging persistente en EEPROM
- [ ] Watchdog timer para auto-recuperación
- [ ] Calibración configurable desde HMI
- [ ] Modo diagnóstico de sensores
- [ ] Comunicación Ethernet/WiFi para monitoreo remoto
- [ ] API REST para integración con ERP
- [ ] Dashboard web en tiempo real

---

## Licencia

Propietario - Todos los derechos reservados.

Este código es propiedad privada y no debe ser distribuido sin autorización.

---

## Contacto y Soporte

Para consultas técnicas o soporte, contactar al equipo de desarrollo.

---

## Historial de Versiones

| Versión | Fecha | Cambios |
|---------|-------|---------|| 1.1.0 | 2026-01-20 | Módulo RTC con mutex thread-safe |
| - | - | Centralización de direcciones VP en enum |
| - | - | Corrección índice batch_measurement |
| - | - | Guía de pruebas HMI || 1.0.0 | 2026-01-16 | Versión inicial estable |
| - | - | Sistema de medición funcional |
| - | - | Integración Modbus + DWIN + Printer |
| - | - | Documentación completa |

---

**Desarrollado con STM32CubeIDE y Azure ThreadX**
