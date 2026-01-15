# Leather Gauge Measurement Algorithm

## Overview
The measurement algorithm in `lgc_main_task.c` processes sensor data from 11 Modbus sensors to measure leather pieces on a conveyor belt system. Each sensor has 10 photoreceptors, totaling 110 measurement points across the width (X-axis), and an encoder provides step pulses for length (Y-axis) measurement.

## Hardware Architecture

```
┌─────────────────────────────────────────────┐
│         Conveyor Belt (Y-axis)              │
│        ←─ Movement Direction                │
├─────────────────────────────────────────────┤
│  ┌─┬─┬─┬─┬─┬─┬─┬─┬─┬─┐ Sensor 0           │
│  ├─┼─┼─┼─┼─┼─┼─┼─┼─┼─┤ (Photoreceptors)   │
│  ├─┼─┼─┼─┼─┼─┼─┼─┼─┼─┤ Sensor 1           │
│  ├─┼─┼─┼─┼─┼─┼─┼─┼─┼─┤ ...                │
│  │ │ │ │ │ │ │ │ │ │ │ Sensor 10          │
│  └─┴─┴─┴─┴─┴─┴─┴─┴─┴─┘                    │
└─────────────────────────────────────────────┘
         X-axis (Width = 110 pixels)
```

### Constants Configuration
- **LGC_SENSOR_NUMBER**: 11 sensors
- **LGC_PHOTORECEPTORS_PER_SENSOR**: 10 bits per sensor register
- **LGC_PIXEL_WIDTH_MM**: 10.0 mm (width of one photoreceptor)
- **LGC_ENCODER_STEP_MM**: 5.0 mm (length increment per encoder pulse)
- **LGC_LEATHER_END_HYSTERESIS**: 3 consecutive empty steps to confirm leather end

## Data Structures

### Global Measurements Structure
```c
typedef struct {
    uint16_t current_batch_index;           // Current batch (0 to BATCH_COUNT_MAX-1)
    uint16_t current_leather_index;         // Index within batch (0 to batch_limit-1)
    float current_leather_area;             // Running total for current piece
    float leather_measurement[300];         // Individual leather areas
    float batch_measurement[200];           // Batch totals
    uint8_t is_measuring;                   // State: measuring or idle
    uint8_t no_detection_count;             // Hysteresis counter
} lgc_measurements_t;
```

### Sensor Data
```c
typedef struct {
    uint8_t state;                          // LGC_STOP, LGC_RUNNING, LGC_FAIL
    uint16_t sensor_status;                 // Bit flags for sensor errors
    uint16_t sensor[11];                    // Raw data from each sensor
} lgc_t;
```

## Measurement Algorithm Flow

### 1. Sensor Reading Loop (Every Encoder Pulse)
```
For each of 11 sensors:
    └─ Attempt Modbus read with retry (up to 4 retries)
       ├─ Success: Clear sensor error flag
       └─ Failure: Set sensor error flag, delay 20ms
       
If all sensors healthy (status == 0):
    └─ Call lgc_process_measurement()
```

### 2. Active Bits Counting
```
active_bits = 0
For each sensor (0 to 10):
    └─ For each photoreceptor bit (0 to 9):
        └─ If bit set in sensor[i]:
            └─ Increment active_bits
            
Return active_bits (range: 0-110)
```

### 3. Slice Area Calculation
```
width_mm = active_bits * LGC_PIXEL_WIDTH_MM
area_mm² = width_mm * LGC_ENCODER_STEP_MM

Example:
- 50 active bits × 10mm = 500mm width
- 500mm × 5mm = 2500 mm² per step
```

### 4. Leather Detection State Machine

