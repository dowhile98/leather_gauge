# Action Plan: Daisy Chain Hardware Triggering Implementation

## Overview
Transition the sensor acquisition logic from Modbus Polling (sequential/slow) to a Hardware-Triggered Daisy Chain (burst/fast) to achieve a ~3ms cycle time.

## Phase 1: Bus Mutation (UART3)
**Target Files:** `lgc_interface_modbus.c`, `lgc_interface_modbus.h`

1. **Add Bus Mode Control:**
   - Define `UART_BusMode_t` enum.
   - Implement `lgc_modbus_set_bus_mode(UART_BusMode_t mode)`.
2. **Update Rx Callback:**
   - Modify `lgc_modbus_rx_callback` to bypass `lwrb` (Ring Buffer) when in `DAISY_CHAIN` mode.
   - In `DAISY_CHAIN` mode, copy data directly to a `burst_buffer[33]` and notify `lgc_main_task`.
3. **Pin Control:**
   - Ensure `DIR_SENSORES` pin is held LOW (RX mode) during the entire Production state.

## Phase 2: Hardware Trigger & Sync
**Target Files:** `lgc_main_task.c`, `gpio.c`

1. **GPIO Setup:**
   - Configure a GPIO as Output for the `MASTER_START_TRIGGER`.
2. **Trigger Logic:**
   - Create `lgc_trigger_chain()` function.
   - Integration: Trigger pulse must be fired inside the encoder pulse ISR or the high-priority measurement task.
3. **Safety Timeout:**
   - Implement a 10ms watchdog timer that fires if the 33-byte burst doesn't complete after a trigger.

## Phase 3: HMI Integration
**Target Files:** `lgc_hmi_task.c`

1. **Page Detection:**
   - **HMI_PAGE1 (Production):** Switch to `BUS_MODE_DAISY_CHAIN`.
   - **HMI_PAGE3/4 (Sensor Test):** Switch to `BUS_MODE_MODBUS`.
   - **Other Config Pages:** Default to `BUS_MODE_MODBUS`.
2. **Event Notification:**
   - Use `osSetEventBits` to signal `lgc_main_task` about mode transitions.

## Phase 4: Data Processing
**Target Files:** `lgc_measurements.c`

1. **Burst Parser:**
   - Create `error_t lgc_parse_sensor_burst(uint8_t *raw_data, uint16_t len)`.
   - Logic: Verify 11 IDs (1 to 11) and extract 10 bits of photodiode data from each packet.
   - Map bits to the global `measurement_slice` structure.

## Verification Checklist
- [ ] Verify trigger pulse width (Oscilloscope).
- [ ] Confirm UART3 RX idle detection timing.
- [ ] Validate that Modbus polling is completely inactive during `PAGE1`.
- [ ] Measure total cycle time (Target < 5ms).
