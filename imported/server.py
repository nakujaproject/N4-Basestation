import serial
import json
import logging
import time
import threading
import paho.mqtt.client as mqtt
from serial.tools import list_ports
from threading import Lock
from datetime import datetime
import subprocess
import os
import signal
import atexit
import csv
from pathlib import Path

# === OPTIONAL GUI (tkinter) ===
try:
    import tkinter as tk
    from tkinter import ttk
    TK_AVAILABLE = True
except Exception:
    TK_AVAILABLE = False

# === CONFIG ===
SERIAL_BAUD = 115200
MQTT_BROKER = "localhost"  # Update with your broker IP
MQTT_PORT = 1883
MQTT_TELEMETRY_TOPIC = "n4/flight-computer-1"
MQTT_COMMAND_TOPIC = "n4/commands"
MQTT_LOG_TOPIC = "n4/logs"
MQTT_STATUS_TOPIC = "n4/base-station-status"
APP_TELEMETRY_TOPIC = "n4/app/flight-computer-1"  # Unified app channel
APP_LOG_TOPIC = "n4/app/logs"
CONNECTION_TIMEOUT = 15000  # 15 seconds in milliseconds
HEARTBEAT_INTERVAL = 5      # Send status every 5 seconds
COMMAND_TIMEOUT = 3         # Wait 3 seconds for command response
AUTO_DETECT_INTERVAL = 5    # Check for active connection every 5 seconds
USB_RECONNECT_INTERVAL = 2  # Check for USB reconnection every 2 seconds
USB_MONITOR_ENABLED = True  # Enable/disable USB monitoring
PORT_8080 = 8080           # Port for development server

# === CSV LOGGING CONFIG ===
CSV_LOGGING_ENABLED = True
CSV_LOG_DIR = "telemetry_logs"
CSV_FIELDNAMES = [
    'timestamp', 'iso_timestamp', 'record_number', 'operation_mode', 'state',
    'ax', 'ay', 'az', 'pitch', 'roll',
    'gx', 'gy', 'gz',
    'latitude', 'longitude', 'gps_altitude', 'gps_time',
    'pressure', 'temperature', 'agl_altitude', 'velocity',
    'pyro1_state', 'pyro2_state',
    'battery_voltage', 'wifi_rssi',
    'kalman_altitude', 'kalman_vertical_velocity',
    'communication_mode', 'raw_data'
]

# === APP STARTUP MANAGEMENT ===
app_processes = []  # Store spawned processes

def kill_processes_on_port(port):
    """Kill any processes running on the specified port using Windows netstat and taskkill"""
    try:
        # Use netstat to find processes using the port
        netstat_cmd = f"netstat -ano | findstr :{port}"
        result = subprocess.run(netstat_cmd, shell=True, capture_output=True, text=True)
        
        if result.returncode == 0 and result.stdout:
            lines = result.stdout.strip().split('\n')
            pids = set()
            
            for line in lines:
                parts = line.split()
                if len(parts) >= 5 and 'LISTENING' in line:
                    pid = parts[-1]
                    if pid.isdigit():
                        pids.add(pid)
            
            for pid in pids:
                try:
                    logger.info(f"🔪 Killing process {pid} on port {port}")
                    subprocess.run(f"taskkill /PID {pid} /F", shell=True, capture_output=True)
                except Exception as e:
                    logger.warning(f"⚠️ Could not kill process {pid}: {e}")
                    
    except Exception as e:
        logger.warning(f"⚠️ Error killing processes on port {port}: {e}")

def start_app_services():
    """Start all app services"""
    global app_processes
    
    # Kill any existing processes on port 8080
    kill_processes_on_port(PORT_8080)
    time.sleep(1)
    
    logger.info("🚀 Starting application services...")
    
    try:
        # Start npm dev server
        logger.info("📦 Starting npm dev server...")
        npm_process = subprocess.Popen(
            ["cmd", "/c", "npm", "run", "dev"],
            shell=True,
            cwd=os.getcwd(),
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            creationflags=subprocess.CREATE_NEW_PROCESS_GROUP
        )
        app_processes.append(("npm", npm_process))
        logger.info("✅ npm dev server started")
        
        # Start tileserver-gl
        logger.info("🗺️ Starting tileserver...")
        tileserver_process = subprocess.Popen(
            ["cmd", "/c", "tileserver-gl", "--file", "osm-2020-02-10-v3.11_africa_kenya.mbtiles"],
            shell=True,
            cwd=os.getcwd(),
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            creationflags=subprocess.CREATE_NEW_PROCESS_GROUP
        )
        app_processes.append(("tileserver", tileserver_process))
        logger.info("✅ Tileserver started")
        
        # Start mosquitto
        logger.info("🦟 Starting mosquitto...")
        mosquitto_process = subprocess.Popen(
            ["cmd", "/c", "mosquitto", "-c", "mosquitto.conf", "-v"],
            shell=True,
            cwd=os.getcwd(),
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            creationflags=subprocess.CREATE_NEW_PROCESS_GROUP
        )
        app_processes.append(("mosquitto", mosquitto_process))
        logger.info("✅ Mosquitto started")
        
        # Give services time to start
        time.sleep(3)
        
        logger.info("🎉 All application services started successfully!")
        
        # Wait a bit more for Mosquitto to fully initialize
        logger.info("⏳ Waiting for Mosquitto to initialize...")
        time.sleep(2)
        
    except Exception as e:
        logger.error(f"❌ Failed to start app services: {e}")

