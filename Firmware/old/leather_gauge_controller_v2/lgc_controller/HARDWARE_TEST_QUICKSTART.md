# Hardware Testing Quick Start Guide

**Session 4.3 Complete** - Firmware ready for hardware validation  
**Last Updated:** February 12, 2026

---

## ✅ Prerequisites

- ✅ Firmware compiled successfully (Session 4.3)
- ✅ ISensorReader wrapper wired in DI Container
- ✅ Zero compilation errors validated
- ⏳ Hardware available (STM32F446RC + 11 RS-485 sensors)

---

## 🔧 Hardware Setup

### Required Components

| Component              | Specification                          | Quantity |
| ---------------------- | -------------------------------------- | -------- |
| **MCU**                | STM32F446RC (LQFP64)                   | 1        |
| **RS-485 Transceiver** | MAX485 or SN75176                      | 1        |
| **Sensors**            | Leather Gauge Sensors (LwPKT protocol) | 11       |
| **Encoder**            | Incremental rotary (1000 PPR)          | 1        |
| **Display**            | DWIN 7" TFT (UART, 115200 baud)        | 1        |
| **EEPROM**             | AT24C32 (I2C, 32Kbit)                  | 1        |
| **Power Supply**       | 12V DC, 2A minimum                     | 1        |
| **Debug Adapter**      | ST-Link V2 or V3                       | 1        |

### Wiring Diagram

```
STM32F446RC (LQFP64)              RS-485 Network (Twisted Pair)
┌──────────────────────┐          ┌────────────────────────────┐
│                      │          │                            │
│ PA2 (UART2_TX) ──────┼──────►DE │ MAX485                     │
│ PA3 (UART2_RX) ──────┼──────►RE │   A ─┬─────────────────────┼─► 120Ω termination
│                      │          │   B ─┘                     │
│ PA9 (UART1_TX) ──────┼──────────┼───────────────────────────►│ DWIN Display RX
│ PA10 (UART1_RX) ─────┼──────────┼◄──────────────────────────┤ DWIN Display TX
│                      │          │                            │
│ PA15 (TIM2_CH1) ─────┼──────────┼───────────────────────────►│ Encoder A (pull-up 10kΩ)
│ PB3 (TIM2_CH2) ──────┼──────────┼───────────────────────────►│ Encoder B (pull-up 10kΩ)
│                      │          │                            │
│ PB6 (I2C1_SCL) ──────┼──────────┼───────────────────────────►│ AT24C32 SCL (pull-up 4.7kΩ)
│ PB7 (I2C1_SDA) ──────┼──────────┼───────────────────────────►│ AT24C32 SDA (pull-up 4.7kΩ)
│                      │          │                            │
│ BOOT0 ──► GND        │          │ RS-485 Network:            │
│ NRST ──► 10kΩ → VCC  │          │   Sensor 1 (Addr 0x01)     │
│                      │          │   Sensor 2 (Addr 0x02)     │
└──────────────────────┘          │   ...                      │
                                   │   Sensor 11 (Addr 0x0B)    │
                                   └────────────────────────────┘
```

### Pin Configuration

| Pin      | Function    | Notes                      |
| -------- | ----------- | -------------------------- |
| **PA2**  | UART2_TX    | RS-485 (9600 baud, 8N1)    |
| **PA3**  | UART2_RX    | RS-485 (DMA enabled)       |
| **PA9**  | UART1_TX    | DWIN Display (115200 baud) |
| **PA10** | UART1_RX    | DWIN Display               |
| **PA15** | TIM2_CH1    | Encoder Channel A (EXTI)   |
| **PB3**  | TIM2_CH2    | Encoder Channel B (EXTI)   |
| **PB6**  | I2C1_SCL    | EEPROM (100 kHz, pull-up)  |
| **PB7**  | I2C1_SDA    | EEPROM (pull-up)           |
| **PC13** | GPIO_Output | LED status (optional)      |

### Sensor Addressing

**CRITICAL:** Verify each sensor has unique address (0x01-0x0B)

```
Sensor 1:  Address 0x01  (closest to leather entrance)
Sensor 2:  Address 0x02
Sensor 3:  Address 0x03
Sensor 4:  Address 0x04
Sensor 5:  Address 0x05
Sensor 6:  Address 0x06
Sensor 7:  Address 0x07
Sensor 8:  Address 0x08
Sensor 9:  Address 0x09
Sensor 10: Address 0x0A
Sensor 11: Address 0x0B  (closest to leather exit)
```

