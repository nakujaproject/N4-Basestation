/**
 * N4 Simulated Base Station - Flight Simulator
 * 
 * This code simulates a complete rocket flight profile for testing the base station
 * software without needing actual rocket hardware. It generates realistic telemetry
 * data that changes over time to simulate: launch, ascent, apogee, descent, and landing.
 * 
 * Features:
 * - Realistic flight physics simulation
 * - Bluetooth output via HC-05/HC-06
 * - Beacon parsing code (commented out - ready for real hardware)
 * - JSON telemetry format matching base station expectations
 * - Device identifier appended for COM port detection
 * - Interactive commands: ARM, DISARM, RESET, RESTART
 * 
 * Hardware Setup:
 * - ESP32 DevKit
 * - HC-05/HC-06 Bluetooth module connected to UART2 (GPIO 16/17)
 * - Baud rate: 115200
 * 
 * Upload this to your ESP32 to test the base station software end-to-end.
 */

#include <HardwareSerial.h>
#include <WiFi.h>
#include <esp_now.h>
#include "esp_wifi.h"
#include <ArduinoJson.h>

// ====== Bluetooth Serial Configuration ======
HardwareSerial BTSerial(2);  // Use UART2 for HC-05/HC-06
#define BT_TX 17  // ESP32 TX to HC-05 RX
#define BT_RX 16  // ESP32 RX to HC-05 TX

// ====== Device Identification ======
const char* DEVICE_ID = "ESP32:N4_BASE_BT_1";

// ====== Simulation Configuration ======
const unsigned long TELEMETRY_INTERVAL = 100;  // Send telemetry every 100ms (10 Hz)
const unsigned long HEARTBEAT_INTERVAL = 10000;  // Status message every 10s

// ====== Flight Simulation Parameters ======
enum FlightPhase {
  PRE_LAUNCH = 0,
  POWERED_ASCENT = 1,
  COASTING = 2,
  APOGEE = 3,
  DROGUE_DESCENT = 4,
  MAIN_DESCENT = 5,
  LANDED = 6
};

struct FlightSimulator {
  FlightPhase phase;
  float time;  // Flight time in seconds
  float altitude;  // Meters AGL
  float velocity;  // m/s (positive = up)
  float acceleration;  // m/s²
  float max_altitude;
  bool drogue_deployed;
  bool main_deployed;
  uint32_t record_number;
  
  // Constants
  const float GRAVITY = 9.81;
  const float MOTOR_THRUST_ACCEL = 80.0;  // m/s² during powered flight
  const float MOTOR_BURN_TIME = 3.5;  // seconds
  const float DRAG_COEFF_ASCENT = 0.015;
  const float DRAG_COEFF_DROGUE = 0.25;
  const float DRAG_COEFF_MAIN = 1.5;
  const float DROGUE_DEPLOY_ALT = 450.0;  // meters
  const float MAIN_DEPLOY_ALT = 200.0;  // meters
};

FlightSimulator sim;

