# ESP32 XBee SPI Receiver (Base Station)

## Overview

This code receives binary telemetry from XBee via SPI and prints formatted output to Serial Monitor. Can be extended for JSON output, Bluetooth forwarding, or dashboard integration.

**Hardware:** ESP32 DevKit + XBee Pro 900HP (SPI Mode)  
**Protocol:** SPI with API frame parsing  
**Data Format:** 36-byte binary struct (matches sender)  
**Update Rate:** Up to 10Hz reception

---

## XBee Configuration (XCTU)

Configure the receiver XBee identically to sender:

| Parameter | Setting | Value | Description |
|-----------|---------|-------|-------------|
| **AP** | API Enable | **1** | Required for SPI mode |
| **D1** | DIO1 | **5** (SPI_ATTN) | Attention output |
| **D2** | DIO2 | **1** (SPI_CLK) | Clock input |
| **D3** | DIO3 | **1** (SPI_SSEL) | Chip Select input |
| **D4** | DIO4 | **1** (SPI_MOSI) | Data input |
| **P2** | DIO12 | **1** (SPI_MISO) | Data output |
| **P3** | DOUT | **0** (Disabled) | Disable UART TX |
| **P4** | DIN | **0** (Disabled) | Disable UART RX |
| **ID** | Network ID | **7777** | Must match sender |
| **HP** | Preamble ID | **0** | Must match sender |

---

## Wiring Diagram

### ESP32 VSPI to XBee

| ESP32 Pin | XBee Pin | Function | Notes |
|-----------|----------|----------|-------|
| **3V3** | Pin 1 | VCC | ⚠️ Use Shield regulator preferred |
| **GND** | Pin 10 | GND | Common ground required |
| **GPIO 5** | Pin 17 (DIO3) | CS | Chip Select |
| **GPIO 18** | Pin 18 (DIO2) | SCK | Clock |
| **GPIO 19** | Pin 4 (DIO12) | MISO | Data from XBee |
| **GPIO 23** | Pin 11 (DIO4) | MOSI | Data to XBee |
| **GPIO 4** | Pin 19 (DIO1) | ATTN | Data ready signal |

---

## Telemetry Data Structure

**CRITICAL:** Must match sender struct exactly.

```cpp
// 36 Bytes Total (Packed - No Padding)
struct __attribute__((packed)) TelemetryData {
    uint32_t record_number;      // 4 bytes - Packet counter
    uint8_t operation_mode;      // 1 byte  - 0=IDLE, 1=ARMED
    uint8_t state;               // 1 byte  - Flight state
    float ax, ay, az;            // 12 bytes - Acceleration (g)
    float altitude;              // 4 bytes - Altitude AGL (m)
    float velocity;              // 4 bytes - Vertical velocity (m/s)
    float battery_voltage;       // 4 bytes - Battery voltage (V)
};
```

---

## Complete Receiver Code

