# Nakuja N4 Basestation

Real-time telemetry monitoring and remote rocket control system with integrated service management, auto USB reconnection, and Bluetooth support.

---

## � Documentation

### Main Guides
- **[README.md](README.md)** - This file (quick reference & overview)
- **[SETUP.md](SETUP.md)** - Complete setup guide with architecture and troubleshooting

### Specialized Guides
- **[BLUETOOTH_SETUP.md](BLUETOOTH_SETUP.md)** - Comprehensive Bluetooth integration guide
  - HC-05 module setup and configuration
  - Automated COM port detection
  - Development journey with challenges and solutions
  - Troubleshooting common Bluetooth issues

### Research & Analysis
- **[research/COMMAND_INTERFACE_IMPLEMENTATION.md](research/COMMAND_INTERFACE_IMPLEMENTATION.md)** - Command system architecture
- **[research/VISUAL_FEEDBACK_IMPLEMENTATION.md](research/VISUAL_FEEDBACK_IMPLEMENTATION.md)** - UI/UX feedback systems
- **[research/Range_Test_2_Report.md](research/Range_Test_2_Report.md)** - Field test results and analysis
- **[Telemetry_Analysis.ipynb](Telemetry_Analysis.ipynb)** - 🆕 Jupyter notebook for post-flight telemetry analysis
- **[TELEMETRY_ANALYSIS_GUIDE.md](TELEMETRY_ANALYSIS_GUIDE.md)** - 🆕 Complete guide for using the analysis notebook

---

## �🚀 Quick Start

### Single Command Startup (Recommended)
```bash
python start_basestation.py
```

**This single command starts everything:**
- ✅ Python telemetry server with auto USB reconnection
- ✅ React dashboard (Vite dev server)
- ✅ Tileserver-GL for maps
- ✅ Mosquitto MQTT broker
- ✅ Node.js API server
- ✅ CSV logging
- ✅ Automatic port cleanup

**Access the dashboard:** http://localhost:5173

Press `Ctrl+C` to stop all services gracefully.

---

## 🔧 Features

### Unified Single-File Architecture
- **One Entry Point**: `start_basestation.py` manages everything
- **Service Orchestration**: Spawns and coordinates all required services
- **Integrated Server**: Built-in telemetry processing, no subprocess needed
- **Clean Organization**: Research/test files moved to `research/` directory

### Auto USB/Bluetooth Reconnection
- **Smart Detection**: Auto-detects ESP32 USB and HC-05 Bluetooth connections
- **Multi-Protocol**: Supports both USB serial and Bluetooth SPP (COM port)
- **Background Monitoring**: Continuous port monitoring with auto-reconnect
- **Port Auto-Detection**: Automatically finds ESP32/Bluetooth devices
- **Seamless Recovery**: Reconnects automatically when hardware is reconnected
- **No Crashes**: Graceful handling of disconnections without infinite loops

### Communication Modes
- **Beacon Mode (Default)**: Direct ESP-NOW to base station via serial
- **MQTT Mode**: ESP32 ↔ MQTT broker ↔ Flight computer
- **Dual Mode**: Both protocols active simultaneously
- **Auto-Fallback**: Automatic switching when one mode fails

### Command Interface
- **Arm/Disarm**: Main pyro channel control (`ARM`, `DISARM`)
- **Drogue Control**: `ARM_DROGUE`, `DISARM_DROGUE` / `DROGUE_ON`, `DROGUE_OFF`
- **Main Chute Control**: `ARM_MAIN`, `DISARM_MAIN` / `MAIN_ON`, `MAIN_OFF`
- **PWM Configuration**: `SET_PWM:{"vcc":14.8,"drogue_v":9.0,"main_v":10.0,"drogue_time":3000,"main_time":5000}`
- **Status**: `PWM_STATUS`, `HELP`

### Data Logging
- **CSV Logging**: Timestamped telemetry logs in `telemetry_logs/`
- **27 Data Fields**: Complete flight data capture
- **Real-time Processing**: Immediate logging with buffer management

### Simulation Mode
- **Testing**: Full telemetry simulation for development
- **Configurable Rate**: Set via `N4_SIM_RATE` environment variable
- **Command Testing**: Test arm/disarm/PWM commands without hardware

---

## 1. Prerequisites