// ====== Telemetry Data Structure (25 fields + Kalman) ======
struct TelemetryData {
  uint32_t record_number;
  uint8_t operation_mode;  // 0=SAFE, 1=ARMED
  uint8_t state;  // Flight phase
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

TelemetryData telemetry;

// ====== State Tracking ======
bool rocketArmed = false;
bool flightActive = false;
unsigned long lastTelemetryTime = 0;
unsigned long lastHeartbeatTime = 0;
uint32_t packetsReceived = 0;

// ====== PWM Configuration (for command compatibility) ======
struct PWMConfigStatus {
  float vcc = 14.8;
  float drogue_v = 9.0;
  float main_v = 10.0;
  unsigned long drogue_time_ms = 3000;
  unsigned long main_time_ms = 5000;
  bool config_received = true;
} pwm_status;

// ====== Helper Functions ======
String repeatChar(char c, int count) {
  String result = "";
  for (int i = 0; i < count; i++) {
    result += c;
  }
  return result;
}

void sendLogMessage(const char* level, const char* message, const char* source) {
  DynamicJsonDocument logDoc(256);
  logDoc["level"] = level;
  logDoc["message"] = message;
  logDoc["source"] = source;
  logDoc["timestamp"] = millis();

  String logString;
  serializeJson(logDoc, logString);

  Serial.print("LOG:");
  Serial.println(logString);

  BTSerial.print("LOG:");
  BTSerial.println(logString);
}

// ====== Flight Simulation Engine ======
void initializeSimulation() {
  sim.phase = PRE_LAUNCH;
  sim.time = 0;
  sim.altitude = 0;
  sim.velocity = 0;
  sim.acceleration = 0;
  sim.max_altitude = 0;
  sim.drogue_deployed = false;
  sim.main_deployed = false;
  sim.record_number = 0;
  
  // Initialize telemetry to pre-launch state
  telemetry.record_number = 0;
  telemetry.operation_mode = rocketArmed ? 1 : 0;
  telemetry.state = PRE_LAUNCH;
  telemetry.ax = -0.59;
  telemetry.ay = -0.02;
  telemetry.az = 0.69;
  telemetry.pitch = -36.0;
  telemetry.roll = -2.0;
  telemetry.gx = -5.5;
  telemetry.gy = 3.0;
  telemetry.gz = 2.8;
  telemetry.latitude = -1.2921;  // Nairobi coordinates
  telemetry.longitude = 36.8219;
  telemetry.gps_altitude = 1661.0;  // Nairobi elevation
  telemetry.gps_time = 0;
  telemetry.pressure = 858.0;
  telemetry.temperature = 26.7;
  telemetry.altitude_agl = 0.0;
  telemetry.velocity = 0.0;
  telemetry.drogue_pin_state = 0;
  telemetry.main_chute_pin_state = 0;
  telemetry.battery_voltage = 14.8;
  telemetry.wifi_rssi = -45;
  telemetry.kalman_altitude = 0.0;
  telemetry.kalman_vertical_velocity = 0.0;
  
  sendLogMessage("INFO", "🔄 Flight simulation initialized", "Simulator");
}

void updateFlightSimulation(float dt) {
  if (!flightActive || !rocketArmed) {
    sim.phase = PRE_LAUNCH;
    return;
  }
  
  sim.time += dt;
  sim.record_number++;
  
  // State machine for flight phases
  switch (sim.phase) {
    case PRE_LAUNCH:
      // Waiting for launch - this shouldn't happen if flightActive is true
      break;
      
    case POWERED_ASCENT:
      if (sim.time < sim.MOTOR_BURN_TIME) {
        // Motor is burning
        sim.acceleration = sim.MOTOR_THRUST_ACCEL - sim.GRAVITY - sim.DRAG_COEFF_ASCENT * sim.velocity * sim.velocity;
        sim.velocity += sim.acceleration * dt;
        sim.altitude += sim.velocity * dt;
      } else {
        // Motor burnout - transition to coasting
        sim.phase = COASTING;
        sendLogMessage("INFO", "🔥 Motor burnout - Coasting phase", "Simulator");
      }
      break;
      
    case COASTING:
      // No thrust, only gravity and drag
      sim.acceleration = -sim.GRAVITY - sim.DRAG_COEFF_ASCENT * sim.velocity * sim.velocity;
      sim.velocity += sim.acceleration * dt;
      sim.altitude += sim.velocity * dt;
      
      // Check for apogee (velocity crosses zero)
      if (sim.velocity <= 0) {
        sim.phase = APOGEE;
        sim.max_altitude = sim.altitude;
        sendLogMessage("INFO", ("🎯 APOGEE reached at " + String(sim.altitude, 1) + "m").c_str(), "Simulator");
      }
      break;
      
    case APOGEE:
      // Deploy drogue immediately at apogee
      sim.drogue_deployed = true;
      sim.phase = DROGUE_DESCENT;
      telemetry.drogue_pin_state = 1;
      sendLogMessage("INFO", "🪂 Drogue chute deployed", "Simulator");
      break;
      
    case DROGUE_DESCENT:
      // Falling with drogue chute
      sim.acceleration = -sim.GRAVITY + sim.DRAG_COEFF_DROGUE * sim.velocity * sim.velocity;
      sim.velocity += sim.acceleration * dt;
      sim.altitude += sim.velocity * dt;
      
      // Check for main chute deployment altitude
      if (sim.altitude <= sim.MAIN_DEPLOY_ALT && !sim.main_deployed) {
        sim.main_deployed = true;
        sim.phase = MAIN_DESCENT;
        telemetry.main_chute_pin_state = 1;
        sendLogMessage("INFO", "🪂 Main chute deployed at " + String(sim.altitude, 1) + "m", "Simulator");
      }
      
      // Check for landing
      if (sim.altitude <= 0) {
        sim.altitude = 0;
        sim.velocity = 0;
        sim.acceleration = 0;
        sim.phase = LANDED;
        flightActive = false;
        sendLogMessage("INFO", "✅ LANDED - Flight complete", "Simulator");
      }
      break;
      
    case MAIN_DESCENT:
      // Falling with main chute
      sim.acceleration = -sim.GRAVITY + sim.DRAG_COEFF_MAIN * sim.velocity * sim.velocity;
      sim.velocity += sim.acceleration * dt;
      sim.altitude += sim.velocity * dt;
      
      // Check for landing
      if (sim.altitude <= 0) {
        sim.altitude = 0;
        sim.velocity = 0;
        sim.acceleration = 0;
        sim.phase = LANDED;
        flightActive = false;
        sendLogMessage("INFO", "✅ LANDED - Flight complete", "Simulator");
      }
      break;
      
    case LANDED:
      // Flight is over
      sim.velocity = 0;
      sim.acceleration = 0;
      break;
  }
  
  // Keep altitude non-negative
  if (sim.altitude < 0) sim.altitude = 0;
}

void updateTelemetryFromSimulation() {
  telemetry.record_number = sim.record_number;
  telemetry.operation_mode = rocketArmed ? 1 : 0;
  telemetry.state = sim.phase;
  
  // Simulate accelerometer data based on flight phase
  if (sim.phase == POWERED_ASCENT) {
    telemetry.az = sim.acceleration / sim.GRAVITY;  // Gs
    telemetry.ax = (random(-50, 50) / 100.0);
    telemetry.ay = (random(-50, 50) / 100.0);
  } else if (sim.phase >= DROGUE_DESCENT) {
    telemetry.az = sim.acceleration / sim.GRAVITY;
    telemetry.ax = (random(-30, 30) / 100.0);
    telemetry.ay = (random(-30, 30) / 100.0);
  } else {
    telemetry.az = 0.69 + (random(-20, 20) / 100.0);
    telemetry.ax = -0.59 + (random(-20, 20) / 100.0);
    telemetry.ay = -0.02 + (random(-20, 20) / 100.0);
  }
  
  // Simulate gyroscope data
  if (sim.phase == POWERED_ASCENT || sim.phase == COASTING) {
    telemetry.gx = random(-100, 100) / 10.0;
    telemetry.gy = random(-100, 100) / 10.0;
    telemetry.gz = random(-100, 100) / 10.0;
  } else {
    telemetry.gx = random(-50, 50) / 10.0;
    telemetry.gy = random(-50, 50) / 10.0;
    telemetry.gz = random(-50, 50) / 10.0;
  }
  
  // Attitude (pitch/roll)
  if (sim.phase == PRE_LAUNCH || sim.phase == LANDED) {
    telemetry.pitch = -36.0 + (random(-10, 10) / 10.0);
    telemetry.roll = -2.0 + (random(-10, 10) / 10.0);
  } else {
    telemetry.pitch = random(-900, 900) / 10.0;
    telemetry.roll = random(-900, 900) / 10.0;
  }
  
  // Altitude and velocity
  telemetry.altitude_agl = sim.altitude;
  telemetry.velocity = sim.velocity;
  
  // Kalman filter estimates (add some realistic noise/filtering)
  telemetry.kalman_altitude = sim.altitude + (random(-50, 50) / 100.0);
  telemetry.kalman_vertical_velocity = sim.velocity + (random(-30, 30) / 100.0);
  
  // Barometric data
  float pressure_change = sim.altitude * 0.12;  // Approximate hPa change per meter
  telemetry.pressure = 858.0 - pressure_change + (random(-10, 10) / 100.0);
  telemetry.temperature = 26.7 - (sim.altitude * 0.0065) + (random(-5, 5) / 10.0);  // Temperature lapse rate
  
  // GPS (simulate small drift during flight)
  if (sim.phase != PRE_LAUNCH && sim.phase != LANDED) {
    telemetry.latitude += (random(-10, 10) / 100000.0);
    telemetry.longitude += (random(-10, 10) / 100000.0);
  }
  telemetry.gps_altitude = 1661.0 + sim.altitude + (random(-50, 50) / 10.0);
  telemetry.gps_time = millis();
  
  // Parachute states
  telemetry.drogue_pin_state = sim.drogue_deployed ? 1 : 0;
  telemetry.main_chute_pin_state = sim.main_deployed ? 1 : 0;
  
  // Battery voltage (slight drain during flight)
  telemetry.battery_voltage = 14.8 - (sim.time * 0.01);
  telemetry.battery_voltage = max(telemetry.battery_voltage, 12.0f);
  
  // RSSI (varies with altitude and phase)
  if (sim.altitude < 500) {
    telemetry.wifi_rssi = -45 + random(-10, 10);
  } else {
    telemetry.wifi_rssi = -70 + random(-15, 15);
  }
}

void sendTelemetryJSON() {
  // Update telemetry from simulation
  updateTelemetryFromSimulation();
  
  DynamicJsonDocument doc(1024);

  doc["record_number"] = telemetry.record_number;
  doc["operation_mode"] = telemetry.operation_mode;
  doc["state"] = telemetry.state;
  doc["battery_voltage"] = telemetry.battery_voltage;
  doc["wifi_rssi"] = telemetry.wifi_rssi;

  JsonObject acc_data = doc.createNestedObject("acc_data");
  acc_data["ax"] = telemetry.ax;
  acc_data["ay"] = telemetry.ay;
  acc_data["az"] = telemetry.az;
  acc_data["pitch"] = telemetry.pitch;
  acc_data["roll"] = telemetry.roll;

  JsonObject gyro_data = doc.createNestedObject("gyro_data");
  gyro_data["gx"] = telemetry.gx;
  gyro_data["gy"] = telemetry.gy;
  gyro_data["gz"] = telemetry.gz;

  JsonObject gps_data = doc.createNestedObject("gps_data");
  gps_data["latitude"] = telemetry.latitude;
  gps_data["longitude"] = telemetry.longitude;
  gps_data["gps_altitude"] = telemetry.gps_altitude;
  gps_data["time"] = telemetry.gps_time;

  JsonObject alt_data = doc.createNestedObject("alt_data");
  alt_data["pressure"] = telemetry.pressure;
  alt_data["temperature"] = telemetry.temperature;
  alt_data["AGL"] = telemetry.altitude_agl;
  alt_data["velocity"] = telemetry.velocity;
  alt_data["kalman_altitude"] = telemetry.kalman_altitude;
  alt_data["kalman_vertical_velocity"] = telemetry.kalman_vertical_velocity;

  JsonObject chute_state = doc.createNestedObject("chute_state");
  chute_state["pyro1_state"] = telemetry.drogue_pin_state;
  chute_state["pyro2_state"] = telemetry.main_chute_pin_state;

  JsonObject conn_status = doc.createNestedObject("connection_status");
  conn_status["connected"] = true;
  conn_status["has_ever_connected"] = true;
  conn_status["packet_age_ms"] = 0;
  conn_status["timeout_exceeded"] = false;
  conn_status["rssi"] = telemetry.wifi_rssi;

  doc["communication_mode"] = "Bluetooth-Simulated";
  doc["timestamp"] = millis();
  doc["packets_received"] = packetsReceived++;

  String jsonString;
  serializeJson(doc, jsonString);
  
  // Always append device ID for identification
  jsonString += "|" + String(DEVICE_ID);
  jsonString += "\n";

  // Send to both Serial and Bluetooth
  Serial.print(jsonString);
  BTSerial.print(jsonString);
}

void handleCommands() {
  // Check Serial
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    processCommand(command);
  }
  