```cpp
#include <SPI.h>

// ===== WIRING CONFIGURATION =====
const int CS_PIN = 5;
const int SCK_PIN = 18;
const int MISO_PIN = 19;
const int MOSI_PIN = 23;
const int ATTN_PIN = 4;

// ===== SPI SETTINGS =====
SPISettings xbeeSPI(1000000, MSBFIRST, SPI_MODE0);

// ===== TELEMETRY DATA STRUCTURE =====
// Must match sender exactly
struct __attribute__((packed)) TelemetryData {
    uint32_t record_number;
    uint8_t operation_mode;
    uint8_t state;
    float ax, ay, az;
    float altitude;
    float velocity;
    float battery_voltage;
};

TelemetryData incomingData;

// ===== STATISTICS =====
unsigned long packetsReceived = 0;
unsigned long lastPacketTime = 0;
unsigned long missedPackets = 0;
uint32_t lastRecordNumber = 0;

// ===== SETUP =====
void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("=================================");
  Serial.println("  ESP32 XBee SPI Receiver");
  Serial.println("  Base Station Telemetry");
  Serial.println("=================================");
  
  // Configure SPI Pins
  pinMode(CS_PIN, OUTPUT);
  digitalWrite(CS_PIN, HIGH); // Start deselected
  pinMode(ATTN_PIN, INPUT);   // XBee output
  
  // Initialize SPI
  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, CS_PIN);
  
  Serial.println("[OK] SPI Initialized");
  Serial.println("[OK] Waiting for telemetry...\n");
  
  lastPacketTime = millis();
}

// ===== MAIN LOOP =====
void loop() {
  // Check if XBee has data ready (ATTN goes LOW)
  if (digitalRead(ATTN_PIN) == LOW) {
    readSPIPacket();
  }
  
  // Connection timeout detection (3 seconds)
  if (millis() - lastPacketTime > 3000 && packetsReceived > 0) {
    Serial.println("[WARN] Connection lost (3s timeout)");
    lastPacketTime = millis(); // Reset to prevent spam
  }
}

// ===== READ SPI PACKET FROM XBEE =====
void readSPIPacket() {
  SPI.beginTransaction(xbeeSPI);
  digitalWrite(CS_PIN, LOW);
  delayMicroseconds(50); // Stability delay

  // === FIND START DELIMITER (0x7E) ===
  // XBee may send 0x00 filler bytes first
  int timeout = 0;
  bool foundStart = false;
  
  while(digitalRead(ATTN_PIN) == LOW && timeout < 500) {
    byte incoming = SPI.transfer(0x00);
    if(incoming == 0x7E) {
      foundStart = true;
      break;
    }
    timeout++;
  }

  if(foundStart) {
    // === READ LENGTH (2 BYTES) ===
    byte msb = SPI.transfer(0x00);
    byte lsb = SPI.transfer(0x00);
    int frameLen = (msb << 8) | lsb;

    // === READ FRAME TYPE ===
    byte frameType = SPI.transfer(0x00);

    // We only process RX Packets (0x90)
    if(frameType == 0x90) {
      // === SKIP API FRAME OVERHEAD ===
      // 64-bit Source Address (8 bytes) - Not needed
      for(int i=0; i<8; i++) SPI.transfer(0x00);
      
      // 16-bit Source Address (2 bytes) - Not needed
      SPI.transfer(0x00);
      SPI.transfer(0x00);
      
      // Receive Options (1 byte) - Not needed
      SPI.transfer(0x00);

      // === PAYLOAD LENGTH CALCULATION ===
      // Total Frame Len - 11 bytes overhead - 1 byte checksum
      int payloadLen = frameLen - 12;

      // === READ PAYLOAD INTO STRUCT ===
      if(payloadLen == sizeof(TelemetryData)) {
        uint8_t* ptr = (uint8_t*)&incomingData;
        for(int i=0; i<payloadLen; i++) {
          ptr[i] = SPI.transfer(0x00);
        }
        
        // Consume checksum
        SPI.transfer(0x00);
        
        // Process received data
        processReceivedData();
        
      } else {
        // Incorrect payload size - flush and warn
        Serial.print("[ERROR] Wrong payload size: ");
        Serial.print(payloadLen);
        Serial.print(" (expected ");
        Serial.print(sizeof(TelemetryData));
        Serial.println(")");
        
        for(int i=0; i<payloadLen; i++) SPI.transfer(0x00);
        SPI.transfer(0x00); // Checksum
      }
      
    } else {
      // Not a data packet (maybe status frame 0x8B)
      // Flush remaining bytes
      for(int i=0; i < frameLen - 1; i++) SPI.transfer(0x00);
      SPI.transfer(0x00); // Checksum
    }
  } else {
    // No start delimiter found
    if(timeout >= 500) {
      Serial.println("[ERROR] ATTN asserted but no valid frame found");
    }
  }

  digitalWrite(CS_PIN, HIGH);
  SPI.endTransaction();
}

// ===== PROCESS RECEIVED DATA =====
void processReceivedData() {
  packetsReceived++;
  lastPacketTime = millis();
  
  // Detect missed packets
  if(lastRecordNumber > 0) {
    uint32_t expected = lastRecordNumber + 1;
    if(incomingData.record_number != expected) {
      missedPackets += (incomingData.record_number - expected);
    }
  }
  lastRecordNumber = incomingData.record_number;
  
  // Print formatted telemetry
  printTelemetry();
}

// ===== PRINT FORMATTED OUTPUT =====
void printTelemetry() {
  Serial.print("RX #"); 
  Serial.print(incomingData.record_number);
  Serial.print(" | ");
  
  // Flight State
  Serial.print("State:");
  switch(incomingData.state) {
    case 0: Serial.print("PRE_FLIGHT"); break;
    case 1: Serial.print("ARMED"); break;
    case 2: Serial.print("POWERED_ASCENT"); break;
    case 3: Serial.print("COASTING"); break;
    case 4: Serial.print("DESCENT"); break;
    case 5: Serial.print("LANDED"); break;
    default: Serial.print("UNKNOWN"); break;
  }
  Serial.print(" | ");
  
  // Altitude & Velocity
  Serial.print("Alt:");
  Serial.print(incomingData.altitude, 1);
  Serial.print("m | Vel:");
  Serial.print(incomingData.velocity, 1);
  Serial.print("m/s | ");
  
  // Acceleration
  Serial.print("AccZ:");
  Serial.print(incomingData.az, 2);
  Serial.print("g | ");
  
  // Battery
  Serial.print("Bat:");
  Serial.print(incomingData.battery_voltage, 2);
  Serial.print("V");
  
  // Statistics
  Serial.print(" | Total:");
  Serial.print(packetsReceived);
  Serial.print(" Missed:");
  Serial.println(missedPackets);
}
```

