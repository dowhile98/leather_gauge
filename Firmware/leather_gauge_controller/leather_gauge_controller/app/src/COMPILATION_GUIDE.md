# Guía de Compilación y Deployment

## Prerequisitos

```bash
# Verificar instalación de herramientas
arm-none-eabi-gcc --version
arm-none-eabi-gdb --version
make --version
st-flash --version  # Si usas ST-Link

# O si usas otro programador:
# openocd --version
```

---

## Paso 1: Preparación del Código

### 1.1 Verificar Valores de Macros

Antes de compilar, **DEBE ajustar los siguientes valores** según su hardware:

```c
// En lgc_main_task.c, líneas 30-45

/* Pixel width in mm - AJUSTAR SEGÚN ANCHO REAL DEL FOTORECEPTOR */
#define LGC_PIXEL_WIDTH_MM 10.0f    // ← VERIFICAR CON HARDWARE

/* Encoder step distance in mm - AJUSTAR SEGÚN DISTANCIA REAL */
#define LGC_ENCODER_STEP_MM 5.0f    // ← VERIFICAR CON HARDWARE

/* Hysteresis for leather detection - PUEDE AUMENTAR SI HAY RUIDO */
#define LGC_LEATHER_END_HYSTERESIS 3 // ← AJUSTAR SEGÚN NECESIDAD
```

**Método 1: Editar archivo directamente**

```bash
# Abrir en editor
nano leather_gauge_controller/app/src/lgc_main_task.c
# Buscar y cambiar línea 30-45
```

**Método 2: Sobrescribir en compilación (no recomendado)**

```bash
# Ver Paso 3.1
```

### 1.2 Verificar Dirección de Registro Modbus

En `lgc_main_task.c`, línea ~113:

```c
err = lgc_modbus_read_holding_regs(i + 1, 45, &data.sensor[i], 1);
                                          ↑↑
                                   Dirección (verificar)
```

Si tus sensores están en otra dirección, cambiar `45` por la correcta.

---

## Paso 2: Limpiar Build Anterior

```bash
# Posicionarse en raíz del proyecto
cd leather_gauge_controller

# Limpiar compilación anterior
make clean

# Opcional: limpiar compilación profunda
make distclean
```

---

## Paso 3: Compilar

### 3.1 Compilación Estándar

```bash
# Compilación simple
make

# Con output detallado
make VERBOSE=1

# Con jobs paralelos (más rápido)
make -j4
```

### 3.2 Compilación con Macros Sobrescritas (Avanzado)

```bash
# Cambiar valores sin editar archivo
make CFLAGS="-D LGC_PIXEL_WIDTH_MM=12.5f -D LGC_ENCODER_STEP_MM=3.0f"

# Combinado
make clean && make -j4 CFLAGS="-D LGC_LEATHER_END_HYSTERESIS=5"
```

### 3.3 Verificación de Compilación

**Sin errores - Esperado**:

```
Linking target...
text    data     bss     dec     hex filename
12345   1234     5678   19257   4b39 ./build/leather_gauge_controller.elf
```

**Con advertencias - Normal**:

```
warning: unused variable 'xxx' [-Wunused-variable]
```

**Con errores - PROBLEMA**:

```
error: undefined reference to 'function_name'
error: struct "XXX" has no field "YYY"
```

Si hay errores, verificar:

1. ¿Está compilando el archivo `lgc_main_task.c` correcto?
2. ¿Existen todas las dependencias (headers)?
3. ¿Está la estructura `LGC_CONF_TypeDef_t` definida?

---

## Paso 4: Generar Imagen

```bash
# Generar BIN (más pequeño)
arm-none-eabi-objcopy -O binary ./build/leather_gauge_controller.elf leather_gauge_controller.bin

# Generar HEX (para algunos programadores)
arm-none-eabi-objcopy -O ihex ./build/leather_gauge_controller.elf leather_gauge_controller.hex

# Generar LIST (para análisis)
arm-none-eabi-objdump -D ./build/leather_gauge_controller.elf > leather_gauge_controller.list
```