  // Check Bluetooth
  if (BTSerial.available()) {
    String command = BTSerial.readStringUntil('\n');
    command.trim();
    processCommand(command);
  }
}

void processCommand(String command) {
  command.toUpperCase();
  
  if (command == "ARM") {
    rocketArmed = true;
    sendLogMessage("INFO", "✅ System ARMED - Ready for launch", "Command");
    BTSerial.println("ACK:ARMED");
  }
  else if (command == "DISARM") {
    rocketArmed = false;
    flightActive = false;
    initializeSimulation();
    sendLogMessage("INFO", "⚠️ System DISARMED", "Command");
    BTSerial.println("ACK:DISARMED");
  }
  else if (command == "LAUNCH" || command == "START") {
    if (rocketArmed && !flightActive) {
      flightActive = true;
      sim.phase = POWERED_ASCENT;
      sim.time = 0;
      sendLogMessage("INFO", "🚀 LAUNCH - Flight simulation started!", "Command");
      BTSerial.println("ACK:LAUNCH");
    } else if (!rocketArmed) {
      sendLogMessage("WARNING", "Cannot launch - System not armed", "Command");
      BTSerial.println("ERROR:NOT_ARMED");
    } else {
      sendLogMessage("WARNING", "Flight already in progress", "Command");
      BTSerial.println("ERROR:FLIGHT_ACTIVE");
    }
  }
  else if (command == "RESET" || command == "RESTART") {
    rocketArmed = false;
    flightActive = false;
    initializeSimulation();
    sendLogMessage("INFO", "🔄 System reset to pre-launch state", "Command");
    BTSerial.println("ACK:RESET");
  }
  else if (command == "STATUS") {
    String phaseNames[] = {"PRE_LAUNCH", "POWERED_ASCENT", "COASTING", "APOGEE", 
                          "DROGUE_DESCENT", "MAIN_DESCENT", "LANDED"};
    String status = "STATUS:";
    status += rocketArmed ? "ARMED" : "SAFE";
    status += ":PHASE:" + phaseNames[sim.phase];
    status += ":ALT:" + String(sim.altitude, 1) + "m";
    status += ":VEL:" + String(sim.velocity, 1) + "m/s";
    status += ":TIME:" + String(sim.time, 1) + "s";
    BTSerial.println(status);
    Serial.println(status);
  }
  else if (command.startsWith("SET_PWM:") || command.startsWith("CMD_SET_PWM_CONFIG:")) {
    // Parse and acknowledge PWM config (compatibility with base station commands)
    sendLogMessage("INFO", "PWM config received (simulated acceptance)", "Command");
    BTSerial.println("PWM_CONFIG_OK:Vcc=14.8,Drogue=9.0V(3000ms),Main=10.0V(5000ms)");
  }
  else if (command == "Q" || command == "2" || command == "STOP") {
    flightActive = false;
    sendLogMessage("INFO", "⏹️ Simulation paused", "Command");
    BTSerial.println("ACK:STOPPED");
  }
}

