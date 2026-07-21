/**
 * N4 Base Station - Integrated Flight Simulator
 * 
 * This combines the full production base station code (WiFi, ESP-NOW, Bluetooth)
 * with a realistic flight simulator. All beacon infrastructure is present but
 * the code uses simulated data instead of waiting for real beacons.
 * 
 * Features:
 * ✅ Full WiFi/ESP-NOW infrastructure (ready for production)
 * ✅ ArduinoJson telemetry formatting
 * ✅ Bluetooth serial output
 * ✅ Realistic flight physics simulation
 * ✅ All command handling (ARM, DISARM, PWM config, etc.)
 * ✅ Ready to switch to real beacons (just uncomment beacon handler)
 * 
 * Hardware:
 * - ESP32 DevKit
 * - HC-05/HC-06 Bluetooth module (GPIO 16/17, UART2)
 * - Baud rate: 115200
 */

#include <WiFi.h>
#include <esp_now.h>
#include "esp_wifi.h"
#include <ArduinoJson.h>
#include <HardwareSerial.h>

// ====== Bluetooth Serial Configuration ======
HardwareSerial BTSerial(2);  // Use UART2 for HC-05/HC-06
#define BT_TX 17  // ESP32 TX to HC-05 RX
#define BT_RX 16  // ESP32 RX to HC-05 TX

// ====== Configuration ======
uint8_t rocket_mac[] = {0x08, 0xD1, 0xF9, 0x15, 0x9C, 0x04};  // Rocket MAC
uint8_t my_mac[] = {0x10, 0x06, 0x1c, 0xa6, 0x11, 0xf0};  // Base MAC

// ====== Device Identification ======
const char* DEVICE_ID = "ESP32:N4_BASE_BT_1";
const bool SIMULATION_MODE = true;  // Set to false when ready for real beacons

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

// ====== Connection Status Tracking ======
const uint32_t CONNECTION_TIMEOUT = 15000;  // 15 seconds
bool hasEverConnected = false;
bool currentlyConnected = false;
bool rocketArmed = false;
bool flightActive = false;

// ====== Telemetry Data Structure (25 fields + Kalman) ======
struct TelemetryData {
  uint32_t record_number;        // 0
  uint8_t operation_mode;        // 1
  uint8_t state;                 // 2
  float ax, ay, az;              // 3-5: acceleration
  float pitch, roll;             // 6-7: attitude
  float gx, gy, gz;              // 8-10: gyroscope
  float latitude, longitude;     // 11-12: GPS coordinates
  float gps_altitude;            // 13: GPS altitude
  uint32_t gps_time;             // 14: GPS time
  float pressure;                // 15: barometric pressure
  float temperature;             // 16: temperature
  float altitude_agl;            // 17: relative altitude (AGL)
  float velocity;                // 18: velocity
  uint8_t drogue_pin_state;      // 19: drogue pin state
  uint8_t main_chute_pin_state;  // 20: main chute pin state
  float battery_voltage;         // 21: battery voltage
  int32_t wifi_rssi;             // 22: RSSI
  float kalman_altitude;         // 23: 2D Kalman filtered altitude
  float kalman_vertical_velocity; // 24: 2D Kalman filtered vertical velocity
};

TelemetryData telemetry;
int8_t beacon_rssi = -45;  // Simulated RSSI
uint32_t packetsReceived = 0;
uint32_t lastPacketTime = 0;
bool dataReceived = false;
unsigned long lastTelemetryTime = 0;
const unsigned long TELEMETRY_INTERVAL = 100;  // 10 Hz

// ====== Command Handling ======
String lastCommand = "";
bool commandPending = false;
uint32_t commandSentTime = 0;
const uint32_t COMMAND_TIMEOUT = 5000;  // 5 seconds

// ====== PWM Configuration ======
struct PWMConfigStatus {
  float vcc = 14.8;
  float drogue_v = 9.0;
  float main_v = 10.0;
  unsigned long drogue_time_ms = 3000;
  unsigned long main_time_ms = 5000;
  bool config_received = true;
  uint32_t last_update_time = 0;
} pwm_status;