**How to verify addresses:**

1. Connect one sensor at a time to RS-485 bus
2. Send LwPKT READ_CONFIG command (0x31)
3. Response contains sensor address in ADDR field
4. If wrong, use WRITE_CONFIG command (0x21) to set correct address

---

## 🚀 Flashing Firmware

### Method 1: OpenOCD + ST-Link (Recommended)

```bash
# Navigate to project directory
cd /home/tecna-smart-lab/GitHub/leather_gauge/Firmware/leather_gauge_controller_v2

# Clean build
cd Debug
make clean

# Build firmware (8 parallel jobs)
make -j8

# Verify ELF file generated
ls -lh leather_gauge_controller.elf
# Expected output: ~250KB ELF file

# Flash via ST-Link V2/V3
openocd -f interface/stlink.cfg -f target/stm32f4x.cfg \
  -c "program leather_gauge_controller.elf verify reset exit"

# Expected output:
# Open On-Chip Debugger 0.11.0
# ...
# ** Programming Finished **
# ** Verify OK **
# shutdown command invoked
```

### Method 2: STM32CubeProgrammer (GUI)

1. Open STM32CubeProgrammer
2. Connect to ST-Link
3. Select ELF file: `Debug/leather_gauge_controller.elf`
4. Click **Download** (automatically erases + programs + verifies)
5. Click **Disconnect**
6. Press **RESET** button on board

### Verification

After flashing, device should:

- ✅ LED blinks (if connected to PC13)
- ✅ UART2 DMA starts (RS-485 reception enabled)
- ✅ Display shows boot screen (Page 1, VP_STATE = 0)

---

## 🔍 Debug Output Setup

### Connect Debug UART (Optional but Recommended)

**Option 1: USB-to-Serial Adapter**

```bash
# Connect adapter to UART1 (same as DWIN display)
# TX → PA10 (STM32 RX)
# RX → PA9 (STM32 TX)
# GND → GND

# Open serial terminal (Linux)
screen /dev/ttyUSB0 115200

# Or use minicom
minicom -D /dev/ttyUSB0 -b 115200

# Or use PuTTY (Windows)
# Serial port: COM3 (adjust as needed)
# Baud rate: 115200
# Data bits: 8, Parity: None, Stop bits: 1
```

**Option 2: SWV (Serial Wire Viewer)**

```bash
# In STM32CubeIDE or OpenOCD
# Enable ITM (Instrumentation Trace Macrocell)
# Clock: 168 MHz (STM32F446RC)
# SWO Frequency: 2 MHz

# View ITM Data Port 0 in real-time
```

### Expected Log Output (Boot Sequence)

```
[INFO] STM32F446RC - Leather Gauge Controller v2.0
[INFO] CPU: 168 MHz, Flash: 256KB, RAM: 128KB
[INFO] ThreadX RTOS initialized
[INFO] ===== DI Container Initialization =====
[INFO] LwPKT Agent initialized (UART2, 9600 baud)
[INFO] LwPKT Agent started (DMA reception enabled)
[INFO] LwPKT Sensor Reader wrapper initialized
[INFO] Encoder adapter initialized (TIM2, 1000 PPR)
[INFO] EEPROM adapter initialized (I2C1, AT24C32)
[INFO] Configuration loaded from EEPROM (CRC OK)
[INFO] Display adapter initialized (UART1, 115200 baud)
[INFO] Event Publisher initialized (max 8 observers)
[INFO] DI Container: All interfaces wired successfully ✅
[INFO] ===== Task Startup =====
[INFO] Main Task started (priority 10, stack 256 words)
[INFO] HMI Task started (priority 11, stack 512 words)
[INFO] ===== System Ready =====
[DEBUG] Waiting for encoder pulse...
```

---

## ✅ Test 1: Sensor Communication (CASCADE Read)

### Objective

Verify all 11 sensors respond to CASCADE command with correct FLAGS sequence.

### Procedure

**Step 1: Disable Encoder Dependency (Temporary)**

To test sensors without encoder, comment out encoder check in `lgc_main_task.c`:

