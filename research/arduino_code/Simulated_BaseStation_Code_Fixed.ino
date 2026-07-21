/**
 * N4 Simulated Base Station - Flight Simulator (SIMPLIFIED - NO DEPENDENCIES)
 * 
 * This is a simplified version that works without ArduinoJson or WiFi libraries.
 * Upload this version if you're having boot loop issues.
 * 
 * Features:
 * - Realistic flight physics simulation
 * - Bluetooth output via HC-05/HC-06
 * - JSON telemetry format matching base station expectations
 * - Device identifier appended for COM port detection
 * - Interactive commands: ARM, LAUNCH, DISARM, RESET, STATUS
 * 
 * Hardware Setup:
 * - ESP32 DevKit
 * - HC-05/HC-06 Bluetooth module connected to UART2 (GPIO 16/17)
 * - Baud rate: 115200
 */

#include <HardwareSerial.h>

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

// ====== Telemetry Data Structure ======
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

// ====== Helper Functions ======
void sendLogMessage(const char* level, const char* message, const char* source) {
  String logString = "LOG:{\"level\":\"";
  logString += level;
  logString += "\",\"message\":\"";
  logString += message;
  logString += "\",\"source\":\"";
  logString += source;
  logString += "\",\"timestamp\":";
  logString += millis();
  logString += "}";
  
  Serial.println(logString);
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
  
  sendLogMessage("INFO", "Flight simulation initialized", "Simulator");
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
        sendLogMessage("INFO", "Motor burnout - Coasting phase", "Simulator");
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
        char msg[60];
        snprintf(msg, sizeof(msg), "APOGEE reached at %.1fm", sim.altitude);
        sendLogMessage("INFO", msg, "Simulator");
      }
      break;
      
    case APOGEE:
      // Deploy drogue immediately at apogee
      sim.drogue_deployed = true;
      sim.phase = DROGUE_DESCENT;
      telemetry.drogue_pin_state = 1;
      sendLogMessage("INFO", "Drogue chute deployed", "Simulator");
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
        char msg[60];
        snprintf(msg, sizeof(msg), "Main chute deployed at %.1fm", sim.altitude);
        sendLogMessage("INFO", msg, "Simulator");
      }
      
      // Check for landing
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
        sendLogMessage("INFO", "LANDED - Flight complete", "Simulator");
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
  if (telemetry.battery_voltage < 12.0) telemetry.battery_voltage = 12.0;
  
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
  
  // Build JSON string manually (no ArduinoJson dependency)
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
  
  json += "\"communication_mode\":\"Bluetooth-Simulated\",";
  json += "\"timestamp\":" + String(millis()) + ",";
  json += "\"packets_received\":" + String(packetsReceived++);
  json += "}";
  
  // Always append device ID for identification
  json += "|" + String(DEVICE_ID);
  json += "\n";
  
  // Send to both Serial and Bluetooth
  Serial.print(json);
  BTSerial.print(json);
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
    } else {
      sendLogMessage("WARNING", "Flight already in progress", "Command");
      BTSerial.println("ERROR:FLIGHT_ACTIVE");
    }
  }
  else if (command == "RESET" || command == "RESTART") {
    rocketArmed = false;
    flightActive = false;
    initializeSimulation();
    sendLogMessage("INFO", "System reset to pre-launch state", "Command");
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
  else if (command == "Q" || command == "2" || command == "STOP") {
    flightActive = false;
    sendLogMessage("INFO", "Simulation paused", "Command");
    BTSerial.println("ACK:STOPPED");
  }
}

void sendHeartbeat() {
  String status = "STATUS:{";
  status += "\"type\":\"status\",";
  status += "\"armed\":" + String(rocketArmed ? "true" : "false") + ",";
  status += "\"flight_active\":" + String(flightActive ? "true" : "false") + ",";
  status += "\"packets_received\":" + String(packetsReceived) + ",";
  status += "\"uptime\":" + String(millis()) + ",";
  status += "\"simulation_mode\":true,";
  status += "\"flight\":{";
  status += "\"phase\":" + String(sim.phase) + ",";
  status += "\"time\":" + String(sim.time, 1) + ",";
  status += "\"altitude\":" + String(sim.altitude, 1) + ",";
  status += "\"velocity\":" + String(sim.velocity, 1) + ",";
  status += "\"max_altitude\":" + String(sim.max_altitude, 1) + ",";
  status += "\"drogue_deployed\":" + String(sim.drogue_deployed ? "true" : "false") + ",";
  status += "\"main_deployed\":" + String(sim.main_deployed ? "true" : "false");
  status += "}}";
  
  Serial.println(status);
}

// ====== SETUP ======
void setup() {
  Serial.begin(115200);
  BTSerial.begin(115200, SERIAL_8N1, BT_RX, BT_TX);
  
  delay(1000);
  
  Serial.println("\n============================================================");
  Serial.println("N4 SIMULATED BASE STATION - Flight Simulator");
  Serial.println("============================================================");
  Serial.println("Device ID: " + String(DEVICE_ID));
  Serial.println("Mode: Realistic flight simulation with Bluetooth output");
  Serial.println("============================================================\n");
  
  // Initialize simulation
  initializeSimulation();
  
  // Send startup log
  sendLogMessage("INFO", "N4 Simulated Base Station - Flight Simulator", "System");
  sendLogMessage("INFO", "Telemetry rate: 10 Hz (100ms)", "System");
  sendLogMessage("INFO", "Bluetooth: HC-05/HC-06 on UART2 (115200 baud)", "System");
  sendLogMessage("INFO", "Ready for commands", "System");
  sendLogMessage("INFO", "Commands: ARM, LAUNCH, DISARM, RESET, STATUS", "System");
  
  lastTelemetryTime = millis();
  lastHeartbeatTime = millis();
  
  Serial.println("✅ Setup complete - sending telemetry\n");
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