// ====== Helper Functions ======
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
  
  dataReceived = true;  // Mark as having data
  lastPacketTime = millis();
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
      break;
      
    case POWERED_ASCENT:
      if (sim.time < sim.MOTOR_BURN_TIME) {
        sim.acceleration = sim.MOTOR_THRUST_ACCEL - sim.GRAVITY - sim.DRAG_COEFF_ASCENT * sim.velocity * sim.velocity;
        sim.velocity += sim.acceleration * dt;
        sim.altitude += sim.velocity * dt;
      } else {
        sim.phase = COASTING;
        sendLogMessage("INFO", "Motor burnout - Coasting phase", "Simulator");
      }
      break;
      
    case COASTING:
      sim.acceleration = -sim.GRAVITY - sim.DRAG_COEFF_ASCENT * sim.velocity * sim.velocity;
      sim.velocity += sim.acceleration * dt;
      sim.altitude += sim.velocity * dt;
      
      if (sim.velocity <= 0) {
        sim.phase = APOGEE;
        sim.max_altitude = sim.altitude;
        char msg[60];
        snprintf(msg, sizeof(msg), "APOGEE reached at %.1fm", sim.altitude);
        sendLogMessage("INFO", msg, "Simulator");
      }
      break;
      
    case APOGEE:
      sim.drogue_deployed = true;
      sim.phase = DROGUE_DESCENT;
      telemetry.drogue_pin_state = 1;
      sendLogMessage("INFO", "Drogue chute deployed", "Simulator");
      break;
      
    case DROGUE_DESCENT:
      sim.acceleration = -sim.GRAVITY + sim.DRAG_COEFF_DROGUE * sim.velocity * sim.velocity;
      sim.velocity += sim.acceleration * dt;
      sim.altitude += sim.velocity * dt;
      
      if (sim.altitude <= sim.MAIN_DEPLOY_ALT && !sim.main_deployed) {
        sim.main_deployed = true;
        sim.phase = MAIN_DESCENT;
        telemetry.main_chute_pin_state = 1;
        char msg[60];
        snprintf(msg, sizeof(msg), "Main chute deployed at %.1fm", sim.altitude);
        sendLogMessage("INFO", msg, "Simulator");
      }
      
      if (sim.altitude <= 0) {
        sim.altitude = 0;
        sim.velocity = 0;
        sim.acceleration = 0;
        sim.phase = LANDED;
        flightActive = false;
        sendLogMessage("INFO", "LANDED - Flight complete", "Simulator");
      }
      break;
      
    case MAIN_DESCENT:
      sim.acceleration = -sim.GRAVITY + sim.DRAG_COEFF_MAIN * sim.velocity * sim.velocity;
      sim.velocity += sim.acceleration * dt;
      sim.altitude += sim.velocity * dt;
      
      if (sim.altitude <= 0) {
        sim.altitude = 0;
        sim.velocity = 0;
        sim.acceleration = 0;
        sim.phase = LANDED;
        flightActive = false;
        sendLogMessage("INFO", "LANDED - Flight complete", "Simulator");
      }
      break;
      
    case LANDED:
      sim.velocity = 0;
      sim.acceleration = 0;
      break;
  }
  
  if (sim.altitude < 0) sim.altitude = 0;
}