---

## Paso 5: Flashear en Hardware

### 5.1 Usando ST-Link (UART + ST-Link V2)

```bash
# Limpiar memoria
st-flash erase

# Flashear binario
st-flash write leather_gauge_controller.bin 0x08000000

# Reset
st-flash reset
```

### 5.2 Usando OpenOCD

```bash
# Iniciar OpenOCD (en otra terminal)
openocd -f interface/stlink.cfg -f target/stm32f4x.cfg

# En otra terminal, conectar con GDB
arm-none-eabi-gdb ./build/leather_gauge_controller.elf

# En GDB:
(gdb) target remote localhost:3333
(gdb) load
(gdb) reset
(gdb) quit
```

### 5.3 Usando CubeProgrammer (GUI)

```bash
# Abrir interfaz gráfica
STM32CubeProgrammer &

# 1. Conectar al STM32
# 2. Abrir archivo: leather_gauge_controller.bin
# 3. Dirección: 0x08000000
# 4. Click en "Download"
```

---

## Paso 6: Verificar Funcionamiento

### 6.1 Conexión Serial

```bash
# Configurar puerto serial (típicamente /dev/ttyUSB0 o /dev/ttyACM0)
minicom -b 115200 -D /dev/ttyUSB0

# Alternativa: screen
screen /dev/ttyUSB0 115200

# Alternativa: picocom
picocom -b 115200 /dev/ttyUSB0
```

### 6.2 Verificar Output en Serial

Esperado al iniciar:

```
Initializing Modbus...
Creating encoder semaphore...
Creating mutex...
Encoder initialized
Ready for measurement...
```

### 6.3 Monitoreo de Mediciones

```bash
# Monitorear output en tiempo real
tail -f /tmp/leather_gauge_log.txt

# O con grep para filtrar
tail -f /tmp/leather_gauge_log.txt | grep "Leather\|Batch"
```

---

## Paso 7: Testing Básico

### 7.1 Test 1: Lectura de Sensores

**Acción**: Colocar algo frente a los sensores

**Output esperado**:

```
Sensor 0: 0x03FF (OK)
Sensor 1: 0x01FF (OK)
Sensor 2: 0x00FF (OK)
...
Active bits: 30
```

### 7.2 Test 2: Detección de Cuero

**Acción**: Pasar cuero bajo sensores lentamente

**Output esperado**:

```
Bits: 50, Area: 2500.0 mm²
Measuring: 1, no_detect_count: 0
...
Bits: 0, no_detect_count: 1
Bits: 0, no_detect_count: 2
Bits: 0, no_detect_count: 3 → Leather complete!
Saved leather[0]: 12500.0 mm²
```

### 7.3 Test 3: Múltiples Cueros

**Acción**: Pasar 3-5 cueros bajo sensores

**Output esperado**:

```
Leather[0]: 12500 mm²
Leather[1]: 11250 mm²
Leather[2]: 13750 mm²
Batch[0] complete: 37500 mm² (3 items)
```

---

## Solución de Problemas de Compilación

### Problema: "undefined reference to 'xxx'"

**Causa**: Función no implementada o no vinculada

**Solución**:

```bash
# Verificar que el archivo fuente existe
find . -name "*xxx*" -type f

# Verificar que la función está en el makefile
grep "xxx.c\|xxx.o" Debug/makefile

# Recompilar limpio
make clean && make
```

### Problema: "struct has no field 'xxx'"

**Causa**: Campo de estructura mal nombrado

**Solución**:

```bash
# Verificar definición correcta
grep -r "struct.*TypeDef" --include="*.h"

# En este caso, el campo se llama 'batch' no 'batch_limit'
# ✓ Correcto: config->batch
# ✗ Incorrecto: config->batch_limit
```