def cleanup_processes():
    """Clean up spawned processes"""
    global app_processes
    logger.info("🧹 Cleaning up processes...")
    
    for name, process in app_processes:
        try:
            if process.poll() is None:  # Process is still running
                logger.info(f"🔪 Terminating {name} process...")
                process.terminate()
                try:
                    process.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    logger.warning(f"⚠️ Force killing {name} process...")
                    process.kill()
        except Exception as e:
            logger.error(f"❌ Error cleaning up {name}: {e}")
    
    app_processes.clear()

# Register cleanup function
atexit.register(cleanup_processes)

# === CSV LOGGING ===
csv_writer = None
csv_file_handle = None
csv_lock = threading.Lock()

def setup_csv_logging():
    """Setup CSV logging with timestamped filename"""
    global csv_writer, csv_file_handle
    
    if not CSV_LOGGING_ENABLED:
        return
    
    try:
        # Create logs directory if it doesn't exist
        log_dir = Path(CSV_LOG_DIR)
        log_dir.mkdir(exist_ok=True)
        
        # Create timestamped filename
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        csv_filename = log_dir / f"telemetry_{timestamp}.csv"
        
        # Open CSV file for writing
        csv_file_handle = open(csv_filename, 'w', newline='', encoding='utf-8')
        csv_writer = csv.DictWriter(csv_file_handle, fieldnames=CSV_FIELDNAMES)
        csv_writer.writeheader()
        csv_file_handle.flush()
        
        logger.info(f"📄 CSV logging enabled: {csv_filename}")
        
    except Exception as e:
        logger.error(f"❌ Failed to setup CSV logging: {e}")
        csv_writer = None

def log_to_csv(data_dict):
    """Log telemetry data to CSV file"""
    global csv_writer, csv_file_handle
    
    if not CSV_LOGGING_ENABLED or not csv_writer:
        return
    
    try:
        with csv_lock:
            # Create timestamp
            now = datetime.now()
            timestamp = now.timestamp()
            iso_timestamp = now.isoformat()
            
            # Extract Kalman data from multiple possible locations (ESP32 nested format)
            kalman_alt = (
                data_dict.get('kalman_data', {}).get('altitude', '') or
                data_dict.get('kalman_altitude', '') or
                data_dict.get('alt_data', {}).get('kalman_altitude', '')
            )
            kalman_vel = (
                data_dict.get('kalman_data', {}).get('vertical_velocity', '') or
                data_dict.get('kalman_vertical_velocity', '') or
                data_dict.get('alt_data', {}).get('kalman_vertical_velocity', '')
            )
            
            # Prepare CSV row with safe value extraction
            csv_row = {
                'timestamp': timestamp,
                'iso_timestamp': iso_timestamp,
                'record_number': data_dict.get('record_number', ''),
                'operation_mode': data_dict.get('operation_mode', ''),
                'state': data_dict.get('state', ''),
                'ax': data_dict.get('acc_data', {}).get('ax', ''),
                'ay': data_dict.get('acc_data', {}).get('ay', ''),
                'az': data_dict.get('acc_data', {}).get('az', ''),
                'pitch': data_dict.get('acc_data', {}).get('pitch', ''),
                'roll': data_dict.get('acc_data', {}).get('roll', ''),
                'gx': data_dict.get('gyro_data', {}).get('gx', ''),
                'gy': data_dict.get('gyro_data', {}).get('gy', ''),
                'gz': data_dict.get('gyro_data', {}).get('gz', ''),
                'latitude': data_dict.get('gps_data', {}).get('latitude', ''),
                'longitude': data_dict.get('gps_data', {}).get('longitude', ''),
                'gps_altitude': data_dict.get('gps_data', {}).get('gps_altitude', ''),
                'gps_time': data_dict.get('gps_data', {}).get('time', ''),
                'pressure': data_dict.get('alt_data', {}).get('pressure', ''),
                'temperature': data_dict.get('alt_data', {}).get('temperature', ''),
                'agl_altitude': data_dict.get('alt_data', {}).get('AGL', ''),
                'velocity': data_dict.get('alt_data', {}).get('velocity', ''),
                'pyro1_state': data_dict.get('chute_state', {}).get('pyro1_state', ''),
                'pyro2_state': data_dict.get('chute_state', {}).get('pyro2_state', ''),
                'battery_voltage': data_dict.get('battery_voltage', ''),
                'wifi_rssi': data_dict.get('wifi_rssi', ''),
                'kalman_altitude': kalman_alt,
                'kalman_vertical_velocity': kalman_vel,
                'communication_mode': data_dict.get('communication_mode', ''),
                'raw_data': json.dumps(data_dict) if isinstance(data_dict, dict) else str(data_dict)
            }
            
            csv_writer.writerow(csv_row)
            csv_file_handle.flush()  # Ensure data is written immediately
            
    except Exception as e:
        logger.error(f"❌ Failed to log to CSV: {e}")