```c
// In lgc_main_task.c - lgc_main_task_entry()
void lgc_main_task_entry(ULONG param)
{
    // ...initialization...

    while (1)
    {
        // TEMPORARY: Comment out encoder wait
        // tx_event_flags_get(&encoder_events, ENCODER_PULSE_FLAG,
        //                    TX_OR, &flags, TX_WAIT_FOREVER);

        // Instead: Manual trigger with 1s delay
        tx_thread_sleep(100);  // 1s @ 100 Hz tick

        // Read sensors (CASCADE mode)
        LgcSensorArray_t sensor_data;
        Result_t res = sensor_reader->read_cascade_mode(
            sensor_reader->context, &sensor_data);

        if (res == ERR_OK) {
            printf("[OK] Cascade read: %u sensors, %u ms\r\n",
                   sensor_data.number_of_sensors, elapsed_ms);
        } else {
            printf("[ERROR] Cascade read failed: %d\r\n", res);
        }
    }
}
```

**Step 2: Rebuild and Flash**

```bash
cd Debug
make clean && make -j8
openocd -f interface/stlink.cfg -f target/stm32f4x.cfg \
  -c "program leather_gauge_controller.elf verify reset exit"
```

**Step 3: Observe Serial Output**

Expected output (every 1 second):

```
[DEBUG] CASCADE command sent (CMD=0x12, ADDR=0xFF, LEN=0)
[DEBUG] TX: 01 FF 12 00 00 4A
[DEBUG] Waiting for 11 responses...
[DEBUG] RX Sensor 1: FLAGS=1, Data=0x03FF (10 bits active)
[DEBUG] RX Sensor 2: FLAGS=2, Data=0x0000 (no leather)
[DEBUG] RX Sensor 3: FLAGS=3, Data=0x0200 (bit 9 active)
...
[DEBUG] RX Sensor 11: FLAGS=0, Data=0x0040 (bit 6 active)
[INFO] Cascade complete: 11 sensors, 548ms elapsed ✅
[OK] Cascade read: 11 sensors, 548 ms
```

### Success Criteria

- ✅ All 11 sensors respond (no timeout)
- ✅ FLAGS sequence correct (1→2→3→...→11→0)
- ✅ Latency < 600ms (target: ~550ms)
- ✅ No CRC errors
- ✅ Task unblocks after semaphore signal

### Troubleshooting

| Issue                        | Possible Cause    | Solution                                |
| ---------------------------- | ----------------- | --------------------------------------- |
| **Timeout (ERR_TIMEOUT)**    | RS-485 wiring     | Check A/B polarity, swap if needed      |
| **Wrong FLAGS sequence**     | Sensor addressing | Verify each sensor addr (0x01-0x0B)     |
| **CRC error**                | Noise, baud rate  | Add shielding, verify 9600 baud exactly |
| **Only 1-5 sensors respond** | Weak termination  | Add 120Ω resistor at both ends of bus   |
| **Random garbage**           | DE/RE pin issue   | Tie DE/RE to VCC (always transmit mode) |

---

## ✅ Test 2: Encoder Synchronization

### Objective

Verify encoder pulse triggers sensor read exactly.

### Procedure

**Step 1: Re-enable Encoder Dependency**

Uncomment the encoder wait in `lgc_main_task.c`:

```c
// Restore original code
tx_event_flags_get(&encoder_events, ENCODER_PULSE_FLAG,
                   TX_OR, &flags, TX_WAIT_FOREVER);
```

**Step 2: Rebuild and Flash**

```bash
cd Debug && make -j8
openocd -f interface/stlink.cfg -f target/stm32f4x.cfg \
  -c "program leather_gauge_controller.elf verify reset exit"
```

**Step 3: Manually Rotate Encoder**

Slowly rotate encoder (1 pulse per second).

**Step 4: Observe Serial Output**

```
[DEBUG] Waiting for encoder pulse...
[ISR] Encoder pulse detected (position=1)
[INFO] Main task woken by encoder event
[DEBUG] Reading 11 sensors (CASCADE mode)...
[INFO] Cascade complete: 11 sensors, 551ms
[INFO] Slice processed: area=0.55 dm² (55 bits active)
[INFO] Accumulated area: 0.55 dm²

[DEBUG] Waiting for encoder pulse...
[ISR] Encoder pulse detected (position=2)
[INFO] Main task woken by encoder event
[DEBUG] Reading 11 sensors (CASCADE mode)...
[INFO] Cascade complete: 11 sensors, 549ms
[INFO] Slice processed: area=0.60 dm² (60 bits active)
[INFO] Accumulated area: 1.15 dm² (2 slices)
```

