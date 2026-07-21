/*
 * XBee Pro 900HP - UART CSV Receiver (ESP32 - Ground Station)
 * 
 * FINAL PRODUCTION CODE - Flight Ready
 * 
 * Receives CSV telemetry via high-speed UART from XBee
 * Parses and prints formatted data to Serial Monitor
 * Can forward to Python dashboard via USB Serial
 * 
 * XBee Configuration Required (XCTU) - CRITICAL:
 * - AP = 0 (Transparent Mode - NOT API Mode!)
 * - BD = 7 (115200 baud)
 * - P3 = 1 (UART DOUT Enabled)
 * - P4 = 1 (UART DIN Enabled)
 * - D1, D2, D3, D4, P2 = 0 (Disable SPI pins)
 * - ID = 7777 (Network ID - must match sender)
 * - HP = 0 (Preamble ID - must match sender)
 * 
 * Wiring (ESP32 -> XBee):
 * ESP32 GND     -> XBee Pin 10 (GND)
 * ESP32 3V3     -> XBee Pin 1  (VCC) - Use Shield regulator if available
 * ESP32 GPIO 32 -> XBee Pin 3  (DIN/RX)
 * ESP32 GPIO 34 -> XBee Pin 2  (DOUT/TX)
 * 
 * Serial Monitor: 115200 baud
 * 
 * CSV Format Received: timestamp,state,altitude,velocity,accel_z,battery
 * Example: 1250,1,145.2,340.0,15.2,4.1
 */

#include <HardwareSerial.h>

// --- CONFIGURATION ---
#define RX_PIN 34  // Connect to XBee DOUT (Pin 2)
#define TX_PIN 32  // Connect to XBee DIN  (Pin 3)
#define BAUD_RATE 115200

HardwareSerial XBeeSerial(2);

// Statistics
unsigned long packetsReceived = 0;
unsigned long lastPacketTime = 0;
unsigned long parseErrors = 0;

// Parsed telemetry values
struct TelemetryData {
  unsigned long timestamp;
  int state;
  float altitude;
  float velocity;
  float accel_z;
  float battery;
} telemetry;

void setup() {
  Serial.begin(115200); // USB to PC
  
  // Initialize XBee UART
  XBeeSerial.begin(BAUD_RATE, SERIAL_8N1, RX_PIN, TX_PIN);
  
  Serial.println("====================================");
  Serial.println("  GROUND STATION - UART CSV MODE");
  Serial.println("====================================");
  Serial.println();
  Serial.println("Configuration:");
  Serial.println("  Baud Rate: 115200");
  Serial.println("  Format: CSV (Comma-Separated)");
  Serial.println();
  Serial.println("Waiting for telemetry...");
  Serial.println();
}

void loop() {
  // Check if data is available in the XBee Buffer
  if (XBeeSerial.available()) {
    
    // Read the entire line until the Newline character ('\n')
    String incomingLine = XBeeSerial.readStringUntil('\n');
    
    // Trim whitespace (removes \r or extra spaces)
    incomingLine.trim();

    // Basic Validation: If line is not empty, process it
    if (incomingLine.length() > 0) {
      packetsReceived++;
      lastPacketTime = millis();
      
      // Parse CSV and print formatted output
      if (parseCSV(incomingLine)) {
        printFormattedData();
      } else {
        parseErrors++;
        Serial.println("ERROR: Failed to parse CSV line");
      }
    }
  }
  
  // Connection timeout detection (3 seconds)
  if (packetsReceived > 0 && (millis() - lastPacketTime > 3000)) {
    Serial.println();
    Serial.println("WARNING: No packets for 3 seconds");
    Serial.println("Check:");
    Serial.println("  - Rocket is powered on");
    Serial.println("  - XBee ID/HP settings match");
    Serial.println("  - Rocket is within range");
    Serial.println();
    delay(2000); // Print once every 2 seconds
  }
}

// Parse CSV string into telemetry struct
// Expected format: "1250,1,145.2,340.0,15.2,4.1"
bool parseCSV(String data) {
  int fieldCount = 0;
  int startIndex = 0;
  
  for (int i = 0; i <= data.length(); i++) {
    if (i == data.length() || data.charAt(i) == ',') {
      String field = data.substring(startIndex, i);
      
      switch(fieldCount) {
        case 0: telemetry.timestamp = field.toInt(); break;
        case 1: telemetry.state = field.toInt(); break;
        case 2: telemetry.altitude = field.toFloat(); break;
        case 3: telemetry.velocity = field.toFloat(); break;
        case 4: telemetry.accel_z = field.toFloat(); break;
        case 5: telemetry.battery = field.toFloat(); break;
      }
      
      fieldCount++;
      startIndex = i + 1;
    }
  }
  
  // Valid CSV should have 6 fields
  return (fieldCount == 6);
}

void printFormattedData() {
  Serial.print("Packet #"); Serial.print(packetsReceived);
  Serial.print(" [T="); Serial.print(telemetry.timestamp); Serial.print("ms]");
  Serial.println();
  
  // Flight State
  Serial.print("  State: ");
  switch(telemetry.state) {
    case 0: Serial.print("PAD      "); break;
    case 1: Serial.print("BOOST    "); break;
    case 2: Serial.print("COAST    "); break;
    case 3: Serial.print("DROGUE   "); break;
    case 4: Serial.print("MAIN     "); break;
    case 5: Serial.print("LANDED   "); break;
    default: Serial.print("UNKNOWN  "); break;
  }
  
  // Altitude & Velocity
  Serial.print("| Alt: ");
  Serial.print(telemetry.altitude, 1);
  Serial.print("m | Vel: ");
  Serial.print(telemetry.velocity, 1);
  Serial.print("m/s");
  Serial.println();
  
  // Acceleration & Battery
  Serial.print("  AccZ: ");
  Serial.print(telemetry.accel_z, 1);
  Serial.print("g | Battery: ");
  Serial.print(telemetry.battery, 2);
  Serial.print("V");
  Serial.println();
  
  // Statistics
  Serial.print("  Total Rx: "); Serial.print(packetsReceived);
  Serial.print(" | Parse Errors: "); Serial.print(parseErrors);
  Serial.println();
  Serial.println();
}