def close_csv_logging():
    """Close CSV logging file"""
    global csv_file_handle
    
    try:
        if csv_file_handle:
            csv_file_handle.close()
            logger.info("📄 CSV logging closed")
    except Exception as e:
        logger.error(f"❌ Error closing CSV: {e}")

# Register CSV cleanup
atexit.register(close_csv_logging)

# === SERIAL CONNECTION ===
serial_conn = None
serial_lock = threading.Lock()
usb_reconnect_thread = None
usb_monitoring = True

def find_esp32_port():
    """Find ESP32 port with enhanced detection and availability check"""
    ports = list_ports.comports()
    esp32_ids = ['ESP32', 'CP210', 'CH340', 'CH341', 'Silicon Labs']
    bt_ids = ['HC-05', 'HC-06', 'Bluetooth', 'BTHENUM', 'Standard Serial over Bluetooth']
    
    # First pass: USB/TTL adapters or native ESP32
    for port in ports:
        desc = (port.description or "") + " " + (port.manufacturer or "")
        hwid = port.hwid or ""
        if any(id in desc for id in esp32_ids) or '10C4:EA60' in hwid:
            try:
                test_conn = serial.Serial(port.device, SERIAL_BAUD, timeout=0.5)
                test_conn.close()
                logger.debug(f"Found available ESP32/USB at {port.device}")
                return port.device
            except Exception as e:
                logger.debug(f"ESP32/USB found at {port.device} but not available: {e}")
                continue

    # Second pass: Bluetooth SPP (HC-05/HC-06)
    for port in ports:
        desc = (port.description or "") + " " + (port.manufacturer or "")
        hwid = port.hwid or ""
        if any(id in desc for id in bt_ids) or 'BTHENUM' in hwid:
            try:
                # HC-05 default is often 9600; we'll just test if port opens at 9600
                test_conn = serial.Serial(port.device, 9600, timeout=0.5)
                test_conn.close()
                logger.debug(f"Found available Bluetooth (HC-xx) at {port.device}")
                return port.device
            except Exception as e:
                logger.debug(f"Bluetooth port {port.device} not available: {e}")
                continue
    return None

def open_serial():
    global serial_conn
    with serial_lock:
        if serial_conn and serial_conn.is_open:
            try:
                serial_conn.close()
            except:
                pass
        
        port = find_esp32_port()
        if not port:
            logger.debug("No ESP32 port found")
            return False
            
        try:
            # Try configured baud first
            try_bauds = [SERIAL_BAUD]
            # Add a common fallback for Bluetooth HC-05/06
            if 9600 not in try_bauds:
                try_bauds.append(9600)

            last_err = None
            for baud in try_bauds:
                try:
                    serial_conn = serial.Serial(
                        port=port,
                        baudrate=baud,
                        timeout=1,
                        write_timeout=1,
                        bytesize=serial.EIGHTBITS,
                        parity=serial.PARITY_NONE,
                        stopbits=serial.STOPBITS_ONE
                    )
                    time.sleep(2)  # Wait for connection
                    serial_conn.reset_input_buffer()
                    logger.info(f"✅ Serial connected to {port} @ {baud} bps")
                    return True
                except Exception as e:
                    last_err = e
                    # If first baud fails, try next
                    try:
                        if serial_conn:
                            serial_conn.close()
                    except Exception:
                        pass
            # If we reach here, all baud attempts failed
            raise last_err if last_err else Exception("Unknown serial open error")
        except Exception as e:
            logger.warning(f"❌ Serial connection failed to {port}: {e}")
            return False

def close_serial():
    global serial_conn
    with serial_lock:
        if serial_conn and serial_conn.is_open:
            try:
                serial_conn.close()
                logger.info("📴 Serial port closed")
            except:
                pass

