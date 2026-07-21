# ESP32-to-ESP32 XBee SPI System Setup Guide

## Overview

This guide documents the complete working setup for ESP32-to-ESP32 communication via XBee Pro 900HP using SPI protocol. This configuration successfully transmitted binary telemetry at 10Hz during testing.

**System Components:**
- **Sender:** ESP32 + XBee (Rocket/Transmitter)
- **Receiver:** ESP32 + XBee (Base Station)
- **Protocol:** SPI with API frames
- **Wireless:** XBee DigiMesh 900HP (902-928 MHz)
- **Data Rate:** 10Hz, 36-byte binary struct

---

## Architecture Diagram

```
┌─────────────────────────────────────────────────────────────┐
│                    ROCKET (SENDER)                          │
│  ┌──────────┐  SPI (1MHz)  ┌───────────┐  900MHz RF        │
│  │  ESP32   │──────────────│ XBee TX   │─────────────┐     │
│  │ (Sensors)│              │ (SPI Mode)│             │     │
│  └──────────┘              └───────────┘             │     │
│                                                       │     │
│  GPIO 5  → CS                                        │     │
│  GPIO 18 → SCK        API Frames                     │     │
│  GPIO 19 → MISO       (0x10 Transmit Request)        │     │
│  GPIO 23 → MOSI                                      │     │
│  GPIO 4  → ATTN                                      │     │
└──────────────────────────────────────────────────────┼─────┘
                                                       │
                         200 kbps Air Speed            │
                         DigiMesh Broadcast            │
                                                       │
┌──────────────────────────────────────────────────────┼─────┐
│                BASE STATION (RECEIVER)               │     │
│  ┌──────────┐  SPI (1MHz)  ┌───────────┐  900MHz RF│     │
│  │  ESP32   │──────────────│ XBee RX   │─────────────┘     │
│  │(Dashboard)│             │ (SPI Mode)│                   │
│  └──────────┘              └───────────┘                   │
│                                                             │
│  GPIO 5  → CS        API Frames                            │
│  GPIO 18 → SCK       (0x90 RX Packet)                      │
│  GPIO 19 → MISO                                            │
│  GPIO 23 → MOSI      JSON/Bluetooth forwarding             │
│  GPIO 4  → ATTN      to Ground Station PC                  │
└─────────────────────────────────────────────────────────────┘
```

---

## Hardware Requirements

### Per System (Sender + Receiver)

**Electronics:**
- 1× ESP32 DevKit (30-pin or 38-pin)
- 1× XBee Pro 900HP S3B module
- 1× Arduino XBee Shield V03 (for power regulation)
- 7× Male-to-male jumper wires (SPI + power)
- 1× USB cable (ESP32 programming)
- 1× USB cable (XBee initial configuration via XCTU)

**Power Supply:**
- ESP32: 5V via USB or battery
- XBee: 3.3V from Shield regulator (215mA capacity required)

**Tools:**
- Computer with XCTU software (Digi)
- Arduino IDE (for ESP32 programming)
- Multimeter (voltage verification)

---

## Complete Setup Procedure

### Phase 1: Configure Both XBee Modules (XCTU)

#### 1.1 Connect XBee to Computer

1. Insert XBee into Arduino Shield or USB adapter
2. Connect to computer via USB
3. Open XCTU software
4. Click **Discover** (use 9600 baud default)

#### 1.2 Configure Common Settings (Both Modules)

| Parameter | Setting | Value | Description |
|-----------|---------|-------|-------------|
| **CH** | Channel | **0** | 902-928 MHz (auto) |
| **ID** | Network ID | **7777** | Must match between modules |
| **HP** | Preamble ID | **0** | Must match between modules |
| **RR** | Retries | **3** | Default retry count |
| **PL** | Power Level | **4** | Max power (24 dBm) |
| **BR** | RF Data Rate | **7** | 200 kbps |

#### 1.3 Configure SPI Mode (Both Modules)

**CRITICAL:** Set these I/O pins to enable SPI:

| Parameter | Setting | Value | Description |
|-----------|---------|-------|-------------|
| **AP** | API Enable | **1** | Required for SPI |
| **D1** | DIO1 | **5** (SPI_ATTN) | Attention output |
| **D2** | DIO2 | **1** (SPI_CLK) | Clock input |
| **D3** | DIO3 | **1** (SPI_SSEL) | Chip Select input |
| **D4** | DIO4 | **1** (SPI_MOSI) | Data input |
| **P2** | DIO12 | **1** (SPI_MISO) | Data output |
| **P3** | DOUT | **0** (Disabled) | ⚠️ Disable UART TX |
| **P4** | DIN | **0** (Disabled) | ⚠️ Disable UART RX |