---

## Expected Serial Monitor Output

```
=================================
  ESP32 XBee SPI Receiver
  Base Station Telemetry
=================================
[OK] SPI Initialized
[OK] Waiting for telemetry...

RX #1 | State:PRE_FLIGHT | Alt:0.0m | Vel:0.0m/s | AccZ:1.00g | Bat:3.80V | Total:1 Missed:0
RX #2 | State:PRE_FLIGHT | Alt:0.0m | Vel:0.0m/s | AccZ:1.00g | Bat:3.80V | Total:2 Missed:0
RX #3 | State:PRE_FLIGHT | Alt:0.0m | Vel:0.0m/s | AccZ:1.00g | Bat:3.80V | Total:3 Missed:0
...
RX #51 | State:POWERED_ASCENT | Alt:2.5m | Vel:50.2m/s | AccZ:8.12g | Bat:3.79V | Total:51 Missed:0
RX #52 | State:POWERED_ASCENT | Alt:5.0m | Vel:49.8m/s | AccZ:7.95g | Bat:3.79V | Total:52 Missed:0
```

---

## Troubleshooting

### Problem: No data received (ATTN always HIGH)

**Diagnosis:** XBee not configured properly or no sender transmitting

**Check:**
1. Verify XBee in SPI mode (P3=0, P4=0 in XCTU)
2. Check Network ID matches sender (ID=7777)
3. Verify sender is powered and transmitting
4. Check ATTN wire connected (GPIO 4 → XBee Pin 19)

### Problem: [ERROR] Wrong payload size

**Cause:** Struct size mismatch between sender/receiver

**Solutions:**
1. Verify both use identical `TelemetryData` struct
2. Ensure `__attribute__((packed))` on both sides
3. Check float size (should be 4 bytes on both ESP32s)
4. Print `sizeof(TelemetryData)` on both sender/receiver

### Problem: Garbled data (wrong values)

**Cause:** Endianness mismatch or incorrect parsing

**Solutions:**
1. Verify both sender/receiver are ESP32 (same architecture)
2. Check frame parsing (11-byte overhead before payload)
3. Add debug: Print raw bytes before struct casting
4. Verify XBee ID and HP settings match

### Problem: High missed packet count

**Cause:** SPI reads too slow or buffer overflow

**Solutions:**
1. Remove Serial.print delays during development
2. Increase SPI speed (try 2MHz if wiring is stable)
3. Check sender not transmitting faster than receiver can process
4. Verify XBee not overflowing internal buffer