def monitor_usb_connection():
    """Monitor USB connection and auto-reconnect"""
    global serial_conn, usb_monitoring
    
    logger.info("🔍 Starting USB connection monitor...")
    consecutive_failures = 0
    max_consecutive_failures = 3
    
    while usb_monitoring:
        try:
            # Check if current connection is still valid
            if serial_conn and serial_conn.is_open:
                try:
                    # Test the connection by checking if port still exists
                    current_port = serial_conn.port
                    available_ports = [port.device for port in list_ports.comports()]
                    
                    if current_port not in available_ports:
                        logger.warning("⚠️ USB device disconnected, closing serial connection")
                        close_serial()
                        consecutive_failures = 0  # Reset failure counter
                    else:
                        # Connection is good, reset failure counter
                        consecutive_failures = 0
                        
                except Exception as e:
                    logger.warning(f"⚠️ Serial connection lost: {e}")
                    close_serial()
                    consecutive_failures = 0  # Reset failure counter
            else:
                # Try to reconnect if no connection
                esp32_port = find_esp32_port()
                if esp32_port:
                    logger.info(f"🔌 ESP32 detected at {esp32_port}, attempting reconnection...")
                    if open_serial():
                        logger.info("✅ USB reconnection successful!")
                        consecutive_failures = 0  # Reset failure counter
                    else:
                        consecutive_failures += 1
                        if consecutive_failures <= max_consecutive_failures:
                            logger.debug(f"❌ Reconnection failed (attempt {consecutive_failures}/{max_consecutive_failures}), will retry...")
                        else:
                            logger.debug(f"❌ Max reconnection attempts reached, waiting longer...")
                            time.sleep(USB_RECONNECT_INTERVAL * 3)  # Wait longer after repeated failures
                            consecutive_failures = 0  # Reset after longer wait
                else:
                    # No ESP32 detected, reset failure counter
                    consecutive_failures = 0
                        
            time.sleep(USB_RECONNECT_INTERVAL)
            
        except Exception as e:
            logger.error(f"❌ USB monitor error: {e}")
            time.sleep(USB_RECONNECT_INTERVAL)

def start_usb_monitor():
    """Start USB monitoring thread"""
    global usb_reconnect_thread
    if USB_MONITOR_ENABLED and (usb_reconnect_thread is None or not usb_reconnect_thread.is_alive()):
        usb_reconnect_thread = threading.Thread(target=monitor_usb_connection, daemon=True)
        usb_reconnect_thread.start()

def stop_usb_monitor():
    """Stop USB monitoring"""
    global usb_monitoring
    usb_monitoring = False

# === LOGGING ===
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger("BaseStationServer")

# === CONNECTION STATUS ===
class ConnectionStatus:
    def __init__(self):
        self.lock = Lock()
        self.last_serial = 0
        self.last_mqtt = 0
        self.current_mode = "UNKNOWN"
        
    def update_serial(self):
        with self.lock:
            self.last_serial = time.time()
            
    def update_mqtt(self):
        with self.lock:
            self.last_mqtt = time.time()
            
    def get_mode(self):
        with self.lock:
            now = time.time()
            serial_active = (now - self.last_serial) < AUTO_DETECT_INTERVAL*2
            mqtt_active = (now - self.last_mqtt) < AUTO_DETECT_INTERVAL*2
            
            if serial_active and mqtt_active:
                return "DUAL"
            elif mqtt_active:
                return "MQTT"
            elif serial_active:
                return "BEACON"
            return "UNKNOWN"

connection_status = ConnectionStatus()

# === LATEST TELEMETRY CACHE FOR PRINTING & GUI ===
last_telemetry = {}
last_telemetry_lock = Lock()

def update_last_telemetry(data: dict):
    """Store the latest telemetry record (thread-safe) and print summary with RSSI, location, timestamp."""
    global last_telemetry
    with last_telemetry_lock:
        last_telemetry = data.copy()
        last_telemetry['received_at'] = time.time()
    # Print concise summary focusing on requested fields (RSSI, location, timestamp)
    try:
        gps = data.get('gps_data', {})
        alt_data = data.get('alt_data', {})
        kalman = data.get('kalman_data', {})
        rssi = data.get('wifi_rssi', data.get('rssi', ''))
        lat = gps.get('latitude')
        lon = gps.get('longitude')
        gps_alt = gps.get('altitude') or gps.get('gps_altitude')
        agl = alt_data.get('AGL')
        kal_alt = kalman.get('altitude')
        ts = datetime.utcnow().isoformat(timespec='seconds') + 'Z'
        logger.info(f"TELEM t={ts} RSSI={rssi} lat={lat} lon={lon} gps_alt={gps_alt} AGL={agl} kal_alt={kal_alt}")
    except Exception as e:
        logger.debug(f"Telemetry summary print failed: {e}")

# === MQTT CLIENT ===
client = mqtt.Client(callback_api_version=mqtt.CallbackAPIVersion.VERSION1, protocol=mqtt.MQTTv311)
mqtt_connected = False

