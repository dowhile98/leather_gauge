# Leather Gauge Controller Firmware - Copilot Instructions

## Project Overview

Industrial embedded firmware for **STM32F446RCTx** implementing an automatic leather measurement system. Uses **Azure ThreadX RTOS** with modular architecture and hardware abstraction layers.

**Purpose:** Measure leather piece area in real-time using 11 Modbus sensors (110 photocells) synchronized with a rotary encoder on a conveyor system.

## Architecture Layers (leather_gauge_controller/)

```
app/           → Application logic: main_task, hmi, printer, sensor processing
modules/       → Hardware abstraction: encoder, modbus, eeprom, digital inputs
osal/          → OS Abstraction Layer (supports 14 different RTOS including ThreadX)
Core/          → STM32 HAL initialization (CubeMX generated)
Middlewares/   → Azure ThreadX RTOS + USBX stack
```

## System Architecture

### RTOS Tasks

| Task | Priority | Stack | Responsibility |
|------|----------|-------|----------------|
| **lgc_main_task** | 10 | 256 words | Core measurement algorithm, encoder sync, Modbus sensor reading |
| **lgc_hmi_task** | 11 | 512 words | DWIN display updates, user interface |
| **lgc_printer_task** | 14 | 512 words | ESC/POS thermal printer reports |

### State Machine

```c
typedef enum {
    LGC_STOP = 0,      // Idle, waiting for START button
    LGC_RUNNING,       // Actively measuring leather
    LGC_FAIL,          // Failure detected (GUARD triggered)
} LGC_State_t;
```

### Measurement Algorithm (Slice Integration)

1. **Encoder pulse triggers ISR** → releases semaphore
2. **Main task reads 11 Modbus sensors** (register 0x2D)
3. **Count active bits** (photocells detecting leather)
4. **Calculate slice area:** `active_bits × 10mm × 5mm`
5. **Accumulate to current leather area**
6. **Detect leather end:** 3 consecutive empty slices (hysteresis)
7. **Store measurement** and update batch counters

## Key Design Patterns

### Module-Based Hardware Abstraction

Each hardware peripheral has a dedicated module with init + callback:

```c
// Example: Encoder module
error_t lgc_module_encoder_init(encoder_callback_t callback);

// Digital input module
error_t lgc_module_input_init(input_callback_t callback);
```

### OSAL Portability Pattern

All RTOS primitives abstracted through `os_port.h`:

```c
OsMutex mutex;
OsSemaphore encoder_flag;
OsEvent events;
OsTaskId task_id;

osCreateMutex(&mutex);
osWaitForSemaphore(&encoder_flag, timeout);
osSetEventBits(&events, LGC_EVENT_START);
```

**Supported RTOS:** ThreadX (active), FreeRTOS, µC/OS-II, µC/OS-III, CMSIS-RTOS, RTX, SafeRTOS, Zephyr, ChibiOS, embOS, PX5, Windows, POSIX, None.

### Callback Registration Pattern

Modules use callback functions for event notification:

```c
// Button callback
void lgc_buttons_callback(uint8_t button_id, uint32_t event) {
    if (event == LWBTN_EVT_ONPRESS) {
        // Handle button press
        osSetEventBits(&events, LGC_EVENT_START);
    }
}

// Encoder callback (called from ISR)
static void lgc_encoder_callback(void) {
    osReleaseSemaphore(&encoder_flag);  // Notify main task
}
```

### Thread-Safe Data Access

Protected data structures with mutex:

```c
// Global data protected by mutex
static lgc_t data;
static OsMutex mutex;

// Safe getter function
void lgc_get_measurements(lgc_measurements_t *out) {
    osAcquireMutex(&mutex, osWaitForever);
    memcpy(out, &measurements, sizeof(lgc_measurements_t));
    osReleaseMutex(&mutex);
}
```

## Naming Conventions (STRICT)