void sendHeartbeat() {
  DynamicJsonDocument statusDoc(512);
  statusDoc["type"] = "status";
  statusDoc["armed"] = rocketArmed;
  statusDoc["flight_active"] = flightActive;
  statusDoc["packets_received"] = packetsReceived;
  statusDoc["uptime"] = millis();
  statusDoc["simulation_mode"] = true;

  JsonObject flight_info = statusDoc.createNestedObject("flight");
  flight_info["phase"] = sim.phase;
  flight_info["time"] = sim.time;
  flight_info["altitude"] = sim.altitude;
  flight_info["velocity"] = sim.velocity;
  flight_info["max_altitude"] = sim.max_altitude;
  flight_info["drogue_deployed"] = sim.drogue_deployed;
  flight_info["main_deployed"] = sim.main_deployed;

  JsonObject pwm_info = statusDoc.createNestedObject("pwm_config");
  pwm_info["vcc"] = pwm_status.vcc;
  pwm_info["drogue_voltage"] = pwm_status.drogue_v;
  pwm_info["main_voltage"] = pwm_status.main_v;
  pwm_info["drogue_duration_ms"] = pwm_status.drogue_time_ms;
  pwm_info["main_duration_ms"] = pwm_status.main_time_ms;

  String statusString;
  serializeJson(statusDoc, statusString);
  
  Serial.print("STATUS:");
  Serial.println(statusString);
}