def setup_mqtt_connection():
    """Setup MQTT connection with retry logic"""
    global client, mqtt_connected
    
    max_retries = 10
    retry_delay = 2
    
    for attempt in range(max_retries):
        try:
            logger.info(f"🔌 MQTT connection attempt {attempt + 1}/{max_retries}")
            client.connect(MQTT_BROKER, MQTT_PORT, 60)
            client.loop_start()
            
            # Wait a moment to see if connection succeeds
            time.sleep(1)
            if mqtt_connected:
                logger.info("✅ MQTT connected successfully")
                return True
                
        except Exception as e:
            logger.warning(f"⚠️ MQTT connection attempt {attempt + 1} failed: {e}")
            
        if attempt < max_retries - 1:
            logger.info(f"⏳ Retrying in {retry_delay} seconds...")
            time.sleep(retry_delay)
    
    logger.error("❌ Failed to connect to MQTT after all attempts")
    return False

def on_mqtt_connect(client, userdata, flags, rc):
    global mqtt_connected
    if rc == 0:
        mqtt_connected = True
        client.subscribe(MQTT_COMMAND_TOPIC)
        client.subscribe(MQTT_TELEMETRY_TOPIC)
        logger.info("✅ MQTT connected")
    else:
        mqtt_connected = False
        logger.error(f"❌ MQTT connection failed with code {rc}")

def on_mqtt_disconnect(client, userdata, rc):
    global mqtt_connected
    mqtt_connected = False
    logger.warning("⚠️ MQTT disconnected")

def on_mqtt_message(client, userdata, msg):
    try:
        if msg.topic == MQTT_COMMAND_TOPIC:
            cmd = msg.payload.decode().strip()
            logger.info(f"📨 MQTT command: {cmd}")
            send_command(cmd, "MQTT")
            
        elif msg.topic == MQTT_TELEMETRY_TOPIC:
            payload = msg.payload.decode(errors='replace').strip()
            if not payload:
                return
                
            # Forward to app with consistent format
            try:
                # Check if it's CSV format (like your example)
                if "," in payload and not payload.startswith("{"):
                    fields = payload.split(",")
                    if len(fields) >= 25:  # Updated to match new 25-field CSV format
                        data = {
                            "record_number": int(fields[0]),
                            "operation_mode": int(fields[1]),
                            "state": int(fields[2]),
                            "acc_data": {
                                "ax": float(fields[3]),
                                "ay": float(fields[4]),
                                "az": float(fields[5]),
                                "pitch": float(fields[6]),
                                "roll": float(fields[7])
                            },
                            "gyro_data": {
                                "gx": float(fields[8]),
                                "gy": float(fields[9]),
                                "gz": float(fields[10])
                            },
                            "gps_data": {
                                "latitude": float(fields[11]),
                                "longitude": float(fields[12]),
                                "altitude": float(fields[13]),
                                "time": int(fields[14])
                            },
                            "alt_data": {
                                "pressure": float(fields[15]),
                                "temperature": float(fields[16]),
                                "AGL": float(fields[17]),
                                "velocity": float(fields[18])
                            },
                            "chute_state": {
                                "drogue": int(fields[19]),
                                "main": int(fields[20])
                            },
                            "battery_voltage": float(fields[21]),
                            "wifi_rssi": int(fields[22]),
                            "kalman_data": {
                                "altitude": float(fields[23]),
                                "vertical_velocity": float(fields[24])
                            },
                            "communication_mode": "MQTT",
                            "timestamp": time.time(),
                            "raw": payload  # Include raw data for reference
                        }
                        update_last_telemetry(data)
                        if mqtt_connected:
                            client.publish(APP_TELEMETRY_TOPIC, json.dumps(data))
                            connection_status.update_mqtt()
                            logger.debug(f"📡 Forwarded MQTT telemetry (CSV format)")
                        else:
                            logger.debug("📡 MQTT not connected, skipping telemetry forward")
                        # Always log to CSV
                        log_to_csv(data)
                    else:
                        logger.warning(f"⚠️ Invalid CSV format (got {len(fields)} fields, expected 25): {payload}")
                else:
                    # Try to parse as JSON if not CSV
                    try:
                        data = json.loads(payload)
                        # Handle both nested kalman_data and flat kalman fields from ESP32
                        if 'kalman_altitude' in data and 'kalman_vertical_velocity' in data:
                            data['kalman_data'] = {
                                'altitude': data.get('kalman_altitude', 0),
                                'vertical_velocity': data.get('kalman_vertical_velocity', 0)
                            }
                        elif 'kalman_data' not in data:
                            # Add empty kalman_data if not present
                            data['kalman_data'] = {
                                'altitude': 0,
                                'vertical_velocity': 0
                            }
                        
                        data["communication_mode"] = "MQTT"
                        update_last_telemetry(data)
                        if mqtt_connected:
                            client.publish(APP_TELEMETRY_TOPIC, json.dumps(data))
                            connection_status.update_mqtt()
                            logger.debug(f"📡 Forwarded MQTT telemetry (JSON format)")
                        else:
                            logger.debug("📡 MQTT not connected, skipping telemetry forward")
                        log_to_csv(data)
                    except json.JSONDecodeError:
                        logger.warning(f"⚠️ Unrecognized MQTT payload format: {payload}")
            except Exception as e:
                logger.error(f"❌ MQTT telemetry processing error: {e}")
                
    except Exception as e:
        logger.error(f"❌ MQTT message handling error: {e}")

