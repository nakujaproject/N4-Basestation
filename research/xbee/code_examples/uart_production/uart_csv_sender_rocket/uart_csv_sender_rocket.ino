/*
 * XBee Pro 900HP - UART CSV Sender (ESP32 - Rocket)
 * 
 * FINAL PRODUCTION CODE - Flight Ready
 * 
 * Transmits CSV telemetry at 50Hz via high-speed UART to XBee
 * Format: timestamp,state,altitude,velocity,accel_z,battery
 * 
 * XBee Configuration Required (XCTU) - CRITICAL:
 * - AP = 0 (Transparent Mode - NOT API Mode!)
 * - BD = 7 (115200 baud)
 * - P3 = 1 (UART DOUT Enabled)
 * - P4 = 1 (UART DIN Enabled)
 * - D1, D2, D3, D4, P2 = 0 (Disable SPI pins)
 * - ID = 7777 (Network ID)
 * - HP = 0 (Preamble ID)
 * - DL = FFFF (Broadcast destination)
 * 
 * Wiring (ESP32 -> XBee):
 * ESP32 GND     -> XBee Pin 10 (GND)
 * ESP32 3V3     -> XBee Pin 1  (VCC) - Use Shield regulator if available
 * ESP32 GPIO 32 -> XBee Pin 3  (DIN/RX)
 * ESP32 GPIO 34 -> XBee Pin 2  (DOUT/TX)
 * 
 * IMPORTANT: GPIO 34 is input-only on ESP32, so it's perfect for RX
 * 
 * Serial Monitor: 115200 baud (for debugging)
 */

#include <HardwareSerial.h>

// --- CONFIGURATION ---
#define RX_PIN 34  // Connect to XBee DOUT (Pin 2)
#define TX_PIN 32  // Connect to XBee DIN  (Pin 3)
#define BAUD_RATE 115200

// Use Serial2 (Hardware UART)
HardwareSerial XBeeSerial(2);

unsigned long lastTxTime = 0;
const int TX_INTERVAL = 20; // 20ms = 50Hz Update Rate

// Flight Variables (Replace with real sensor reads)
int flight_state = 1;      // 0=Pad, 1=Boost, 2=Coast, 3=Drogue, 4=Main, 5=Landed
float altitude = 0.0;
float velocity = 0.0;
float accel_z = 0.0;
float battery_voltage = 4.2;

void setup() {
  Serial.begin(115200); // USB Debug
  
  // Initialize XBee UART
  XBeeSerial.begin(BAUD_RATE, SERIAL_8N1, RX_PIN, TX_PIN);
  
  Serial.println("====================================");
  Serial.println("  ROCKET SENDER - UART CSV MODE");
  Serial.println("====================================");
  Serial.println();
  Serial.println("Configuration:");
  Serial.println("  Update Rate: 50Hz (20ms)");
  Serial.println("  Baud Rate: 115200");
  Serial.println("  Format: CSV (Comma-Separated)");
  Serial.println();
  Serial.println("CSV Header:");
  Serial.println("  time,state,alt,vel,acc,batt");
  Serial.println();
  Serial.println("Starting transmission...");
  Serial.println();
  
  // Give XBee a moment to wake up
  delay(1000);
}

void loop() {
  unsigned long currentMillis = millis();

  if (currentMillis - lastTxTime >= TX_INTERVAL) {
    lastTxTime = currentMillis;

    // ===== SIMULATION DATA =====
    // DELETE THIS BLOCK when you add real sensors
    altitude += 3.5;          // Climbing at ~175 m/s
    velocity = 340.0;         // Mach 1
    accel_z = 15.0 + random(-10, 10) / 10.0; // Vibration noise
    battery_voltage = 4.2 - (currentMillis / 1000000.0); // Slow drain
    
    // State progression for testing
    if (altitude > 1000) flight_state = 2; // Coast
    if (altitude > 2000) flight_state = 3; // Drogue
    if (altitude > 3000) { altitude = 3000; flight_state = 4; } // Main
    // ===========================

    // ===== REPLACE WITH REAL SENSOR READS =====
    // altitude = barometer.getAltitudeAGL();
    // velocity = kalman.getVerticalVelocity();
    // accel_z = imu.getAccelZ();
    // battery_voltage = readBatteryVoltage();
    // flight_state = stateMachine.getCurrentState();
    // ==========================================

    // Construct CSV String
    // Format: "1250,1,145.2,340.0,15.2,4.1"
    String dataPacket = String(currentMillis) + "," + 
                        String(flight_state) + "," + 
                        String(altitude, 1) + "," + 
                        String(velocity, 1) + "," + 
                        String(accel_z, 1) + "," + 
                        String(battery_voltage, 2);

    // Send to XBee (println adds the '\n' newline automatically)
    XBeeSerial.println(dataPacket);
    
    // Optional: Print to USB for debugging (comment out for flight to reduce overhead)
    Serial.println(dataPacket);
  }
}
