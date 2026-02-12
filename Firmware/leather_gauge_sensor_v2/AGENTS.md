# AGENT HANDBOOK — LEATHER GAUGE SENSOR V2

This repository targets STM32G030C8 (CubeIDE generated) and is mid‑refactor toward Clean Architecture with LwPKT. Agents must enforce TDD, SOLID, and zero dynamic allocation.

## Scope

Use this guide for coding, refactors, docs, and automation performed by agentic assistants. Follow all sections; when conflicts arise, Copilot rules take precedence.

## Quick Start

- Build firmware (default Debug): `make -C Debug all`
- Clean artifacts: `make -C Debug clean`
- Generate .list/.map are produced by the default build; size via `arm-none-eabi-size Debug/leather_gauge_sensor_v2.elf`
- Flash/debug: use STM32CubeIDE or your usual OpenOCD/SWD workflow (not scripted here).
- Tests: No automated tests currently present. New code must add Unity/CMock tests first (see Testing Expectations).

## Repository Layout (high level)

- `Core/`, `Drivers/`: Cube HAL sources.
- `leather_gauge_sensor/app`: composition root (`leather_gauge.c`).
- `leather_gauge_sensor/core`: domain orchestration (`lg_core.*`).
- `leather_gauge_sensor/interfaces`: port definitions.
- `leather_gauge_sensor/adapters`: HAL-backed implementations (sensor/comms/storage).
- `Third_Party/`: `lwpkt`, `lwrb`, `DSP_Biquad`.
- `.github/agents`: prewritten agent prompts.
- `.github/copilot-instructions.md`: authoritative style/architecture rules.

## Build & Tooling

- Compiler: `arm-none-eabi-gcc` (GNU Tools for STM32 13.3.rel1) with `-mthumb -mcpu=cortex-m0plus` and nano specs.
- Linker script: `STM32G030C8TX_FLASH.ld` at repo root.
- Primary build: `make -C Debug all` (auto includes subdir.mk fragments). Do not hand-edit `Debug/makefile`.
- To rebuild a single file, delete its object (e.g., `rm Debug/leather_gauge_sensor/core/lg_core.o`) then run `make -C Debug`.
- No lint/format scripts are defined; apply style rules manually (see Style Guide).

## Testing Expectations

### Unit Tests (Firmware - Pending)

- Current tree lacks test harness files (no Unity/CMock sources found).
- Mandate from Copilot rules: practice TDD; write failing Unity/CMock tests before implementation for new logic.
- Proposed pattern for new tests (to add):
  - Place domain-level tests under `tests/` (PC-buildable, no HAL includes).
  - Use mocks for adapters (communication, sensor, storage) to keep core pure.
  - Example single test invocation (after you add runner): `ctest -R name` or `./build/tests/test_suite --gtest_filter=Suite.Test` — choose and document when introduced.
- Until tests exist, clearly justify any change that cannot be covered and leave TODOs with planned test names.

### Hardware Tests (PC-based via RS-485)

**Status:** ✅ IMPLEMENTED (2026-02-09)

Python scripts for hardware validation with RS-485 USB converters:

**Files:**

- `main.py` - Full test suite with CLI args
- `quick_test.py` - Quick validation script (edit PORT and run)
- `lwpkt.py` - LwPKT protocol encoder/decoder (Python implementation)
- `requirements.txt` - Python dependencies (`pyserial>=3.5`)
- `TESTING_PC.md` - Complete documentation

**Usage:**

```bash
# Install dependencies
pip install -r requirements.txt

# Quick test (edit PORT in quick_test.py first)
python quick_test.py

# Individual sensor read
python main.py --port /dev/ttyUSB0 --mode individual --sensor 1

# Cascade read (all sensors)
python main.py --port /dev/ttyUSB0 --mode cascade --sensors 11

# Performance comparison
python main.py --port /dev/ttyUSB0 --mode compare --sensors 11

# Interactive mode
python main.py --port /dev/ttyUSB0 --mode interactive
```

**Common Issues:**

