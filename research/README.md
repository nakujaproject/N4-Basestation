# N4 Base Station Research & Development

This directory contains all research, development files, testing tools, and documentation for the N4 Base Station project. Files are organized by category for easy navigation.

## 📁 Directory Structure

```
research/
├── arduino_code/           # ESP32/Arduino code for base station hardware
├── bluetooth/              # Bluetooth communication research & tools
├── xbee/                   # XBee Pro 900HP long-range telemetry research
├── simulation/             # Flight simulation & testing tools
├── analysis/               # Data analysis & visualization tools
├── range_tests/            # Range test results & reports
├── documentation/          # Technical documentation & guides
├── scripts/                # Utility scripts & server code
└── README.md              # This file
```

---

## 🔌 arduino_code/

ESP32 Arduino sketches for base station hardware.

### Files:
- **Basestation_Code_6_Bluetooth.ino** - Production base station code
  - Receives real beacon telemetry from rocket
  - Outputs via USB Serial AND Bluetooth SPP
  - Full ESP-NOW command handling
  - Device identifier for auto COM port detection
  
- **Simulated_BaseStation_Integrated.ino** - Integrated flight simulator
  - Complete flight physics simulation (7 phases)
  - Uses simulated data with full production infrastructure
  - Perfect for development without real rocket
  - Bluetooth output at 460800 baud
  
- **Simulated_BaseStation_Code.ino** - Initial simulator version
  - Early development version
  - Kept for reference
  
- **Simulated_BaseStation_Code_Fixed.ino** - Fixed simulator version
  - Intermediate development version
  - Kept for reference
  
- **bluetooth_pairing_test.ino** - Bluetooth connection test code
  - Simple test to verify HC-05/HC-06 communication
  - Used for troubleshooting Bluetooth issues

### Usage:
1. Open `.ino` file in Arduino IDE
2. Select board: "ESP32 Dev Module"
3. Upload to ESP32
4. Use production code for real flights, simulator for development

---

## 📡 bluetooth/

Bluetooth communication research, configuration tools, and troubleshooting guides.

### Files:
- **HC05_AT_Command_Setup.ino** - AT command interface for HC-05/HC-06
  - Configure baud rate, name, password
  - Proven working code for module configuration
  - Essential for setting up new Bluetooth modules
  
- **HC05_BAUDRATE_CONFIG.md** - Complete baud rate configuration guide
  - Why default rates (9600/38400) are too slow
  - Step-by-step AT command instructions
  - HC-05 vs HC-06 differences
  - Troubleshooting common issues
  
- **README.md** - Comprehensive Bluetooth documentation
  - Hardware wiring diagrams
  - Performance comparisons
  - Quick start guides
  - AT command reference
  
- **bluetooth_setup.py** - Automated Bluetooth pairing tool
  - Python script to detect and pair HC-05/HC-06
  - Auto-detection of COM ports
  - Used by start_basestation_integrated.py
  
- **bluetooth_monitor.py** - Bluetooth connection monitor
  - Real-time monitoring of Bluetooth status
  - Logs connection events
  