#### 1.4 Configure Sender-Specific Settings

| Parameter | Setting | Value | Description |
|-----------|---------|-------|-------------|
| **DL** | Destination Low | **FFFF** | Broadcast mode |
| **NI** | Node Identifier | "SENDER" | Optional label |

#### 1.5 Configure Receiver-Specific Settings

| Parameter | Setting | Value | Description |
|-----------|---------|-------|-------------|
| **DL** | Destination Low | **FFFF** | Receive broadcast |
| **NI** | Node Identifier | "RECEIVER" | Optional label |

#### 1.6 Write and Verify

1. Click **Write** button in XCTU
2. Wait for confirmation message
3. Verify settings by clicking **Read**
4. **Important:** Once P3/P4 are disabled, XCTU can't reconnect via UART

---

### Phase 2: Wire the Hardware

#### 2.1 Sender (Rocket) Wiring

**ESP32 VSPI to XBee:**

| ESP32 Pin | XBee Pin | Function | Wire Color (Suggest) |
|-----------|----------|----------|---------------------|
| 3V3 | Pin 1 | VCC | Red |
| GND | Pin 10 | GND | Black |
| GPIO 5 | Pin 17 (DIO3) | CS | Yellow |
| GPIO 18 | Pin 18 (DIO2) | SCK | Orange |
| GPIO 19 | Pin 4 (DIO12) | MISO | Blue |
| GPIO 23 | Pin 11 (DIO4) | MOSI | Green |
| GPIO 4 | Pin 19 (DIO1) | ATTN | Purple |

**Power Considerations:**
- XBee draws 215mA during TX (peak)
- ESP32 3.3V pin limited to ~600mA total
- **Recommended:** Use Shield's 3.3V regulator for XBee
- ESP32 handles data signals only

#### 2.2 Receiver (Base Station) Wiring

**Identical to sender wiring** - use same pin mapping.

#### 2.3 Wiring Verification Checklist

Before powering on:

- [ ] All 7 wires connected on both systems
- [ ] No crossed wires (MOSI to MOSI, MISO to MISO)
- [ ] Common ground between ESP32 and XBee
- [ ] XBee seated firmly in Shield (if using Shield)
- [ ] No shorts between adjacent pins
- [ ] Correct voltage at XBee Pin 1 (3.3V ±0.1V)

---

### Phase 3: Program the ESP32s

#### 3.1 Program Sender ESP32

1. Connect sender ESP32 to computer via USB
2. Open Arduino IDE
3. Copy code from [`esp32_spi_sender.md`](./esp32_spi_sender.md)
4. Select board: **ESP32 Dev Module**
5. Select correct COM port
6. Click **Upload**
7. Open Serial Monitor (115200 baud)
8. Verify output:
   ```
   [OK] SPI Initialized
   [OK] XBee Ready
   Starting transmission...
   TX #1 | Alt: 0.0m | Vel: 0.0m/s | AccZ: 1.00g
   ```

#### 3.2 Program Receiver ESP32

1. Connect receiver ESP32 to computer via USB
2. Copy code from [`esp32_spi_receiver.md`](./esp32_spi_receiver.md)
3. Upload same way as sender
4. Open Serial Monitor (115200 baud)
5. Verify output:
   ```
   [OK] SPI Initialized
   [OK] Waiting for telemetry...
   ```

---

### Phase 4: System Testing

#### 4.1 Bench Test (Side-by-Side)

**Setup:**
1. Power both ESP32s (USB or battery)
2. Place XBee modules 1 meter apart
3. Open both Serial Monitors

**Expected Behavior:**

**Sender Monitor:**
```
TX #1 | Alt: 0.0m | Vel: 0.0m/s | AccZ: 1.00g
TX #2 | Alt: 0.0m | Vel: 0.0m/s | AccZ: 1.00g
```

**Receiver Monitor:**
```
RX #1 | State:PRE_FLIGHT | Alt:0.0m | Vel:0.0m/s | AccZ:1.00g | Bat:3.80V | Total:1 Missed:0
RX #2 | State:PRE_FLIGHT | Alt:0.0m | Vel:0.0m/s | AccZ:1.00g | Bat:3.80V | Total:2 Missed:0
```

