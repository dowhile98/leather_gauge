# LwPKT Communication Layer Integration (TDD + Clean Architecture)

## Project Context

**Target Platform:** STM32G030C8 (Cortex-M0+, 64KB Flash, 8KB RAM)  
**Current State:** Modbus-RTU implementation causing performance bottlenecks when polling 11 sensors sequentially  
**Goal:** Migrate to LwPKT (Lightweight Packet Protocol) for optimized multi-drop RS-485 communication  
**Architecture:** Clean Architecture with SOLID principles, TDD-first approach, zero dynamic allocation

## Problem Statement

The existing Modbus implementation reads multiple registers per sensor, resulting in:

- High latency (~1.5s for 11 sensors at 19200 baud)
- Unnecessary data transfer (reading full register maps when only sensor value is needed)
- Inflexible command structure (limited by Modbus function codes)

**Performance Target:** Reduce polling cycle to <50ms for 11 sensors with single-value reads.

## Architectural Mandates (SOLID + Clean Architecture)

### 1. Dependency Inversion Principle (DIP)

- **CRITICAL:** Core domain logic (`lg_core.c`) must NOT include `stm32g0xx_hal.h` or any hardware headers
- Communication must be abstracted behind `ICommInterface_t` (defined in `lg_i_comm.h`)
- LwPKT adapter implements the interface, hiding protocol details from domain

### 2. Single Responsibility Principle (SRP)

Each module has ONE reason to change:

- **LwPKT Adapter:** Encode/decode LwPKT frames, manage RS-485 DE pin
- **Core Domain:** Command routing, validation, business logic
- **UART Driver:** Hardware abstraction for TX/RX/DMA

### 3. Open/Closed Principle (OCP)

- Design allows adding new command handlers without modifying core routing logic
- Use command dispatch table (function pointers) for extensibility

### 4. Interface Segregation Principle (ISP)

- Split communication concerns:
  - `IPacketEncoder_t`: Frame creation
  - `IPacketDecoder_t`: Frame parsing
  - `ITransport_t`: Physical layer (UART/DMA)

### 5. Liskov Substitution Principle (LSP)

- Any `ICommInterface_t` implementation must honor contract:
  - `Send()` returns `ERR_OK` on successful transmission
  - `Receive()` populates buffer with valid packet or returns error
  - No hidden side effects (e.g., blocking ISRs)

## Test-Driven Development Workflow

### Phase 1: Define Interfaces (RED)

```c
// File: leather_gauge_sensor/interfaces/lg_i_lwpkt.h

/**
 * @brief LwPKT packet encoder interface.
 * @note  Must be reentrant (no internal state).
 */
typedef struct {
    /**
     * @brief Encode a command packet.
     * @param[in]  from      Source address (1-11).
     * @param[in]  to        Destination address (0=broadcast, 1-11=specific).
     * @param[in]  cmd       Command ID (see lg_domain_types.h).
     * @param[in]  payload   Payload buffer (may be NULL if len=0).
     * @param[in]  len       Payload length in bytes.
     * @param[out] out_buf   Output buffer for encoded frame.
     * @param[in]  out_size  Size of output buffer.
     * @param[out] out_len   Actual encoded length.
     * @return ERR_OK | ERR_INVALID_PARAM | ERR_BUFFER_FULL
     */
    Result_t (*Encode)(uint8_t from, uint8_t to, uint8_t cmd,
                       const uint8_t *payload, uint16_t len,
                       uint8_t *out_buf, uint16_t out_size, uint16_t *out_len);

    /**
     * @brief Decode a received frame.
     * @param[in]  frame      Raw frame buffer.
     * @param[in]  frame_len  Frame length in bytes.
     * @param[out] from       Parsed source address.
     * @param[out] to         Parsed destination address.
     * @param[out] cmd        Parsed command ID.
     * @param[out] payload    Pointer to payload within frame (NOT copied).
     * @param[out] len        Payload length.
     * @return ERR_OK | ERR_CRC_FAIL | ERR_INVALID_PARAM
     */
    Result_t (*Decode)(const uint8_t *frame, uint16_t frame_len,
                       uint8_t *from, uint8_t *to, uint8_t *cmd,
                       const uint8_t **payload, uint16_t *len);
} ILwPktCodec_t;
```