| Element | Convention | Example |
|---------|-----------|---------|
| **Files** | `snake_case` | `lgc_main_task.c`, `lgc_module_encoder.h` |
| **Module Prefix** | `lgc_` for all project files | `lgc_interface_modbus.c` |
| **Types/Structs** | `PascalCase_t` | `LGC_State_t`, `lgc_measurements_t` |
| **Public Functions** | `lgc_module_action` | `lgc_module_encoder_init`, `lgc_get_measurements` |
| **Private/Static Functions** | `lgc_snake_case` | `lgc_encoder_callback`, `lgc_process_measurement` |
| **Static Variables** | `s_` prefix (file-local) | `s_modbus_buffer`, `s_is_initialized` |
| **Global Variables** | Avoid (use getters) | `extern OsEvent events;` |
| **Macros/Constants** | `LGC_ALL_CAPS` | `LGC_ENCODER_STEP_MM`, `LGC_EVENT_START` |
| **OSAL Types** | `OsPascalCase` | `OsMutex`, `OsSemaphore`, `OsTaskId` |

## Error Handling

All initialization and I/O functions return `error_t` from `error.h`:

```c
typedef enum {
    NO_ERROR = 0,
    ERROR_FAILURE,
    ERROR_INVALID_PARAMETER,
    ERROR_TIMEOUT,
    ERROR_BUSY,
    ERROR_NOT_IMPLEMENTED,
    // ... others
} error_t;
```

**Always check return values:**

```c
error_t err = lgc_module_encoder_init(callback);
if (err != NO_ERROR) {
    // Handle error
}
```


## Type Requirements

- Use `<stdint.h>`: `uint32_t`, `uint8_t`, `int16_t`, `size_t` (never generic `int`)
- Use `<stdbool.h>`: `bool`, `true`, `false`
- Use `<string.h>`: `memcpy`, `memset` for safe memory operations
- Always `NULL`-check pointers before dereferencing
- Use `const` for input parameters not modified
- Use `float` for measurements and calculations (hardware FPU available)

## Memory Management Rules

- **Static allocation preferred** - avoid `malloc`/`free` in runtime paths
- Use ThreadX byte pools for dynamic memory if needed:
  ```c
  TX_BYTE_POOL byte_pool;
  tx_byte_allocate(&byte_pool, (void **)&ptr, size, TX_NO_WAIT);
  ```
- Ring buffers for UART RX (lwrb library)
- Fixed-size arrays for measurements (MAX_LEATHERS=300, MAX_BATCHES=200)
- Stack size per task defined in constants (main=256 words, hmi=512 words)

## ISR Pattern (Deferred Processing)

**Rule:** ISRs must be minimal and fast. Use semaphore/event notification for deferred processing.

```c
// Example: Encoder ISR callback
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    if (GPIO_Pin == DI_0_INT_Pin) {  // Encoder pulse
        lgc_encoder_callback();       // Minimal callback
    }
}

// Encoder callback (called from ISR context)
static void lgc_encoder_callback(void) {
    osReleaseSemaphore(&encoder_flag);  // Signal main task
    // NO BLOCKING CALLS HERE
    // NO COMPLEX LOGIC HERE
}

// Main task processes the event
void lgc_main_task_entry(void *param) {
    for (;;) {
        if (osWaitForSemaphore(&encoder_flag, timeout) == TRUE) {
            // Do heavy processing here (Modbus read, calculations)
            lgc_process_measurement(&config);
        }
    }
}
```

**Never in ISR:**
- `osDelay()` or any blocking calls
- Heavy calculations or floating-point math
- Modbus/UART transactions
- Mutex acquire (deadlock risk)

## Communication Protocols

### Modbus RTU (Sensors)

```c
// Read 10 holding registers from sensor ID 1-11
// Register 0x2D (45) contains photocell data
error_t err = lgc_modbus_read_holding_regs(
    sensor_id,           // 1-11
    0x2D,               // Register address
    &data.sensor[i],    // Output buffer
    LGC_PHOTORECEPTORS_PER_SENSOR  // 10 registers
);

// Configuration:
// - Baudrate: 9600 bps
// - UART3 with DMA
// - Timeout: 100ms
// - Retries: 4
// - Direction control: DIR_SENSORES pin (RS-485)
```

### DWIN Display Protocol

```c
// Update display variable
dwin_var_write(&dwin, addr, value, size);

// Example: Update leather count
dwin_var_write(&dwin, DWIN_ADDR_LEATHER_COUNT, 
               &data.leather_count, sizeof(uint32_t));

// Configuration:
// - USART6
// - Baudrate: 115200 bps
// - Direction control: DIR_DISPLAY pin
```

