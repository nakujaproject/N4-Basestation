# Nakuja N4 Basestation

For monitoring telemetry and remote rocket setup.

## 1. Prerequisites

- **Required Tools**:
  - Git
  - Node.js
  - npm 
  - Docker
  - Mosquitto

## 2. Initial Setup

### Clone Repository
```bash
# Clone your project repository
git clone http://nakujaproject/n4-basestation
cd n4-basestation

# Switch to specific branch if needed
git checkout -b <branch-name>
```

### Install Dependencies
```bash
# Install project dependencies
npm install
```

## 3. Docker Configuration

Install [Docker](https://www.docker.com/) on your computer.

1. If you are using Windows, please follow [steps for installing Docker Desktop on Windows.](https://docs.docker.com/desktop/install/windows-install/)

2. If you are using macOS, please be sure to follow the steps outlined in [Docker Docs for how to install Docker Desktop for Mac](https://docs.docker.com/desktop/install/mac-install/)

3. If you are using Linux, please be sure to follow the steps outlined in Docker Docs for how to install [Docker Engine](https://docs.docker.com/engine/install/ubuntu/) and [Docker Desktop for linux](https://docs.docker.com/desktop/install/linux-install/)

## Docker post-install for linux

1. Create a docker group

    ```bash
        #!/bin/bash
    sudo groupadd docker
    ```

2. Add your user to the `docker` group.

    ```bash
        #!/bin/bash
    sudo usermod -aG docker $USER
    ```

3. Log out and log back in so that your group membership is re-evaluated.

4. Verify that you can run `docker` commands without `sudo`.

5. For more information follow this [link](https://docs.docker.com/engine/install/linux-postinstall/)

Then install TileServer-GL in order to serve the map.
```bash
docker pull maptiler/tileserver-gl
```
Now download the vector tiles in form of MBTiles file from the OpenMapTiles Downloads and save it in your current directory.
Go to [Kenya's](https://data.maptiler.com/downloads/tileset/osm/africa/kenya/) map to get the relevant mbtiles file.

Example using a mbtiles file
```bash
docker run --rm -it -v $(pwd):/data -p 8080:8080 maptiler/tileserver-gl --file osm-2020-02-10-v3.11_africa_kenya.mbtiles
[in your browser, visit http://[server ip]:8080]
```

## 4. Environment Configuration

### Create `.env` or `.env.local` File
```env

# MQTT Configuration
VITE_MQTT_HOST="localhost"
VITE_WS_PORT=1783

# Video Configuration
VITE_VIDEO_URL = "192.168.X.X:XXXX"
```

## 5. Running the Project

```bash
# Run vite development server
npm run dev

# Start Docker if not started
docker run --rm -it -v $(pwd):/data -p 8080:8080 maptiler/tileserver-gl --file osm-2020-02-10-v3.11_africa_kenya.mbtiles

# Start Mosquitto service
mosquitto -c mosquitto.conf

# Access Services
# React App: http://localhost:5173
# MapTiler: http://localhost:8080
```

## 6. Troubleshooting

### Common Issues
- Verify Docker services are running
- Check environment configurations
- Ensure network ports are available
- Confirm Node.js versions

### Debugging Commands
```bash
# Resolve npm install dependency conflicts
npm install --force

# If map is not rendering restart docker 
```

# MQTT Configuration

## Overview
MQTT topic structure, data formats, and configuration details used. The system uses MQTT for bi-directional communication between the flight computer and ground station.

## Environment Configuration
Create a `.env` file in the root directory with the following variables:

```env
# MQTT Configuration
VITE_MQTT_HOST=localhost    # WebSocket URL for MQTT broker
VITE_WS_PORT=1783                          # WebSocket port for MQTT

# API Configuration
VITE_STREAM_URL=http://ip-addr:port    # Video stream server URL
```

## Port Configuration

| Service | Specified Port | Description |
|---------|-------------|-------------|
| MQTT WebSocket | 1783 | MQTT broker WebSocket port for dashboard communication |
| MQTT TCP | 1882 | MQTT broker TCP port for wifi device connections |
| Video Stream | XXXX | RTSP stream server port |
| Dashboard | 5173 | Development server port (when running `npm run dev`) |
| Dashboard | 80 | Production server port (when running built version) |

## MQTT Connection Configuration
- Protocol: MQTT over WebSocket
- Default Port: Define in environment variable `VITE_WS_PORT`
- Host: Define in environment variable `VITE_MQTT_HOST`
- Client ID Format: `dashboard-[random-hex]`
- Keep Alive Interval: 3600 seconds

## Topics Structure

### Subscribe Topics
The dashboard subscribes to the following topics:

1. `n4/telemetry` - Main telemetry data from the flight computer
2. `n4/logs` - System logs and status messages. 

### Publish Topics
The dashboard publishes to:

1. `n4/commands` - Control commands to the flight computer. To arm or disarm.

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
  "level": string,     // "INFO", "ERROR", "WARN", "DEBUG".
  "message": string,   // Log message content
  "source": string     // "Flight Computer" or "Base Station" or other source identifier
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
- 0: Pre-Flight
- 1: Powered Flight
- 2: Apogee
- 3: Drogue Deployed
- 4: Main Deployed
- 5: Rocket Descent
- 6: Post Flight

## Connection Status Monitoring
- Base station connection status is monitored continuously
- Flight computer data staleness is checked every 500ms
- Connection is marked as "No Recent Data" if no telemetry is received for > 5 seconds

## Video Stream Configuration
- The dashboard expects an RTSP stream to be available at the URL specified in `VITE_STREAM_URL`
- The video component will automatically attempt to connect to this stream
- Ensure the RTSP server is properly configured and accessible from the dashboard's network


## Error Handling
1. Connection failures are logged with timestamps
2. Parsing errors for incoming messages are captured and reported
3. Command transmission failures are logged and reported to the user
4. Data staleness is monitored and reported in the UI

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