client.on_connect = on_mqtt_connect
client.on_disconnect = on_mqtt_disconnect
client.on_message = on_mqtt_message

# Don't connect immediately - will connect after Mosquitto starts

# === DATA HANDLING ===
def process_serial_data(line):
    """Process incoming serial data and forward to app with consistent format"""
    try:
        line = line.strip()
        if not line:
            return
            
        # Skip log/status messages
        if line.startswith(("LOG:", "STATUS:")):
            logger.debug(f"📝 [SERIAL] {line}")
            return
        
        # Skip incomplete JSON lines (common with serial communication)
        if line.startswith('{') and not line.endswith('}'):
            logger.debug(f"⚠️ Incomplete JSON line, skipping: {line[:50]}...")
            return
            
        # Check if it's CSV format (like your MQTT data)
        if "," in line and not line.startswith("{"):
            fields = line.split(",")
            if len(fields) >= 25:  # Updated to match new 25-field CSV format
                data = {
                    "record_number": int(fields[0]),
                    "operation_mode": int(fields[1]),
                    "state": int(fields[2]),
                    "acc_data": {
                        "ax": float(fields[3]),
                        "ay": float(fields[4]),
                        "az": float(fields[5]),
                        "pitch": float(fields[6]),
                        "roll": float(fields[7])
                    },
                    "gyro_data": {
                        "gx": float(fields[8]),
                        "gy": float(fields[9]),
                        "gz": float(fields[10])
                    },
                    "gps_data": {
                        "latitude": float(fields[11]),
                        "longitude": float(fields[12]),
                        "altitude": float(fields[13]),
                        "time": int(fields[14])
                    },
                    "alt_data": {
                        "pressure": float(fields[15]),
                        "temperature": float(fields[16]),
                        "AGL": float(fields[17]),
                        "velocity": float(fields[18])
                    },
                    "chute_state": {
                        "drogue": int(fields[19]),
                        "main": int(fields[20])
                    },
                    "battery_voltage": float(fields[21]),
                    "wifi_rssi": int(fields[22]),
                    "kalman_data": {
                        "altitude": float(fields[23]),
                        "vertical_velocity": float(fields[24])
                    },
                    "communication_mode": "BEACON",
                    "timestamp": time.time(),
                    "raw": line  # Include raw data for reference
                }
                update_last_telemetry(data)
                if mqtt_connected:
                    client.publish(APP_TELEMETRY_TOPIC, json.dumps(data))
                    connection_status.update_serial()
                    logger.debug(f"📡 Forwarded beacon telemetry")
                else:
                    logger.debug("📡 MQTT not connected, skipping telemetry forward")
                log_to_csv(data)
            else:
                logger.warning(f"⚠️ Invalid CSV format: {line}")
        else:
            # Try to parse as JSON if not CSV
            try:
                data = json.loads(line)
                
                # Normalize Kalman data from ESP32's nested structure
                # ESP32 sends kalman data nested within alt_data
                kalman_alt = (
                    data.get('alt_data', {}).get('kalman_altitude') or
                    data.get('kalman_data', {}).get('altitude') or
                    data.get('kalman_altitude', 0)
                )
                kalman_vel = (
                    data.get('alt_data', {}).get('kalman_vertical_velocity') or
                    data.get('kalman_data', {}).get('vertical_velocity') or
                    data.get('kalman_vertical_velocity', 0)
                )
                
                # Ensure normalized kalman_data structure for app consistency
                data['kalman_data'] = {
                    'altitude': kalman_alt,
                    'vertical_velocity': kalman_vel
                }
                
                # Also add flat fields for backward compatibility
                data['kalman_altitude'] = kalman_alt
                data['kalman_vertical_velocity'] = kalman_vel
                
                # Normalize communication mode
                data["communication_mode"] = data.get("communication_mode", "BEACON").upper()
                update_last_telemetry(data)
                if mqtt_connected:
                    client.publish(APP_TELEMETRY_TOPIC, json.dumps(data))
                    connection_status.update_serial()
                    logger.info(f"📡 Forwarded beacon JSON telemetry - Record #{data.get('record_number', 'N/A')} - Kalman Alt: {kalman_alt}, Vel: {kalman_vel}")
                else:
                    logger.debug("📡 MQTT not connected, skipping telemetry forward")
                log_to_csv(data)
            except json.JSONDecodeError:
                logger.warning(f"⚠️ Unrecognized serial payload format: {line}")
                
    except Exception as e:
        logger.error(f"❌ Serial data processing error: {e}")

