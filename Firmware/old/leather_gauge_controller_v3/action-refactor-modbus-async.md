# Action: Refactorización Asíncrona de Modbus RTU

**Estado:** ✅ **EN IMPLEMENTACIÓN** (Fase B/C completadas)  
**Impacto:** Crítico (Eliminación de Latencia y Jitter de Medición)  
**Tecnologías:** STM32 HAL, Azure ThreadX, nanoMODBUS, DMA+IDLE

---

## 🚀 Progreso de Implementación (Feb 13, 2026)

### Archivos Creados

| Archivo                                    | Descripción                      | Estado |
| ------------------------------------------ | -------------------------------- | ------ |
| `domain/interfaces/lgc_i_sensor_cache.h`   | Interfaz para sensor cache (DIP) | ✅     |
| `domain/interfaces/lgc_modbus_fsm_types.h` | Tipos FSM para Modbus async      | ✅     |
| `adapters/sensor_cache/lgc_sensor_cache.h` | Header del sensor cache          | ✅     |
| `adapters/sensor_cache/lgc_sensor_cache.c` | Implementación thread-safe       | ✅     |
| `app/inc/lgc_modbus_task.h`                | Header de la tarea Modbus        | ✅     |
| `app/src/lgc_modbus_task.c`                | FSM asíncrona completa           | ✅     |
| `tests/unit/test_lgc_sensor_cache.c`       | Tests unitarios (Unity)          | ✅     |
| `tests/CMakeLists.txt`                     | Build system para tests          | ✅     |

### Archivos Modificados

| Archivo                                 | Cambios                                    | Estado |
| --------------------------------------- | ------------------------------------------ | ------ |
| `app/src/lgc.c`                         | Instancia sensor cache + modbus task       | ✅     |
| `app/inc/lgc.h`                         | Exports `lgc_get_sensor_cache_interface()` | ✅     |
| `app/src/lgc_main_task.c`               | Usa sensor cache en lugar de polling       | ✅     |
| `modules/modbus/lgc_interface_modbus.h` | API async (`lgc_modbus_send_raw_pdu`)      | ✅     |
| `modules/modbus/lgc_inteface_modbus.c`  | Implementación async + signal FSM          | ✅     |

### Pendiente

- [ ] Fase A: Incrementar baudrate a 38400 bps en CubeMX
- [ ] Validación en hardware real
- [ ] Medición de tiempos de ciclo con nuevo sistema

---

## 1. Diagnóstico del Problema Actual

El sistema presenta un retardo de ciclo crítico debido a una arquitectura de comunicación síncrona y bloqueante localizada en `lgc_main_task.c`.

### Hallazgos Clave:

1. **Acoplamiento de Tareas:** La `Main_Task` (encargada de procesar pulsos de encoder) realiza el polling de los 11 sensores Modbus de forma secuencial dentro de su propio contexto de ejecución.
2. **Cascada de Timeouts:** El uso de `nmbs_read_holding_registers` (bloqueante) sumado a un `LGC_SENSOR_READ_RETRY` de 10 reintentos provoca que un solo sensor fuera de línea bloquee la CPU por hasta 100ms+. Multiplicado por 11 sensores, el tiempo de "atrapado" puede superar el segundo, perdiendo pulsos críticos del encoder.
3. **Bajo Ancho de Banda:** A 9600 bps, el tiempo físico de transmisión por sensor es de ~30ms (incluyendo silencios T3.5 y procesamiento del esclavo). Ciclo total: **~330ms**.

## 2. Solución Propuesta: Arquitectura Orientada a Eventos

Se propone desacoplar totalmente la comunicación del procesamiento mediante una **Máquina de Estados (FSM) Asíncrona** y una mejora en la capa física.

### Fase A: Optimización de Hardware

- **Incremento de Baudrate:** Subir a **38400 bps**.
  - _Mejora:_ El tiempo de transmisión pura baja de 15.6ms a 3.9ms por trama de 15 bytes.
  - _Resultado:_ El ciclo total teórico (Tx+Rx+T3.5+Proc) baja de 330ms a **~170ms** (reducción del 48%).