- **Required Tools**:
  - Git
  - Python 3.x
  - Node.js & npm
  - Mosquitto MQTT broker
  - Windows PowerShell or Command Prompt

- **Python Packages**:
  ```bash
  pip install pyserial paho-mqtt
  ```

- **Hardware** (Production):
  - ESP32 base station
  - HC-05 Bluetooth module (optional, GPIO 16/17, 115200 baud)
  - USB cable for ESP32

---

## 2. Initial Setup

### Clone Repository
```bash
git clone https://github.com/nakujaproject/n4-basestation
cd n4-basestation
```

### Install Dependencies
```bash
# Frontend dependencies
npm install

# Python dependencies
pip install pyserial paho-mqtt
```

### Bluetooth Setup (Optional)

**HC-05 Configuration:**
- **Connection**: ESP32 UART2 (GPIO 16 RX, GPIO 17 TX)
- **Baud Rate**: 115200
- **Device Name**: N4_Base_BT
- **Default PIN**: 1234 or 0000

**Windows Setup:**
1. Pair HC-05 in Windows Bluetooth settings
2. Note the incoming COM port (e.g., COM12)
3. Set environment variable: `set N4_COM_PORT=COM12`
4. Python's `pyserial` handles the COM port automatically (no PyBluez needed)

### Environment Variables (Optional)
```bash
# Windows Command Prompt
set N4_COM_PORT=COM12          # Specify serial port
set N4_SIM=1                   # Enable simulation mode
set N4_SIM_RATE=20             # Simulation rate in Hz
set N4_USE_GUI=1               # Enable Tkinter GUI (optional)
```

```bash
# Windows PowerShell
$env:N4_COM_PORT="COM12"
$env:N4_SIM="1"
$env:N4_SIM_RATE="20"
```

---

## 3. Running the Base Station

### Production Mode
```bash
python start_basestation.py
```

### Simulation Mode (No Hardware)
```bash
set N4_SIM=1
python start_basestation.py
```

### Specify COM Port
```bash
set N4_COM_PORT=COM5
python start_basestation.py
```

---

## 4. Architecture Overview

### Single Entry Point

![N4 Base Station System Architecture](diagrams/output/system_architecture.png)

### Data Flow

![N4 Base Station Telemetry Data Flow](diagrams/output/data_flow.png)

### Command Flow

![N4 Base Station Command Flow](diagrams/output/command_flow.png)

---

## 5. Project Structure

| Path | Description |
|------|-------------|
| `start_basestation.py` | Main entry point — manages all services |
| `mosquitto.conf` | MQTT broker configuration |
| `package.json` | Node.js / React dependencies |
| `vite.config.js` | Vite bundler configuration |
| `server.js` | Node.js API server |
| `index.html` | React app shell |
| `src/` | React frontend source (App.jsx, components/, routes/, utils/) |
| `telemetry_logs/` | Auto-created CSV telemetry logs |
| `research/` | Archived analysis and test scripts |
| `diagrams/` | Architecture diagrams (Python scripts + PNG output) |

---

## 6. Telemetry Data Format

### CSV Format (25 fields)
```
record_number, operation_mode, state,
ax, ay, az, pitch, roll,
gx, gy, gz,
latitude, longitude, gps_altitude, gps_time,
pressure, temperature, agl_altitude, velocity,
drogue_state, main_state,
battery_voltage, wifi_rssi,
kalman_altitude, kalman_vertical_velocity
```

### JSON Format
```json
{
  "record_number": 123,
  "operation_mode": 1,
  "state": 2,
  "acc_data": {"ax": 0.1, "ay": 0.2, "az": 9.8, "pitch": 0.0, "roll": 0.0},
  "gyro_data": {"gx": 0.0, "gy": 0.0, "gz": 0.0},
  "gps_data": {"latitude": -1.286389, "longitude": 36.817223, "altitude": 1660, "time": 1234567890},
  "alt_data": {"pressure": 85000, "temperature": 25.0, "AGL": 100.5, "velocity": 50.2},
  "chute_state": {"pyro1_state": 0, "pyro2_state": 0},
  "battery_voltage": 11.8,
  "wifi_rssi": -50,
  "kalman_data": {"altitude": 100.3, "vertical_velocity": 49.8},
  "communication_mode": "BEACON"
}
```