def send_command(cmd, source):
    """Send command to flight computer with improved error handling"""
    if not serial_conn or not serial_conn.is_open:
        logger.debug("Serial not connected, attempting to connect...")
        if not open_serial():
            logger.warning("⚠️ Cannot send command - no serial connection")
            return False
    
    try:
        with serial_lock:
            if not serial_conn or not serial_conn.is_open:
                return False
                
            serial_conn.reset_input_buffer()
            serial_conn.reset_output_buffer()
            serial_conn.write((cmd + "\n").encode())
            serial_conn.flush()
            
            logger.info(f"📤 Sent command: {cmd} (source: {source})")
            
            # Wait for response
            start_time = time.time()
            while time.time() - start_time < COMMAND_TIMEOUT:
                if serial_conn.in_waiting:
                    response = serial_conn.readline().decode(errors='replace').strip()
                    if response:
                        logger.info(f"📩 Response: {response}")
                        return True
                time.sleep(0.1)
                
            logger.warning("⌛ No response received")
            return False
            
    except Exception as e:
        logger.error(f"❌ Command failed: {e}")
        # Don't close serial connection here, let the monitor handle it
        return False

# === MAIN LOOP ===
def main_loop():
    """Main processing loop with improved error handling"""
    last_status = 0
    
    while True:
        try:
            # Check serial for data (with improved error handling)
            if serial_conn and serial_conn.is_open:
                try:
                    while serial_conn.in_waiting:
                        line = serial_conn.readline().decode(errors='replace').strip()
                        if line:
                            process_serial_data(line)
                except Exception as e:
                    logger.warning(f"⚠️ Serial read error: {e}")
                    # Don't close connection here, let monitor handle it
            
            # Send status periodically
            if time.time() - last_status > HEARTBEAT_INTERVAL:
                status = {
                    "mode": connection_status.get_mode(),
                    "timestamp": time.time(),
                    "serial_connected": serial_conn is not None and serial_conn.is_open,
                    "mqtt_connected": mqtt_connected,
                    "last_serial": connection_status.last_serial,
                    "last_mqtt": connection_status.last_mqtt
                }
                if mqtt_connected:
                    client.publish(MQTT_STATUS_TOPIC, json.dumps(status))
                last_status = time.time()
            
            time.sleep(0.05)  # Reduced sleep for better responsiveness
            
        except KeyboardInterrupt:
            logger.info("🛑 Keyboard interrupt received")
            break
        except Exception as e:
            logger.error(f"❌ Main loop error: {e}")
            time.sleep(1)

def command_interface():
    """Interactive command interface"""
    print("\nN4 Command Interface (type 'help' for commands)")
    print("App services starting in background...")
    while True:
        try:
            cmd = input("n4> ").strip().lower()
            if cmd == "help":
                print("Commands: status, mqtt, beacon, dual, arm, disarm, reset, restart_app, toggle_usb, toggle_csv, quit")
            elif cmd == "quit":
                break
            elif cmd == "restart_app":
                logger.info("🔄 Restarting application services...")
                cleanup_processes()
                time.sleep(2)
                start_app_services()
            elif cmd == "toggle_usb":
                global USB_MONITOR_ENABLED
                USB_MONITOR_ENABLED = not USB_MONITOR_ENABLED
                if USB_MONITOR_ENABLED:
                    print("✅ USB monitoring enabled")
                    start_usb_monitor()
                else:
                    print("❌ USB monitoring disabled")
                    stop_usb_monitor()
            elif cmd == "toggle_csv":
                global CSV_LOGGING_ENABLED
                CSV_LOGGING_ENABLED = not CSV_LOGGING_ENABLED
                if CSV_LOGGING_ENABLED:
                    print("✅ CSV logging enabled")
                    setup_csv_logging()
                else:
                    print("❌ CSV logging disabled")
                    close_csv_logging()
            elif cmd in ["status", "mqtt", "beacon", "dual", "arm", "disarm", "reset"]:
                send_command(cmd.upper(), "CONSOLE")
            else:
                print("Unknown command")
        except (EOFError, KeyboardInterrupt):
            break
        except Exception as e:
            print(f"Error: {e}")

# === SIMPLE TKINTER GUI ===
gui_thread = None

