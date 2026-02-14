# Refactor Plan: Leather Gauge Sensor V2

This document outlines the plan to refactor the firmware architecture to adhere to **Clean Architecture** and **SOLID** principles, specifically migrating the communication layer from Modbus to **LwPKT** (Lightweight Packet Protocol) over RS-485.

## 1. Architectural Goals

*   **Decoupling:** Business logic must not depend on hardware (STM32 HAL) or specific libraries directly.
*   **Testability:** Core logic must be testable on a PC without hardware.
*   **Dependency Inversion:** High-level policy (App/Core) should define interfaces; low-level details (Drivers/Libs) should implement them.
*   **Single Responsibility:** Separate concerns (Communication, Sensing, Storage).

## 2. Proposed Architecture (Layered)

We will structure the code into three main layers:

### A. Core (Domain Layer)
*   **Role:** Contains the business logic and entities.
*   **Dependencies:** None (Pure C).
*   **Components:**
    *   **Entities:** `SensorConfig`, `SensorData`.
    *   **Interfaces (Ports):**
        *   `ISensor`: Methods to read raw/filtered data.
        *   `IComms`: Methods to send/receive data.
        *   `IPersistence`: Methods to save/load config.
    *   **Use Cases:**
        *   `ProcessMeasurement`: Reads sensor, filters, checks thresholds.
        *   `HandleCommand`: Parses incoming commands and triggers actions.

### B. Infrastructure (Adapter Layer)
*   **Role:** Implements the interfaces defined by the Core using specific hardware or libraries.
*   **Dependencies:** Core, STM32 HAL, Third_Party Libs (`lwpkt`, `lwrb`, `DSP_Biquad`).
*   **Components:**
    *   **Stm32AdcAdapter:** Implements `ISensor` using STM32 ADC + `DSP_Biquad`.
    *   **LwPktAdapter:** Implements `IComms` using `lwpkt` and `lwrb` over UART (RS-485).
    *   **EepromAdapter:** Implements `IPersistence` using the existing EEPROM logic.

### C. Application (Composition Root)
*   **Role:** Wires everything together.
*   **Location:** `app/Src/leather_gauge.c`.
*   **Responsibilities:**
    *   Initialize hardware (HAL).
    *   Instantiate Adapters.
    *   Inject Adapters into Use Cases/Core services.
    *   Run the Super Loop.

## 3. Communication Protocol (LwPKT Migration)

We will replace `nanoMODBUS` with `lwpkt`.

### Hardware Interface (RS-485)
*   **UART:** DMA-based TX/RX (handled by `lwrb` ring buffers).
*   **DE Pin:** Driver Enable for RS-485. Controlled via `lwpkt` hooks (`LWPKT_EVT_PRE_WRITE` / `LWPKT_EVT_POST_WRITE`).

### Command Map
| Old Modbus Register | New LwPKT Command (ID) | Description | Payload |
| :--- | :--- | :--- | :--- |
| `S1_ADDR` | `CMD_READ_RAW` (0x10) | Get Raw ADC | None -> `uint16_t` |
| `D1_ADDR` | `CMD_READ_VAL` (0x11) | Get Filtered | None -> `float` |
| `OFFSET_S1_ADDR` | `CMD_SET_OFFSET` (0x20) | Set Zero Offset | `float` |
| `FILTER_FC_ADDR` | `CMD_SET_FILTER` (0x21) | Set Cutoff Freq | `float` |
| `DI_VALUE_ADDR` | `CMD_GET_STATUS` (0x30) | Get Status/Threshold | None -> `uint8_t` |

## 4. Implementation Steps

### Step 1: Define Interfaces
Create `leather_gauge_sensor/interfaces/` and define:
*   `lg_i_sensor.h`: `init`, `read_raw`, `read_filtered`, `set_calibration`.
*   `lg_i_comm.h`: `init`, `send`, `process`.
*   `lg_i_storage.h`: `load_config`, `save_config`.

### Step 2: Implement Sensor Adapter
Refactor `modules/sensor` to implement `lg_i_sensor.h`.
*   Encapsulate `DSP_Biquad` inside this adapter.
*   Remove direct HAL dependency if possible (pass data in, or use a pure `IAdc` interface). *For now, keeping HAL inside the Adapter is acceptable as long as the Interface is clean.*

### Step 3: Implement LwPKT Adapter
Create `modules/comms/lg_adapter_lwpkt.c`.
*   **Init:** Setup `lwrb` buffers and `lwpkt` instance.
*   **RS-485:** Implement `evt_fn` to toggle DE pin (GPIO) on Write events.
*   **Process:** Call `lwpkt_read` and `lwpkt_process` in the polling function.
*   **Dispatch:** Map received `cmd` to Core function calls.

### Step 4: Refactor Main App (`leather_gauge.c`)
*   Remove `lg_module_modbus` calls.
*   Initialize `LwPktAdapter`.
*   In `lg_sensor_run`, call `Comms_Process()` and `Sensor_Process()`.

## 5. File Structure Changes

```text
leather_gauge_sensor/
├── interfaces/             <-- NEW
│   ├── lg_i_sensor.h
│   ├── lg_i_comm.h
│   └── lg_i_storage.h
├── core/                   <-- NEW (Domain)
│   ├── lg_domain_types.h
│   └── lg_command_processor.c
├── adapters/               <-- RENAME/MOVE from modules
│   ├── sensor/             <-- Implements lg_i_sensor
│   ├── comms/              <-- NEW (LwPKT impl)
│   └── storage/            <-- EEPROM impl
└── app/
    └── Src/leather_gauge.c <-- Composition Root
```

## 6. Testing Strategy
*   **Unit Tests:** Compile `core/` and `adapters/` (mocking HAL) on PC.
*   **Integration:** Verify `lwpkt` packet exchange using a Python script on PC sending bytes to the STM32 UART.