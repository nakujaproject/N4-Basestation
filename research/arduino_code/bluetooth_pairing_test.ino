/**
 * N4 Bluetooth Pairing & Telemetry Test
 * 
 * This ESP32 code helps identify which Bluetooth COM port is the correct one
 * by appending device ID to all telemetry packets.
 * 
 * Behavior:
 * 1. Continuously sends telemetry at 10 Hz with |ESP32:N4_BASE_BT_1 appended
 * 2. Python script detects this identifier to find correct COM port
 * 3. No handshake needed - one-way communication
 * 
 * Upload this to your ESP32 with HC-05 connected to test Bluetooth setup
 */

#include <HardwareSerial.h>

// Bluetooth Serial Configuration
HardwareSerial BTSerial(2);  // UART2
#define BT_TX 17  // ESP32 TX to HC-05 RX
#define BT_RX 16  // ESP32 RX to HC-05 TX

// Identification Configuration
const char* DEVICE_ID = "ESP32:N4_BASE_BT_1";
const unsigned long TELEMETRY_INTERVAL = 100;  // Send telemetry every 100ms (10 Hz)

// State tracking
unsigned long lastTelemetryTime = 0;
bool stopTransmission = false;
uint32_t recordNumber = 0;
uint32_t packetsReceived = 0;

// Helper function to repeat a character
String repeatChar(char c, int count) {
  String result = "";
  for (int i = 0; i < count; i++) {
    result += c;
  }
  return result;
}

// Simulated telemetry data structure
struct SimulatedTelemetry {
  uint32_t record_number;
  uint8_t operation_mode;  // 0=SAFE, 1=ARMED
  uint8_t state;  // Flight state
  float ax, ay, az;
  float pitch, roll;
  float gx, gy, gz;
  float latitude, longitude;
  float gps_altitude;
  uint32_t gps_time;
  float pressure;
  float temperature;
  float altitude_agl;
  float velocity;
  uint8_t drogue_pin_state;
  uint8_t main_chute_pin_state;
  float battery_voltage;
  int32_t wifi_rssi;
  float kalman_altitude;
  float kalman_vertical_velocity;
};

SimulatedTelemetry telemetry;

void setup() {
  Serial.begin(115200);
  BTSerial.begin(115200, SERIAL_8N1, BT_RX, BT_TX);
  
  delay(500);
  
  Serial.println("\n" + repeatChar('=', 60));
  Serial.println("N4 Bluetooth Telemetry Test");
  Serial.println(repeatChar('=', 60));
  Serial.println("Device ID: " + String(DEVICE_ID));
  Serial.println("Mode: Continuous telemetry with ID appended");
  Serial.println(repeatChar('=', 60) + "\n");
  
  // Initialize simulated telemetry with realistic values
  initializeTelemetry();
  
  lastTelemetryTime = millis();
  
  // Start sending telemetry with ID appended
  Serial.println("📡 Sending telemetry at 10 Hz with |" + String(DEVICE_ID) + " appended");
  Serial.println("✅ Ready for laptop connection\n");
}

void initializeTelemetry() {
  telemetry.record_number = 0;
  telemetry.operation_mode = 0;  // SAFE mode
  telemetry.state = 0;  // PRE_LAUNCH
  telemetry.ax = -0.59;
  telemetry.ay = -0.02;
  telemetry.az = 0.69;
  telemetry.pitch = -36.0;
  telemetry.roll = -2.0;
  telemetry.gx = -5.5;
  telemetry.gy = 3.0;
  telemetry.gz = 2.8;
  telemetry.latitude = 0.0;
  telemetry.longitude = 0.0;
  telemetry.gps_altitude = 0.0;
  telemetry.gps_time = 0;
  telemetry.pressure = 858.0;
  telemetry.temperature = 26.7;
  telemetry.altitude_agl = 0.0;
  telemetry.velocity = 0.0;
  telemetry.drogue_pin_state = 0;
  telemetry.main_chute_pin_state = 0;
  telemetry.battery_voltage = 12.6;
  telemetry.wifi_rssi = -75;
  telemetry.kalman_altitude = 0.0;
  telemetry.kalman_vertical_velocity = 0.0;
}

void updateTelemetry() {
  // Simulate realistic sensor variations
  telemetry.record_number = recordNumber++;
  
  // Add small random variations to simulate real sensors
  telemetry.ax += (random(-10, 10) / 100.0);
  telemetry.ay += (random(-10, 10) / 100.0);
  telemetry.az += (random(-10, 10) / 100.0);
  
  telemetry.pitch += (random(-50, 50) / 100.0);
  telemetry.roll += (random(-50, 50) / 100.0);
  
  telemetry.gx += (random(-20, 20) / 10.0);
  telemetry.gy += (random(-20, 20) / 10.0);
  telemetry.gz += (random(-20, 20) / 10.0);
  
  telemetry.pressure += (random(-5, 5) / 100.0);
  telemetry.temperature += (random(-2, 2) / 100.0);
  
  telemetry.altitude_agl += (random(-10, 10) / 10.0);
  telemetry.kalman_altitude = telemetry.altitude_agl + (random(-5, 5) / 100.0);
  telemetry.kalman_vertical_velocity = (random(-200, 200) / 100.0);
  
  telemetry.gps_time = millis();
  telemetry.wifi_rssi = -75 + random(-10, 10);
  
  // Keep values within reasonable bounds
  telemetry.ax = constrain(telemetry.ax, -2.0, 2.0);
  telemetry.ay = constrain(telemetry.ay, -2.0, 2.0);
  telemetry.az = constrain(telemetry.az, -2.0, 2.0);
  telemetry.pitch = constrain(telemetry.pitch, -90.0, 90.0);
  telemetry.roll = constrain(telemetry.roll, -90.0, 90.0);
  telemetry.altitude_agl = constrain(telemetry.altitude_agl, -5.0, 5.0);
}