```
                    ┌────────────────────┐
                    │   IDLE STATE       │
                    │ is_measuring = 0   │
                    └──────────┬─────────┘
                               │
                    ┌──────────▼──────────┐
                    │ Active bits > 0?    │
                    └──────────┬──────────┘
                               │
                   ┌───────────┴──────────────┐
                   │                         │
                   ▼ YES                     ▼ NO
          ┌─────────────────────┐   ┌─────────────────┐
          │ START NEW LEATHER   │   │ Increment       │
          │ is_measuring = 1    │   │ no_detection_   │
          │ area = 0            │   │ count++         │
          └──────────┬──────────┘   └────────┬────────┘
                     │                       │
                     └───────────┬───────────┘
                                 │
                    ┌────────────▼──────────┐
                    │ area += slice_area    │
                    │ no_detect_count = 0   │
                    └────────────┬──────────┘
                                 │
                    ┌────────────▼────────────────────┐
                    │ no_detect_count ≥ HYSTERESIS?   │
                    └────────────┬─────────────────┬──┘
                                 │                 │
                            YES  │                 │ NO
                                 ▼                 │
                    ┌─────────────────────────┐    │
                    │ END LEATHER             │    │
                    │ ├─ Save to leather[i]  │    │
                    │ ├─ Add to batch[i]     │    │
                    │ ├─ i++ (leather_index) │    │
                    │ └─ Check batch_limit   │    │
                    └─────────┬───────────────┘    │
                              │                    │
                              └────────┬───────────┘
                                       │
                              ┌────────▼────────┐
                              │ LOOP CONTINUES  │
                              │ Next encoder    │
                              │ pulse           │
                              └─────────────────┘
```

### 5. Batch Management Logic

```
When leather_index reaches batch_limit:
    ├─ leather_index = 0 (reset for new batch)
    ├─ batch_index++ (move to next batch)
    └─ Validate batch_index < BATCH_COUNT_MAX
       └─ If overflow: Error condition
```

**Memory Layout**
```
leather_measurement[300]:
  │ Leather 0 area │ Leather 1 area │ ... │ Leather 299 │
  └─ Batch 0 ─────┘ Batch 1 ───────┘
    (0 to limit-1)  (0 to limit-1)

batch_measurement[200]:
  │ Batch 0 sum │ Batch 1 sum │ ... │ Batch 199 sum │
```

## Helper Functions

### `lgc_count_active_bits()`
Counts photoreceptors detecting leather across all sensors.
- Scans 11 sensors × 10 bits = 110 total pixels
- Returns: 0 to 110 active bits

### `lgc_calculate_slice_area(uint16_t active_bits)`
Converts active bit count to area measurement.
- Formula: `area = active_bits × pixel_width × encoder_step`
- Returns area in mm² (or configured units)

### `lgc_process_measurement(LGC_CONF_TypeDef_t *config)`
Main processing function called on each encoder pulse.
- Detects leather start/stop
- Accumulates area measurements
- Manages batch transitions
- Validates array bounds

## Configuration Fields Used

From `LGC_CONF_TypeDef_t`:
- **batch_limit**: Number of leather pieces per batch (typical: 10-200)
- **units**: Output unit (dm², ft², m², etc.)
- **conversion**: Scaling factor for unit conversion

## Error Handling

1. **Sensor Failures**: 
   - Individual sensor status tracked in `sensor_status` bitmap
   - Measurement skipped if any sensor fails
   - Retry up to 4 times with 20ms delays

2. **Array Overflow**:
   - `current_leather_index` validated against `LGC_LEATHER_COUNT_MAX`
   - `current_batch_index` validated against `LGC_LEATHER_BATCH_COUNT_MAX`
   - If batch array full: measurement stops (TODO: handle gracefully)

3. **Hysteresis**:
   - Prevents false leather-end detection due to sensor noise
   - Requires 3 consecutive empty steps to confirm end
   - Counter resets when leather detected again

## Typical Measurement Example

```
Config: batch_limit = 10 leather pieces per batch

Encoder pulses received:
Pulse 1: 80 bits active → area = 800mm × 5mm = 4000mm²
Pulse 2: 85 bits active → area = 850mm × 5mm = 4250mm²
Pulse 3: 90 bits active → area = 900mm × 5mm = 4500mm²
Pulse 4: 0 bits active  → no_detect_count = 1
Pulse 5: 0 bits active  → no_detect_count = 2
Pulse 6: 0 bits active  → no_detect_count = 3 (hysteresis threshold)
         ├─ Save leather[0] = 4000 + 4250 + 4500 = 12750mm²
         ├─ batch[0] += 12750
         ├─ leather_index = 1

Pulse 7: 75 bits active → NEW LEATHER detected, area = 3750mm²
...continues for remaining pieces
```

## Testing Checklist

- [ ] Verify bit counting matches hardware configuration
- [ ] Test leather start detection (active → measuring)
- [ ] Test leather end detection with hysteresis
- [ ] Validate batch transitions at batch_limit
- [ ] Check array bounds during overflow conditions
- [ ] Verify sensor failure handling doesn't corrupt data
- [ ] Confirm area calculations with known pixel dimensions
- [ ] Test edge cases (very short/long leather pieces)