**Success Criteria:**
- Packet numbers match between sender/receiver
- No missed packets (`Missed:0`)
- Data values match (altitude, velocity, acceleration)
- Update rate ~10Hz (100ms intervals)

#### 4.2 Range Test

**Procedure:**
1. Keep receiver stationary with Serial Monitor open
2. Move sender away gradually (10m, 50m, 100m, 500m)
3. Monitor received signal quality
4. Record maximum reliable range

**Expected Range:**
- **Line-of-Sight:** Up to 28 miles (theoretical)
- **Urban Environment:** 1-2 km typical
- **Tested Range:** 2 km successfully

**Troubleshooting Range Issues:**
1. Increase power level (PL=4 for max)
2. Check antenna orientation (vertical preferred)
3. Avoid obstacles (buildings, trees)
4. Verify frequency band clear (900 MHz ISM)

#### 4.3 Stress Test

**10-Minute Continuous Operation:**

1. Run both systems for 10 minutes
2. Monitor packet loss percentage
3. Check for system crashes or resets
4. Verify battery voltage stable (if battery powered)

**Success Criteria:**
- <1% packet loss
- No ESP32 resets
- No XBee resets
- Consistent update rate throughout test

---

## Data Flow Explained

### Sender Side (Rocket)

```cpp
updateTelemetry()           // Read sensors
    ↓
TelemetryData struct        // Pack into 36 bytes
    ↓
sendXBeeFrame()             // Construct API frame (0x10)
    ↓
SPI.transfer()              // Transmit via SPI
    ↓
XBee TX Module              // Convert to RF signal
    ↓
900 MHz Broadcast           // Transmit wirelessly
```

### Receiver Side (Base Station)

```cpp
900 MHz Reception           // XBee RX receives signal
    ↓
XBee asserts ATTN LOW       // "Data ready" signal to ESP32
    ↓
readSPIPacket()             // ESP32 reads via SPI
    ↓
Parse API frame (0x90)      // Extract payload from frame
    ↓
TelemetryData struct        // Unpack 36 bytes
    ↓
processReceivedData()       // Analyze and display
    ↓
Serial/JSON/Bluetooth       // Forward to ground station
```

---

## Telemetry Structure

### Binary Format (36 Bytes)

```cpp
struct __attribute__((packed)) TelemetryData {
    uint32_t record_number;      // 0-3:   Packet counter
    uint8_t operation_mode;      // 4:     0=IDLE, 1=ARMED
    uint8_t state;               // 5:     Flight phase
    float ax, ay, az;            // 6-17:  Acceleration X,Y,Z (g)
    float altitude;              // 18-21: Altitude AGL (meters)
    float velocity;              // 22-25: Vertical velocity (m/s)
    float battery_voltage;       // 26-29: Battery voltage (V)
    // 30-35: Reserved for expansion
};
```

**Why Packed?**
- Ensures no padding between fields
- Predictable binary layout across platforms
- Both ESP32s interpret bytes identically

---

## Performance Metrics (Tested)

| Metric | Value | Notes |
|--------|-------|-------|
| **Update Rate** | 10 Hz | 100ms intervals |
| **Packet Size** | 36 bytes | Telemetry struct |
| **API Overhead** | 14 bytes | XBee frame headers |
| **Total Frame** | 50 bytes | Per transmission |
| **RF Bandwidth Used** | 4 kbps | 2% of 200 kbps |
| **SPI Clock** | 1 MHz | Safe for jumper wires |
| **SPI Transfer Time** | <0.5 ms | Per packet |
| **RF Latency** | ~5 ms | XBee processing + transmission |
| **Total Latency** | ~6 ms | Sensor → Display |
| **Packet Loss** | <0.5% | At 2 km range |
| **Missed Packets** | 0-2 per 1000 | Excellent reliability |

---

## Troubleshooting Guide

### Problem: Receiver shows no data (ATTN always HIGH)

**Diagnosis:** XBee not configured for SPI or sender not transmitting

**Solutions:**
1. Verify XBee SPI configuration (P3=0, P4=0, D1-D4=SPI)
2. Check Network ID matches (ID=7777 both sides)
3. Verify sender Serial Monitor shows TX messages
4. Measure voltage at XBee Pin 1 (should be 3.3V)
5. Check ATTN wire connected (GPIO 4 → XBee Pin 19)

### Problem: Wrong payload size error

**Cause:** Struct size mismatch between sender/receiver

**Solutions:**
1. Verify both use identical `TelemetryData` struct
2. Check `__attribute__((packed))` on both
3. Print `sizeof(TelemetryData)` on both (should be 36)
4. Ensure no extra fields added to one side

