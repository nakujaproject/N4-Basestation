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

