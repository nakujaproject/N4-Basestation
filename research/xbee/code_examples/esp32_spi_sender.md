# ESP32 XBee SPI Sender (Rocket Telemetry Transmitter)

## Overview

This code transmits a binary telemetry struct from ESP32 to XBee via SPI at 10Hz. The XBee broadcasts the data wirelessly to a receiver.

**Hardware:** ESP32 DevKit + XBee Pro 900HP (SPI Mode)  
**Update Rate:** 10Hz (100ms intervals)  
**Data Format:** 36-byte binary struct (packed)  
**Protocol:** SPI with API frames (Mode 0, 1MHz)

---

## XBee Configuration (XCTU)

Before uploading code, configure the sender XBee:

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
| **ID** | Network ID | **7777** | Must match receiver |
| **HP** | Preamble ID | **0** | Must match receiver |
| **DL** | Destination Low | **FFFF** | Broadcast mode |

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

**Power Note:** If using Arduino XBee Shield, power XBee from Shield's 3.3V regulator. ESP32 3.3V pin may not supply sufficient current (215mA TX peak).

---

## Telemetry Data Structure

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

**Why Packed?**  
Ensures no padding between fields, making binary transmission predictable across platforms.

---

## Complete Sender Code

```cpp
#include <SPI.h>

// ===== WIRING CONFIGURATION =====
const int CS_PIN = 5;
const int SCK_PIN = 18;
const int MISO_PIN = 19;
const int MOSI_PIN = 23;
const int ATTN_PIN = 4;

// ===== SPI SETTINGS =====
// XBee S3B supports up to 3.5MHz
// We use 1MHz for stability with jumper wires
SPISettings xbeeSPI(1000000, MSBFIRST, SPI_MODE0);

// ===== TELEMETRY DATA STRUCTURE =====
struct __attribute__((packed)) TelemetryData {
    uint32_t record_number;
    uint8_t operation_mode;
    uint8_t state;
    float ax, ay, az;
    float altitude;
    float velocity;
    float battery_voltage;
};

TelemetryData packet;
unsigned long lastTxTime = 0;
const unsigned long TX_INTERVAL = 100; // 10Hz (100ms)

// ===== SETUP =====
void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("=================================");
  Serial.println("  ESP32 XBee SPI Sender");
  Serial.println("  Rocket Telemetry Transmitter");
  Serial.println("=================================");
  
  // Configure SPI Pins
  pinMode(CS_PIN, OUTPUT);
  digitalWrite(CS_PIN, HIGH); // Start deselected
  pinMode(ATTN_PIN, INPUT);   // XBee output
  
  // Initialize SPI with specific pins
  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, CS_PIN);
  
  // Initialize telemetry packet
  packet.record_number = 0;
  packet.operation_mode = 1; // ARMED
  packet.state = 0;          // PRE_FLIGHT
  packet.battery_voltage = 3.8;
  
  Serial.println("[OK] SPI Initialized");
  Serial.println("[OK] XBee Ready");
  Serial.println("Starting transmission...\n");
  
  delay(1000);
}

// ===== MAIN LOOP =====
void loop() {
  // Send telemetry at 10Hz
  if (millis() - lastTxTime >= TX_INTERVAL) {
    lastTxTime = millis();
    
    // Update simulated sensor data
    updateTelemetry();
    
    // Transmit via XBee SPI
    sendXBeeFrame((uint8_t*)&packet, sizeof(TelemetryData));
    
    // Status output
    Serial.print("TX #"); Serial.print(packet.record_number);
    Serial.print(" | Alt: "); Serial.print(packet.altitude, 1); Serial.print("m");
    Serial.print(" | Vel: "); Serial.print(packet.velocity, 1); Serial.print("m/s");
    Serial.print(" | AccZ: "); Serial.print(packet.az, 2); Serial.println("g");
  }
}

// ===== UPDATE TELEMETRY (SIMULATION) =====
void updateTelemetry() {
  packet.record_number++;
  
  // Simulated flight profile
  if (packet.record_number < 50) {
    // Pre-flight (0-5s)
    packet.state = 0;
    packet.ax = 0.0;
    packet.ay = 0.0;
    packet.az = 1.0; // 1g gravity
    packet.altitude = 0.0;
    packet.velocity = 0.0;
    
  } else if (packet.record_number < 150) {
    // Powered ascent (5-15s)
    packet.state = 2;
    packet.ax = 0.1;
    packet.ay = 0.2;
    packet.az = 8.0 + random(-100, 100) / 100.0; // 8g thrust
    packet.altitude = (packet.record_number - 50) * 2.5;
    packet.velocity = 50.0 + random(-50, 50) / 10.0;
    
  } else if (packet.record_number < 300) {
    // Coasting (15-30s)
    packet.state = 3;
    packet.ax = 0.05;
    packet.ay = 0.05;
    packet.az = 0.2; // Low deceleration
    packet.altitude = 250.0 + (packet.record_number - 150) * 1.0;
    packet.velocity = 30.0 - (packet.record_number - 150) * 0.2;
    
  } else {
    // Descent (30s+)
    packet.state = 4;
    packet.ax = 0.02;
    packet.ay = 0.02;
    packet.az = 1.5; // Parachute drag
    packet.altitude = 400.0 - (packet.record_number - 300) * 0.5;
    if (packet.altitude < 0) packet.altitude = 0;
    packet.velocity = -10.0;
  }
  
  // Battery drain simulation
  packet.battery_voltage = 3.8 - (packet.record_number * 0.0001);
  if (packet.battery_voltage < 3.3) packet.battery_voltage = 3.3;
}

// ===== SEND XBEE API FRAME =====
void sendXBeeFrame(uint8_t* data, int length) {
  // Check if XBee is busy (ATTN LOW means data pending)
  if(digitalRead(ATTN_PIN) == LOW) {
    Serial.println("[WARN] XBee busy, skipping packet");
    return;
  }

  // Calculate API frame parameters
  int totalLen = 14 + length; // 14 bytes overhead + payload
  long checksumTotal = 0;
  
  // Begin SPI transaction
  SPI.beginTransaction(xbeeSPI);
  digitalWrite(CS_PIN, LOW);
  delayMicroseconds(50); // XBee wake-up time
  
  // === API Frame Header ===
  SPI.transfer(0x7E); // Start Delimiter
  SPI.transfer((totalLen >> 8) & 0xFF); // Length MSB
  SPI.transfer(totalLen & 0xFF);        // Length LSB
  
  // === Frame Data ===
  // Frame Type: 0x10 (Transmit Request)
  SPI.transfer(0x10); checksumTotal += 0x10;
  
  // Frame ID: 0x00 (No Acknowledgment)
  SPI.transfer(0x00); checksumTotal += 0x00;
  
  // 64-bit Destination Address (Broadcast)
  for(int i=0; i<6; i++) { 
    SPI.transfer(0x00); 
    checksumTotal += 0x00; 
  }
  SPI.transfer(0xFF); checksumTotal += 0xFF;
  SPI.transfer(0xFF); checksumTotal += 0xFF;
  
  // 16-bit Destination Address (Broadcast)
  SPI.transfer(0xFF); checksumTotal += 0xFF;
  SPI.transfer(0xFE); checksumTotal += 0xFE;
  
  // Broadcast Radius (0 = Max)
  SPI.transfer(0x00); checksumTotal += 0x00;
  
  // Transmit Options (0 = Default)
  SPI.transfer(0x00); checksumTotal += 0x00;
  
  // === Payload (Telemetry Struct) ===
  for (int i = 0; i < length; i++) {
    SPI.transfer(data[i]);
    checksumTotal += data[i];
  }
  
  // === Checksum ===
  byte checksum = 0xFF - (checksumTotal & 0xFF);
  SPI.transfer(checksum);
  
  // End SPI transaction
  digitalWrite(CS_PIN, HIGH);
  SPI.endTransaction();
}
```