### Success Criteria

- ✅ Every encoder pulse triggers sensor read
- ✅ No missed pulses (position increments linearly)
- ✅ Latency encoder→read < 5ms
- ✅ Sensor read completes before next pulse (at slow speed)

---

## ✅ Test 3: HMI Display Update

### Objective

Verify VP addresses update correctly on DWIN display.

### Procedure

**Step 1: Place Leather Piece on Sensors**

Position leather strip covering sensors 3-8 (example).

**Step 2: Rotate Encoder (10 pulses)**

Simulate leather passing through (10 × 5mm = 50mm length).

**Step 3: Observe Display (Page 1)**

| VP Address | Variable            | Expected Value | Actual Value |
| ---------- | ------------------- | -------------- | ------------ |
| **0x1110** | VP_STATE            | 1 (Running)    | ****\_****   |
| **0x1050** | VP_BATCH_COUNT      | 1              | ****\_****   |
| **0x1051** | VP_LEATHER_COUNT    | 0→1            | ****\_****   |
| **0x1060** | VP_CURRENT_AREA     | 0.00→3.00 dm²  | ****\_****   |
| **0x1080** | VP_ACCUMULATED_AREA | 0.00→3.00 dm²  | ****\_****   |

**Step 4: Remove Leather (3 Empty Slices)**

Rotate encoder 3 more times without leather.

**Expected Behavior:**

- Current area freezes at 3.00 dm²
- Leather count increments to 1
- Accumulated area shows 3.00 dm²
- Next leather resets current area to 0.00

### Success Criteria

- ✅ Display updates within 100ms of encoder pulse
- ✅ Values match expected (±1% tolerance)
- ✅ Piece detection reliable (3-slice hysteresis works)
- ✅ Total accumulates correctly

---

## 📊 Performance Profiling

### Measure Latency (Instrumentation)

Add timing code in `lgc_main_task.c`:

```c
// In encoder pulse handler
uint32_t t_start = tx_time_get();  // ThreadX ticks (100 Hz)

Result_t res = sensor_reader->read_cascade_mode(ctx, &sensor_data);

uint32_t t_end = tx_time_get();
uint32_t elapsed_ms = (t_end - t_start) * 10;  // Convert ticks to ms

if (elapsed_ms > 600) {
    printf("[WARN] Slow sensor read: %u ms (target: <600ms)\r\n", elapsed_ms);
}
```

### Expected Performance

| Stage                 | Time (µs) | Time (ms) | Cumulative (ms) |
| --------------------- | --------- | --------- | --------------- |
| Encoder ISR           | ~10       | 0.01      | 0.01            |
| Task wakeup           | ~500      | 0.5       | 0.51            |
| Wrapper overhead      | ~100      | 0.1       | 0.61            |
| Agent TX              | ~5,000    | 5         | 5.61            |
| Agent RX (11 sensors) | ~545,000  | 545       | **550.61**      |
| Callback + semaphore  | ~50       | 0.05      | 550.66          |
| Domain processing     | ~2,000    | 2         | **552.66** ✅   |

**Total:** 552.66ms (vs 2000ms Modbus RTU = **73% faster**)

### Measure CPU Usage (ThreadX Stats)

```c
// In debug build, enable ThreadX performance info
#define TX_ENABLE_EVENT_TRACE
#define TX_THREAD_ENABLE_PERFORMANCE_INFO

// In main loop
TX_THREAD_PERFORMANCE_INFO perf;
tx_thread_performance_info_get(&main_task_thread, &perf);

printf("[PERF] Main Task: %lu execution time (%lu%% CPU)\r\n",
       perf.total_execution_time,
       (perf.total_execution_time * 100) / total_system_time);
```

**Target CPU Usage:**

- Idle: <5%
- Active (continuous measurement): <60%

---

## 🐛 Common Issues & Solutions

### Issue 1: Hard Fault on Startup

**Symptom:** Board resets immediately after flashing.

**Possible Causes:**

1. Stack overflow (ThreadX task stack too small)
2. NULL pointer dereference (DI Container not initialized)
3. MPU violation (accessing unaligned memory)

