# N4 Base Station - Setup Guide

## Overview
The N4 Base Station is a comprehensive ground control system for the NAKUJA N4 rocket. It receives telemetry data via ESP-NOW beacon mode or MQTT, displays real-time flight data, and sends commands to the rocket.

## Architecture

```
Flight Computer (ESP32)
    ↓ ESP-NOW Beacon
Base Station (ESP32) ← → HC-05 Bluetooth
    ↓ Serial (USB or Bluetooth)
Ground Station PC
    ├─ start_basestation.py (Main Server)
    │   ├─ Serial Handler (USB/Bluetooth)
    │   ├─ MQTT Broker (Mosquitto)
    │   ├─ CSV Logger
    │   └─ Command Interface
    ├─ Node.js API (server.js)
    ├─ React Dashboard (Vite)
    └─ TileServer-GL (Map tiles)
```

## Quick Start

### 1. Prerequisites
- Python 3.8+
- Node.js 18+ 
- Mosquitto MQTT broker installed
- TileServer-GL (via npm or global install)

### 2. Installation

```bash
# Install Python dependencies
pip install pyserial paho-mqtt

# Install Node.js dependencies
npm install

# Ensure mosquitto is in PATH
# Ensure tileserver-gl is installed: npm install -g tileserver-gl
```

### 3. Configuration

**Environment Variables:**
- `N4_COM_PORT`: Serial port for base station (default: auto-detect, fallback: COM13)
- `N4_SIM`: Set to `1` for simulation mode (default: 0)
- `N4_USE_GUI`: Set to `1` for Tkinter GUI (default: 0)

**Connection Setup:**
- **USB:** Connect base station ESP32 via USB (auto-detected)
- **Bluetooth:** Pair HC-05 module, note COM port, set `N4_COM_PORT`

### 4. Run

**Single Command:**
```bash
python start_basestation.py
```

This starts:
- Python telemetry server with serial/MQTT handling
- Mosquitto MQTT broker
- Node.js API server (port 3000)
- React dashboard (Vite dev server, port 5173)
- TileServer-GL for maps (port 8080)

**Access Dashboard:**
Open browser to `http://localhost:5173`

## Communication Modes

### Beacon Mode (Default)
- Flight computer broadcasts telemetry via ESP-NOW beacon frames
- Base station ESP32 captures and forwards via serial
- Lower power, longer range
- **25-field CSV format** with Kalman filter data

### MQTT Mode
- Direct WiFi connection between flight computer and ground station
- Higher data rate, requires WiFi infrastructure
- JSON format telemetry

### Command Protocol
Commands sent from dashboard → MQTT → Python server → Serial → Base station ESP32 → Flight computer

**Available Commands:**
- `ARM` / `DISARM` - Arm/disarm rocket
- `DROGUE_ON` / `DROGUE_OFF` - Drogue chute control
- `MAIN_ON` / `MAIN_OFF` - Main chute control
- `RESET` - Reset flight computer
- `STATUS` - Request status
- `BEACON` / `MQTT` - Switch communication mode
- `AUTO_ON` / `AUTO_OFF` - Auto-fallback mode
- `SET_PWM:{"vcc":14.8,"drogue_v":9.0,...}` - Configure PWM settings
- `PWM_STATUS` - Query PWM configuration
- `HELP` - List available commands

## Data Format

### 25-Field CSV (Beacon Mode)
```
record_number,operation_mode,state,ax,ay,az,pitch,roll,gx,gy,gz,
latitude,longitude,gps_altitude,gps_time,pressure,temperature,
altitude_agl,velocity,drogue_state,main_state,battery_voltage,
rssi,kalman_altitude,kalman_vertical_velocity
```

### JSON (MQTT Mode)
```json
{
  "record_number": 1234,
  "operation_mode": 1,
  "state": 0,
  "acc_data": {"ax": 0, "ay": 0, "az": 1, "pitch": 0, "roll": 0},
  "gyro_data": {"gx": 0, "gy": 0, "gz": 0},
  "gps_data": {"latitude": 0, "longitude": 0, "gps_altitude": 0, "time": 0},
  "alt_data": {"pressure": 850, "temperature": 25, "AGL": 100, "velocity": 50},
  "chute_state": {"pyro1_state": 0, "pyro2_state": 0},
  "battery_voltage": 12.0,
  "wifi_rssi": -50,
  "kalman_data": {"altitude": 100, "vertical_velocity": 50},
  "communication_mode": "Beacon"
}
```

## Logging

**CSV Telemetry Logs:**
- Location: `telemetry_logs/telemetry_YYYYMMDD_HHMMSS.csv`
- Auto-generated on startup
- Contains all telemetry data with timestamps

**System Logs:**
- Console output with timestamps
- Includes connection status, command logs, errors

## Bluetooth Setup (HC-05)

### Base Station ESP32 Wiring
```
HC-05 VCC  → ESP32 5V
HC-05 GND  → ESP32 GND
HC-05 TX   → ESP32 GPIO16 (RX)
HC-05 RX   → ESP32 GPIO17 (TX)
```

### Windows Pairing
1. Power on HC-05 (should be visible as `N4_Base_BT`)
2. Open Windows Bluetooth settings
3. Pair with HC-05 (PIN: 1234 or 0000)
4. Note the assigned COM port (e.g., COM9)
5. Set `N4_COM_PORT` environment variable if not auto-detected

### Serial Configuration
- **Baud Rate:** 115200 (HC-05 must be configured for 115200)
- **Data Bits:** 8
- **Parity:** None
- **Stop Bits:** 1

## Troubleshooting

**Serial Connection Issues:**
- Check COM port in Device Manager
- Verify baud rate (115200)
- Try manual COM port: `set N4_COM_PORT=COM9`
- Check USB cable / Bluetooth pairing

**MQTT Connection Failed:**
- Ensure Mosquitto is installed and in PATH
- Check if port 1883 is available: `netstat -ano | findstr :1883`
- Kill conflicting processes

**Map Tiles Not Loading:**
- Verify `osm-2020-02-10-v3.11_africa_kenya.mbtiles` exists
- Check TileServer-GL is running on port 8080
- Browser console for tile loading errors

**Dashboard Not Updating:**
- Check MQTT connection status in sidebar
- Verify serial connection (green indicator)
- Check browser console for errors

## File Structure

```
N4-Basestation/
├── start_basestation.py      # Main server (combined functionality)
├── server.js                  # Node.js API server
├── mosquitto.conf            # MQTT broker config
├── package.json              # Node.js dependencies
├── vite.config.js            # Vite build config
├── src/                      # React dashboard source
│   ├── App.jsx               # Main app component
│   ├── components/           # UI components
│   └── utils/                # Helper functions
├── public/                   # Static assets
├── telemetry_logs/           # CSV telemetry data
├── research/                 # Development/testing scripts
└── docs/                     # Additional documentation

```

## Development

**React Dashboard:**
```bash
npm run dev:client  # Start Vite dev server only
```

**Python Server Only:**
```bash
python start_basestation.py  # Full stack
```

**Simulation Mode:**
```bash
set N4_SIM=1
python start_basestation.py  # Generates simulated telemetry
```

## Production Deployment

For field use:
1. Build React app: `npm run build`
2. Serve via Node.js or nginx
3. Run `start_basestation.py` as background service
4. Use Bluetooth for wireless telemetry
5. Ensure laptop battery backup

## Support

For issues or questions:
- Check `basestation.log` for errors
- Review `research/` directory for examples
- Consult NAKUJA Project documentation

---

**NAKUJA Project - N4 Rocket Ground Station**
*Last Updated: January 2026*