// ====== BEACON CODE (COMMENTED OUT FOR SIMULATION) ======
/*
// When ready to test with real hardware, uncomment this section

// Beacon configuration
uint8_t rocket_mac[] = {0x08, 0xD1, 0xF9, 0x15, 0x9C, 0x04};  // Rocket MAC
uint8_t my_mac[] = {0x10, 0x06, 0x1c, 0xa6, 0x11, 0xf0};  // Base MAC

void handleBeacon(const wifi_promiscuous_pkt_t* pkt) {
  const uint8_t* payload = pkt->payload;
  int len = pkt->rx_ctrl.sig_len;
  int8_t beacon_rssi = pkt->rx_ctrl.rssi;

  if (len < 60 || payload[0] != 0x80) return;
  if (memcmp(&payload[10], rocket_mac, 6) != 0) return;

  for (int i = 36; i < len - 2; i++) {
    if (payload[i] == 0xDD) {
      uint8_t data_len = payload[i + 1];
      char csv_data[512];
      memcpy(csv_data, &payload[i + 2], data_len);
      csv_data[data_len] = '\0';

      if (parseCSV(csv_data, telemetry)) {
        packetsReceived++;
        lastPacketTime = millis();
        
        // Append beacon data to simulated data flow
        // This would override simulation with real data when available
        sendTelemetryJSON();
      }
      break;
    }
  }
}

void promiscuousRx(void* buf, wifi_promiscuous_pkt_type_t type) {
  if (type == WIFI_PKT_MGMT) {
    handleBeacon((wifi_promiscuous_pkt_t*)buf);
  }
}

void setupBeaconListening() {
  WiFi.mode(WIFI_STA);
  esp_wifi_set_mac(WIFI_IF_STA, my_mac);
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);

  wifi_promiscuous_filter_t filter = {
    .filter_mask = WIFI_PROMIS_FILTER_MASK_MGMT
  };
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_promiscuous_filter(&filter);
  esp_wifi_set_promiscuous_rx_cb(promiscuousRx);
}
*/