### ESC/POS Printer

```c
// Print batch report
ESP_POS_Printer_Init(&printer);
ESP_POS_Printer_SetAlign(&printer, ESC_POS_ALIGN_CENTER);
ESP_POS_Printer_Text(&printer, "LEATHER MEASUREMENT REPORT");
ESP_POS_Printer_PrintAndFeed(&printer, 3);
```

## File Organization

| Path | Purpose |
|------|---------|
| `Core/` | STM32CubeMX generated (main.c, stm32f4xx_it.c, HAL init) |
| `leather_gauge_controller/app/` | Application tasks (main, hmi, printer) |
| `leather_gauge_controller/modules/` | Hardware abstraction modules |
| `leather_gauge_controller/osal/` | OS Abstraction Layer (14 RTOS ports) |
| `leather_gauge_controller/config/` | Configuration headers |
| `Middlewares/ST/threadx/` | Azure ThreadX RTOS |
| `Drivers/STM32F4xx_HAL_Driver/` | STM32 HAL |
| `docs/` | System architecture documentation |

## Key Measurement Algorithm Constants

```c
#define LGC_PIXEL_WIDTH_MM 10.0f           // Photocell width
#define LGC_ENCODER_STEP_MM 5.0f           // Encoder distance per pulse
#define LGC_LEATHER_END_HYSTERESIS 3       // Empty steps to end detection
#define LGC_SENSOR_READ_RETRY 4            // Modbus retry attempts
#define LGC_PHOTORECEPTORS_PER_SENSOR 10   // Photocells per sensor
```

**Slice Area Calculation:**
```c
float slice_area = active_bits * LGC_PIXEL_WIDTH_MM * LGC_ENCODER_STEP_MM;
// Example: 50 bits active → 50 × 10mm × 5mm = 2500 mm²
```

## Code Generation Guidelines

When writing new code for this project:

### 1. **Module Structure** (for new hardware drivers)

```c
// lgc_module_newdriver.h
#ifndef LGC_MODULE_NEWDRIVER_H
#define LGC_MODULE_NEWDRIVER_H

#include "error.h"
#include <stdint.h>

// Callback type
typedef void (*newdriver_callback_t)(uint32_t event);

// Public API
error_t lgc_module_newdriver_init(newdriver_callback_t callback);
error_t lgc_module_newdriver_read(uint8_t *data, size_t len);
error_t lgc_module_newdriver_write(const uint8_t *data, size_t len);

#endif // LGC_MODULE_NEWDRIVER_H
```

```c
// lgc_module_newdriver.c
#include "lgc_module_newdriver.h"
#include "os_port.h"

// Static variables
static newdriver_callback_t s_callback = NULL;
static bool s_initialized = false;

// Implementation
error_t lgc_module_newdriver_init(newdriver_callback_t callback) {
    if (callback == NULL) {
        return ERROR_INVALID_PARAMETER;
    }
    
    s_callback = callback;
    s_initialized = true;
    
    return NO_ERROR;
}
```

### 2. **Task Structure** (for new RTOS tasks)

```c
// Task entry function
void lgc_newtask_entry(void *param) {
    // Initialization
    error_t err = lgc_module_init();
    if (err != NO_ERROR) {
        // Handle error
    }
    
    // Main loop
    for (;;) {
        // Wait for event
        if (osWaitForEventBits(&events, EVENT_FLAG, TRUE, TRUE, timeout)) {
            // Process event
        }
        
        // Small delay to prevent starvation
        osDelay(10);
    }
}
```

### 3. **Thread-Safe Data Access**

```c
// Always use mutex for shared data
void lgc_set_data(uint32_t value) {
    osAcquireMutex(&mutex, osWaitForever);
    data.field = value;
    osReleaseMutex(&mutex);
}

uint32_t lgc_get_data(void) {
    uint32_t value;
    osAcquireMutex(&mutex, osWaitForever);
    value = data.field;
    osReleaseMutex(&mutex);
    return value;
}
```

### 4. **State Machine Implementation**