void sendTelemetryJSON() {
  // Match the exact JSON format that server.py expects
  String json = "{";
  json += "\"record_number\":" + String(telemetry.record_number) + ",";
  json += "\"operation_mode\":" + String(telemetry.operation_mode) + ",";
  json += "\"state\":" + String(telemetry.state) + ",";
  json += "\"battery_voltage\":" + String(telemetry.battery_voltage, 1) + ",";
  json += "\"wifi_rssi\":" + String(telemetry.wifi_rssi) + ",";
  
  json += "\"acc_data\":{";
  json += "\"ax\":" + String(telemetry.ax, 2) + ",";
  json += "\"ay\":" + String(telemetry.ay, 2) + ",";
  json += "\"az\":" + String(telemetry.az, 2) + ",";
  json += "\"pitch\":" + String(telemetry.pitch, 2) + ",";
  json += "\"roll\":" + String(telemetry.roll, 2);
  json += "},";
  
  json += "\"gyro_data\":{";
  json += "\"gx\":" + String(telemetry.gx, 2) + ",";
  json += "\"gy\":" + String(telemetry.gy, 2) + ",";
  json += "\"gz\":" + String(telemetry.gz, 2);
  json += "},";
  
  json += "\"gps_data\":{";
  json += "\"latitude\":" + String(telemetry.latitude, 6) + ",";
  json += "\"longitude\":" + String(telemetry.longitude, 6) + ",";
  json += "\"gps_altitude\":" + String(telemetry.gps_altitude, 1) + ",";
  json += "\"time\":" + String(telemetry.gps_time);
  json += "},";
  
  json += "\"alt_data\":{";
  json += "\"pressure\":" + String(telemetry.pressure, 2) + ",";
  json += "\"temperature\":" + String(telemetry.temperature, 2) + ",";
  json += "\"AGL\":" + String(telemetry.altitude_agl, 2) + ",";
  json += "\"velocity\":" + String(telemetry.velocity, 2) + ",";
  json += "\"kalman_altitude\":" + String(telemetry.kalman_altitude, 2) + ",";
  json += "\"kalman_vertical_velocity\":" + String(telemetry.kalman_vertical_velocity, 2);
  json += "},";
  
  json += "\"chute_state\":{";
  json += "\"pyro1_state\":" + String(telemetry.drogue_pin_state) + ",";
  json += "\"pyro2_state\":" + String(telemetry.main_chute_pin_state);
  json += "},";
  
  json += "\"connection_status\":{";
  json += "\"connected\":true,";
  json += "\"has_ever_connected\":true,";
  json += "\"packet_age_ms\":0,";
  json += "\"timeout_exceeded\":false,";
  json += "\"rssi\":" + String(telemetry.wifi_rssi);
  json += "},";
  
  json += "\"communication_mode\":\"Bluetooth\",";
  json += "\"timestamp\":" + String(millis()) + ",";
  json += "\"packets_received\":" + String(packetsReceived++);
  json += "}";
  
  // Always append device ID for identification
  json += "|" + String(DEVICE_ID);
  json += "\n";
  
  BTSerial.print(json);
}

void loop() {
  unsigned long currentTime = millis();
  
  // Send telemetry data continuously
  if (!stopTransmission && (currentTime - lastTelemetryTime >= TELEMETRY_INTERVAL)) {
    updateTelemetry();
    sendTelemetryJSON();
    lastTelemetryTime = currentTime;
    
    // Print status to Serial monitor periodically
    if (telemetry.record_number % 50 == 0) {
      Serial.print("📊 Record #");
      Serial.print(telemetry.record_number);
      Serial.print(" | Alt: ");
      Serial.print(telemetry.altitude_agl, 2);
      Serial.print("m | Vel: ");
      Serial.print(telemetry.kalman_vertical_velocity, 2);
      Serial.println(" m/s");
    }
  }
  
  // Check for incoming commands via Bluetooth
  if (BTSerial.available()) {
    String command = BTSerial.readStringUntil('\n');
    command.trim();
    
    // Print all received data to Serial monitor
    Serial.println("📨 Received: " + command);
    
    String commandUpper = command;
    commandUpper.toUpperCase();
    
    // Check for 'q' to stop transmission
    if (commandUpper == "Q") {
      stopTransmission = true;
      BTSerial.println("ACK:TRANSMISSION_STOPPED");
      Serial.println("🛑 'q' received. Halting transmission.");
    }
    else if (commandUpper == "2") {
      stopTransmission = true;
      BTSerial.println("ACK:TRANSMISSION_STOPPED");
      Serial.println("🛑 Stop command received. Halting transmission.");
    }
    else if (commandUpper == "ARM") {
      telemetry.operation_mode = 1;
      BTSerial.println("ACK:ARMED");
      Serial.println("✅ System ARMED");
    }
    else if (commandUpper == "DISARM") {
      telemetry.operation_mode = 0;
      BTSerial.println("ACK:DISARMED");
      Serial.println("✅ System DISARMED");
    }
    else if (commandUpper == "STATUS") {
      String status = "STATUS:";
      status += (telemetry.operation_mode == 1) ? "ARMED" : "SAFE";
      status += ":RECORD:" + String(telemetry.record_number);
      status += ":ALT:" + String(telemetry.altitude_agl, 2);
      BTSerial.println(status);
    }
  }
  
  delay(10);  // Small delay to prevent overwhelming the CPU
}