### Phase 2: Write Failing Tests (RED)

```c
// File: tests/test_lwpkt_adapter.c

void test_Encode_ValidCommand_ReturnsOK(void) {
    uint8_t buffer[32];
    uint16_t out_len;
    uint8_t payload[] = {0x01, 0x02};

    Result_t res = codec.Encode(1, 2, CMD_READ_SENSOR, payload, 2,
                                 buffer, sizeof(buffer), &out_len);

    TEST_ASSERT_EQUAL(ERR_OK, res);
    TEST_ASSERT_GREATER_THAN(0, out_len);
    TEST_ASSERT_EQUAL(LWPKT_START_BYTE, buffer[0]);  // START
    // Verify CRC, STOP byte, etc.
}

void test_Decode_CorruptedCRC_ReturnsCRCFail(void) {
    uint8_t frame[] = {0xAA, 0x01, 0x02, 0x10, 0x00, 0xFF, 0x55};  // Bad CRC
    uint8_t from, to, cmd;
    const uint8_t *payload;
    uint16_t len;

    Result_t res = codec.Decode(frame, sizeof(frame), &from, &to, &cmd, &payload, &len);

    TEST_ASSERT_EQUAL(ERR_CRC_FAIL, res);
}
```

### Phase 3: Implement Minimum Code (GREEN)

```c
// File: leather_gauge_sensor/adapters/comms_lwpkt/lwpkt_adapter.c

static lwpkt_t s_lwpkt_instance;
static lwrb_t s_tx_ringbuffer;
static lwrb_t s_rx_ringbuffer;
static uint8_t s_tx_buffer[256];
static uint8_t s_rx_buffer[256];

Result_t LwPktAdapter_Init(LwPktAdapter_t *self, const LwPktAdapterConfig_t *config) {
    if (self == NULL || config == NULL) {
        return ERR_NULL_POINTER;
    }

    // Initialize ring buffers (static allocation)
    lwrb_init(&s_tx_ringbuffer, s_tx_buffer, sizeof(s_tx_buffer));
    lwrb_init(&s_rx_ringbuffer, s_rx_buffer, sizeof(s_rx_buffer));

    // Initialize LwPKT with event callback
    lwpkt_init(&s_lwpkt_instance, &s_tx_ringbuffer, lwpkt_event_callback, self);

    // Store UART/GPIO handles (DI)
    self->uart_handle = config->uart_handle;
    self->de_pin = config->de_pin;
    self->de_port = config->de_port;

    return ERR_OK;
}
```

### Phase 4: Refactor (REFACTOR)

- Extract CRC calculation to pure function (testable on PC)
- Separate RS-485 DE pin control into `ITransportControl_t` interface
- Move buffer management to dedicated module

## LwPKT Protocol Specification

### Packet Structure (Variable Length)

```
+-------+------+----+-------+-----+-----+---------+-----+------+
| START | FROM | TO | FLAGS | CMD | LEN |  DATA   | CRC | STOP |
+-------+------+----+-------+-----+-----+---------+-----+------+
   1B     1B*   1B*   1B*    1B*   2B    0-255B    1B    1B

* Optional fields (enabled via lwpkt_opts.h)
```

### Field Definitions

- **START:** `0xAA` (fixed)
- **FROM:** Source address (1-11 for sensors, 0xFF for master)
- **TO:** Destination (0=broadcast, 1-11=specific sensor)
- **FLAGS:** User-defined (bit 0: response expected, bit 1: last sensor in chain)
- **CMD:** Command ID (see Command Map below)
- **LEN:** Payload length (little-endian, uint16_t)
- **DATA:** Payload (max 255 bytes)
- **CRC:** 8-bit CRC-8/CCITT (polynomial 0x07)
- **STOP:** `0x55` (fixed)

### Command Map (Domain Layer)

```c
// File: leather_gauge_sensor/core/lg_domain_types.h

typedef enum {
    CMD_READ_SENSOR       = 0x10,  // Request sensor value (no payload)
    CMD_READ_SENSOR_RESP  = 0x90,  // Response (4B float + 1B sensor_id)
    CMD_WRITE_CONFIG      = 0x20,  // Write config (see ConfigPayload_t)
    CMD_WRITE_CONFIG_RESP = 0xA0,  // ACK/NACK
    CMD_CALIBRATE         = 0x30,  // Trigger calibration
    CMD_ERROR             = 0xFF,  // Error response (1B error code)
} LgCommand_t;
```