def start_gui():
    """Launch a simple Tkinter GUI showing RSSI, location, altitudes, and Arm/Disarm buttons."""
    if not TK_AVAILABLE:
        logger.warning("Tkinter not available; GUI disabled")
        return

    def gui_loop():
        root = tk.Tk()
        root.title("N4 Base Station")
        root.geometry("420x260")

        style = ttk.Style(root)
        try:
            style.theme_use('clam')
        except Exception:
            pass

        # Labels
        vars_map = {
            'timestamp': tk.StringVar(value='-'),
            'rssi': tk.StringVar(value='-'),
            'lat': tk.StringVar(value='-'),
            'lon': tk.StringVar(value='-'),
            'gps_alt': tk.StringVar(value='-'),
            'agl': tk.StringVar(value='-'),
            'kal_alt': tk.StringVar(value='-'),
            'vel': tk.StringVar(value='-'),
            'mode': tk.StringVar(value='UNKNOWN'),
        }

        row = 0
        for label, key in [
            ("Mode", 'mode'),
            ("Timestamp", 'timestamp'),
            ("RSSI", 'rssi'),
            ("Latitude", 'lat'),
            ("Longitude", 'lon'),
            ("GPS Alt", 'gps_alt'),
            ("AGL", 'agl'),
            ("Kalman Alt", 'kal_alt'),
            ("Velocity", 'vel'),
        ]:
            ttk.Label(root, text=label+':').grid(column=0, row=row, sticky='e', padx=6, pady=2)
            ttk.Label(root, textvariable=vars_map[key], width=20).grid(column=1, row=row, sticky='w')
            row += 1

        # Command buttons
        btn_frame = ttk.Frame(root)
        btn_frame.grid(column=0, row=row, columnspan=2, pady=10)

        def send_cmd(c):
            ok = send_command(c, "GUI")
            if not ok:
                logger.warning(f"GUI command {c} failed")

        for c in ["ARM", "DISARM", "STATUS"]:
            ttk.Button(btn_frame, text=c, command=lambda cc=c: send_cmd(cc)).pack(side='left', padx=5)

        def refresh():
            with last_telemetry_lock:
                data = last_telemetry.copy()
            if data:
                gps = data.get('gps_data', {})
                alt_data = data.get('alt_data', {})
                kalman = data.get('kalman_data', {})
                vars_map['timestamp'].set(datetime.utcnow().strftime('%H:%M:%S'))
                vars_map['rssi'].set(data.get('wifi_rssi', data.get('rssi', '-')))
                vars_map['lat'].set(f"{gps.get('latitude', '-'):.6f}" if isinstance(gps.get('latitude'), (int,float)) else '-')
                vars_map['lon'].set(f"{gps.get('longitude', '-'):.6f}" if isinstance(gps.get('longitude'), (int,float)) else '-')
                ga = gps.get('altitude') or gps.get('gps_altitude')
                vars_map['gps_alt'].set(f"{ga:.1f}" if isinstance(ga, (int,float)) else '-')
                agl = alt_data.get('AGL')
                vars_map['agl'].set(f"{agl:.1f}" if isinstance(agl, (int,float)) else '-')
                kal_alt = kalman.get('altitude')
                vars_map['kal_alt'].set(f"{kal_alt:.1f}" if isinstance(kal_alt, (int,float)) else '-')
                vel = alt_data.get('velocity') or kalman.get('vertical_velocity')
                vars_map['vel'].set(f"{vel:.2f}" if isinstance(vel, (int,float)) else '-')
                vars_map['mode'].set(connection_status.get_mode())
            root.after(500, refresh)

        refresh()
        root.mainloop()

    # Run GUI in its own thread so CLI still works
    gt = threading.Thread(target=gui_loop, daemon=True)
    gt.start()
    return gt

if __name__ == '__main__':
    logger.info("🚀 Starting N4 Base Station Server")
    try:
        headless = os.environ.get('N4_HEADLESS') == '1'
        if not headless:
            # Start application services first
            start_app_services()
        else:
            logger.info("HEADLESS mode: skipping external services start")

        # Setup CSV logging
        setup_csv_logging()

        # Setup MQTT connection after services are started (unless headless and broker unavailable)
        setup_mqtt_connection()

        # Start USB monitoring
        start_usb_monitor()

        # Attempt initial serial connection
        open_serial()

        # Start main loop in background
        loop_thread = threading.Thread(target=main_loop, daemon=True)
        loop_thread.start()

        # Start GUI (non-blocking) unless headless
        if not headless:
            start_gui()

        # Run command interface (blocking until quit)
        command_interface()

    except KeyboardInterrupt:
        logger.info("🛑 Shutting down...")
    finally:
        # Cleanup
        stop_usb_monitor()
        close_serial()
        try:
            client.disconnect()
        except Exception:
            pass
        cleanup_processes()
        logger.info("👋 Shutdown complete")