- Linux permissions: `sudo usermod -a -G dialout $USER` (logout/login required)
- Timeout errors: Check A-B wiring, 120Ω termination, 12-24VDC power
- CRC errors: Add twisted pair cable, reduce cable length
- Port not found: Verify converter with `ls /dev/ttyUSB*` or `dmesg | tail`

See `TESTING_PC.md` for complete troubleshooting guide.

## Architectural Mandates (from Copilot instructions)

- **Role:** Act as Senior Embedded Architect; prioritize correctness (tests) > maintainability > optimization.
- **TDD:** No code without a failing test first (Red/Green/Refactor cadence).
- **SOLID:** Enforce SRP, OCP, LSP, ISP, DIP. High-level modules never include HAL headers.
- **Clean Architecture boundaries:**
  - Core/domain must be HAL-free and PC-compilable.
  - Infrastructure/adapters may include HAL and third-party libs.
  - Application layer wires dependencies; uses dependency injection, not global lookups.
- **Hardware abstraction:** Expose drivers via v-table structs; prefer interfaces named `I*Interface_t`.
- **Concurrency:** Keep ISRs minimal; defer work to main loop/queues. Never block in ISR.

## Style Guide (C99/C11, per Copilot rules)

- **Types:** Always use stdint/stddef/stdbool types (`uint8_t`, `bool`). Avoid native `int/long` for logic. `char` only for text buffers.
- **Memory:** Dynamic allocation (`malloc/calloc/free`) is forbidden. Use static or pooled memory; pass buffer sizes explicitly.
- **Pointers:** Validate public API pointers against NULL at entry; enforce const-correctness for read-only buffers.
- **Naming:**
  - Files: `snake_case` (e.g., `gps_driver.c`).
  - Types/structs: `PascalCase_t` (`GpsData_t`).
  - Interfaces: `I` + `PascalCase_t` (`ISensorInterface_t`).
  - Public functions: `Module_Action` (`Gps_GetData`).
  - Private static functions: `snake_case`.
  - Statics: prefix `s_` (e.g., `s_is_initialized`).
  - Macros/constants: `UPPER_SNAKE_CASE`.
- **Functions:** Keep one responsibility; inject dependencies via `Init(const IInterface_t *dep)`; avoid hidden globals.
- **Error handling:** Use `Result_t`/`lg_result_t` style enums (`ERR_OK`, `ERR_INVALID_PARAM`, `LG_ERROR`); callers must check returns. When signaling protocol errors, prefer explicit response codes (see `lg_core.c` usage of `cmd | 0x80`).
- **Documentation:** Public APIs require Doxygen blocks with `@brief`, `@param[in]`, `@return`, and notes on thread-safety and NULL requirements.
- **Formatting:** Stay consistent with existing code (brace on same line as control, 4-space indents common in Cube output). Keep lines <= 120 chars. Avoid trailing whitespace.
- **Includes:** Core must not include HAL headers (`stm32g0xx_hal.h` etc.). Prefer forward declarations in headers; include order: own header, std headers, project headers.

## Domain & Adapter Guidance

- Core orchestrates sensor/comms/storage via interfaces (`lg_core.*`). Maintain pure data structs (`lg_domain_types.h`).
- Adapters (`adapters/*`) encapsulate HAL access; ensure they satisfy interfaces in `interfaces/*` and stay substitutable (LSP).
- When modifying protocol handling (`lg_core.c`), keep command decoding centralized in `handle_command`; ensure payload validation before use.
- Config changes must persist via storage adapter (`save_config`) and update running adapter state (consider adding `set_offset`/`set_calibration` interface to avoid re-init).

## Third-Party Libraries

- `lwpkt` + `lwrb`: used for RS-485 packetization/ring buffer. Hook DE pin in `evt_fn` (pre/post write) for RS-485 driver enable.
- `DSP_Biquad`: used for sensor filtering; keep inside sensor adapter and avoid leaking DSP types into domain.

## Dependency Rules

