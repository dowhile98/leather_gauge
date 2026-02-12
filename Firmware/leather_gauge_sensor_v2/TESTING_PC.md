# Testing desde PC - Leather Gauge Sensor V2

Guía completa para probar los sensores desde una computadora usando conversor RS-485 USB.

## 📋 Requisitos

### Hardware

- **Conversor RS-485 a USB**: Compatible con CH340, FTDI, CP2102, etc.
- **Sensores conectados al bus RS-485**: Hasta 11 sensores (direcciones 1-11)
- **Alimentación**: 12-24VDC para los sensores

### Software

```bash
# Linux/Mac/Windows
pip install pyserial

# Verificar instalación
python -c "import serial; print(serial.__version__)"
```

### Permisos en Linux

Si obtienes error de permisos:

```bash
# Agregar usuario al grupo dialout
sudo usermod -a -G dialout $USER

# O configurar udev rules
sudo chmod 666 /dev/ttyUSB0
```

## 🔌 Conexión del Hardware

```
┌─────────────┐      ┌──────────────┐      ┌──────────┐
│ Computadora │──USB─│ RS-485 USB   │──A/B─│ Sensor 1 │
│             │      │ Converter    │      │ (Addr: 1)│
└─────────────┘      └──────────────┘      └────┬─────┘
                                                 │ A/B
                                          ┌──────┴─────┐
                                          │ Sensor 2   │
                                          │ (Addr: 2)  │
                                          └────┬───────┘
                                               │ A/B
                                          ┌────┴───────┐
                                          │ Sensor 11  │
                                          │ (Addr: 11) │
                                          └────────────┘
```

### Terminación del Bus RS-485

- Agregar resistencia de 120Ω entre A-B **solo** en los extremos del bus
- No colocar terminación en cada sensor intermedio

## 🚀 Uso Básico

### 1. Identificar Puerto Serial

**Linux:**

```bash
# Listar puertos USB
ls /dev/ttyUSB* /dev/ttyACM*

# Ver información detallada
dmesg | grep tty
```

**Windows:**

```powershell
# Device Manager → Ports (COM & LPT)
# O usar modo.com
mode
```

**Mac:**

```bash
ls /dev/cu.usb*
```

### 2. Lectura Individual

Leer sensor específico (modo tradicional):

```bash
# Leer sensor con dirección 1
python main.py --port /dev/ttyUSB0 --mode individual --sensor 1

# Leer sensor 5
python main.py --port COM3 --mode individual --sensor 5
```

**Salida esperada:**

```
🔧 Inicializando LeatherGaugeTester
   Puerto: /dev/ttyUSB0
   Baudrate: 115200
   Timeout: 0.5s
✅ Conectado a /dev/ttyUSB0

============================================================
TEST: LECTURA INDIVIDUAL
============================================================

📊 Leyendo Sensor 1 (Individual)...
📤 TX → Addr:01 CMD:10 FLAGS:00000000 LEN:0
   Bytes: AA FF 01 00 10 00 8C 55
📥 RX ← From:01 CMD:90 FLAGS:00000000 LEN:40
   Data: 00 00 20 41 CD CC 28 41 9A 99 31 41...
✅ Sensor 1: ['10.00', '10.55', '11.10', '10.32', '9.87', '10.44', '10.11', '9.99', '10.66', '10.22']

📈 Valores del Sensor 1:
   Canal 0:   10.000
   Canal 1:   10.550
   Canal 2:   11.100
   ...
```

### 3. Lectura en Cascada (Optimizada)

Leer todos los sensores secuencialmente (modo rápido):

```bash
# Leer 11 sensores en cascada
python main.py --port /dev/ttyUSB0 --mode cascade --sensors 11

# Solo 5 sensores
python main.py --port /dev/ttyUSB0 --mode cascade --sensors 5
```

**Ventaja:** ~500ms para 11 sensores vs ~1.5s en modo individual (67% más rápido)

**Salida esperada:**

```
⚡ Lectura en Cascada (11 sensores)...
📤 TX → Addr:FF CMD:12 FLAGS:00000001 LEN:0
   Bytes: AA FF FF 01 12 00 3A 55
📥 RX ← From:01 CMD:92 FLAGS:00000002 LEN:40
✅ Sensor 1: ['10.00', '10.55', '11.10']... FLAGS=2
📥 RX ← From:02 CMD:92 FLAGS:00000003 LEN:40
✅ Sensor 2: ['9.87', '10.21', '10.09']... FLAGS=3
...
📥 RX ← From:11 CMD:92 FLAGS:00000000 LEN:40
✅ Sensor 11: ['10.33', '10.44', '10.12']... FLAGS=0
🏁 Fin de cascada detectado (FLAGS=0)

📊 Cascada completada en 0.487s (11/11 sensores)
```

