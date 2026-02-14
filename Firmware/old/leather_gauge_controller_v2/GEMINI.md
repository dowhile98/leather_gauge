# Leather Gauge Controller V2 - Project Context

This document provides a comprehensive overview of the `Leather Gauge Controller V2` firmware project, detailing its purpose, architecture, development practices, and build process. This context is essential for any future interactions with the codebase.

## Project Overview

The **Leather Gauge Controller** is an embedded firmware solution for high-precision, automatic measurement of leather piece areas in industrial settings. It utilizes an **STM32F446RCTx** microcontroller, operating with the **Azure ThreadX RTOS**. The system processes data from 11 photoelectric sensors (110 photocells total) via an LwPKT cascade communication protocol, synchronizing measurements with a rotary encoder.

The project is currently undergoing a significant **refactoring effort** to transition from a monolithic design to a **Clean Architecture** based on **SOLID principles**. This aims to enhance modularity, testability, and maintainability.

**Key Features:**

*   **High-Precision Measurement:** Integration by "slices" synchronized with an encoder.
*   **Multi-tasking:** Azure ThreadX RTOS for concurrent and deterministic operation.
*   **Robust Communication:** LwPKT cascade for sensor data acquisition.
*   **Human-Machine Interface (HMI):** DWIN tactile display for real-time visualization and configuration.
*   **Persistent Storage:** EEPROM I2C for configuration and batch data.
*   **Integrated RTC:** Real-Time Clock for synchronized date/time.

## Architecture

The project is actively migrating to a **Clean Architecture** model, structured into distinct layers to enforce dependency rules (dependencies point inwards).

**Architectural Layers (Target):**

1.  **Presentation Layer (`leather_gauge_controller/app`):** Handles UI (HMI, Printer) and contains the Dependency Injection (DI) Container.
2.  **Domain Layer (Core - `leather_gauge_controller/domain`):** Contains pure business logic, entities, and use cases, completely decoupled from hardware.
3.  **Interface Layer (Ports - `leather_gauge_controller/domain/interfaces`):** Defines abstractions (interfaces) that the Domain Layer depends on.
4.  **Infrastructure Layer (Adapters - `leather_gauge_controller/adapters`):** Implements the interfaces defined in the Domain Layer, containing hardware-specific logic and third-party middleware integrations.
5.  **Hardware Abstraction Layer (HAL + RTOS):** STM32 HAL drivers, Azure ThreadX, and various third-party libraries (nanoMODBUS, lwrb, lwbtn, dwin, at24cxx).

The project uses the **Observer Pattern** (Publisher-Subscriber) for reactive event handling, reducing coupling and improving CPU utilization compared to polling.

## Building and Running

The project can be built using STM32CubeIDE or via command-line `make` commands.

### Requirements

*   **STM32CubeIDE** 1.x or higher
*   **GNU ARM Embedded Toolchain** 13.2.1 or compatible
*   **STM32CubeMX** (included in STM32CubeIDE)

### Building (Command Line)

To build the project from the command line:

```bash
# Navigate to the project root directory
cd /home/tecna-smart-lab/GitHub/leather_gauge/Firmware/leather_gauge_controller_v2

# Build Debug configuration
make -C Debug clean
make -C Debug all -j$(nproc)
```

The output binaries will be generated in `Debug/`.

### Flashing and Debugging

*   **Via STM32CubeIDE:** Connect an ST-LINK debugger, then right-click the project -> `Run As` -> `STM32 C/C++ Application` or `Debug` (F11).
*   **Via Command Line (ST-LINK):**
    ```bash
    st-flash write Debug/leather_gauge_controller.bin 0x08000000
    ```
*   **Via Command Line (OpenOCD):**
    ```bash
    openocd -f interface/stlink.cfg -f target/stm32f4x.cfg \
      -c "program Debug/leather_gauge_controller.elf verify reset exit"
    ```

## Testing

The project employs a multi-faceted testing strategy:

