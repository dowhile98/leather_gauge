# Leather Gauge Sensor Firmware Context

## Project Overview
This project is the firmware for the **Leather Gauge Sensor v4**, based on the **STM32G030C8TX** microcontroller. The system is designed to measure leather thickness (or a similar physical property) using 10 sensor channels via ADC.

### Key Technologies
- **MCU:** STM32G030C8TX (ARM Cortex-M0+)
- **Framework:** STM32Cube HAL
- **Middlewares:**
  - **nanoMODBUS:** Compact Modbus RTU implementation.
  - **lwrb:** Lightweight Ring Buffer for efficient UART data handling.
  - **DSP Biquad:** Digital Signal Processing for ADC data filtering.
- **Protocols:** Modbus RTU over RS485 (USART1).
- **Persistence:** Flash Emulated EEPROM for configuration storage.

## Architecture
The firmware follows a modular structure separated into an application layer and hardware-abstracted modules:

- **Core/:** Standard STM32CubeIDE generated entry point and peripheral initialization.
- **leather_gauge_sensor/app/:** Main application logic (`leather_gauge.c`), coordinating initialization and the main execution loop.
- **leather_gauge_sensor/modules/:**
  - **sensor:** Handles ADC readings via DMA, triggered by TIM3. Implements Biquad low-pass filtering and threshold detection with hysteresis.
  - **modbus:** Implements the Modbus RTU server, mapping internal sensor data and configuration to Modbus registers and coils.
  - **eeprom:** Provides persistent storage of settings (offsets, baudrate, filter cutoff, etc.) by emulating EEPROM in the MCU's internal Flash (last 2KB page).
- **leather_gauge_sensor/config/:** Central configuration file (`leather_gauge_config.h`).

## Building and Running
This project is structured for **STM32CubeIDE**.

### Build Commands
- **Build (Debug):** `make -C Debug all` (assuming a standard STM32CubeIDE Makefile environment).
- **Clean:** `make -C Debug clean`

### Flashing
- Use STM32CubeProgrammer or OpenOCD with an ST-LINK debugger.
- Target Address: `0x08000000`

## Development Conventions
- **Module Pattern:** Each module has a clear `init` and `run/pool` or interface functions.
- **Naming Convention:** Functions are typically prefixed with `lg_module_` or `lg_sensor_`.
- **Interrupts:**
  - `HAL_ADC_ConvCpltCallback`: Processes raw ADC data immediately upon DMA completion.
  - `HAL_UARTEx_RxEventCallback`: Handles incoming UART data via Idle Line detection and DMA.
- **Error Handling:** Standard `Error_Handler()` is used for critical peripheral failures.

## Modbus Register Map (Inferred)
- **Coils:** Digital detection results for the 10 sensors.
- **Holding Registers:**
  - `0-9`: Sensor Offsets
  - `SERVER_ADDR`: Modbus Device Address
  - `FILTER_FC_ADDR`: Cutoff Frequency for the Biquad filter (scaled x10)
  - `SENSOR_THRESHOLD_ADDR`: Detection threshold
  - `S1-S10`: Filtered Sensor Values
  - `A1-A10`: Raw ADC Values
  - `D1-D10`: Processed (Offset-applied) Values
  - `DI_VALUE_ADDR`: Digital Bitfield of detections
