# LwPKT Codec Unit Tests

## Overview

This directory contains **TDD-first unit tests** for the LwPKT codec interface (`lg_i_lwpkt.h`).

## Test Philosophy (TDD)

1. **RED**: Tests written first (currently failing because implementation incomplete)
2. **GREEN**: Minimal implementation to pass tests
3. **REFACTOR**: Clean up code while keeping tests green

## Prerequisites

### Unity Testing Framework

The tests require [Unity](https://github.com/ThrowTheSwitch/Unity) test framework.

**Option 1: System Install**

```bash
# Ubuntu/Debian
sudo apt-get install unity-dev

# Or manually:
cd /home/tecna-smart-lab/GitHub/leather_gauge/Firmware/leather_gauge_sensor_v2/Third_Party
git clone https://github.com/ThrowTheSwitch/Unity.git
```

**Option 2: Update CMakeLists.txt**
Uncomment Unity source inclusion in `CMakeLists.txt` after downloading.

### Compiler

- GCC (native x86_64, NOT arm-none-eabi)
- CMake 3.15+

## Build & Run

### Step 1: Configure

```bash
cd /home/tecna-smart-lab/GitHub/leather_gauge/Firmware/leather_gauge_sensor_v2/tests
mkdir -p build && cd build
cmake ..
```

### Step 2: Build

```bash
cmake --build .
```

### Step 3: Run Tests

```bash
# Run all tests
ctest --output-on-failure

# Run with verbose output
./test_lwpkt_codec
```

## Expected Results

### Current Status (RED Phase)

Tests **SHOULD FAIL** initially because:

- LwPKT library API needs validation
- Codec implementation may need adjustments
- Ring buffer behavior may differ from expectations

### Target Status (GREEN Phase)

All tests pass:

```
test_lwpkt_codec.c:XX:test_Encode_ValidCommand_NoPayload_ReturnsOK:PASS
test_lwpkt_codec.c:XX:test_Decode_ValidFrame_ExtractsFields:PASS
...
-----------------------
XX Tests 0 Failures 0 Ignored
OK
```

## Test Coverage

### Encode Tests

- ✅ Valid command without payload
- ✅ Valid command with payload
- ✅ NULL buffer rejection
- ✅ Buffer too small detection

### Decode Tests

- ✅ Valid frame parsing
- ✅ CRC validation (corrupted frame rejection)
- ✅ NULL input rejection
- ✅ Zero-length frame rejection

### Round-Trip Tests

- ✅ Encode → Decode consistency for all command types

## Integration with Firmware Build

These tests are **PC-only** (no HAL dependencies). They validate:

1. Codec logic correctness
2. Interface contract compliance
3. Memory safety (static allocation only)

The actual firmware build (STM32) uses the same `lg_lwpkt_codec.c` but with:

- Hardware UART/DMA integration (separate test)
- RS-485 DE pin control (mocked in unit tests)

## Troubleshooting

### "undefined reference to Unity functions"

→ Install Unity framework (see Prerequisites)

### "lwpkt.h: No such file or directory"

→ Check include paths in CMakeLists.txt
→ Ensure LwPKT library exists in `Third_Party/lwpkt/`

### Test fails with "LG_ERROR on Encode"

→ Check LwPKT API return codes (may differ from documented)
→ Enable verbose logging in codec implementation

## Next Steps

After tests pass (GREEN phase):

1. Run static analysis: `cppcheck leather_gauge_sensor/adapters/comms_lwpkt/`
2. Measure code coverage: `gcov lg_lwpkt_codec.c`
3. Refactor for readability (keep tests green)
4. Integrate with adapter (`lg_adapter_comm.c`)

## Continuous Integration (Future)

Add to `.github/workflows/tests.yml`:

```yaml
- name: Run Unit Tests
  run: |
    cd tests
    mkdir build && cd build
    cmake ..
    cmake --build .
    ctest --output-on-failure
```

## References

- Interface Definition: `../leather_gauge_sensor/interfaces/lg_i_lwpkt.h`
- Implementation: `../leather_gauge_sensor/adapters/comms_lwpkt/lg_lwpkt_codec.c`
- LwPKT Library: `../Third_Party/lwpkt/`
- TDD Guidelines: `../.github/copilot-instructions.md`