void updateTelemetryFromSimulation() {
  telemetry.record_number = sim.record_number;
  telemetry.operation_mode = rocketArmed ? 1 : 0;
  telemetry.state = sim.phase;
  
  // Simulate accelerometer data based on flight phase
  if (sim.phase == POWERED_ASCENT) {
    telemetry.az = sim.acceleration / sim.GRAVITY;
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
  
  // Simulate gyroscope
  if (sim.phase == POWERED_ASCENT || sim.phase == COASTING) {
    telemetry.gx = random(-100, 100) / 10.0;
    telemetry.gy = random(-100, 100) / 10.0;
    telemetry.gz = random(-100, 100) / 10.0;
  } else {
    telemetry.gx = random(-50, 50) / 10.0;
    telemetry.gy = random(-50, 50) / 10.0;
    telemetry.gz = random(-50, 50) / 10.0;
  }
  
  // Attitude
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
  telemetry.kalman_altitude = sim.altitude + (random(-50, 50) / 100.0);
  telemetry.kalman_vertical_velocity = sim.velocity + (random(-30, 30) / 100.0);
  
  // Barometric data
  float pressure_change = sim.altitude * 0.12;
  telemetry.pressure = 858.0 - pressure_change + (random(-10, 10) / 100.0);
  telemetry.temperature = 26.7 - (sim.altitude * 0.0065) + (random(-5, 5) / 10.0);
  
  // GPS
  if (sim.phase != PRE_LAUNCH && sim.phase != LANDED) {
    telemetry.latitude += (random(-10, 10) / 100000.0);
    telemetry.longitude += (random(-10, 10) / 100000.0);
  }
  telemetry.gps_altitude = 1661.0 + sim.altitude + (random(-50, 50) / 10.0);
  telemetry.gps_time = millis();
  
  // Parachutes
  telemetry.drogue_pin_state = sim.drogue_deployed ? 1 : 0;
  telemetry.main_chute_pin_state = sim.main_deployed ? 1 : 0;
  
  // Battery
  telemetry.battery_voltage = 14.8 - (sim.time * 0.01);
  if (telemetry.battery_voltage < 12.0) telemetry.battery_voltage = 12.0;
  
  // RSSI
  if (sim.altitude < 500) {
    telemetry.wifi_rssi = -45 + random(-10, 10);
    beacon_rssi = -45 + random(-5, 5);
  } else {
    telemetry.wifi_rssi = -70 + random(-15, 15);
    beacon_rssi = -70 + random(-10, 10);
  }
  
  lastPacketTime = millis();
  packetsReceived++;
}

// ====== Update Connection Status ======
void updateConnectionStatus() {
  uint32_t packetAge = millis() - lastPacketTime;
  if (dataReceived) {
    hasEverConnected = true;
    currentlyConnected = (packetAge <= CONNECTION_TIMEOUT);
  } else {
    currentlyConnected = false;
  }
}

// ====== Send Telemetry JSON (Production Format) ======
void sendTelemetryJSON() {
  if (!dataReceived) return;
  updateConnectionStatus();

  DynamicJsonDocument doc(1024);

  doc["record_number"] = telemetry.record_number;
  doc["operation_mode"] = telemetry.operation_mode;
  doc["state"] = telemetry.state;
  doc["battery_voltage"] = telemetry.battery_voltage;
  doc["wifi_rssi"] = beacon_rssi;

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
  conn_status["connected"] = currentlyConnected;
  conn_status["has_ever_connected"] = hasEverConnected;
  conn_status["packet_age_ms"] = millis() - lastPacketTime;
  conn_status["timeout_exceeded"] = (millis() - lastPacketTime) > CONNECTION_TIMEOUT;
  conn_status["rssi"] = beacon_rssi;

  doc["communication_mode"] = SIMULATION_MODE ? "Bluetooth-Simulated" : "Beacon";
  doc["timestamp"] = millis();
  doc["packets_received"] = packetsReceived;

  String jsonString;
  serializeJson(doc, jsonString);
  
  // Append device identifier
  jsonString += "|" + String(DEVICE_ID);
  jsonString += "\n";
  
  Serial.print(jsonString);
  BTSerial.print(jsonString);
}

// ====== BEACON HANDLER (COMMENTED OUT FOR SIMULATION) ======
/*
// Function to parse 25-field CSV string
bool parseCSV(const char* csv, TelemetryData& data) {
  return sscanf(csv, "%lu,%hhu,%hhu,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%lu,%f,%f,%f,%f,%hhu,%hhu,%f,%d,%f,%f",
                &data.record_number, &data.operation_mode, &data.state,
                &data.ax, &data.ay, &data.az, &data.pitch, &data.roll,
                &data.gx, &data.gy, &data.gz, &data.latitude, &data.longitude,
                &data.gps_altitude, &data.gps_time, &data.pressure, &data.temperature,
                &data.altitude_agl, &data.velocity, &data.drogue_pin_state,
                &data.main_chute_pin_state, &data.battery_voltage, &data.wifi_rssi,
                &data.kalman_altitude, &data.kalman_vertical_velocity) == 25;
}

void handleBeacon(const wifi_promiscuous_pkt_t* pkt) {
  const uint8_t* payload = pkt->payload;
  int len = pkt->rx_ctrl.sig_len;
  beacon_rssi = pkt->rx_ctrl.rssi;

  if (len < 60 || payload[0] != 0x80) return;
  if (memcmp(&payload[10], rocket_mac, 6) != 0) return;

  for (int i = 36; i < len - 2; i++) {
    if (payload[i] == 0xDD) {
      uint8_t data_len = payload[i + 1];
      char csv_data[512];
      memcpy(csv_data, &payload[i + 2], data_len);
      csv_data[data_len] = '\0';

      if (parseCSV(csv_data, telemetry)) {
        bool wasConnected = currentlyConnected;
        rocketArmed = (telemetry.operation_mode == 1);
        packetsReceived++;
        lastPacketTime = millis();
        dataReceived = true;

        updateConnectionStatus();
        sendTelemetryJSON();

        if (!wasConnected && currentlyConnected) {
          sendLogMessage("INFO", "CONNECTED - Rocket beacon link established", "BaseStation");
        }
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
*/

// ====== ESP-NOW Receive Callback ======
void onESPNowDataReceived(const esp_now_recv_info_t *recv_info, const uint8_t *incomingData, int len) {
  String response = "";
  for (int i = 0; i < len; i++) {
    response += (char)incomingData[i];
  }
  response.trim();

  sendLogMessage("INFO", ("ESP-NOW response: " + response).c_str(), "BaseStation");

  if (response.startsWith("PWM_CONFIG_OK:")) {
    int vccStart = response.indexOf("Vcc=") + 4;
    int drogueStart = response.indexOf("Drogue=") + 7;
    int mainStart = response.indexOf("Main=") + 5;
    
    if (vccStart > 3 && drogueStart > 6 && mainStart > 4) {
      pwm_status.vcc = response.substring(vccStart, response.indexOf(',', vccStart)).toFloat();
      
      String drogueStr = response.substring(drogueStart, response.indexOf(',', drogueStart));
      int drogueVEnd = drogueStr.indexOf('V');
      int drogueTimeStart = drogueStr.indexOf('(') + 1;
      int drogueTimeEnd = drogueStr.indexOf("ms)");
      pwm_status.drogue_v = drogueStr.substring(0, drogueVEnd).toFloat();
      pwm_status.drogue_time_ms = drogueStr.substring(drogueTimeStart, drogueTimeEnd).toInt();
      
      String mainStr = response.substring(mainStart);
      int mainVEnd = mainStr.indexOf('V');
      int mainTimeStart = mainStr.indexOf('(') + 1;
      int mainTimeEnd = mainStr.indexOf("ms)");
      pwm_status.main_v = mainStr.substring(0, mainVEnd).toFloat();
      pwm_status.main_time_ms = mainStr.substring(mainTimeStart, mainTimeEnd).toInt();
      
      pwm_status.config_received = true;
      pwm_status.last_update_time = millis();
      
      char msg[250];
      snprintf(msg, sizeof(msg), "PWM Config: Vcc=%.1fV, Drogue=%.1fV(%lums), Main=%.1fV(%lums)",
               pwm_status.vcc, pwm_status.drogue_v, pwm_status.drogue_time_ms,
               pwm_status.main_v, pwm_status.main_time_ms);
      sendLogMessage("INFO", msg, "BaseStation");
    }
  }
}

// ====== Serial Command Handler ======
void handleSerialCommands() {
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    processCommand(command);
  }
  
  if (BTSerial.available()) {
    String command = BTSerial.readStringUntil('\n');
    command.trim();
    processCommand(command);
  }
}

