# Nakuja N4 Basestation

For monitoring telemetry and remote rocket setup.

---

## 1. Prerequisites

- **Required Tools**:
  - Git
  - Node.js
  - npm 
  - Docker
  - Mosquitto
  - Python 3.x

---

## 2. Initial Setup

### Clone Repository
```bash
# Clone your project repository
git clone https://nakujaproject/n4-basestation
cd n4-basestation

# Switch to specific branch if needed
git checkout -b <branch-name>
```

### Install Frontend Dependencies
```bash
# Install project dependencies for the React frontend
npm install
```

### Backend Setup

The backend is built using Python and Flask to handle serial communications and provide API endpoints. It works with the ESP Now configuration to receive the data through a serial port directly from the ESP then send this data via http to the frontend.

#### Create a Python Virtual Environment (Optional but Recommended)
```bash
python -m venv venv
# Activate the virtual environment:
# On macOS/Linux:
source venv/bin/activate
# On Windows:
venv\Scripts\activate
```

#### Install Python Dependencies
```bash
pip install flask flask-cors pyserial
```

#### Run the Backend Server
```bash
python server.py
```
This will start the Flask backend on [http://localhost:5000](http://localhost:5000).  
You can verify it by visiting [http://localhost:5000/debug](http://localhost:5000/debug).

---

## 3. Docker Configuration

Install [Docker](https://www.docker.com/) on your computer.

1. **Windows**: Follow [Docker Desktop for Windows installation](https://docs.docker.com/desktop/install/windows-install/).
2. **macOS**: Follow [Docker Desktop for Mac installation](https://docs.docker.com/desktop/install/mac-install/).
3. **Linux**: Follow the instructions for [Docker Engine](https://docs.docker.com/engine/install/ubuntu/) and [Docker Desktop for Linux](https://docs.docker.com/desktop/install/linux-install/).

### Docker Post-Install for Linux

1. **Create a Docker group**
    ```bash
    sudo groupadd docker
    ```

2. **Add your user to the `docker` group**
    ```bash
    sudo usermod -aG docker $USER
    ```

3. **Log out and log back in** so that your group membership is re-evaluated.

4. **Verify** that you can run `docker` commands without `sudo`.

5. For more details, see the [Docker post-installation steps](https://docs.docker.com/engine/install/linux-postinstall/).

---

## 4. Map Server Setup with TileServer-GL

TileServer-GL is used to serve the map.

1. **Pull the TileServer-GL Docker image:**
    ```bash
    docker pull maptiler/tileserver-gl
    ```

2. **Download the vector tiles (MBTiles file):**  
   Visit [Kenya's MapTiler page](https://data.maptiler.com/downloads/tileset/osm/africa/kenya/) and download the relevant MBTiles file.

3. **Run the TileServer-GL container:**  
    Replace `osm-2020-02-10-v3.11_africa_kenya.mbtiles` with your downloaded file name.
    ```bash
    docker run --rm -it -v $(pwd):/data -p 8080:8080 maptiler/tileserver-gl --file osm-2020-02-10-v3.11_africa_kenya.mbtiles
    ```
    In your browser, visit [http://localhost:8080](http://localhost:8080) (or your server IP if running remotely).

---

## 5. Environment Configuration

### Create a `.env` or `.env.local` File
Place this file in the root directory of the project.

```env
# MQTT Configuration
VITE_MQTT_HOST="localhost"
VITE_WS_PORT=1783

# Video Configuration
VITE_VIDEO_URL="192.168.X.X:XXXX"


```

---

## 6. Running the Project

In separate terminal windows (or tabs), run the following services:

1. **Run the Backend Server**
    ```bash
    python server.py
    ```
    - Flask backend will run at [http://localhost:5000](http://localhost:5000).

2. **Run the Frontend (React) Application**
    ```bash
    npm run dev
    ```
    - The Vite development server typically runs at [http://localhost:5173](http://localhost:5173).

3. **Start the TileServer-GL Docker Container (Map Server)**
    ```bash
    docker run --rm -it -v $(pwd):/data -p 8080:8080 maptiler/tileserver-gl --file osm-2020-02-10-v3.11_africa_kenya.mbtiles
    ```
    - Access the map at [http://localhost:8080](http://localhost:8080).

4. **Start Mosquitto (MQTT Broker)**
    ```bash
    mosquitto -c mosquitto.conf
    ```

---

## 7. Troubleshooting

### Common Issues
- Verify that all required services (Docker, Mosquitto, backend, and frontend) are running.
- Ensure environment configurations in the `.env` file are correct.
- Check that network ports are available and not blocked by a firewall.
- Confirm you are using compatible versions of Node.js and Python.

### Debugging Commands
```bash
# Resolve npm install dependency conflicts
npm install --force

# If the map is not rendering, try restarting Docker.
```

---

# MQTT Configuration

## Overview
The system uses MQTT for bi-directional communication between the flight computer and the ground station.

## Environment Configuration
Ensure your `.env` file includes the following variables:

```env
# MQTT Configuration
VITE_MQTT_HOST=localhost    # WebSocket URL for the MQTT broker
VITE_WS_PORT=1783           # WebSocket port for MQTT

# API Configuration
VITE_STREAM_URL=http://ip-addr:port    # Video stream server URL
```

## Port Configuration

| Service          | Specified Port | Description                                                           |
|------------------|----------------|-----------------------------------------------------------------------|
| MQTT WebSocket   | 1783           | MQTT broker WebSocket port for dashboard communication                |
| MQTT TCP         | 1882           | MQTT broker TCP port for Wi-Fi device connections                     |
| Video Stream     | XXXX           | RTSP stream server port                                               |
| Dashboard        | 5173           | Development server port (when running `npm run dev`)                  |
| Dashboard        | 80             | Production server port (when running the built version)               |
| Flask Server     | 5000           | Backend server port                                                   |

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