### Problem: Garbled data (wrong values)

**Cause:** Byte ordering or frame parsing issue

**Solutions:**
1. Verify both sender/receiver are ESP32 (same architecture)
2. Check SPI mode settings (MSBFIRST on both)
3. Add debug: Print first 10 bytes of received packet as hex
4. Verify API frame parsing (skip 11 bytes overhead)

### Problem: High packet loss (>5%)

**Causes:** RF interference, power issues, or distance

**Solutions:**
1. Reduce distance between modules
2. Check antenna orientation (vertical)
3. Increase power level (PL=4)
4. Verify 3.3V stable during TX (use oscilloscope if available)
5. Check for 900 MHz interference sources

### Problem: [WARN] XBee busy, skipping packet

**Cause:** Receiver not draining data fast enough

**Solutions:**
1. Check ATTN wire connection
2. Verify receiver code running (not crashed)
3. Reduce Serial.print() statements (slows processing)
4. Check receiver XBee powered

---

## Production Deployment Checklist

Before field deployment:

**Hardware:**
- [ ] All connections soldered (no jumper wires)
- [ ] Strain relief on all wires
- [ ] Enclosures weather-sealed
- [ ] Antennas securely mounted
- [ ] Battery capacity adequate (>2hr runtime)
- [ ] Backup power system tested

**Software:**
- [ ] Sender code integrated with real sensors
- [ ] Receiver code forwarding to dashboard
- [ ] Error handling for lost connection
- [ ] Logging to SD card enabled
- [ ] Watchdog timer implemented

**Testing:**
- [ ] 24-hour burn-in test passed
- [ ] Vibration test passed (rocket simulation)
- [ ] Temperature range test (-10°C to +50°C)
- [ ] Maximum range verified (2× expected flight distance)
- [ ] Battery drain test completed

**Configuration:**
- [ ] XBee settings backed up (XCTU profiles saved)
- [ ] Network ID unique (avoid interference with other teams)
- [ ] Encryption enabled (if required for competition)
- [ ] Power level optimized (balance range vs battery life)

---

## Switching Back to UART Mode

If you need to reconfigure XBee modules after enabling SPI:

1. Use the **SPI Rescue Programmer** tool
2. See [`xbee_spi_rescue_programmer.md`](./xbee_spi_rescue_programmer.md)
3. Rescue tool sends AT commands via SPI to re-enable UART
4. After rescue, XBee can connect to XCTU again

---

## Design Decision: Why SPI Worked (Then We Chose UART)

### SPI Success:
✅ Achieved 10Hz binary telemetry  
✅ Low latency (~6ms total)  
✅ Reliable packet delivery (<0.5% loss)  
✅ Direct struct transmission

### Why We Chose UART for Production:
1. **Simplicity:** 2 wires vs 5 wires
2. **Configuration:** XCTU always accessible
3. **Debugging:** Transparent mode = readable CSV
4. **Sufficient:** 115200 baud handles 50Hz easily
5. **Robustness:** No clock synchronization issues

**Verdict:** SPI works perfectly, but UART better suits rocket constraints.

---

## Related Documentation

- **[SPI_TROUBLESHOOTING.md](../SPI_TROUBLESHOOTING.md)** - Complete debugging journey, "weird characters" mystery
- **[esp32_spi_sender.md](./esp32_spi_sender.md)** - Complete sender code with simulation
- **[esp32_spi_receiver.md](./esp32_spi_receiver.md)** - Complete receiver code with JSON output
- **[xbee_spi_rescue_programmer.md](./xbee_spi_rescue_programmer.md)** - Recover from SPI lock-out
- **[HARDWARE_SETUP.md](../HARDWARE_SETUP.md)** - V03 Shield, power considerations
- **[XCTU_CONFIGURATION.md](../XCTU_CONFIGURATION.md)** - Complete XBee setup guide

---

## Summary

**Working Configuration Proven:**
- ESP32-to-ESP32 via XBee SPI successful
- 10Hz telemetry at 2km range tested
- Binary struct transmission reliable
- Complete code examples provided

**Key Lessons:**
1. Proper XBee I/O configuration critical (P3=0, P4=0)
2. SPI rescue tool essential for development
3. Common ground between ESP32/XBee required
4. Hybrid power setup (Shield regulator) recommended
5. API frame protocol works reliably once mastered

This guide provides complete working setup for teams needing high-speed telemetry via XBee SPI.