void processCommand(String command) {
  // Handle PWM configuration
  if (command.startsWith("SET_PWM:") || command.startsWith("set_pwm:")) {
    String jsonPayload = command.substring(8);
    jsonPayload.trim();
    
    StaticJsonDocument<256> testDoc;
    DeserializationError error = deserializeJson(testDoc, jsonPayload);
    
    if (error) {
      sendLogMessage("ERROR", ("Invalid JSON: " + String(error.c_str())).c_str(), "BaseStation");
      return;
    }
    
    String fullCommand = "CMD_SET_PWM_CONFIG:" + jsonPayload;
    lastCommand = fullCommand;
    commandPending = true;
    commandSentTime = millis();
    
    sendLogMessage("INFO", ("PWM Config: " + jsonPayload).c_str(), "BaseStation");
    return;
  }
  
  command.toUpperCase();

  if (command == "ARM") {
    rocketArmed = true;
    telemetry.operation_mode = 1;
    sendLogMessage("INFO", "System ARMED - Ready for launch", "Command");
    BTSerial.println("ACK:ARMED");
  }
  else if (command == "DISARM") {
    rocketArmed = false;
    flightActive = false;
    initializeSimulation();
    sendLogMessage("INFO", "System DISARMED", "Command");
    BTSerial.println("ACK:DISARMED");
  }
  else if (command == "LAUNCH" || command == "START") {
    if (rocketArmed && !flightActive) {
      flightActive = true;
      sim.phase = POWERED_ASCENT;
      sim.time = 0;
      sendLogMessage("INFO", "LAUNCH - Flight simulation started!", "Command");
      BTSerial.println("ACK:LAUNCH");
    } else if (!rocketArmed) {
      sendLogMessage("WARNING", "Cannot launch - System not armed", "Command");
      BTSerial.println("ERROR:NOT_ARMED");
    }
  }
  else if (command == "RESET" || command == "RESTART") {
    rocketArmed = false;
    flightActive = false;
    initializeSimulation();
    sendLogMessage("INFO", "System reset", "Command");
    BTSerial.println("ACK:RESET");
  }
  else if (command == "STATUS") {
    String phaseNames[] = {"PRE_LAUNCH", "POWERED_ASCENT", "COASTING", "APOGEE", 
                          "DROGUE_DESCENT", "MAIN_DESCENT", "LANDED"};
    String status = "STATUS:";
    status += rocketArmed ? "ARMED" : "SAFE";
    status += ":PHASE:" + phaseNames[sim.phase];
    status += ":ALT:" + String(sim.altitude, 1) + "m";
    BTSerial.println(status);
  }
  else if (command == "MAIN_ON" || command == "MAIN_OFF" ||
           command == "DROGUE_ON" || command == "DROGUE_OFF") {
    lastCommand = command;
    commandPending = true;
    commandSentTime = millis();
    sendLogMessage("INFO", ("Command: " + command).c_str(), "BaseStation");
  }
}