### 4. Lectura RAW ADC

Ver valores crudos del ADC (12-bit, sin calibración):

```bash
python main.py --port /dev/ttyUSB0 --mode raw --sensor 1
```

**Uso:** Diagnóstico de sensores, verificar que ADC funcione correctamente.

### 5. Comparación de Modos

Probar latencia de ambos modos:

```bash
python main.py --port /dev/ttyUSB0 --mode compare --sensors 11
```

**Salida esperada:**

```
============================================================
TEST: COMPARACIÓN DE MODOS
============================================================

🐢 Modo Individual:
📊 Leyendo Sensor 1 (Individual)...
📊 Leyendo Sensor 2 (Individual)...
...
   Tiempo total: 1.523s

⚡ Modo Cascada:
⚡ Lectura en Cascada (11 sensores)...
   Tiempo total: 0.487s

📊 Mejora: 68.0% más rápido en modo cascada
```

### 6. Modo Interactivo

Comandos manuales en consola:

```bash
python main.py --port /dev/ttyUSB0 --mode interactive
```

**Comandos disponibles:**

```
🎮 Modo Interactivo
Comandos disponibles:
  r [addr]       - Leer sensor individual
  c              - Lectura en cascada
  s [addr]       - Estado del sensor
  f [addr] [hz]  - Configurar filtro
  q              - Salir

> r 1                    # Leer sensor 1
> c                      # Cascada completa
> s 3                    # Estado del sensor 3
> f 2 5.0                # Filtro del sensor 2 a 5Hz
> q                      # Salir
```

## 📊 Comandos Disponibles

| Comando            | Código | Descripción                    | Payload         |
| ------------------ | ------ | ------------------------------ | --------------- |
| `CMD_READ_SENSOR`  | 0x10   | Lee valores calibrados         | -               |
| `CMD_READ_RAW`     | 0x11   | Lee valores ADC (12-bit)       | -               |
| `CMD_READ_CASCADE` | 0x12   | Lee en cascada (broadcast)     | -               |
| `CMD_GET_STATUS`   | 0x31   | Estado de thresholds digitales | -               |
| `CMD_SET_FILTER`   | 0x22   | Configura filtro Biquad        | 4 bytes (float) |

### Respuestas

| Comando                 | Código | Payload               |
| ----------------------- | ------ | --------------------- |
| `CMD_READ_SENSOR_RESP`  | 0x90   | 40 bytes (10x float)  |
| `CMD_READ_RAW_RESP`     | 0x91   | 20 bytes (10x uint16) |
| `CMD_READ_CASCADE_RESP` | 0x92   | 40 bytes (10x float)  |
| `CMD_GET_STATUS_RESP`   | 0xB1   | 2 bytes (uint16)      |

## 🐛 Troubleshooting

### Error: "No such file or directory: '/dev/ttyUSB0'"

**Problema:** Puerto no existe o conversor RS-485 no conectado.

**Solución:**

```bash
# Listar puertos disponibles
ls /dev/tty*

# Verificar si kernel detecta USB
dmesg | tail -20

# Probar con otro puerto
python main.py --port /dev/ttyACM0 --mode individual --sensor 1
```

### Error: "Permission denied: '/dev/ttyUSB0'"

**Problema:** Usuario no tiene permisos sobre puerto serial.

**Solución:**

```bash
# Temporal (requiere sudo cada vez)
sudo python main.py --port /dev/ttyUSB0 ...

# Permanente: agregar usuario a grupo dialout
sudo usermod -a -G dialout $USER
# Cerrar sesión y volver a entrar
```

### Timeout Esperando Respuesta

**Problema:** Sensor no responde.

**Checar:**

1. **Cable RS-485**: Verificar A-B correctamente conectados
2. **Polaridad**: A del conversor → A de sensores, B → B
3. **Alimentación**: Sensores tienen 12-24VDC
4. **Dirección**: Sensor configurado con dirección correcta (1-11)
5. **Baudrate**: Debe ser 115200 (verificar en firmware si cambió)
6. **Terminación**: Resistencias de 120Ω en extremos del bus

**Debug:**

```bash
# Aumentar timeout
python main.py --port /dev/ttyUSB0 --mode individual --sensor 1 --timeout 2.0

# Probar lectura raw (más simple)
python main.py --port /dev/ttyUSB0 --mode raw --sensor 1
```

### CRC Error Frecuente

**Problema:** Ruido eléctrico en el bus RS-485.

**Solución:**

- Usar cable par trenzado (twisted pair) para A-B
- Reducir longitud del cable (<500m para RS-485)
- Separar cable RS-485 de cables de potencia
- Agregar terminación de 120Ω en extremos