- Only adapters may include HAL or touch GPIO/UART/DMA. Core/domain must remain HAL-free.
- No direct HAL calls from application logic outside adapter boundaries.
- Keep globals minimal; prefer context structs passed through APIs. If static state is required, namespace with `s_` and document reentrancy.

## Error Handling & Safety

- Validate buffer lengths and command payload sizes before memcpy. Reject invalid lengths with `ERR_INVALID_PARAM` and communicate to caller.
- In ISRs, do minimal work: copy data, set flags/queues, return quickly (<50 µs target noted for PPS callbacks in Copilot rules).
- Avoid blocking waits; prefer non-blocking polls in super loop.

## Logging/Tracing

- No logging framework present. If instrumentation needed, guard with compile-time flags and keep ISR-safe (no printf in ISR).

## Configuration

- Config header: `leather_gauge_sensor/config/leather_gauge_config.h` governs application constants. Update with care; ensure defaults sync with storage schema.

## Protocol Notes (LwPKT Migration)

- **Status:** Migration COMPLETED (2026-02-09)
- Command map defined in `lg_domain_types.h`:
  - Read: 0x10-0x12 (Individual + Cascade modes)
  - Write/Config: 0x20-0x22
  - Control: 0x30-0x31
  - Error: 0xFF
- Response pattern: Echo `cmd` with data; errors send `cmd | 0x80` + 1-byte error code
- **Cascade Protocol:** Uses LwPKT FLAGS field (32-bit) for broadcast sequencing
  - Master sends: `CMD_READ_CASCADE + FLAGS=1` (broadcast to 0xFF)
  - Each sensor checks: `if (FLAGS == my_address) respond + set FLAGS=next_address`
  - Performance: ~550ms for 11 sensors (vs 1.5s individual mode)
- See `docs/CASCADE_READ_PROTOCOL.md` for full specification

## Documentation & Diagrams

- Use Doxygen for public headers; add Mermaid diagrams for complex interactions when touching architecture docs (see `.github/agents/api-documenter.agent.md`).

## Working with Cube-Generated Files

- Do not hand-edit `Debug/makefile`, `objects.mk`, or `subdir.mk` files; regenerate via CubeIDE if build graph changes.
- Keep HAL initializers in `Core/Src` unless refactoring into adapters; if moved, preserve CubeMX regeneration safety (use `/* USER CODE BEGIN */` blocks when necessary).

## Commits & Branches

- Follow repository’s commit style (inspect `git log` when committing). Do not commit secrets. Only commit when explicitly requested.

## When Adding Tests (recommended template)

- Layout example:
  - `tests/` (PC build): domain tests using Unity/CMock.
  - `tests/mocks/` for interface mocks generated by CMock.
- Build command example (after you add CMake): `cmake -S tests -B build/tests && cmake --build build/tests`.
- Run single test (example): `ctest --test-dir build/tests -R lg_core_handles_filter_update`.
- Keep tests hardware-free; mock HAL-dependent adapters.

## Contributions Checklist for Agents

- Uphold Copilot instructions: TDD first, Clean Architecture boundaries, no dynamic allocation.
- Maintain naming and type rules; enforce const-correctness.
- Validate inputs and sizes; return meaningful `Result_t` codes.
- Keep ISR sections minimal; push work to main loop.
- Update docs/Doxygen when touching public APIs; mention protocol impacts.
- Prefer small, composable modules; avoid god objects.

## Known Gaps / TODOs for Agents

- Unity/CMock framework pending installation (tests written, need runner)
- Sensor offset updates saved but not applied live (need `ISensor::set_calibration()` method)
- Composition root (`leather_gauge.c`) needs refactor for dependency injection
- Hardware testing with 11 physical sensors via PC scripts available (see `main.py`)
- Logic analyzer validation of timing (<500ms target for cascade reads) - Scripts available for initial validation

## References

- Copilot rules: `.github/copilot-instructions.md` (authoritative style/architecture source).
- Refactor plan: `REFACTOR_PLAN.md`.
- Project context: `GEMINI.md`.
- Agent prompts: `.github/agents/*.agent.md` (c-pro, prompt-engineer, firmware-documenter) for role expectations.