- **bluetooth_pairing_test/** - Test Arduino project
  - Simple pairing verification code
  - Used for hardware debugging

### Key Insights:
- Default HC-05/HC-06 baud rates are too slow for real-time telemetry
- Recommended: 460800 baud for smooth 10 Hz updates
- Minimum: 115200 baud if module doesn't support higher rates
- Bluetooth modules must be power cycled after pairing for SPP to work

---

## 🚀 simulation/

Flight simulation tools and testing environments.

### Files:
- **csv_telemetry_simulator.py** - CSV-based telemetry playback
  - Replays recorded flight data
  - Useful for testing dashboard without hardware
  
- **flight_test_simulator.py** - Realistic flight physics simulator
  - 7-phase flight model (launch → landing)
  - Motor thrust, drag, parachute deployment
  - Generates realistic sensor noise
  
- **run_test_environment.py** - Complete test environment orchestrator
  - Starts all services (MQTT, Node, Python server)
  - Launches simulator or connects to real hardware
  
- **test_flight_simulator.py** - Unit tests for flight simulator
  - Validates physics calculations
  - Ensures realistic flight profiles
  
- **quick_test.py** - Quick telemetry validation
  - Fast sanity checks
  - Verify JSON format and data ranges
  
- **SIMULATION_TESTING_GUIDE.md** - Complete simulation testing guide
  - Hardware setup instructions
  - Bluetooth troubleshooting (critical reset procedure)
  - Step-by-step testing workflow
  - Transition to real hardware
  
- **flight_data_simul.sh** - Bash script for flight data simulation
- **log_simul.sh** - Log simulation script
- **simul_data.sh** - General data simulation script

### Usage:
```bash
# Run integrated simulator with Bluetooth
python start_basestation_integrated.py

# Run CSV playback
python research/simulation/csv_telemetry_simulator.py

# Run flight simulator standalone
python research/simulation/flight_test_simulator.py
```

---

## 📊 analysis/

Data analysis, visualization, and performance evaluation tools.

### Files:
- **analyze_telemetry.py** - Comprehensive telemetry analysis
  - Parses CSV logs
  - Generates flight statistics
  - Identifies anomalies
  
- **compare_flights.py** - Multi-flight comparison tool
  - Compare different flights side-by-side
  - Analyze performance differences
  
- **data_quality_check.py** - Data quality validator
  - Checks for missing data
  - Validates sensor ranges
  - Reports data health metrics
  
- **deployment_report.py** - Deployment report generator
  - Creates deployment summaries
  - Mission statistics
  
- **flight_curves.py** - Flight trajectory visualization
  - Plots altitude, velocity, acceleration curves
  - Exports charts for reports
  
- **kalman_vs_raw.py** - Kalman filter performance analysis
  - Compares filtered vs raw altitude data
  - Evaluates filter effectiveness
  
- **performance_metrics.py** - Flight performance calculator
  - Max altitude, velocity, acceleration
  - Apogee time, descent rates
  
- **plot_range_test2.py** - Range test visualization
  - Generates plots for range test data
  - RSSI vs distance analysis
  
- **isolate_flights.py** - Flight extraction tool
  - Separates individual flights from logs
  - Creates per-flight CSV files
  
- **view_telemetry.py** - Real-time telemetry viewer
  - Live data visualization during flights
  - Debug tool for active connections
  
- **install_analysis_deps.py** - Dependency installer
  - Installs matplotlib, pandas, numpy, etc.
  - Run once for analysis tools

### Usage:
```bash
# Install analysis dependencies
python research/analysis/install_analysis_deps.py

# Analyze a flight
python research/analysis/analyze_telemetry.py telemetry_logs/telemetry_20260113.csv

# Compare two flights
python research/analysis/compare_flights.py flight1.csv flight2.csv

# Plot flight curves
python research/analysis/flight_curves.py flight.csv
```

---

## 📡 range_tests/

Range test results, reports, and visualizations.

### Files:
- **Range_Test_2_Report.md** - Complete Range Test 2 analysis
  - Test conditions and setup
  - Results and findings
  - Range limitations
  - Recommendations
  
- **Range_Test_2_GPS.png** - GPS trajectory plot
  - Shows ground station and rocket positions
  - Distance measurements
  
- **Range_Test_2_RSSI.png** - RSSI vs Distance plot
  - Signal strength degradation
  - Connection quality analysis

### Key Findings:
- Effective range: ~500-600m with clear line of sight
- RSSI degrades predictably with distance
- Beacon mode more reliable than WiFi at long range
- Obstacles significantly impact range

---

## 📚 documentation/

Technical documentation, implementation guides, and setup instructions.

### Files:
- **BLUETOOTH_SETUP.md** - Complete Bluetooth setup guide
  - Hardware wiring
  - Pairing procedure
  - Troubleshooting
  - COM port detection
  
- **COMMUNICATION_MODES.md** - Communication mode switching guide
  - Force USB Serial mode
  - Force Bluetooth mode
  - Auto-detect mode
  - Environment variables
  - Command-line flags
  
- **SETUP.md** - Complete system setup guide
  - Software installation
  - Dependencies
  - Initial configuration
  - First run instructions
  
- **COMMAND_INTERFACE_IMPLEMENTATION.md** - Command system documentation
  - ARM/DISARM commands
  - PWM configuration
  - Mode switching (MQTT/Beacon)
  - ESP-NOW command protocol
  
- **VISUAL_FEEDBACK_IMPLEMENTATION.md** - Dashboard visual feedback guide
  - Connection indicators
  - Status colors
  - Real-time updates
  - Error handling

### Usage:
Start here for initial setup and understanding the system architecture.

---

## 🛠️ scripts/

Utility scripts and server code.

### Files:
- **server.py** - Python telemetry server
  - Receives data from ESP32 (USB/Bluetooth)
  - Forwards to MQTT broker
  - Handles command input
  - Device identifier parsing
  - Communication mode control (USB/Bluetooth/Auto)
  
- **test_data_parsing.py** - Data parsing validation
  - Tests JSON/CSV parsing
  - Validates data formats
  
- **test_esp32_parsing.py** - ESP32 data format tests
  - Validates ESP32 JSON output
  - Tests 25-field CSV parsing

### Usage:
```bash
# Run telemetry server (usually called by start_basestation_integrated.py)
python research/scripts/server.py

# Test data parsing
python research/scripts/test_data_parsing.py
```

---

## 🚀 Quick Start

### 1. Hardware Setup
```
ESP32         HC-05/HC-06
-----         -----------
GPIO 17  →    RX
GPIO 16  ←    TX
3.3V     →    VCC
GND      →    GND
```

### 2. Configure Bluetooth Module
```bash
# Upload AT command tool
# Upload: research/arduino_code/bluetooth/HC05_AT_Command_Setup.ino

# Set baud rate to 460800
# HC-05: AT+UART=460800,0,0
# HC-06: AT+BAUD9
```

### 3. Upload Base Station Code
```bash
# For development/testing:
# Upload: research/arduino_code/Simulated_BaseStation_Integrated.ino

# For real flights:
# Upload: research/arduino_code/Basestation_Code_6_Bluetooth.ino
```

### 4. Start Base Station
```bash
# From main directory
python start_basestation_integrated.py

# Or with specific mode
python start_basestation_integrated.py --force-bluetooth
python start_basestation_integrated.py --force-usb
```

### 5. Open Dashboard
```
http://localhost:5173
```

---

## 📖 Documentation Reading Order

For new developers, read in this order:

1. **research/documentation/SETUP.md** - System setup
2. **research/documentation/BLUETOOTH_SETUP.md** - Bluetooth configuration
3. **research/simulation/SIMULATION_TESTING_GUIDE.md** - Testing workflow
4. **research/documentation/COMMUNICATION_MODES.md** - Mode switching
5. **research/bluetooth/README.md** - Bluetooth deep dive
6. **research/documentation/COMMAND_INTERFACE_IMPLEMENTATION.md** - Commands

---

## 🔍 Common Tasks

### Test System Without Rocket
```bash
# Use simulator
python start_basestation_integrated.py --simulation
```

### Debug Bluetooth Connection
```bash
# Check HC-05/HC-06 configuration
# Upload: research/arduino_code/bluetooth/HC05_AT_Command_Setup.ino
# Type: AT+UART?
```

### Analyze Flight Data
```bash
python research/analysis/analyze_telemetry.py telemetry_logs/flight.csv
python research/analysis/flight_curves.py telemetry_logs/flight.csv
```

### Force USB or Bluetooth
```bash
python start_basestation_integrated.py --force-usb
python start_basestation_integrated.py --force-bluetooth
```

### Run Range Test Analysis
```bash
python research/analysis/plot_range_test2.py
```

---

## 🐛 Troubleshooting

### Issue: Slow Telemetry
**Solution:** Check Bluetooth baud rate
```bash
# See: research/bluetooth/HC05_BAUDRATE_CONFIG.md
# Set to 460800 baud for best performance
```

### Issue: Bluetooth Not Connecting
**Solution:** Power cycle after pairing
```bash
# See: research/simulation/SIMULATION_TESTING_GUIDE.md
# Critical: Turn BT module off/on after pairing
```

### Issue: No Data Received
**Solution:** Check communication mode
```bash
# See: research/documentation/COMMUNICATION_MODES.md
# Try forcing specific mode: --force-usb or --force-bluetooth
```

### Issue: JSON Parse Errors
**Solution:** Device identifier mismatch
```bash
# Server.py strips |ESP32:N4_BASE_BT_1 automatically
# Check ESP32 code has correct identifier
```

---

## 📊 File Statistics

- **Arduino Code:** 5 files (production + simulator variants)
- **Bluetooth Tools:** 7 files (AT commands, guides, setup)
- **Simulation:** 9 files (simulators, test environments)
- **Analysis:** 11 files (visualization, metrics, quality checks)
- **Documentation:** 5 files (setup, commands, modes)
- **Scripts:** 3 files (server, parsers, validators)
- **Range Tests:** 3 files (report + visualizations)

**Total Research Files:** 43+ organized files

---

## 🔄 Version History

- **2026-01-17:** Reorganized research directory structure
- **2026-01-13:** Added 460800 baud Bluetooth configuration
- **2026-01-13:** Created integrated flight simulator
- **2026-01-13:** Added communication mode control
- **2026-01-12:** Added production base station code
- **Previous:** Initial research and development

---

## 👥 Contributing

When adding new research files:

1. **Arduino code** → `arduino_code/`
2. **Bluetooth tools** → `bluetooth/`
3. **Simulators** → `simulation/`
4. **Analysis tools** → `analysis/`
5. **Documentation** → `documentation/`
6. **Utility scripts** → `scripts/`
7. **Test results** → Create new category if needed

Keep the main directory clean - only operational files belong there!

---

## 📞 Support

For questions about specific tools or research areas, refer to the README.md files in each subdirectory for detailed documentation.