*   **Unit Testing:** Using **Unity + CMock** for testing the Domain and Use Cases layers on a PC, mocking interfaces.
*   **Integration Testing:** Performed on actual hardware to validate adapters and full system functionality (latency, stress tests, precision).
*   **Regression Testing:** Comparing measurements against a baseline legacy system.

## Development Conventions

Adherence to strict coding standards and architectural principles is paramount.

### Directory Structure (New Architecture)

The `lgc_controller/` directory is central to the new architecture:

```
lgc_controller/
├── domain/            # Core business logic (entities, use_cases, interfaces)
├── adapters/          # Implementations of interfaces (communication, peripherals, storage)
├── app/               # Application layer (DI Container, main tasks, HMI, printer)
├── middlewares/       # Third-party libraries
├── osal/              # OS abstraction layer
└── config/            # Project-specific configurations
```

### SOLID Principles in Embedded C

The project applies SOLID principles rigorously:

*   **Single Responsibility Principle (SRP):** Each module (e.g., Use Case, Adapter) has one reason to change.
*   **Open/Closed Principle (OCP):** Modules are open for extension (e.g., adding a new sensor protocol by implementing an interface) but closed for modification.
*   **Liskov Substitution Principle (LSP):** Implementations of an interface (e.g., `ISensorReader`) must fulfill its contract.
*   **Interface Segregation Principle (ISP):** Clients depend only on the interfaces they use, leading to smaller, more cohesive interfaces.
*   **Dependency Inversion Principle (DIP):** High-level modules (Domain) depend on abstractions (Interfaces), not on low-level modules (Adapters). Dependencies are injected via a DI Container.

### Coding Rules and Best Practices

*   **Memory Management:** Strictly **static allocation** or ThreadX Byte Pools; `malloc`/`free` are forbidden.
*   **Error Handling:** All functions return `Result_t` enums to indicate success or specific error conditions.
*   **Includes:** No direct includes of `stm32f4xx_hal.h` or other hardware-specific headers within the `domain/` layer.
*   **Global Variables:** Mutable global variables in the Core are prohibited; pass data via parameters or use thread-safe data structures.
*   **Data Types:** Use `stdint.h` types (`uint8_t`, `int16_t`, etc.) and `stdbool.h` (`bool`, `true`, `false`).
*   **Documentation:** All public functions, structs, and interfaces **must** be documented using Doxygen.

### Naming Conventions

*   **Files:** `snake_case` (e.g., `lgc_uc_measure_area.c`).
*   **Types/Structs:** `PascalCase_t` (e.g., `LgcMeasurement_t`).
*   **Interfaces:** `I` + `PascalCase` + `_t` (e.g., `ILgcSensorReader_t`).
*   **Public Functions:** `Module_Action` (e.g., `LgcUC_ProcessSlice`).
*   **Private Functions:** `snake_case` (static).
*   **Variables:** `snake_case` (local), `s_` + `snake_case` (static).
*   **Constants/Macros:** `UPPER_SNAKE_CASE`.

### Version Control and Contribution

*   **Git Flow:** Development follows a Git Flow-like model with feature branches.
*   **Conventional Commits:** Commit messages adhere to standards (e.g., `feat:`, `fix:`, `refactor:`).
*   **Code Review:** All changes are subject to code review.
*   **AI Agent Guidelines:** Follow the guidelines in `.github/copilot-instructions.md`.

## Further Documentation

*   [`REFACTOR_PLAN.md`](REFACTOR_PLAN.md): Detailed plan for the Clean Architecture refactoring.
*   [`docs/SYSTEM_ARCHITECTURE.md`](docs/SYSTEM_ARCHITECTURE.md): Comprehensive system architecture documentation.
*   [`docs/sensor/README.md`](docs/sensor/README.md): Documentation for photoelectric sensors.
*   [`.github/copilot-instructions.md`](.github/copilot-instructions.md): Specific instructions for AI agents regarding coding standards, TDD, and SOLID.