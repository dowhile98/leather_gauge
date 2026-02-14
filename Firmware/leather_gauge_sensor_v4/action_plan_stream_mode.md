# Action Plan: Implementación de Modo STREAM (Daisy Chain)

## 1. Resumen del Objetivo
Implementar un modo de operación de alta velocidad (STREAM) donde los sensores transmiten su información de forma secuencial tras recibir un pulso físico en `PB4` (Trigger_In), propagando la señal a través de `PB12` (Trigger_Out) solo cuando el bus RS485 esté libre.

---

## 2. Definiciones Técnicas y Mux de Protocolo
El sistema operará bajo una lógica de "Mux de Software" basada en estados:

- **Modo MODBUS (Default):** El sensor responde a comandos estándar. Se usa para configuración, calibración de offsets y cambio de ID.
- **Modo STREAM:** El stack Modbus se suspende. El sensor queda "Armado" esperando un flanco en el pin de entrada.

### Trama Fast-Frame (Modo STREAM)
Para maximizar la velocidad, se usará una trama ligera de 3 bytes:
`[ID_Sensor (1 byte)] + [Data_High (1 byte)] + [Data_Low (1 byte)]`

---

## 3. Hoja de Ruta de Implementación

### Fase 1: Estructuras de Datos y Configuración
**Archivos:** `leather_gauge_typedefs.h`, `leather_gauge_config.h`
- [ ] Definir el enumerado `LG_OP_MODE_t` con los estados `LG_MODE_MODBUS` y `LG_MODE_STREAM`.
- [ ] Definir el registro Modbus de control para conmutar el modo de operación.

### Fase 2: Controladores de Hardware (Low-Level)
**Archivos:** `gpio.c`, `stm32g0xx_it.c`
- [ ] Configurar **PB4** como Entrada con Interrupción Externa (EXTI) por flanco de subida.
- [ ] Configurar **PB12** como Salida Push-Pull de alta velocidad.
- [ ] Asegurar que el **DMA** para USART1 TX esté correctamente inicializado.

### Fase 3: Lógica de Comunicación y Cascada
**Archivos:** `lg_module_modbus.c`, `lg_module_sensor.c`
- [ ] **Manejador de Trigger (PB4):** Implementar la ISR que activa el driver RS485 e inicia la transmisión DMA de la trama de 3 bytes.
- [ ] **Propagación (UART TX Callback):** Implementar la lógica que, al terminar de enviar el último bit, libera el bus RS485 y genera el pulso en PB12.
- [ ] **Preparación de Datos:** Asegurar que la trama de 3 bytes contenga el valor digital (10 bits) actualizado por el ADC.

### Fase 4: Integración de la Máquina de Estados e Higiene del Bus
**Archivos:** `leather_gauge.c`, `lg_module_modbus.c`
- [ ] Modificar `lg_sensor_run` para gestionar el flujo según el modo activo.
- [ ] **Higiene del Buffer RX:** 
    - Al entrar en modo `STREAM`, deshabilitar opcionalmente la escritura en el `lwrb` para no acumular ráfagas de otros sensores.
    - Al salir de modo `STREAM` y volver a `MODBUS`, ejecutar un **Flush (Reset)** del `lwrb` y del estado del parser `nmbs`.
- [ ] Implementar el mecanismo de "Armado/Re-armado" para evitar disparos falsos.

---

## 4. Gestión del Tráfico en el Bus Compartido
Dado que el bus RS485 es compartido, todos los sensores recibirán las ráfagas de 3 bytes de sus compañeros. 
- **nanoMODBUS:** Descartará las tramas por error de CRC y longitud inválida.
- **Optimización:** Durante la ráfaga de la cadena, se recomienda que los sensores que ya transmitieron o están esperando trigger entren en un estado de "RX Mute" lógico para ignorar el tráfico de los demás nodos.

```text
[Pin PB4: Flanco de Subida] 
          |
          v
[ISR EXTI: DIR_RS485 = HIGH] --> (Ocupa el bus inmediatamente)
          |
          v
[DMA Start: Transmitir 3 bytes]
          |
          v
[Finaliza Transmisión UART]
          |
          v
[Callback TX: DIR_RS485 = LOW] --> (Libera el bus)
          |
          v
[Callback TX: Pulso en PB12] ----> [Trigger para el Siguiente Nodo]
```

---

## 5. Análisis de Tiempos Estimados (Baudrate 1Mbps)
- **Latencia de Entrada:** ~1µs
- **Transmisión de Trama (30 bits):** 30µs
- **Propagación del Trigger:** ~1µs
- **Total por Nodo:** ~32µs
- **Total Cadena (11 nodos):** **~352µs** (0.35ms)