### Multi-Sensor Polling Strategy

**Master initiates chain:**

1. Master sends `CMD_READ_SENSOR` to sensor 1 (FROM=0xFF, TO=1)
2. Sensor 1 responds with `CMD_READ_SENSOR_RESP` (FROM=1, TO=0xFF, FLAGS=0x00)
3. Master automatically sends `CMD_READ_SENSOR` to sensor 2 (based on FLAGS bit indicating "not last")
4. ...repeat until sensor 11 responds with FLAGS bit 1 set (last sensor)

**Optimization:** Use FLAGS to indicate next sensor in chain, eliminating master wait time.

## Implementation Checklist

### [ ] Step 1: Interface Definition

- [ ] Create `lg_i_lwpkt.h` with `ILwPktCodec_t`
- [ ] Update `lg_i_comm.h` to support LwPKT frame metadata (from/to/cmd)
- [ ] Define command enums in `lg_domain_types.h`

### [ ] Step 2: Test Setup

- [ ] Add Unity/CMock to `tests/` directory
- [ ] Create `test_lwpkt_codec.c` (encoder/decoder unit tests)
- [ ] Create `test_lwpkt_adapter.c` (integration with mocked UART)
- [ ] Write failing test: encode valid packet

### [ ] Step 3: LwPKT Adapter Implementation

- [ ] Implement `LwPktAdapter_Init()` with dependency injection
- [ ] Implement `LwPktCodec_Encode()` using LwPKT library
- [ ] Implement `LwPktCodec_Decode()` with CRC validation
- [ ] Add RS-485 DE pin control in TX/RX callbacks (`lwpkt_event_callback`)

### [ ] Step 4: UART/DMA Integration

- [ ] Create `UartTransport_t` implementing `ITransport_t`
- [ ] Hook DMA RX complete ISR to feed `lwrb_write()`
- [ ] Hook TX complete ISR to release DE pin (RS-485)
- [ ] Ensure ISR duration <50µs (defer work to main loop via semaphore)

### [ ] Step 5: Core Domain Integration

- [ ] Refactor `lg_core.c` to use `ILwPktCodec_t` instead of Modbus functions
- [ ] Update command dispatcher to handle new `LgCommand_t` values
- [ ] Add validation for FROM/TO address ranges (1-11)

### [ ] Step 6: Configuration

- [ ] Update `leather_gauge_config.h` with LwPKT parameters:
  ```c
  #define LG_LWPKT_MAX_PAYLOAD  128
  #define LG_LWPKT_TX_BUFFER_SIZE 256
  #define LG_LWPKT_RX_BUFFER_SIZE 256
  #define LG_SENSOR_ADDRESS 1  // Configure per device
  ```

### [ ] Step 7: Testing & Validation

- [ ] Run unit tests: `ctest --test-dir build/tests`
- [ ] Verify no dynamic allocation (check .map file for malloc/free)
- [ ] Measure polling latency with logic analyzer (target <500ms for 11 sensors)
- [ ] Stress test with intentional CRC errors (must reject gracefully)

## Error Handling Strategy

### Return Codes

```c
typedef enum {
    ERR_OK = 0,
    ERR_ERROR,
    ERR_NULL_POINTER,
    ERR_INVALID_PARAM,
    ERR_BUFFER_FULL,
    ERR_CRC_FAIL,
    ERR_TIMEOUT,
    ERR_BUSY,
    ERR_NOT_INITIALIZED,
} Result_t;
```

### Error Response Packet

When sensor cannot process command:

```c
// Send CMD_ERROR with 1-byte error code
uint8_t error_code = ERR_INVALID_PARAM;
LwPktCodec_Encode(sensor_id, 0xFF, CMD_ERROR, &error_code, 1, tx_buf, sizeof(tx_buf), &len);
```

## Memory Constraints