---

## 7. Troubleshooting

### Common Issues

**Serial Port Not Found**
- Check USB connection
- Verify COM port: `set N4_COM_PORT=COMX`
- Check device manager for ESP32/Bluetooth port
- Try unplugging and reconnecting USB

**Bluetooth Not Connecting**
- Ensure HC-05 is paired in Windows Bluetooth settings
- Check COM port number in device manager (Ports → Bluetooth Serial)
- Verify baud rate is 115200
- Test with: `set N4_COM_PORT=COM12` and restart

**MQTT Connection Failed**
- Ensure Mosquitto is installed and running
- Check firewall rules for port 1883
- Verify [mosquitto.conf](mosquitto.conf) is present

**Map Tiles Not Loading**
- Ensure `osm-2020-02-10-v3.11_africa_kenya.mbtiles` file exists
- Verify tileserver-gl is running on port 8080
- Check [config.json](config.json) for proper tile configuration
- Download Kenya tiles from [MapTiler](https://data.maptiler.com/downloads/tileset/osm/africa/kenya/)

**Services Won't Start**
- Kill processes on conflicting ports:
  ```bash
  netstat -ano | findstr :8080
  taskkill /PID <PID> /F
  ```
- Restart the base station: `python start_basestation.py`

**Telemetry Not Appearing**
- Check serial connection (LED on ESP32)
- Verify base station is receiving data (check terminal logs)
- Test with simulation mode: `set N4_SIM=1`
- Check MQTT topics: `n4/app/flight-computer-1`

**Commands Not Working**
- Verify serial connection is established
- Check terminal logs for "Sent command" confirmation
- Test with simple commands: `ARM`, `DISARM`
- For ESP-NOW issues, check flight computer's serial debug output

### Debug Commands
```bash
# Check if services are running
netstat -ano | findstr :5173  # Vite
netstat -ano | findstr :8080  # Tileserver
netstat -ano | findstr :1883  # MQTT
netstat -ano | findstr :3000  # Node API

# List available serial ports
python -c "from serial.tools import list_ports; print([p.device for p in list_ports.comports()])"

# Test MQTT connection
mosquitto_sub -h localhost -t "n4/#" -v

# Force npm dependency reinstall
Remove-Item -Recurse -Force node_modules
npm install
```

---

## 8. Post-Flight Data Analysis

### 📊 Jupyter Notebook Analysis (Recommended)

**Interactive telemetry analysis notebook:** [Telemetry_Analysis.ipynb](Telemetry_Analysis.ipynb)  
**Complete usage guide:** [TELEMETRY_ANALYSIS_GUIDE.md](TELEMETRY_ANALYSIS_GUIDE.md)

This comprehensive Jupyter notebook provides:
- **File Selection**: Load CSV from `telemetry_logs/` or upload custom files
- **Automatic Phase Detection**: Launch, apogee, landing identification
- **Interactive Visualizations**: Altitude, velocity, acceleration, battery, GPS tracks
- **Flight Statistics**: Max altitude, max velocity, flight duration, apogee time
- **Data Quality Reports**: Missing values, value ranges, data completeness
- **Export Capabilities**: Processed CSV, summary reports, flight phase data

#### Quick Start
```bash
# 1. Install Jupyter and dependencies
pip install -r analysis_requirements.txt

# Or install manually:
# pip install jupyter pandas numpy matplotlib seaborn plotly ipywidgets

# 2. Launch notebook
jupyter notebook Telemetry_Analysis.ipynb

# 3. Select CSV file from dropdown or upload your own
# 4. Run cells to analyze flight data
```

#### Features
- ✅ **File Browser**: Select from existing telemetry logs or upload CSV
- ✅ **Auto-Detection**: Handles both 28-field (base station) and 6-field (XBee UART) formats
- ✅ **Phase Detection**: Automatic launch/apogee/landing identification
- ✅ **Interactive Plots**: Zoom, pan, hover for detailed inspection
- ✅ **GPS Mapping**: Plot flight trajectory on interactive map
- ✅ **Battery Monitoring**: Track voltage drops and system health
- ✅ **Export Reports**: Save processed data and summary statistics

#### Notebook Structure
1. **Data Loading**: File selection widgets (dropdown + upload)
2. **Quality Report**: Data completeness and value ranges
3. **Phase Detection**: Automatic flight phase identification
4. **Altitude Profile**: Interactive altitude/velocity plots with phase markers
5. **Acceleration**: 3-axis acceleration and G-force analysis
6. **Battery Health**: Voltage monitoring and critical level warnings
7. **GPS Trajectory**: Flight path visualization on map
8. **Summary Report**: Comprehensive flight statistics
9. **Data Export**: Save processed data and analysis results

#### CSV Format Support

**Base Station Format (28 fields)**:
```csv
timestamp,iso_timestamp,record_number,operation_mode,state,ax,ay,az,pitch,roll,
gx,gy,gz,latitude,longitude,gps_altitude,gps_time,pressure,temperature,
agl_altitude,velocity,pyro1_state,pyro2_state,battery_voltage,wifi_rssi,
kalman_altitude,kalman_vertical_velocity,communication_mode,raw_data
```

**XBee UART Format (6 fields)**:
```csv
timestamp,state,altitude,velocity,accel_z,battery
```

The notebook automatically detects the format based on column count and adjusts parsing accordingly.

---

## 9. Development & Analysis Tools

All research, analysis, and testing scripts are archived in the `research/` directory:

### Command-Line Telemetry Analysis
```bash
# Analyze flight data
python research/analyze_telemetry.py

# Compare multiple flights
python research/compare_flights.py

# Check data quality
python research/data_quality_check.py

# Plot range test data
python research/plot_range_test2.py
```

### Simulators
```bash
# Flight simulator (CSV telemetry)
python research/flight_test_simulator.py

# CSV telemetry generator
python research/csv_telemetry_simulator.py

# Test environment
python research/run_test_environment.py
```

### Testing
```bash
# Test data parsing
python research/test_data_parsing.py

# Test ESP32 parsing
python research/test_esp32_parsing.py

# Quick functionality test
python research/quick_test.py
```

---

## 9. MQTT Topics & Port Configuration

### MQTT Topics
| Topic | Direction | Description |
|-------|-----------|-------------|
| `n4/flight-computer-1` | ESP32 → Base | Raw telemetry (ESP32 publishes) |
| `n4/app/flight-computer-1` | Base → App | Processed telemetry (app receives) |
| `n4/commands` | App → Base | Command messages (ARM, DISARM, etc.) |
| `n4/logs` | Base → App | System logs |
| `n4/base-station-status` | Base → App | Connection status heartbeat |

### Port Configuration
| Service | Port | Description |
|---------|------|-------------|
| Vite (React) | 5173 | Dashboard frontend |
| Tileserver-GL | 8080 | Map tile server |
| MQTT (TCP) | 1883 | MQTT broker (device connections) |
| MQTT (WebSocket) | 1783 | MQTT WebSocket (browser connections) |
| Node.js API | 3000 | Backend API server |

---

## 10. Hardware Setup

### Base Station ESP32
- **Microcontroller**: ESP32 DevKit
- **Bluetooth Module**: HC-05 (optional)
  - RX → GPIO 17
  - TX → GPIO 16
  - VCC → 5V
  - GND → GND
- **USB**: Connect to PC via USB cable (auto-detected as COM port)

### Flight Computer ESP32
- **Communication**: ESP-NOW to base station
- **Telemetry Rate**: Configurable (default 20Hz)
- **Sensors**: BMP280, MPU6050, GPS module
- **Pyro Channels**: 2 channels (drogue, main)

---

## 11. Contributing

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit changes (`git commit -m 'Add amazing feature'`)
4. Push to branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

---

## 12. License

This project is licensed under the MIT License - see LICENSE file for details.

---

## 13. Contact & Support

- **Repository**: https://github.com/nakujaproject/n4-basestation
- **Issues**: https://github.com/nakujaproject/n4-basestation/issues
- **Team**: Nakuja Project Team

---

## 14. Acknowledgments

- Nakuja Project Team
- MapTiler for map tiles
- Mosquitto MQTT broker
- TileServer-GL
- Vite & React ecosystem

## MQTT Connection Configuration
- **Protocol:** MQTT over WebSocket
- **Default Port:** Use the `VITE_WS_PORT` environment variable
- **Host:** Use the `VITE_MQTT_HOST` environment variable
- **Client ID Format:** `dashboard-[random-hex]`
- **Keep Alive Interval:** 3600 seconds

## Topics Structure

### Subscribe Topics
The dashboard subscribes to the following topics:
1. `n4/telemetry` - Main telemetry data from the flight computer
2. `n4/logs` - System logs and status messages

### Publish Topics
The dashboard publishes to:
1. `n4/commands` - Control commands to the flight computer (e.g., arm/disarm)

## Data Formats

### Telemetry Data (`n4/telemetry`)
```json
{
  "state": number,          // Flight state (0-6)
  "operation_mode": number, // 0: Safe, 1: Armed
  "gps_data": {
    "latitude": number,
    "longitude": number,
    "gps_altitude": number
  },
  "alt_data": {
    "pressure": number,
    "temperature": number,
    "AGL": number,         // Altitude above ground level
    "velocity": number
  },
  "acc_data": {
    "ax": number,          // Acceleration X-axis
    "ay": number,          // Acceleration Y-axis
    "az": number           // Acceleration Z-axis
  },
  "chute_state": {
    "pyro1_state": number, // Drogue parachute state
    "pyro2_state": number  // Main parachute state
  },
  "battery_voltage": number
}
```

### Log Messages (`n4/logs`)
```json
{
  "level": string,     // "INFO", "ERROR", "WARN", "DEBUG"
  "message": string,   // Log message content
  "source": string     // "Flight Computer", "Base Station", or another identifier
}
```

### Commands (`n4/commands`)
```json
{
  "command": string    // "ARM" or "DISARM"
}
```

## Flight States
The system recognizes the following flight states:
- **0:** Pre-Flight
- **1:** Powered Flight
- **2:** Apogee
- **3:** Drogue Deployed
- **4:** Main Deployed
- **5:** Rocket Descent
- **6:** Post Flight

## Connection Status Monitoring
- Base station connection status is monitored continuously.
- Flight computer data staleness is checked every 500ms.
- Connection is marked as "No Recent Data" if no telemetry is received for > 5 seconds.

## Video Stream Configuration
- The dashboard expects an RTSP stream at the URL specified by `VITE_STREAM_URL`.
- Ensure the RTSP server is properly configured and accessible from the dashboard's network.

## Error Handling
1. Connection failures are logged with timestamps.
2. Parsing errors for incoming messages are captured and reported.
3. Command transmission failures are logged and reported to the user.
4. Data staleness is monitored and reflected in the UI.

## Implementation Example

```javascript
// Connect to MQTT broker
const client = new MQTT.Client(
  mqtt_host,
  ws_port,
  `dashboard-${Math.random().toString(16).slice(2, 8)}`
);

// Configure connection
client.connect({
  onSuccess: () => {
    client.subscribe(["n4/telemetry", "n4/logs"]);
  },
  keepAliveInterval: 3600
});

// Send command example
const message = new MQTT.Message(
  JSON.stringify({
    command: "ARM"
  })
);
message.destinationName = "n4/commands";
client.send(message);
```

---

## Map tiles troubleshooting
- Overrides dropdown in the UI lets you Arm/Disarm Drogue/Main, toggle Auto Fallback, and Reset. In simulation mode these commands update the simulated CSV fields for drogue/main.
- The app beeps every 5s when the rocket is not armed; click anywhere once to allow audio.


- The app proxies local tiles from a tileserver running on http://127.0.0.1:8080 via Vite at `/tiles/...`.
- The Map component auto-detects a working style from common tileserver-gl presets (basic-preview, basic, bright, klokantech-basic, positron, osm-bright, streets, voyager). If none respond, it falls back to OpenStreetMap online tiles.
- To force a specific style, add this to an `.env` file and restart the dev server:

```env
VITE_TILES_STYLE=bright
```

- If you still see 500 errors for `/tiles/...`, ensure your tileserver is running and serving the requested style:
  - CLI mode: `tileserver-gl --file osm-2020-02-10-v3.11_africa_kenya.mbtiles`
  - Docker mode: `docker run --rm -p 8080:8080 -v %CD%:/data maptiler/tileserver-gl --file osm-2020-02-10-v3.11_africa_kenya.mbtiles`
  - Visit http://127.0.0.1:8080 in your browser to see available styles and adjust `VITE_TILES_STYLE` accordingly.