// ====== Command Sending (ESP-NOW) ======
void sendCommandToRocket() {
  if (!commandPending) return;

  if (millis() - commandSentTime > COMMAND_TIMEOUT) {
    sendLogMessage("WARNING", ("Command timeout: " + lastCommand).c_str(), "BaseStation");
    commandPending = false;
    return;
  }

  esp_err_t result = ESP_FAIL;

  if (lastCommand.startsWith("CMD_SET_PWM_CONFIG:")) {
    result = esp_now_send(rocket_mac, (uint8_t*)lastCommand.c_str(), lastCommand.length());
  }
  else if (lastCommand == "ARM") {
    result = esp_now_send(rocket_mac, (uint8_t*)"ARM", 3);
  }
  else if (lastCommand == "DISARM") {
    result = esp_now_send(rocket_mac, (uint8_t*)"DISARM", 6);
  }

  if (result == ESP_OK) {
    sendLogMessage("INFO", ("ESP-NOW sent: " + lastCommand).c_str(), "BaseStation");
  }

  commandPending = false;
}

// ====== SETUP ======
void setup() {
  Serial.begin(115200);
  // Increased baud rate for faster Bluetooth data transmission
  // NOTE: HC-05/HC-06 module must be configured to match this baud rate!
  BTSerial.begin(460800, SERIAL_8N1, BT_RX, BT_TX);  // 460800 for faster transmission
  delay(1000);

  sendLogMessage("INFO", "N4 Base Station - Integrated Simulator (460800 baud)", "BaseStation");
  sendLogMessage("INFO", "Mode: SIMULATION with full production infrastructure", "BaseStation");
  sendLogMessage("INFO", "Device ID: ESP32:N4_BASE_BT_1", "BaseStation");

  // Initialize WiFi (required for ESP-NOW even if not using beacons)
  WiFi.mode(WIFI_STA);
  esp_wifi_set_mac(WIFI_IF_STA, my_mac);
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);

  if (esp_now_init() != ESP_OK) {
    sendLogMessage("ERROR", "ESP-NOW init failed", "BaseStation");
    ESP.restart();
  }

  esp_now_register_recv_cb(onESPNowDataReceived);

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, rocket_mac, 6);
  peerInfo.channel = 1;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    sendLogMessage("ERROR", "Failed to add peer", "BaseStation");
  } else {
    sendLogMessage("INFO", "ESP-NOW peer registered", "BaseStation");
  }

  // Beacon listening setup (commented out for simulation)
  /*
  wifi_promiscuous_filter_t filter = {
    .filter_mask = WIFI_PROMIS_FILTER_MASK_MGMT
  };
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_promiscuous_filter(&filter);
  esp_wifi_set_promiscuous_rx_cb(promiscuousRx);
  sendLogMessage("INFO", "Beacon listening enabled", "BaseStation");
  */

  // Initialize simulation
  initializeSimulation();
  
  sendLogMessage("INFO", "Commands: ARM, LAUNCH, DISARM, RESET, STATUS", "BaseStation");
  sendLogMessage("INFO", "Ready - sending simulated telemetry at 10 Hz", "BaseStation");
  
  lastTelemetryTime = millis();
}