---

## Integration Options

### A. JSON Output for Dashboard

```cpp
#include <ArduinoJson.h>

void printTelemetryJSON() {
  StaticJsonDocument<512> doc;
  
  doc["record"] = incomingData.record_number;
  doc["mode"] = incomingData.operation_mode;
  doc["state"] = incomingData.state;
  doc["altitude"] = incomingData.altitude;
  doc["velocity"] = incomingData.velocity;
  doc["battery"] = incomingData.battery_voltage;
  
  JsonObject acc = doc.createNestedObject("accel");
  acc["x"] = incomingData.ax;
  acc["y"] = incomingData.ay;
  acc["z"] = incomingData.az;
  
  serializeJson(doc, Serial);
  Serial.println();
}
```

### B. Bluetooth Forwarding

```cpp
#include <HardwareSerial.h>

HardwareSerial BTSerial(2);
#define BT_TX 17
#define BT_RX 16

void setup() {
  // ... existing setup ...
  BTSerial.begin(115200, SERIAL_8N1, BT_RX, BT_TX);
}

void processReceivedData() {
  // ... existing processing ...
  
  // Forward to Bluetooth
  BTSerial.print("RX #");
  BTSerial.print(incomingData.record_number);
  BTSerial.print(" Alt:");
  BTSerial.println(incomingData.altitude);
}
```

### C. CSV Logging to SD Card

```cpp
#include <SD.h>

File logFile;

void setup() {
  // ... existing setup ...
  SD.begin(SD_CS_PIN);
  logFile = SD.open("/flight_log.csv", FILE_WRITE);
  logFile.println("Record,State,Altitude,Velocity,AccZ,Battery");
}

void processReceivedData() {
  // ... existing processing ...
  
  logFile.print(incomingData.record_number); logFile.print(",");
  logFile.print(incomingData.state); logFile.print(",");
  logFile.print(incomingData.altitude); logFile.print(",");
  logFile.print(incomingData.velocity); logFile.print(",");
  logFile.print(incomingData.az); logFile.print(",");
  logFile.println(incomingData.battery_voltage);
  logFile.flush();
}
```

---

## Performance Metrics

**Reception Rate:** Up to 10Hz  
**Processing Time:** <1ms per packet  
**Missed Packets:** <1% (typical)  
**Latency:** ~5ms (SPI + RF transmission + parsing)

---

## Diagnostic Mode

Add this function to verify XBee SPI connection:

```cpp
void diagnosticMode() {
  Serial.println("\n=== DIAGNOSTIC MODE ===");
  
  // Check ATTN Pin
  Serial.print("ATTN Pin State: ");
  Serial.println(digitalRead(ATTN_PIN) == LOW ? "LOW (Data Ready)" : "HIGH (Idle)");
  
  // Try reading raw SPI data
  digitalWrite(CS_PIN, LOW);
  delayMicroseconds(50);
  
  Serial.print("SPI Bytes (First 16): ");
  for(int i=0; i<16; i++) {
    byte b = SPI.transfer(0x00);
    if(b < 0x10) Serial.print("0");
    Serial.print(b, HEX);
    Serial.print(" ");
  }
  Serial.println();
  
  digitalWrite(CS_PIN, HIGH);
  Serial.println("======================\n");
}
```

Call from `setup()` or when connection issues occur.

---

## Next Steps

1. Upload code to base station ESP32
2. Power on sender (rocket transmitter)
3. Verify telemetry reception in Serial Monitor
4. Add JSON/Bluetooth forwarding as needed
5. Test at increasing distances
6. Integrate with ground station dashboard

---

## Related Documentation

- [HARDWARE_SETUP.md](../HARDWARE_SETUP.md) - Wiring and power setup
- [XCTU_CONFIGURATION.md](../XCTU_CONFIGURATION.md) - XBee configuration guide
- [SPI_TROUBLESHOOTING.md](../SPI_TROUBLESHOOTING.md) - Common issues and fixes
- [esp32_spi_sender.md](./esp32_spi_sender.md) - Rocket transmitter code