### Problema: "conflicting types"

**Causa**: Declaración duplicada o inconsistente de función

**Solución**:

```bash
# Buscar todas las declaraciones
grep -rn "lgc_process_measurement" . --include="*.h" --include="*.c"

# Asegurarse de que solo existe una definición
```

### Problema: Out of Memory o Buffer Overflow

**Causa**: Arrays demasiado grandes para RAM disponible

**Solución**:

```c
// Verificar tamaño de arrays
sizeof(float) * 300 = 1200 bytes (leather_measurement)
sizeof(float) * 200 = 800 bytes (batch_measurement)
Total = 2000 bytes

// STM32F446 tiene 180KB RAM - SUFICIENTE
```

---

## Verificación Final (Checklist)

```
ANTES DE COMPILAR
[ ] Verificar LGC_PIXEL_WIDTH_MM con valor real del hardware
[ ] Verificar LGC_ENCODER_STEP_MM con valor real del hardware
[ ] Verificar dirección de registro Modbus (45)
[ ] Verificar que lgc_main_task.c está actualizado
[ ] Hacer 'make clean'

COMPILACIÓN
[ ] 'make' sin errores
[ ] Warning normales (variables sin usar)
[ ] Binario generado: leather_gauge_controller.bin

FLASHEO
[ ] Hardware STM32 conectado
[ ] Programmer (ST-Link, etc) conectado
[ ] Puerto serial configurado (115200 baud)
[ ] Flasheo completado sin errores

PRUEBA INICIAL
[ ] UART conectado y monitoreado
[ ] Reset ejecutado
[ ] Mensaje de inicialización aparece
[ ] Sensores responden a movimiento

PRUEBA FUNCIONAL
[ ] Lectura de sensores OK
[ ] Detección de cuero OK
[ ] Acumulación de área correcta
[ ] Transición de lote funciona
```

---

## Configuración Recomendada

### Compilación de Producción

```bash
#!/bin/bash
# build_production.sh

set -e  # Exit on error

echo "=== Leather Gauge Production Build ==="

# Limpiar
make clean

# Compilar con optimizaciones
make CFLAGS="-O2 -DNDEBUG" -j4

# Generar imágenes
arm-none-eabi-objcopy -O binary ./build/leather_gauge_controller.elf leather_gauge_controller.bin
arm-none-eabi-objcopy -O ihex ./build/leather_gauge_controller.elf leather_gauge_controller.hex

# Calcular size
echo "=== Final Size ==="
arm-none-eabi-size ./build/leather_gauge_controller.elf

echo "=== Build Complete ==="
ls -lh leather_gauge_controller.*
```

### Compilación de Desarrollo

```bash
#!/bin/bash
# build_debug.sh

set -e

echo "=== Leather Gauge Debug Build ==="

# Limpiar
make clean

# Compilar con debug symbols
make CFLAGS="-g -O0 -DDEBUG" -j4

# No generar .bin (usar .elf directamente con GDB)

echo "=== Build Complete ==="
arm-none-eabi-gdb ./build/leather_gauge_controller.elf
```

---

## Documentación Relacionada

- [QUICK_REFERENCE.md](QUICK_REFERENCE.md) - Referencia rápida de constantes
- [DEBUGGING_GUIDE.md](DEBUGGING_GUIDE.md) - Guía de depuración
- [MEASUREMENT_ALGORITHM.md](MEASUREMENT_ALGORITHM.md) - Algoritmo de medición

---

## Contacto y Soporte

Para problemas de compilación:

1. Revisar logs completos: `make VERBOSE=1`
2. Verificar archivos de cabecera (.h) existen
3. Verificar todos los módulos están compilados
4. Revisar [DEBUGGING_GUIDE.md](DEBUGGING_GUIDE.md)

---

_Guía de Compilación | STM32F446 | ARM Cortex-M4_
_Última actualización: 15 de Enero, 2026_
