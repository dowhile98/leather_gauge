# Diagramas de Secuencia - Lectura Individual vs Cascada

## Modo Individual (Original)

```mermaid
sequenceDiagram
    participant M as Master<br/>(Python/PC)
    participant S1 as Sensor 1
    participant S2 as Sensor 2
    participant S11 as Sensor 11

    Note over M,S11: Lectura Secuencial Individual

    M->>S1: CMD_READ_SENSOR (to=1)
    S1->>M: RESP + data (40B)
    Note right of M: Delay ~150ms

    M->>S2: CMD_READ_SENSOR (to=2)
    S2->>M: RESP + data (40B)
    Note right of M: Delay ~150ms

    M->>S11: CMD_READ_SENSOR (to=11)
    S11->>M: RESP + data (40B)
    Note right of M: Delay ~150ms

    Note over M: Tiempo Total: 11 × 150ms = 1.65s
```

## Modo Cascada (Optimizado con FLAGS)

```mermaid
sequenceDiagram
    participant M as Master<br/>(Python/PC)
    participant S1 as Sensor 1
    participant S2 as Sensor 2
    participant S3 as Sensor 3
    participant S11 as Sensor 11

    Note over M,S11: Lectura en Cascada con Broadcast

    M->>S1: Broadcast: CMD_READ_CASCADE + FLAGS=1
    Note over S1,S11: Todos escuchan, solo FLAGS==1 responde

    S1->>M: RESP + data + FLAGS=2
    Note over S1: Detecta FLAGS==mi_addr<br/>Responde + incrementa FLAGS

    Note over S2,S11: Sensor2 escucha RESP anterior<br/>Ve FLAGS=2, es su turno
    S2->>M: RESP + data + FLAGS=3

    Note over S3,S11: Sensor3 escucha FLAGS=3
    S3->>M: RESP + data + FLAGS=4

    Note over S11: ...sensores 4-10...

    S11->>M: RESP + data + FLAGS=0
    Note over S11: FLAGS=0 indica fin

    Note over M: Tiempo Total: 11 × 50ms = 550ms ⚡
    Note over M: Reducción: 67% más rápido
```

## Flujo de Decisión en Sensor (CMD_READ_CASCADE)

```mermaid
flowchart TD
    A[Recibir Paquete] --> B{cmd == CMD_READ_CASCADE?}
    B -->|No| Z[Ignorar]
    B -->|Sí| C{FLAGS == mi_address?}
    C -->|No| D[Ignorar<br/>No es mi turno]
    C -->|Sí| E[Leer Datos Sensor<br/>current_data.calibrated]
    E --> F[Calcular next_flags<br/>= mi_address + 1]
    F --> G{next_flags > 11?}
    G -->|Sí| H[next_flags = 0<br/>Fin de cadena]
    G -->|No| I[next_flags OK]
    H --> J[Enviar RESP<br/>+ data + FLAGS]
    I --> J
    J --> K[Master recibe]

    style C fill:#ff9
    style E fill:#9f9
    style J fill:#9ff
```

## Manejo de Errores - Timeout en Cascada

```mermaid
sequenceDiagram
    participant M as Master
    participant S1 as Sensor 1
    participant S2 as Sensor 2 (OFFLINE)
    participant S3 as Sensor 3

    M->>S1: Broadcast: CASCADE + FLAGS=1
    S1->>M: RESP + FLAGS=2
    Note over M: Espera Sensor2...

    Note over M,S3: Timeout 100ms
    rect rgb(255, 200, 200)
        Note over M: Sensor2 no responde!
    end

    M->>S3: Broadcast: CASCADE + FLAGS=3
    Note over M: Skip sensor offline<br/>Continúa con siguiente
    S3->>M: RESP + FLAGS=4

    Note over M: Al final, reintenta<br/>Sensor2 en modo individual
    M->>S2: Individual: READ_SENSOR (to=2)
    Note over M: Timeout 200ms → FAIL
    Note over M: Log: Sensor2 OFFLINE
```

## Arquitectura de Capas (Clean Architecture)

```mermaid
graph TB
    subgraph "Core Domain (HAL-Free)"
        A[lg_core.c<br/>handle_command]
        B[lg_domain_types.h<br/>CMD_READ_CASCADE]
    end

    subgraph "Interfaces (DIP)"
        C[lg_i_comm.h<br/>send_with_flags]
        D[lg_i_lwpkt.h<br/>Encode/Decode]
    end

    subgraph "Adapters (HAL Access)"
        E[lg_adapter_comm.c<br/>comm_send_with_flags]
        F[lg_lwpkt_codec.c<br/>lwpkt_write FLAGS]
    end

    subgraph "Infrastructure"
        G[lwpkt library<br/>FLAGS support]
        H[UART/RS-485<br/>HAL]
    end

    A -->|Usa| C
    A -->|Define| B
    C -.Implementa.-> E
    D -.Implementa.-> F
    E -->|Llama| G
    E -->|Controla| H
    F -->|Usa| G

    style A fill:#9f9
    style C fill:#ff9
    style E fill:#9cf
    style G fill:#f99
```

## Comparación de Tráfico en Bus RS-485

```mermaid
gantt
    title Tráfico RS-485: Individual vs Cascada
    dateFormat X
    axisFormat %L ms

    section Individual
    Master→S1     :0, 50
    S1→Master     :50, 100
    Delay         :100, 150
    Master→S2     :150, 200
    S2→Master     :200, 250
    Delay         :250, 300
    Master→S3     :300, 350
    (x11 total)   :350, 1650

    section Cascada
    Master→Broadcast :0, 50
    S1→Master        :50, 100
    S2→Master        :100, 150
    S3→Master        :150, 200
    S4→Master        :200, 250
    S11→Master       :500, 550
```

---

**Leyenda:**

- 🟢 Verde: Capa de dominio (puro C, sin HAL)
- 🟡 Amarillo: Interfaces (abstracciones DIP)
- 🔵 Azul: Adapters (implementaciones concretas)
- 🔴 Rojo: Infraestructura (librerías externas)