```c
switch (lgc_get_state()) {
    case LGC_STOP:
        // Handle STOP state
        if (start_condition) {
            lgc_set_state(LGC_RUNNING);
        }
        break;
        
    case LGC_RUNNING:
        // Handle RUNNING state
        if (stop_condition) {
            lgc_set_state(LGC_STOP);
        } else if (failure_condition) {
            lgc_set_state(LGC_FAIL);
        }
        break;
        
    case LGC_FAIL:
        // Handle FAIL state
        if (recovery_condition) {
            lgc_set_state(LGC_STOP);
        }
        break;
}
```

### 5. **Event-Driven Communication**

```c
// Event definitions
#define LGC_EVENT_START             (1U << 0)
#define LGC_EVENT_STOP              (1U << 1)
#define LGC_FAILURE_DETECTED        (1U << 2)
#define LGC_HMI_UPDATE_REQUIRED     (1U << 3)
#define LGC_PRINTER_EVENT           (1U << 4)

// Producer (main task)
osSetEventBits(&events, LGC_HMI_UPDATE_REQUIRED);

// Consumer (HMI task)
if (osWaitForEventBits(&events, LGC_HMI_UPDATE_REQUIRED, 
                       TRUE, TRUE, timeout) == TRUE) {
    // Update display
    lgc_hmi_update_display();
}
```

## Testing Strategy

**Current Status:** No automated tests (manual testing only)

**Recommended for new code:**

```c
// Unit test structure (Unity framework)
void test_lgc_calculate_slice_area(void) {
    float result = lgc_calculate_slice_area(50);
    TEST_ASSERT_EQUAL_FLOAT(2500.0f, result);
}

// Mock Modbus for testing
error_t mock_modbus_read(uint8_t dev, uint16_t addr, 
                         uint16_t *regs, size_t len) {
    // Return test data
    memset(regs, 0xFF, len * sizeof(uint16_t));
    return NO_ERROR;
}
```

## Documentation Style

Use Doxygen comments for public APIs:

```c
/**
 * @brief Initialize encoder module with callback
 * 
 * Configures GPIO EXTI interrupt for encoder pulses.
 * Callback is executed in ISR context.
 * 
 * @param callback Function called on each encoder pulse (ISR context)
 * @return error_t NO_ERROR on success, ERROR_INVALID_PARAMETER if callback is NULL
 * 
 * @note Callback must be fast and non-blocking
 * @note Call before starting main measurement loop
 */
error_t lgc_module_encoder_init(encoder_callback_t callback);
```

## Common Pitfalls to Avoid

1. **Don't block in ISR context**
   ```c
   // BAD
   void ISR_Callback(void) {
       osDelay(100);  // NEVER!
       lgc_modbus_read(...);  // NEVER!
   }
   
   // GOOD
   void ISR_Callback(void) {
       osReleaseSemaphore(&flag);  // Signal task
   }
   ```

2. **Don't forget mutex for shared data**
   ```c
   // BAD
   data.counter++;  // Race condition!
   
   // GOOD
   osAcquireMutex(&mutex, osWaitForever);
   data.counter++;
   osReleaseMutex(&mutex);
   ```

3. **Always check return values**
   ```c
   // BAD
   lgc_modbus_read_holding_regs(dev, addr, regs, len);
   
   // GOOD
   error_t err = lgc_modbus_read_holding_regs(dev, addr, regs, len);
   if (err != NO_ERROR) {
       // Retry or handle error
   }
   ```

4. **Don't use HAL blocking functions in tasks**
   ```c
   // BAD
   HAL_UART_Transmit(&huart, data, len, HAL_MAX_DELAY);
   
   // GOOD - Use DMA + callback or IT mode
   HAL_UART_Transmit_DMA(&huart, data, len);
   ```

## References

- **System Architecture:** See [docs/SYSTEM_ARCHITECTURE.md](../docs/SYSTEM_ARCHITECTURE.md)
- **README:** See [README.md](../README.md)
- **STM32F446 Reference:** [RM0390](https://www.st.com/resource/en/reference_manual/dm00135183.pdf)
- **ThreadX Documentation:** [Eclipse ThreadX](https://github.com/eclipse-threadx/rtos-docs)
- **nanoMODBUS:** [GitHub](https://github.com/debevv/nanoMODBUS)

---

**When in doubt:** Follow the existing patterns in `lgc_main_task.c`, `lgc_module_encoder.c`, and `lgc_interface_modbus.c`. These files exemplify the project's coding style and architecture.