- **Transporte DMA + IDLE Line:** Utilizar `HAL_UARTEx_ReceiveToIdle_DMA` en USART3. Esto permite que el hardware detecte el fin de la trama Modbus y despierte al RTOS solo cuando el paquete está completo en RAM.

### Fase B: Tarea de Modbus Dedicada (`lgc_modbus_task`)

Crear una nueva tarea con prioridad superior a `Main_Task` para gestionar el bus RS-485 sin bloquear el algoritmo de medición.

#### Estados de la FSM:

1. **MB_IDLE:** Espera notificación de inicio de ronda (encoder) o timer periódico.
2. **MB_SEND_REQ:** Usa `nmbs_send_raw_pdu` para lanzar la petición al sensor `N` vía DMA.
3. **MB_WAIT_RX:** La tarea se suspende en un semáforo con un timeout estricto (ej. 15ms).
4. **MB_PARSE:** Al recibir el semáforo (vía UART RxEvent Callback), usa `nmbs_receive_raw_pdu_response` para procesar el buffer que ya está en RAM.
5. **MB_NEXT:** Incrementa el ID del sensor y vuelve a `SEND_REQ`. Si terminó los 11, vuelve a `IDLE`.

### Fase C: Desacople de Datos (Sensor Cache)

- La `Main_Task` ya no llama a funciones de Modbus.
- Se implementa un buffer de intercambio `lgc_sensor_cache_t` protegido por un **Mutex**.
- La `Modbus_Task` actualiza el cache asíncronamente; la `Main_Task` lee los últimos datos disponibles en cada pulso del encoder sin esperar al bus.

## 3. Refactorización de Código (Draft)

### Nueva Lógica de Lectura (lgc_interface_modbus.c)

Se utilizarán PDUs manuales para evitar el bucle de espera interno de `nanoMODBUS`:

```c
// Lógica dentro de la nueva Modbus Task
void process_sensor_async(uint8_t dev_id) {
    uint8_t pdu_req[5];
    // Preparar PDU manualmente para Read Holding Registers (FC 03)
    pdu_req[0] = 0x03;                    // Función
    pdu_req[1] = 0x00; pdu_req[2] = 0x2D; // Dirección del registro 45
    pdu_req[3] = 0x00; pdu_req[4] = 0x01; // Cantidad de registros 1

    nmbs_set_destination_rtu_address(&nmbs, dev_id);

    // Envío no bloqueante
    nmbs_send_raw_pdu(&nmbs, 0x03, pdu_req, 5);

    // Esperar respuesta de la ISR de UART (liberada por IDLE Line)
    if (osWaitForSemaphore(&modbus_rx_semaphore, 15) == TRUE) {
        uint8_t pdu_res[32];
        if (nmbs_receive_raw_pdu_response(&nmbs, pdu_res, 32) == NMBS_ERROR_NONE) {
            // Actualizar estructura global protegida por Mutex
            lgc_update_sensor_cache(dev_id, (pdu_res[1] << 8) | pdu_res[2]);
        }
    } else {
        // Sensor Offline: Manejo de error sin penalizar el ciclo total
        lgc_mark_sensor_fault(dev_id);
    }
}
```

## 4. Jerarquía de Prioridades Final Sugerida

| Prioridad | Componente         | Responsabilidad                                        |
| :-------- | :----------------- | :----------------------------------------------------- |
| HW High   | `UART3_DMA_IRQ`    | Mover datos RS-485 y liberar semáforo de Rx.           |
| HW High   | `EXTI_Encoder_IRQ` | Contar pulsos y notificar inicio de ciclo de lectura.  |
| Task 9    | `Modbus_Task`      | Gestionar FSM del bus (vaciar cola de peticiones).     |
| Task 10   | `Main_Task`        | Algoritmo de integración (leer cache y calcular área). |
| Task 11   | `HMI_Task`         | Actualizar pantalla DWIN.                              |

## 5. Beneficios Esperados

1. **Jitter Cero:** La tarea de medición siempre tiene datos (los últimos válidos) disponibles instantáneamente.
2. **Resiliencia:** Un sensor dañado solo penaliza un timeout (15ms) en lugar de múltiples reintentos bloqueantes.
3. **Fluidez:** La interfaz DWIN y el sistema en general responderán mejor al no tener la CPU "atrapada" en bucles de espera de UART.