- **Static Allocation Only:** No `malloc`, `calloc`, `free`
- **Ring Buffers:** Pre-allocated TX/RX buffers (256B each)
- **Max Packet Size:** 128B payload (total frame ~140B)
- **Stack Usage:** Keep ISR stack <256B (use static buffers in ISR context)

## Code Style Requirements

### Naming Conventions

```c
// Files
lwpkt_adapter.c, lwpkt_adapter.h

// Types
typedef struct LwPktAdapter_t LwPktAdapter_t;

// Interfaces
typedef struct ILwPktCodec_t ILwPktCodec_t;

// Public functions
Result_t LwPktAdapter_Init(LwPktAdapter_t *self, const LwPktAdapterConfig_t *config);

// Private functions (static)
static void encode_header(uint8_t from, uint8_t to, uint8_t *buffer);

// Statics
static lwpkt_t s_lwpkt_instance;
```

### Documentation (Doxygen)

```c
/**
 * @brief  Initialize LwPKT adapter with UART transport.
 * @note   Must be called once during system startup. Not thread-safe.
 *
 * @param[in,out] self    Pointer to adapter instance (must not be NULL).
 * @param[in]     config  Configuration (UART handle, GPIO pins). Must not be NULL.
 *
 * @return ERR_OK on success, ERR_NULL_POINTER if arguments are NULL.
 *
 * @pre UART peripheral must be initialized (via CubeMX).
 * @post Adapter is ready to send/receive packets.
 */
Result_t LwPktAdapter_Init(LwPktAdapter_t *self, const LwPktAdapterConfig_t *config);
```

## Performance Targets

| Metric                     | Target | Measurement Method         |
| -------------------------- | ------ | -------------------------- |
| Polling cycle (11 sensors) | <500ms | Logic analyzer on TX line  |
| CRC validation time        | <100µs | Cycle counter (DWT_CYCCNT) |
| ISR duration (RX complete) | <50µs  | GPIO toggle in ISR         |
| Packet loss rate           | <0.1%  | 10,000 packet test         |

## Integration with Existing Code

### Before (Modbus)

```c
// lg_core.c (old)
uint16_t registers[10];
ModbusRTU_ReadHoldingRegisters(sensor_addr, 0, 10, registers);
float sensor_value = *(float*)&registers[5];
```

### After (LwPKT)

```c
// lg_core.c (new, using interface)
uint8_t tx_buf[32], rx_buf[32];
uint16_t tx_len, rx_len;

// Encode request
ILwPktCodec_t *codec = LwPktAdapter_GetCodec(&lwpkt_adapter);
codec->Encode(MASTER_ADDR, sensor_addr, CMD_READ_SENSOR, NULL, 0, tx_buf, sizeof(tx_buf), &tx_len);

// Send via ICommInterface_t
Result_t res = comm->Send(tx_buf, tx_len);

// Receive response (blocking with timeout)
res = comm->Receive(rx_buf, sizeof(rx_buf), &rx_len, 100);  // 100ms timeout

// Decode response
uint8_t from, to, cmd;
const uint8_t *payload;
uint16_t payload_len;
codec->Decode(rx_buf, rx_len, &from, &to, &cmd, &payload, &payload_len);

if (cmd == CMD_READ_SENSOR_RESP && payload_len == 4) {
    float sensor_value;
    memcpy(&sensor_value, payload, sizeof(float));
}
```

## Validation Criteria

- [ ] All unit tests pass (0 failures)
- [ ] Core domain compiles without HAL includes (verify with `grep -r "stm32g0xx_hal.h" leather_gauge_sensor/core/`)
- [ ] No dynamic allocation (verify with `nm Debug/leather_gauge_sensor_v2.elf | grep malloc` returns empty)
- [ ] Doxygen warnings = 0 (`doxygen Doxyfile 2>&1 | grep warning`)
- [ ] Polling latency measured <500ms (logic analyzer capture)

## References

- LwPKT Library: `Third_Party/lwpkt/src/lwpkt.c`
- LwRB (Ring Buffer): `Third_Party/lwrb/src/lwrb.c`
- Project SOLID Guidelines: `.github/copilot-instructions.md`
- Refactor Plan: `REFACTOR_PLAN.md`
- Agent Handbook: `AGENTS.md`
