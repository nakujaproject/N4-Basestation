# Arduino Uno Receiver - Transparent Mode

This receiver code runs on Arduino Uno with the V03 Shield, receiving binary telemetry packets from the ESP32 sender.

## How It Works

### Transparent Mode Magic

Even though the Sender (ESP32) uses API frames over SPI, the Receiver XBee (in **Transparent Mode**) automatically:
1. Strips away all API "envelope" headers
2. Outputs **only** the raw payload data to DOUT pin
3. Presents data as continuous serial stream

### Struct Reception

The Arduino receives exactly 53 bytes of binary data representing the `TelemetryPacket` struct. The code:
1. Waits for enough bytes (`sizeof(TelemetryPacket)`)
2. Reads bytes directly into struct memory
3. Interprets bytes as the structured data

---

## Hardware Setup

### Wiring (V03 Shield Bypass Method)

| XBee Pin | Arduino Pin | Function |
|----------|-------------|----------|
| **Pin 2 (DOUT)** | Digital Pin 2 | RX (Receiver Input) |
| **Pin 3 (DIN)** | Digital Pin 3 | TX (Not used, but wired for completeness) |
| **Pin 1 (VCC)** | 3.3V (from Shield) | Power |
| **Pin 10 (GND)** | GND | Ground |

**Why bypass the shield?**
The V03 Shield's switch may cause routing issues. Direct wiring to Pins 2/3 ensures reliable SoftwareSerial communication.

---

## The Telemetry Packet Structure

**CRITICAL:** This struct **MUST** match the Sender's struct exactly - same order, same data types, same packing.

```cpp
struct __attribute__((packed)) TelemetryPacket {
    uint32_t record_number;        // 4 bytes
    uint8_t operation_mode;        // 1 byte
    uint8_t state;                 // 1 byte
    float ax, ay, az;              // 12 bytes (3 floats)
    float pitch, roll;             // 8 bytes
    float gx, gy, gz;              // 12 bytes
    float latitude, longitude;     // 8 bytes
    float gps_altitude;            // 4 bytes
    float pressure, temperature;   // 8 bytes
    float rel_altitude;            // 4 bytes
    uint8_t drogue_pin_state;      // 1 byte
    uint8_t main_chute_pin_state;  // 1 byte
};
// Total: 53 bytes
```

**Note:** `__attribute__((packed))` prevents compiler from adding padding bytes between fields.

---

## Complete Receiver Code

```cpp
#include <SoftwareSerial.h>

// --- WIRING (V03 Shield Bypass) ---
// XBee DOUT (Pin 2) -> Arduino Pin 2
// XBee DIN  (Pin 3) -> Arduino Pin 3
SoftwareSerial xbee(2, 3); // RX, TX

// --- TELEMETRY DATA STRUCTURE ---
// MUST match the Sender's struct exactly!
struct __attribute__((packed)) TelemetryPacket {
    uint32_t record_number;
    uint8_t operation_mode;
    uint8_t state;
    float ax, ay, az;
    float pitch, roll;
    float gx, gy, gz;
    float latitude, longitude, gps_altitude;
    float pressure, temperature, rel_altitude;
    uint8_t drogue_pin_state, main_chute_pin_state;
};

// Create an instance to hold incoming data
TelemetryPacket packet;

void setup() {
  // Debug to PC
  Serial.begin(9600);
  
  // Listen to XBee
  xbee.begin(9600);
  
  Serial.println("--- XBee Telemetry Receiver Started ---");
  Serial.print("Expected Packet Size: ");
  Serial.print(sizeof(TelemetryPacket));
  Serial.println(" bytes");
  Serial.println();
  Serial.println("Waiting for data from Sender...");
}

void loop() {
  // 1. Check if we have enough bytes for a full packet
  if (xbee.available() >= sizeof(TelemetryPacket)) {
    
    // 2. Read the raw bytes directly into the struct's memory
    // We cast the struct address to a (char*) so readBytes can fill it
    xbee.readBytes((char*)&packet, sizeof(TelemetryPacket));
    
    // 3. Print the parsed data
    printPacketData();
  }
}

void printPacketData() {
  Serial.println("========================================");
  Serial.print("Record #:     "); Serial.println(packet.record_number);
  Serial.print("Mode:         "); Serial.println(packet.operation_mode);
  Serial.print("State:        "); Serial.println(packet.state);
  Serial.println("----------------------------------------");
  
  Serial.print("Accel (m/s²): ");
  Serial.print(packet.ax, 2); Serial.print(", ");
  Serial.print(packet.ay, 2); Serial.print(", ");
  Serial.println(packet.az, 2);
  
  Serial.print("Attitude (°): ");
  Serial.print("Pitch="); Serial.print(packet.pitch, 1);
  Serial.print(", Roll="); Serial.println(packet.roll, 1);
  
  Serial.print("Gyro (°/s):   ");
  Serial.print(packet.gx, 1); Serial.print(", ");
  Serial.print(packet.gy, 1); Serial.print(", ");
  Serial.println(packet.gz, 1);
  
  Serial.println("----------------------------------------");
  Serial.print("GPS:          ");
  Serial.print(packet.latitude, 6); Serial.print(", ");
  Serial.println(packet.longitude, 6);
  Serial.print("GPS Alt (m):  "); Serial.println(packet.gps_altitude, 1);
  
  Serial.println("----------------------------------------");
  Serial.print("Pressure:     "); Serial.print(packet.pressure, 2); Serial.println(" hPa");
  Serial.print("Temperature:  "); Serial.print(packet.temperature, 1); Serial.println(" °C");
  Serial.print("Altitude:     "); Serial.print(packet.rel_altitude, 1); Serial.println(" m");
  
  Serial.println("----------------------------------------");
  Serial.print("Drogue:       "); Serial.println(packet.drogue_pin_state ? "DEPLOYED" : "ARMED");
  Serial.print("Main:         "); Serial.println(packet.main_chute_pin_state ? "DEPLOYED" : "ARMED");
  Serial.println("========================================");
  Serial.println();
}
```