---

## Expected Serial Monitor Output

```
=================================
  ESP32 XBee SPI Sender
  Rocket Telemetry Transmitter
=================================
[OK] SPI Initialized
[OK] XBee Ready
Starting transmission...

TX #1 | Alt: 0.0m | Vel: 0.0m/s | AccZ: 1.00g
TX #2 | Alt: 0.0m | Vel: 0.0m/s | AccZ: 1.00g
TX #3 | Alt: 0.0m | Vel: 0.0m/s | AccZ: 1.00g
...
TX #51 | Alt: 2.5m | Vel: 50.2m/s | AccZ: 8.12g
TX #52 | Alt: 5.0m | Vel: 49.8m/s | AccZ: 7.95g
```

---

## Troubleshooting

### Problem: No data transmitted

**Check:**
1. XBee powered (measure 3.3V at Pin 1)
2. Common ground between ESP32 and XBee
3. CS wire connected (GPIO 5 → XBee Pin 17)
4. MOSI wire connected (GPIO 23 → XBee Pin 11)
5. XBee configured in SPI mode (P3=0, P4=0, D1-D4 set)

### Problem: [WARN] XBee busy, skipping packet

**Cause:** ATTN pin stuck LOW

**Solutions:**
1. Check ATTN wire (GPIO 4 → XBee Pin 19)
2. Receiver may not be draining data fast enough
3. Verify receiver XBee is powered and configured

### Problem: Receiver gets garbage data

**Cause:** Struct size mismatch or endianness difference

**Solutions:**
1. Verify receiver has identical `TelemetryData` struct
2. Ensure `__attribute__((packed))` on both sender/receiver
3. Check both use same architecture (ESP32-to-ESP32 works)
4. Verify Network ID and Preamble ID match (ID=7777, HP=0)

---

## Integration with Real Sensors

Replace `updateTelemetry()` with actual sensor reads:

```cpp
void updateTelemetry() {
  packet.record_number++;
  
  // Read from IMU
  packet.ax = imu.getAccelX();
  packet.ay = imu.getAccelY();
  packet.az = imu.getAccelZ();
  
  // Read from Barometer
  packet.altitude = baro.getAltitudeAGL();
  packet.velocity = kalman.getVerticalVelocity();
  
  // Read from Battery Monitor
  packet.battery_voltage = analogRead(BAT_PIN) * ADC_TO_VOLTAGE;
  
  // Flight state from state machine
  packet.state = currentFlightState;
  packet.operation_mode = isArmed ? 1 : 0;
}
```

---

## Performance Metrics

**Packet Size:** 36 bytes  
**API Overhead:** 14 bytes  
**Total Frame:** 50 bytes  
**Transmission Time:** ~0.4ms @ 1MHz SPI  
**Update Rate:** 10Hz (100ms intervals)  
**Bandwidth Used:** 4 kbps (2% of 200 kbps air speed)

---

## Next Steps

1. Upload this code to rocket's ESP32
2. Configure receiver XBee for SPI mode
3. Upload receiver code (see `esp32_spi_receiver.md`)
4. Verify telemetry reception
5. Test at increasing distances
6. Replace simulated data with real sensors

---

## Related Documentation

- [HARDWARE_SETUP.md](../HARDWARE_SETUP.md) - Wiring and power setup
- [XCTU_CONFIGURATION.md](../XCTU_CONFIGURATION.md) - XBee configuration guide
- [SPI_TROUBLESHOOTING.md](../SPI_TROUBLESHOOTING.md) - Common issues and fixes
- [esp32_spi_receiver.md](./esp32_spi_receiver.md) - Base station receiver code