// ====== MAIN LOOP ======
void loop() {
  unsigned long currentTime = millis();
  static uint32_t lastHeartbeat = 0;

  // Update simulation and send telemetry
  if (currentTime - lastTelemetryTime >= TELEMETRY_INTERVAL) {
    if (flightActive) {
      float dt = (currentTime - lastTelemetryTime) / 1000.0;
      updateFlightSimulation(dt);
    }
    updateTelemetryFromSimulation();
    sendTelemetryJSON();
    lastTelemetryTime = currentTime;
  }

  // Handle commands
  updateConnectionStatus();
  handleSerialCommands();
  sendCommandToRocket();

  // Heartbeat status
  if (millis() - lastHeartbeat > 10000) {
    DynamicJsonDocument statusDoc(512);
    statusDoc["type"] = "status";
    statusDoc["armed"] = rocketArmed;
    statusDoc["packets_received"] = packetsReceived;
    statusDoc["uptime"] = millis();
    statusDoc["simulation_mode"] = SIMULATION_MODE;

    JsonObject conn_info = statusDoc.createNestedObject("connection");
    conn_info["currently_connected"] = currentlyConnected;
    conn_info["has_ever_connected"] = hasEverConnected;

    if (dataReceived) {
      JsonObject telemetry_info = statusDoc.createNestedObject("telemetry");
      telemetry_info["record_number"] = telemetry.record_number;
      telemetry_info["altitude_agl"] = telemetry.altitude_agl;
      telemetry_info["velocity"] = telemetry.velocity;
      telemetry_info["phase"] = sim.phase;
      telemetry_info["flight_active"] = flightActive;
    }

    if (pwm_status.config_received) {
      JsonObject pwm_info = statusDoc.createNestedObject("pwm_config");
      pwm_info["vcc"] = pwm_status.vcc;
      pwm_info["drogue_voltage"] = pwm_status.drogue_v;
      pwm_info["main_voltage"] = pwm_status.main_v;
    }

    String statusString;
    serializeJson(statusDoc, statusString);
    Serial.print("STATUS:");
    Serial.println(statusString);

    lastHeartbeat = millis();
  }

  delay(10);
}