---

## Expected Output

When receiving data successfully:

```
========================================
Record #:     1
Mode:         1
State:        5
----------------------------------------
Accel (m/s²): 0.12, -9.81, 0.05
Attitude (°): Pitch=-36.5, Roll=-2.1
Gyro (°/s):   -5.5, 3.0, 2.8
----------------------------------------
GPS:          -1.292100, 36.821900
GPS Alt (m):  1661.5
----------------------------------------
Pressure:     858.42 hPa
Temperature:  26.7 °C
Altitude:     1204.3 m
----------------------------------------
Drogue:       ARMED
Main:         ARMED
========================================
```

---

## Troubleshooting

### Problem: Garbage Data (Crazy Numbers)

**Example:**
```
Altitude:     439281.22 m
Temperature:  -3492.1 °C
```

**Cause:** Receiver started reading **in the middle** of a packet (misaligned).

**Solution:**
1. Press **RESET** button on Arduino Receiver
2. This clears the serial buffer
3. Receiver will wait for start of next fresh packet

**Prevention:** Add packet start marker:
```cpp
// Sender adds: xbee.write(0xAA); xbee.write(0xAA); // Sync bytes
// Receiver looks for 0xAA 0xAA before reading struct
```

### Problem: No Data Received

**Checklist:**
- [ ] XBee modules configured with matching ID and HP?
- [ ] XBee baud rate (BD=3 for 9600) matches Arduino code?
- [ ] Sender module powered and transmitting? (Check LED)
- [ ] Receiver wiring correct? (DOUT to Pin 2)
- [ ] SoftwareSerial baud matches XBee BD setting?

**Test:** Use XCTU Console to send test data from Sender manually.

### Problem: Intermittent Data

**Symptoms:** Some packets received, others missed.

**Possible Causes:**
1. **Signal Quality:** Increase PL (Power Level) in XCTU
2. **Buffer Overflow:** SoftwareSerial can drop bytes at high rates
   - Reduce sender transmit rate (increase delay)
   - Switch to Hardware Serial if available
3. **Interference:** Change HP (Preamble ID) to avoid nearby networks

### Problem: Struct Size Mismatch

**Error:** Packet size doesn't match expected 53 bytes.

**Solution:**
1. Print struct size on both Sender and Receiver:
   ```cpp
   Serial.println(sizeof(TelemetryPacket));
   ```
2. If different, compiler is adding padding
3. Add `__attribute__((packed))` to struct definition
4. Ensure both use same compiler (Arduino IDE vs PlatformIO can differ)

---

## Performance Considerations

### SoftwareSerial Limitations

- **Max Reliable Baud:** ~57600 (use 9600 for stability)
- **Buffer Size:** Only 64 bytes (can overflow if packets come fast)
- **No Hardware Buffering:** CPU must process bytes immediately

### Upgrade Path: Hardware Serial

If you need higher rates, use Hardware Serial:

```cpp
// Instead of SoftwareSerial:
// Use Serial (pins 0/1) for XBee
// Use Serial1 if available (Mega/Due)

void setup() {
  Serial.begin(9600);   // Hardware Serial to XBee
  Serial1.begin(9600);  // Debug to PC (if available)
}

void loop() {
  if (Serial.available() >= sizeof(TelemetryPacket)) {
    Serial.readBytes((char*)&packet, sizeof(TelemetryPacket));
    // Print to Serial1 instead
  }
}
```

---

## Integration with Base Station

To forward received data to the dashboard:

### Option 1: Serial Passthrough

Arduino forwards raw packets to PC via USB:

```cpp
void loop() {
  while (xbee.available()) {
    Serial.write(xbee.read()); // Pass through to PC
  }
}
```

Python script on PC parses the binary struct.

### Option 2: JSON Conversion

Arduino converts struct to JSON:

```cpp
void printPacketJSON() {
  Serial.print("{\"record\":");
  Serial.print(packet.record_number);
  Serial.print(",\"altitude\":");
  Serial.print(packet.rel_altitude);
  // ... etc
  Serial.println("}");
}
```

Dashboard consumes standard JSON telemetry.

---

## Testing Procedure

### 1. Bench Test (Close Range)

1. **Power:** Both modules via USB (modules 1 meter apart)
2. **Configuration:** 
   - PL=0 (lowest power)
   - ID and HP matching
3. **Sender:** Upload code, verify LED flashing
4. **Receiver:** Upload code, open Serial Monitor
5. **Expected:** Clean data every second

### 2. Range Test

1. Increase PL to 4 (maximum power)
2. Move modules progressively further apart
3. Monitor for packet loss (missing record numbers)
4. Note maximum reliable range

### 3. Data Integrity Test

1. Send known test values (e.g., altitude = 1234.56)
2. Verify Receiver displays exact values
3. If values differ, check endianness/packing

---

## Next Steps

- [ ] Test with real ESP32 Sender code
- [ ] Verify all 53 bytes received correctly
- [ ] Test range in open field
- [ ] Integrate with base station dashboard
- [ ] Add error checking (checksum or CRC)
- [ ] Consider adding packet sync markers

---

**Related Documentation:**
- [ESP32 Sender Code](uart_sender_esp32.md)
- [XCTU Configuration](../XCTU_CONFIGURATION.md)
- [Hardware Setup](../HARDWARE_SETUP.md)