### Respuestas Duplicadas en Modo Cascada

**Problema:** Sensores con direcciones duplicadas.

**Solución:**

```bash
# Verificar dirección de cada sensor individualmente
for i in {1..11}; do
    echo "Probando sensor $i..."
    python main.py --port /dev/ttyUSB0 --mode individual --sensor $i
done
```

Si dos sensores responden con la misma dirección, reprogramar uno:

```python
# TODO: Implementar CMD_WRITE_CONFIG para cambiar dirección
```

## 📈 Medición de Performance

### Script de Benchmark

```bash
#!/bin/bash
# benchmark.sh - Medir latencia durante 10 iteraciones

for i in {1..10}; do
    echo "Iteración $i:"
    python main.py --port /dev/ttyUSB0 --mode compare --sensors 11
    sleep 1
done
```

### Latencias Esperadas (11 sensores)

| Modo       | Latencia Teórica | Latencia Medida |
| ---------- | ---------------- | --------------- |
| Individual | ~1.5s            | 1.4-1.6s        |
| Cascada    | ~500ms           | 450-550ms       |

**Factores que afectan latencia:**

- Baudrate (115200 fijo)
- Tiempo de procesamiento del sensor (~10ms por comando)
- Timeout configurado (default: 0.5s)
- Carga del bus USB en PC

## 🔍 Validación del Protocolo

### 1. Test de Filtrado de Dirección

Verificar que solo el sensor correcto responde:

```bash
# Enviar comando a sensor 3, solo 3 debe responder
python main.py --port /dev/ttyUSB0 --mode individual --sensor 3

# Si sensor 5 también responde, hay conflicto de direcciones
```

### 2. Test de FLAGS en Cascada

Verificar secuencia FLAGS=1→2→3...→11→0:

```python
# Ver logs del script:
# ✅ Sensor 1: ... FLAGS=2   ← FLAGS del siguiente sensor
# ✅ Sensor 2: ... FLAGS=3
# ...
# ✅ Sensor 11: ... FLAGS=0  ← Fin de cascada
```

### 3. Test de CRC

El script valida CRC automáticamente. Si hay errores:

```
⚠️  Paquete con CRC inválido descartado
```

Indica ruido eléctrico o problema en conversor RS-485.

## 🧪 Test Completo Recomendado

```bash
# 1. Verificar cada sensor individualmente
echo "=== TEST 1: Individual ==="
for i in {1..11}; do
    python main.py --port /dev/ttyUSB0 --mode individual --sensor $i
done

# 2. Probar modo cascada
echo "=== TEST 2: Cascada ==="
python main.py --port /dev/ttyUSB0 --mode cascade --sensors 11

# 3. Comparar latencias
echo "=== TEST 3: Performance ==="
python main.py --port /dev/ttyUSB0 --mode compare --sensors 11

# 4. Leer valores RAW de sensor crítico
echo "=== TEST 4: RAW ADC ==="
python main.py --port /dev/ttyUSB0 --mode raw --sensor 1
```

## 📡 Analizador Lógico (Opcional)

Para validación profunda del protocolo:

1. **Conectar analizador lógico** a líneas A/B del RS-485
2. **Capturar tráfico** durante lectura cascada
3. **Verificar:**
   - Frame LwPKT: `[0xAA][FROM][TO][FLAGS][CMD][LEN][DATA][CRC][0x55]`
   - Tiempo entre paquetes: <50ms
   - Latencia total: <500ms para 11 sensores
   - FLAGS incrementa correctamente: 1→2→3...→11→0

**Herramientas recomendadas:**

- Saleae Logic Analyzer + UART decoder
- DSLogic + PulseView
- Osciloscopio con decode RS-485

## 📚 Referencias

- **Protocolo LwPKT:** [lwpkt GitHub](https://github.com/MaJerle/lwpkt)
- **CASCADE Protocol:** Ver `docs/CASCADE_READ_PROTOCOL.md`
- **Firmware:** Ver `leather_gauge_sensor/` para implementación
- **Comandos:** Ver `lg_domain_types.h` para códigos completos

## ⚠️ Notas de Seguridad

- **NO conectar/desconectar** sensores con alimentación activa
- **Verificar polaridad** de A-B antes de encender
- **Usar alimentación estabilizada** (12-24VDC, evitar ripple >100mV)
- **Separar tierra** de sensores y conversor USB si hay loops de tierra

## 📞 Soporte

Para reportar issues:

1. Capturar logs completos del script (`python main.py ... > log.txt 2>&1`)
2. Indicar modelo de conversor RS-485 USB usado
3. Especificar si ocurre en modo individual o cascada
4. Incluir configuración del sistema (Linux/Windows, versión Python)