// ====== SETUP ======
void setup() {
  Serial.begin(115200);
  BTSerial.begin(115200, SERIAL_8N1, BT_RX, BT_TX);
  
  delay(500);
  
  Serial.println("\n" + repeatChar('=', 60));
  Serial.println("N4 SIMULATED BASE STATION - Flight Simulator");
  Serial.println(repeatChar('=', 60));
  Serial.println("Device ID: " + String(DEVICE_ID));
  Serial.println("Mode: Realistic flight simulation with Bluetooth output");
  Serial.println(repeatChar('=', 60) + "\n");
  
  // Initialize simulation
  initializeSimulation();
  
  // Send startup log
  sendLogMessage("INFO", "🚀 N4 Simulated Base Station - Flight Simulator", "System");
  sendLogMessage("INFO", "📡 Telemetry rate: 10 Hz (100ms)", "System");
  sendLogMessage("INFO", "🔧 Bluetooth: HC-05/HC-06 on UART2 (115200 baud)", "System");
  sendLogMessage("INFO", "✅ Ready for commands", "System");
  sendLogMessage("INFO", "Commands: ARM, LAUNCH, DISARM, RESET, STATUS", "System");
  
  // Uncomment to enable real beacon listening
  // setupBeaconListening();
  // sendLogMessage("INFO", "📡 Beacon listening enabled (ready for real hardware)", "System");
  
  lastTelemetryTime = millis();
  lastHeartbeatTime = millis();
}

// ====== MAIN LOOP ======
void loop() {
  unsigned long currentTime = millis();
  
  // Update flight simulation
  if (flightActive && (currentTime - lastTelemetryTime >= TELEMETRY_INTERVAL)) {
    float dt = (currentTime - lastTelemetryTime) / 1000.0;  // Convert to seconds
    updateFlightSimulation(dt);
    sendTelemetryJSON();
    lastTelemetryTime = currentTime;
  }
  
  // Send telemetry even when not flying (pre-launch state)
  if (!flightActive && (currentTime - lastTelemetryTime >= TELEMETRY_INTERVAL)) {
    sendTelemetryJSON();
    lastTelemetryTime = currentTime;
  }
  
  // Send heartbeat status
  if (currentTime - lastHeartbeatTime >= HEARTBEAT_INTERVAL) {
    sendHeartbeat();
    lastHeartbeatTime = currentTime;
  }
  
  // Handle incoming commands
  handleCommands();
  
  delay(10);  // Small delay to prevent overwhelming the CPU
}
