/*
 * XBee Pro 900HP - UART Receiver (Arduino Uno)
 * 
 * Receives binary telemetry packets via UART from XBee
 * Uses SoftwareSerial on pins 2 (RX) and 3 (TX)
 * Parses 36-byte struct and prints to Serial Monitor
 * 
 * XBee Configuration Required (XCTU):
 * - AP = 0 (Transparent Mode - NOT API Mode)
 * - BD = 3 (9600 baud) - Must match sketch
 * - ID = 7777 (Network ID - must match sender)
 * - HP = 0 (Preamble ID - must match sender)
 * - DL = FFFF (Broadcast destination)
 * - P3 = 1 (UART DOUT Enabled - Default)
 * - P4 = 1 (UART DIN Enabled - Default)
 * 
 * Wiring (Arduino Uno -> XBee):
 * Arduino GND -> XBee Pin 10 (GND)
 * Arduino 5V  -> XBee Shield 5V input (Shield regulates to 3.3V)
 * Arduino Pin 2 (RX) -> XBee Pin 2 (DOUT/TX)
 * Arduino Pin 3 (TX) -> XBee Pin 3 (DIN/RX)
 * 
 * IMPORTANT: If using Arduino Uno R4 WiFi or Mega, use HardwareSerial instead:
 *   HardwareSerial xbee = Serial1;
 *   xbee.begin(9600);
 * 
 * Serial Monitor: 9600 baud
 */

#include <SoftwareSerial.h>

// SoftwareSerial: RX=Pin 2, TX=Pin 3
SoftwareSerial xbee(2, 3);

// --- DATA STRUCTURE (Must Match Sender) ---
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
uint8_t buffer[sizeof(TelemetryData)];
int bufferIndex = 0;
unsigned long lastPacketTime = 0;
uint32_t packetsReceived = 0;
uint32_t corruptedPackets = 0;

void setup() {
  Serial.begin(9600);
  xbee.begin(9600);  // Must match XBee BD setting
  
  pinMode(13, OUTPUT); // Built-in LED for activity indicator
  
  Serial.println("====================================");
  Serial.println("  XBee UART Receiver - Arduino Uno");
  Serial.println("====================================");
  Serial.println();
  Serial.println("Waiting for telemetry packets...");
  Serial.println("Expected packet size: " + String(sizeof(TelemetryData)) + " bytes");
  Serial.println();
}

void loop() {
  // Read incoming bytes from XBee
  while (xbee.available()) {
    buffer[bufferIndex++] = xbee.read();
    
    // Once we have a full packet
    if (bufferIndex >= sizeof(TelemetryData)) {
      // Copy buffer to struct
      memcpy(&incomingData, buffer, sizeof(TelemetryData));
      
      // Validate data (basic sanity check)
      if (validatePacket()) {
        packetsReceived++;
        lastPacketTime = millis();
        printData();
        
        // Blink LED to show activity
        digitalWrite(13, HIGH);
        delay(50);
        digitalWrite(13, LOW);
      } else {
        corruptedPackets++;
        Serial.println("ERROR: Corrupted packet detected");
      }
      
      // Reset buffer for next packet
      bufferIndex = 0;
    }
  }
  
  // Connection timeout detection (5 seconds)
  if (packetsReceived > 0 && (millis() - lastPacketTime > 5000)) {
    Serial.println("WARNING: No packets received for 5 seconds");
    delay(1000); // Print once per second
  }
}

bool validatePacket() {
  // Basic sanity checks to detect corrupted data
  
  // 1. Record number should be incrementing (not wildly out of range)
  static uint32_t lastRecordNumber = 0;
  if (incomingData.record_number > 0 && incomingData.record_number < lastRecordNumber) {
    return false; // Record number went backwards
  }
  if (incomingData.record_number - lastRecordNumber > 100) {
    return false; // Jumped too much (possible corruption)
  }
  lastRecordNumber = incomingData.record_number;
  
  // 2. Operation mode should be 0 or 1
  if (incomingData.operation_mode > 1) {
    return false;
  }
  
  // 3. Altitude should be reasonable (not NaN or extreme values)
  if (isnan(incomingData.altitude) || incomingData.altitude < -100 || incomingData.altitude > 10000) {
    return false;
  }
  
  // 4. Battery voltage should be reasonable (2.5V - 4.5V)
  if (isnan(incomingData.battery_voltage) || incomingData.battery_voltage < 2.5 || incomingData.battery_voltage > 4.5) {
    return false;
  }
  
  return true; // Packet looks valid
}

void printData() {
  Serial.print("Packet #"); Serial.print(incomingData.record_number);
  Serial.print(" | Mode: "); Serial.print(incomingData.operation_mode);
  Serial.print(" | State: "); Serial.print(incomingData.state);
  Serial.println();
  
  Serial.print("  Accel: X="); Serial.print(incomingData.ax, 2);
  Serial.print("g Y="); Serial.print(incomingData.ay, 2);
  Serial.print("g Z="); Serial.print(incomingData.az, 2); Serial.println("g");
  
  Serial.print("  Alt: "); Serial.print(incomingData.altitude, 1); Serial.print("m");
  Serial.print(" | Vel: "); Serial.print(incomingData.velocity, 1); Serial.print("m/s");
  Serial.print(" | Battery: "); Serial.print(incomingData.battery_voltage, 2); Serial.println("V");
  
  Serial.print("  Total Rx: "); Serial.print(packetsReceived);
  Serial.print(" | Corrupted: "); Serial.println(corruptedPackets);
  Serial.println();
}