**Debug Steps:**

1. Enable HardFault_Handler debugging:

```c
void HardFault_Handler(void)
{
    __asm volatile(
        "TST lr, #4 \n"
        "ITE EQ \n"
        "MRSEQ r0, MSP \n"
        "MRSNE r0, PSP \n"
        "B hard_fault_debug \n"
    );
}

void hard_fault_debug(uint32_t *stack_frame)
{
    printf("[FAULT] PC=%08lX, LR=%08lX, R0=%08lX\r\n",
           stack_frame[6], stack_frame[5], stack_frame[0]);
    while (1);  // Halt
}
```

2. Check DI Container initialization:

```c
Result_t res = LgcDI_Init();
if (res != ERR_OK) {
    printf("[ERROR] DI Init failed: %d\r\n", res);
    Error_Handler();
}
```

### Issue 2: Sensors Not Responding

**Symptom:** Timeout (ERR_TIMEOUT) on every read.

**Check RS-485 Hardware:**

1. **Verify wiring:**
   - A and B not swapped
   - GND connected between STM32 and sensors
   - 120Ω termination at both ends

2. **Measure signals with oscilloscope:**
   - Idle state: A=HIGH, B=LOW (differential ~200mV)
   - Active: Toggle at 9600 baud (104µs per bit)

3. **Verify baud rate:**
   ```c
   // In MX_UART2_Init() (Core/Src/usart.c)
   huart2.Init.BaudRate = 9600;  // ← Verify this
   ```

### Issue 3: HMI Display Not Updating

**Symptom:** Display shows 0.00 dm² despite leather detected.

**Check Display Communication:**

1. **Verify UART1 connection:**
   - TX (PA9) → Display RX
   - RX (PA10) → Display TX
   - Baud rate: 115200 (both sides)

2. **Test VP write manually:**

   ```c
   // In lgc_hmi_task.c
   uint16_t test_value = VP_AREA_TO_UINT16(5.25f);  // 525
   display->write_u16(ctx, VP_CURRENT_AREA, test_value);
   ```

3. **Check VP addresses match DWIN project:**
   - Open `.hmi` project in DWIN editor
   - Verify VP_CURRENT_AREA = 0x1060

---

## 📄 Complete Test Report Template

```markdown
# Hardware Integration Test Report

**Date:** ****\_\_\_****
**Tester:** ****\_\_\_****
**Firmware Version:** 2.0.0 (Session 4.3)
**Hardware Revision:** ****\_\_\_****

## Test 1: Sensor Communication

- [ ] All 11 sensors respond (no timeout)
- [ ] FLAGS sequence correct (1→0)
- [ ] Latency measured: **\_\_\_** ms (target: <600ms)
- [ ] No CRC errors
- [ ] Notes: ************************\_************************

## Test 2: Encoder Synchronization

- [ ] Every encoder pulse triggers read
- [ ] No missed pulses (10/10 pulses processed)
- [ ] Encoder→read latency: **\_\_\_** ms (target: <5ms)
- [ ] Notes: ************************\_************************

## Test 3: HMI Display Update

- [ ] VP_CURRENT_AREA updates correctly
- [ ] VP_ACCUMULATED_AREA accumulates correctly
- [ ] Piece detection works (3-slice hysteresis)
- [ ] Update latency < 100ms
- [ ] Notes: ************************\_************************

## Test 4: Stress Test

- [ ] 10 min continuous operation: PASS / FAIL
- [ ] No hard faults or freezes
- [ ] CPU usage measured: **\_\_\_** % (target: <60%)
- [ ] Missed pulses: **\_\_\_** % (target: <5%)
- [ ] Notes: ************************\_************************

## Issues Found

1. ***
2. ***
3. ***

## Overall Status: ✅ PASS / ❌ FAIL / ⏳ PARTIAL

**Signed:** ****\_\_\_****
```

---

**Next Steps After Hardware Test:**

1. Session 4.4: Encoder pulse buffering (if bottleneck confirmed)
2. Session 4.5: Unit tests implementation (CMock + Unity)
3. Session 4.6: Observer pattern refactor (eliminate event flags)
4. Session 5.0: Production release preparation

**Questions?** See [SESSION_4.3_COMPLETE.md](SESSION_4.3_COMPLETE.md) for detailed test procedures and debugging guide.
