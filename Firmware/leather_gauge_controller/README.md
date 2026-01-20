# Leather Gauge Controller

[![STM32](https://img.shields.io/badge/STM32-F446RC-blue.svg)](https://www.st.com/en/microcontrollers-microprocessors/stm32f446rc.html)
[![RTOS](https://img.shields.io/badge/RTOS-Azure_ThreadX-green.svg)](https://github.com/eclipse-threadx/threadx)
[![License](https://img.shields.io/badge/license-Proprietary-red.svg)]()

Sistema embebido industrial para medición automática del área de piezas de cuero en movimiento continuo mediante sensores fotoeléctricos sincronizados con encoder rotativo.

---

## Descripción General

El **Leather Gauge Controller** es un firmware profesional basado en STM32F446RCTx que implementa un sistema de medición de alta precisión para cuero en procesos industriales. El sistema mide automáticamente el área de cada pieza de cuero mientras se desplaza por una banda transportadora, utilizando un arreglo de 11 sensores Modbus con 110 fotocélulas totales.

### Características Principales

- **Medición de precisión**: Algoritmo de integración por "slices" sincronizado con encoder
- **Sistema multi-tarea**: Azure ThreadX RTOS para operación concurrente
- **Comunicación Modbus RTU**: Lectura de 11 sensores distribuidos
- **Interfaz HMI**: Display DWIN para visualización en tiempo real
- **Impresión de reportes**: Impresora térmica ESC/POS
- **Almacenamiento persistente**: EEPROM I2C para configuración
- **Arquitectura robusta**: Thread-safe con mutexes y semáforos
- **Abstracción de hardware**: Capa modular para fácil portabilidad

---

## Especificaciones Técnicas

### Hardware

| Componente | Especificación |
|------------|----------------|
| **MCU** | STM32F446RCTx (ARM Cortex-M4F) |
| **Flash** | 256 KB |
| **RAM** | 128 KB |
| **Frecuencia** | 180 MHz |
| **FPU** | FPv4-SP-D16 (hardware single-precision) |
| **Sensores** | 11 módulos Modbus RTU (110 fotocélulas) |
| **Display** | DWIN LCD UART |
| **Encoder** | Rotativo incremental (EXTI) |
| **Almacenamiento** | AT24Cxx EEPROM I2C |
| **Impresora** | Térmica ESC/POS (UART) |

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

## Estructura del Proyecto

```
leather_gauge_controller/
├── Core/                                  # STM32 HAL initialization
│   ├── Inc/                               # Hardware headers
│   ├── Src/                               # main.c, interrupts, syscalls
│   └── Startup/                           # Startup assembly code
│
├── leather_gauge_controller/              # Application code
│   ├── app/                               # Main application logic
│   │   ├── inc/                           # Application headers
│   │   └── src/                           # Application implementation
│   │       ├── hmi/                       # HMI task (DWIN display)
│   │       ├── printer/                   # Printer task
│   │       └── sensor/                    # Sensor processing
│   │
│   ├── modules/                           # Hardware abstraction modules
│   │   ├── di/                            # Digital inputs (buttons)
│   │   ├── encoder/                       # Rotary encoder interface
│   │   ├── modbus/                        # Modbus RTU client
│   │   ├── eeprom/                        # EEPROM storage
│   │   └── printer/                       # ESC/POS printer driver
│   │
│   ├── osal/                              # OS Abstraction Layer
│   │   ├── include/                       # OSAL API headers
│   │   ├── common/                        # Common utilities
│   │   └── portable/                      # RTOS ports (14 RTOS)
│   │
│   ├── config/                            # Configuration files
│   └── middlewares/                       # External libraries (linked)
│
├── Middlewares/                           # ST middlewares
│   ├── ST/threadx/                        # Azure ThreadX RTOS
│   └── ST/usbx/                           # Azure USBX stack
│
├── Drivers/                               # STM32 drivers
│   ├── CMSIS/                             # ARM CMSIS
│   └── STM32F4xx_HAL_Driver/              # STM32 HAL
│
├── docs/                                  # Documentation
│   ├── SYSTEM_ARCHITECTURE.md             # Comprehensive architecture doc
│   └── Lista de variables.xlsx            # Variable list
│
├── Debug/                                 # Build artifacts
├── STM32F446RCTX_FLASH.ld                 # Linker script
├── leather_gauge_controller.ioc           # STM32CubeMX project
└── README.md                              # This file
```

---

## Arquitectura del Sistema

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

| Tarea | Prioridad | Stack | Función |
|-------|-----------|-------|---------|
| **Main Task** | 10 | 256 words | Medición, encoder, sensores Modbus |
| **HMI Task** | 11 | 512 words | Actualización display DWIN |
| **HMI Update** | 12 | 256 words | Procesamiento eventos UI |
| **DWIN Process** | 13 | 512 words | Protocolo DWIN |
| **Printer Task** | 14 | 512 words | Impresión reportes |

### Algoritmo de Medición

El sistema implementa un **algoritmo de integración por "slices"** (rebanadas):

1. **Trigger**: Cada pulso del encoder (5mm de desplazamiento)
2. **Lectura**: 11 sensores Modbus (110 bits totales)
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

| Configuración | Optimización | Tamaño | Uso |
|---------------|--------------|---------|-----|
| **Debug** | -Og | ~150 KB | Desarrollo, depuración |
| **Release** | -O2 / -Os | ~100 KB | Producción |

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
| Pin | Función | Descripción |
|-----|---------|-------------|
| PA0 | DI_0_INT | Encoder pulse (EXTI interrupt) |
| PC0 | DI_2 | Botón START/STOP |
| PC1 | DI_3 | Botón GUARD (protección) |
| PC2 | DI_4 | Botón SPEEDS (velocidades) |
| PC3 | DI_5 | Botón FEEDBACK |

#### Salidas Digitales
| Pin | Función | Descripción |
|-----|---------|-------------|
| PB0 | DO_0 | LED Running 1 |
| PB1 | DO_1 | LED Running 2 |
| PB3 | DO_2 | LED Running 3 (invertido) |
| PB9 | DO_6 | LED Speed Low |
| PB15 | DO_7 | LED Speed High |
| PC13 | DIR_DISPLAY | RS-485 direction control (DWIN) |
| PB14 | DIR_SENSORES | RS-485 direction control (Modbus) |

#### Comunicación
| Periférico | Pines | Función | Baudrate |
|------------|-------|---------|----------|
| USART3 | PB10/PB11 | Modbus RTU (sensores) | 9600 bps |
| USART6 | PC6/PC7 | DWIN display | 115200 bps |
| UART (TBD) | - | Printer ESC/POS | 9600 bps |
| I2C1 | PB8/PB9 | EEPROM AT24Cxx | 100 kHz |

### Configuración Modbus

| Sensor ID | Dirección | Registro | Función |
|-----------|-----------|----------|---------|
| 1-11 | 0x01-0x0B | 0x2D (45) | Lectura 10 fotocélulas (holding registers) |

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

| Parámetro | Límite |
|-----------|--------|
| Piezas por lote | 300 |
| Lotes totales | 200 |
| Resolución espacial | 10mm × 5mm (pixel × encoder) |
| Ancho máximo medición | 1100mm (110 fotocélulas) |

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

| Función | Archivo | Propósito |
|---------|---------|-----------|
| `lgc_encoder_callback` | lgc_main_task.c | Verificar interrupciones encoder |
| `lgc_modbus_read_holding_regs` | lgc_inteface_modbus.c | Depurar comunicación Modbus |
| `lgc_main_task_entry` | lgc_main_task.c | Lógica principal de medición |
| `lgc_hmi_task` | lgc_hmi_task.c | Depurar interfaz HMI |

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
|---------|-------|---------|
| 1.0.0 | 2026-01-16 | Versión inicial estable |
| - | - | Sistema de medición funcional |
| - | - | Integración Modbus + DWIN + Printer |
| - | - | Documentación completa |

---

**Desarrollado con STM32CubeIDE y Azure ThreadX**

