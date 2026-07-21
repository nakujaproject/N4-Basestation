/**
 * N4 Base Station Code 7 - XBee Edition
 * 
 * PRODUCTION BASE STATION with Bluetooth + XBee Serial Output
 * - Receives real beacon telemetry from rocket
 * - Receives XBee UART telemetry (900MHz radio)
 * - Outputs via USB Serial AND Bluetooth SPP
 * - Device identifier for automatic COM port detection
 * - All production command handling (ARM, DISARM, PWM config, etc.)
 * - Full ESP-NOW bidirectional communication
 * - Three communication modes: MQTT, Beacon, XBee
 * 
 * Hardware:
 * - ESP32 DevKit
 * - HC-05/HC-06 Bluetooth module (GPIO 16/17, UART2)
 * - XBee Pro 900HP (GPIO 32/34, UART3)
 * - Baud rates: USB=115200, Bluetooth=460800, XBee=115200
 * 
 * This code supports three telemetry sources:
 * 1. MQTT beacons (WiFi-based)
 * 2. ESP-NOW beacons (802.11 management frames)
 * 3. XBee UART (900MHz radio link)
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

// ====== XBee Serial Configuration ======
HardwareSerial XBeeSerial(1);  // Use UART1 for XBee
#define XBEE_TX 32  // ESP32 TX to XBee DIN
#define XBEE_RX 34  // ESP32 RX from XBee DOUT
#define XBEE_BAUD 115200

// ====== Communication Mode Enum ======
enum CommunicationMode {
  MODE_MQTT = 0,
  MODE_BEACON = 1,
  MODE_XBEE = 2,
  MODE_AUTO = 3
};

CommunicationMode currentMode = MODE_AUTO;
bool xbeeEnabled = true;  // XBee enabled by default
uint32_t lastXBeePacketTime = 0;

// ====== Configuration ======
uint8_t rocket_mac[] = {0x08, 0xD1, 0xF9, 0x15, 0x9C, 0x04};  // Rocket MAC
uint8_t my_mac[] = {0x10, 0x06, 0x1c, 0xa6, 0x11, 0xf0};      // Base MAC

// ====== Device Identification ======
const char* DEVICE_ID = "ESP32:N4_BASE_XBEE_1";

// Connection status tracking
const uint32_t CONNECTION_TIMEOUT = 15000;  // 15 seconds
bool hasEverConnected = false;
bool currentlyConnected = false;
bool rocketArmed = false;

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
  int32_t wifi_rssi;             // 22: RSSI (0 in beacon mode)
  float kalman_altitude;         // 23: 2D Kalman filtered altitude
  float kalman_vertical_velocity; // 24: 2D Kalman filtered vertical velocity
};

TelemetryData telemetry;
int8_t beacon_rssi = 0;  // Measured beacon RSSI
int8_t xbee_rssi = -50;  // XBee RSSI (estimated from link quality)
uint32_t packetsReceived = 0;
uint32_t xbeePacketsReceived = 0;
uint32_t lastPacketTime = 0;
bool dataReceived = false;

// ====== Command Handling ======
String lastCommand = "";
bool commandPending = false;
uint32_t commandSentTime = 0;
const uint32_t COMMAND_TIMEOUT = 5000;  // 5 seconds

// ====== PWM Configuration ======
struct PWMConfigStatus {
  float vcc = 0;
  float drogue_v = 0;
  float main_v = 0;
  unsigned long drogue_time_ms = 0;
  unsigned long main_time_ms = 0;
  bool config_received = false;
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

// ====== Beacon CSV Parser (25 fields) ======
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

// ====== XBee CSV Parser (6 fields) ======
bool parseXBeeCSV(const char* csv, TelemetryData& data) {
  // Format: timestamp,state,altitude,velocity,accel_z,battery
  uint32_t timestamp;
  uint8_t state;
  float altitude, velocity, accel_z, battery;
  
  int parsed = sscanf(csv, "%lu,%hhu,%f,%f,%f,%f",
                      &timestamp, &state, &altitude, &velocity, &accel_z, &battery);
  
  if (parsed == 6) {
    // Map XBee CSV fields to telemetry structure
    data.record_number = timestamp;
    data.state = state;
    data.altitude_agl = altitude;
    data.kalman_altitude = altitude;  // Use same value
    data.velocity = velocity;
    data.kalman_vertical_velocity = velocity;  // Use same value
    data.az = accel_z;
    data.battery_voltage = battery;
    
    // Set defaults for unavailable fields
    data.operation_mode = 0;
    data.ax = 0; data.ay = 0;
    data.pitch = 0; data.roll = 0;
    data.gx = 0; data.gy = 0; data.gz = 0;
    data.latitude = 0; data.longitude = 0;
    data.gps_altitude = 0; data.gps_time = 0;
    data.pressure = 0; data.temperature = 0;
    data.drogue_pin_state = 0; data.main_chute_pin_state = 0;
    data.wifi_rssi = 0;
    
    return true;
  }
  return false;
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
  
  // Use appropriate RSSI based on mode
  if (currentMode == MODE_XBEE) {
    doc["wifi_rssi"] = xbee_rssi;
  } else {
    doc["wifi_rssi"] = beacon_rssi;
  }

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
  conn_status["rssi"] = currentMode == MODE_XBEE ? xbee_rssi : beacon_rssi;

  // Set communication mode
  if (currentMode == MODE_XBEE) {
    doc["communication_mode"] = "XBee";
  } else if (currentMode == MODE_BEACON) {
    doc["communication_mode"] = "Beacon";
  } else if (currentMode == MODE_MQTT) {
    doc["communication_mode"] = "MQTT";
  } else {
    doc["communication_mode"] = "Auto";
  }
  
  doc["timestamp"] = millis();
  doc["packets_received"] = packetsReceived;
  doc["xbee_packets_received"] = xbeePacketsReceived;

  String jsonString;
  serializeJson(doc, jsonString);
  
  // Append device identifier for COM port auto-detection
  jsonString += "|" + String(DEVICE_ID);
  jsonString += "\n";
  
  // Output to both USB Serial and Bluetooth
  Serial.print(jsonString);
  BTSerial.print(jsonString);
}

// ====== XBee Telemetry Handler ======
void handleXBeeTelemetry() {
  if (!xbeeEnabled || !XBeeSerial.available()) return;
  
  static String xbeeBuffer = "";
  
  while (XBeeSerial.available()) {
    char c = XBeeSerial.read();
    
    if (c == '\n' || c == '\r') {
      if (xbeeBuffer.length() > 0) {
        // Parse CSV: timestamp,state,altitude,velocity,accel_z,battery
        if (parseXBeeCSV(xbeeBuffer.c_str(), telemetry)) {
          xbeePacketsReceived++;
          lastXBeePacketTime = millis();
          lastPacketTime = millis();
          dataReceived = true;
          
          // Update mode to XBee if in auto mode
          if (currentMode == MODE_AUTO) {
            currentMode = MODE_XBEE;
          }
          
          // Send telemetry to dashboard
          if (currentMode == MODE_XBEE) {
            sendTelemetryJSON();
          }
        } else {
          sendLogMessage("WARNING", ("XBee parse failed: " + xbeeBuffer).c_str(), "BaseStation");
        }
        xbeeBuffer = "";
      }
    } else if (c >= 32 && c <= 126) {  // Printable ASCII only
      xbeeBuffer += c;
      if (xbeeBuffer.length() > 200) {  // Prevent buffer overflow
        xbeeBuffer = "";
      }
    }
  }
}

// ====== Beacon Handler ======
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

        // Update mode to Beacon if in auto mode
        if (currentMode == MODE_AUTO) {
          currentMode = MODE_BEACON;
        }

        updateConnectionStatus();
        
        // Send telemetry if in beacon mode
        if (currentMode == MODE_BEACON) {
          sendTelemetryJSON();
        }

        if (!wasConnected && currentlyConnected) {
          sendLogMessage("INFO", "Rocket connected!", "BaseStation");
        }

        static uint8_t lastOperationMode = 255;
        if (lastOperationMode != telemetry.operation_mode) {
          char msg[50];
          snprintf(msg, sizeof(msg), "Operation mode changed: %d", telemetry.operation_mode);
          sendLogMessage("INFO", msg, "BaseStation");
          lastOperationMode = telemetry.operation_mode;
        }
      } else {
        sendLogMessage("ERROR", "Failed to parse 25-field CSV telemetry", "BaseStation");
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
  else if (response.startsWith("PWM_CONFIG_ERROR:")) {
    String error = response.substring(17);
    sendLogMessage("ERROR", ("PWM Config Failed: " + error).c_str(), "BaseStation");
  }
  else if (response.indexOf("Mode:") != -1) {
    sendLogMessage("INFO", ("Rocket status: " + response).c_str(), "BaseStation");
  }
  else if (response.indexOf("ARMED") != -1 || response.indexOf("DISARMED") != -1) {
    sendLogMessage("INFO", ("Arming status: " + response).c_str(), "BaseStation");
  }
}

// ====== Serial Command Handler ======
void handleSerialCommands() {
  // Check USB Serial
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    processCommand(command);
  }
  
  // Check Bluetooth Serial
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
      sendLogMessage("INFO", "Format: SET_PWM:{\"vcc\":14.8,\"drogue_v\":9.0,\"main_v\":10.0,\"drogue_time\":3000,\"main_time\":5000}", "BaseStation");
      return;
    }
    
    String fullCommand = "CMD_SET_PWM_CONFIG:" + jsonPayload;
    lastCommand = fullCommand;
    commandPending = true;
    commandSentTime = millis();
    
    char msg[250];
    snprintf(msg, sizeof(msg), "PWM Config: %s", jsonPayload.c_str());
    sendLogMessage("INFO", msg, "BaseStation");
    return;
  }
  
  command.toUpperCase();

  // Standard commands
  if (command == "ARM" || command == "DISARM" || command == "RESET" ||
      command == "MAIN_ON" || command == "MAIN_OFF" ||
      command == "DROGUE_ON" || command == "DROGUE_OFF") {
    
    lastCommand = command;
    commandPending = true;
    commandSentTime = millis();
    sendLogMessage("INFO", ("Command: " + command).c_str(), "BaseStation");
  }
  // Communication mode switching
  else if (command == "CMD_MQTT_MODE" || command == "MQTT_MODE" || command == "MQTT") {
    currentMode = MODE_MQTT;
    lastCommand = "CMD_MQTT_MODE";
    commandPending = true;
    commandSentTime = millis();
    sendLogMessage("INFO", "Command: Switch to MQTT mode", "BaseStation");
  }
  else if (command == "CMD_BEACON_MODE" || command == "BEACON_MODE" || command == "BEACON") {
    currentMode = MODE_BEACON;
    lastCommand = "CMD_BEACON_MODE";
    commandPending = true;
    commandSentTime = millis();
    sendLogMessage("INFO", "Command: Switch to Beacon mode", "BaseStation");
  }
  else if (command == "CMD_XBEE_MODE" || command == "XBEE_MODE" || command == "XBEE") {
    currentMode = MODE_XBEE;
    xbeeEnabled = true;
    sendLogMessage("INFO", "Command: Switch to XBee mode", "BaseStation");
  }
  else if (command == "CMD_AUTO_FALLBACK_ON" || command == "AUTO_FALLBACK_ON" || command == "AUTO_ON") {
    currentMode = MODE_AUTO;
    sendLogMessage("INFO", "Auto fallback enabled", "BaseStation");
  }
  else if (command == "CMD_AUTO_FALLBACK_OFF" || command == "AUTO_FALLBACK_OFF" || command == "AUTO_OFF") {
    // Keep current mode but disable auto switching
    sendLogMessage("INFO", "Auto fallback disabled", "BaseStation");
  }
  else if (command == "XBEE_ON") {
    xbeeEnabled = true;
    sendLogMessage("INFO", "XBee enabled", "BaseStation");
  }
  else if (command == "XBEE_OFF") {
    xbeeEnabled = false;
    sendLogMessage("INFO", "XBee disabled", "BaseStation");
  }
  else if (command == "PWM_STATUS") {
    if (pwm_status.config_received) {
      char msg[250];
      snprintf(msg, sizeof(msg), "PWM: Vcc=%.1fV, Drogue=%.1fV(%lums), Main=%.1fV(%lums)",
               pwm_status.vcc, pwm_status.drogue_v, pwm_status.drogue_time_ms,
               pwm_status.main_v, pwm_status.main_time_ms);
      sendLogMessage("INFO", msg, "BaseStation");
    } else {
      sendLogMessage("INFO", "No PWM config received yet", "BaseStation");
    }
  }
  else if (command == "STATUS") {
    char msg[200];
    const char* modeStr = currentMode == MODE_MQTT ? "MQTT" : 
                          currentMode == MODE_BEACON ? "Beacon" : 
                          currentMode == MODE_XBEE ? "XBee" : "Auto";
    snprintf(msg, sizeof(msg), "Mode=%s, XBee=%s, Packets=%lu, XBeePackets=%lu", 
             modeStr, xbeeEnabled ? "ON" : "OFF", packetsReceived, xbeePacketsReceived);
    sendLogMessage("INFO", msg, "BaseStation");
  }
  else if (command == "HELP") {
    sendLogMessage("INFO", "Commands: ARM, DISARM, RESET, MAIN_ON, MAIN_OFF, DROGUE_ON, DROGUE_OFF", "BaseStation");
    sendLogMessage("INFO", "Modes: MQTT, BEACON, XBEE, AUTO_ON, AUTO_OFF", "BaseStation");
    sendLogMessage("INFO", "XBee: XBEE_ON, XBEE_OFF", "BaseStation");
    sendLogMessage("INFO", "PWM: SET_PWM:{json}, PWM_STATUS", "BaseStation");
    sendLogMessage("INFO", "Status: STATUS, HELP", "BaseStation");
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
  else if (lastCommand == "RESET") {
    result = esp_now_send(rocket_mac, (uint8_t*)"RESET", 5);
  }
  else if (lastCommand == "MAIN_ON") {
    result = esp_now_send(rocket_mac, (uint8_t*)"MAIN_ON", 7);
  }
  else if (lastCommand == "MAIN_OFF") {
    result = esp_now_send(rocket_mac, (uint8_t*)"MAIN_OFF", 8);
  }
  else if (lastCommand == "DROGUE_ON") {
    result = esp_now_send(rocket_mac, (uint8_t*)"DROGUE_ON", 9);
  }
  else if (lastCommand == "DROGUE_OFF") {
    result = esp_now_send(rocket_mac, (uint8_t*)"DROGUE_OFF", 10);
  }
  else if (lastCommand == "CMD_MQTT_MODE") {
    result = esp_now_send(rocket_mac, (uint8_t*)"CMD_MQTT_MODE", 13);
  }
  else if (lastCommand == "CMD_BEACON_MODE") {
    result = esp_now_send(rocket_mac, (uint8_t*)"CMD_BEACON_MODE", 15);
  }

  if (result == ESP_OK) {
    sendLogMessage("INFO", ("ESP-NOW sent: " + lastCommand).c_str(), "BaseStation");
  } else {
    sendLogMessage("ERROR", ("ESP-NOW failed: " + lastCommand).c_str(), "BaseStation");
  }

  commandPending = false;
}

// ====== SETUP ======
void setup() {
  Serial.begin(115200);
  BTSerial.begin(460800, SERIAL_8N1, BT_RX, BT_TX);  // 460800 for faster transmission
  XBeeSerial.begin(XBEE_BAUD, SERIAL_8N1, XBEE_RX, XBEE_TX);  // XBee UART
  delay(1000);

  sendLogMessage("INFO", "N4 Base Station Code 7 - XBee Edition", "BaseStation");
  sendLogMessage("INFO", "Production with Bluetooth + XBee (900MHz)", "BaseStation");
  sendLogMessage("INFO", "Device ID: ESP32:N4_BASE_XBEE_1", "BaseStation");
  sendLogMessage("INFO", "Baud rates: USB=115200, BT=460800, XBee=115200", "BaseStation");

  // Initialize WiFi
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

  // Enable beacon listening
  wifi_promiscuous_filter_t filter = {
    .filter_mask = WIFI_PROMIS_FILTER_MASK_MGMT
  };
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_promiscuous_filter(&filter);
  esp_wifi_set_promiscuous_rx_cb(promiscuousRx);

  sendLogMessage("INFO", "Beacon listening enabled (25-field CSV)", "BaseStation");
  sendLogMessage("INFO", "XBee UART listening (6-field CSV @ 115200)", "BaseStation");
  sendLogMessage("INFO", "Commands: ARM, DISARM, RESET, MAIN_ON/OFF, DROGUE_ON/OFF", "BaseStation");
  sendLogMessage("INFO", "Modes: MQTT, BEACON, XBEE, AUTO_ON/OFF", "BaseStation");
  sendLogMessage("INFO", "PWM: SET_PWM:{json}, PWM_STATUS", "BaseStation");
  sendLogMessage("INFO", "Ready! Waiting for telemetry...", "BaseStation");
}

// ====== MAIN LOOP ======
void loop() {
  static uint32_t lastHeartbeat = 0;

  updateConnectionStatus();
  handleSerialCommands();
  handleXBeeTelemetry();  // ← Handle XBee telemetry
  sendCommandToRocket();

  // Heartbeat status every 10 seconds
  if (millis() - lastHeartbeat > 10000) {
    DynamicJsonDocument statusDoc(512);
    statusDoc["type"] = "status";
    statusDoc["armed"] = rocketArmed;
    statusDoc["packets_received"] = packetsReceived;
    statusDoc["xbee_packets_received"] = xbeePacketsReceived;
    statusDoc["uptime"] = millis();
    statusDoc["bluetooth_enabled"] = true;
    statusDoc["xbee_enabled"] = xbeeEnabled;
    
    const char* modeStr = currentMode == MODE_MQTT ? "MQTT" : 
                          currentMode == MODE_BEACON ? "Beacon" : 
                          currentMode == MODE_XBEE ? "XBee" : "Auto";
    statusDoc["communication_mode"] = modeStr;

    JsonObject conn_info = statusDoc.createNestedObject("connection");
    conn_info["currently_connected"] = currentlyConnected;
    conn_info["has_ever_connected"] = hasEverConnected;
    conn_info["last_packet_age"] = dataReceived ? (millis() - lastPacketTime) : 0;
    conn_info["timeout_exceeded"] = dataReceived && (millis() - lastPacketTime > CONNECTION_TIMEOUT);

    if (dataReceived) {
      JsonObject telemetry_info = statusDoc.createNestedObject("telemetry");
      telemetry_info["record_number"] = telemetry.record_number;
      telemetry_info["communication_mode"] = modeStr;
      telemetry_info["beacon_rssi"] = beacon_rssi;
      telemetry_info["xbee_rssi"] = xbee_rssi;
      telemetry_info["battery_voltage"] = telemetry.battery_voltage;
      telemetry_info["velocity"] = telemetry.velocity;
      telemetry_info["altitude_agl"] = telemetry.altitude_agl;
      telemetry_info["kalman_altitude"] = telemetry.kalman_altitude;
      telemetry_info["kalman_vertical_velocity"] = telemetry.kalman_vertical_velocity;
      telemetry_info["operation_mode"] = telemetry.operation_mode;
      telemetry_info["state"] = telemetry.state;
    }

    if (pwm_status.config_received) {
      JsonObject pwm_info = statusDoc.createNestedObject("pwm_config");
      pwm_info["vcc"] = pwm_status.vcc;
      pwm_info["drogue_voltage"] = pwm_status.drogue_v;
      pwm_info["main_voltage"] = pwm_status.main_v;
      pwm_info["drogue_duration_ms"] = pwm_status.drogue_time_ms;
      pwm_info["main_duration_ms"] = pwm_status.main_time_ms;
      pwm_info["last_update_sec_ago"] = (millis() - pwm_status.last_update_time) / 1000;
    }

    statusDoc["waiting_for_commands"] = !commandPending;
    statusDoc["monitoring_active"] = true;

    String statusString;
    serializeJson(statusDoc, statusString);
    Serial.print("STATUS:");
    Serial.println(statusString);

    lastHeartbeat = millis();
  }

  // Timeout warning
  if (dataReceived) {
    uint32_t packetAge = millis() - lastPacketTime;
    static bool timeoutWarningGiven = false;

    if (packetAge > CONNECTION_TIMEOUT) {
      if (!timeoutWarningGiven) {
        sendLogMessage("WARNING", "Connection timeout - no packets received", "BaseStation");
        timeoutWarningGiven = true;
      }
    } else {
      timeoutWarningGiven = false;
    }
  }

  delay(10);
}
