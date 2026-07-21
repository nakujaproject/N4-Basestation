import serial
import json
import logging
import time
import threading
import paho.mqtt.client as mqtt
from serial.tools import list_ports
from threading import Lock
from datetime import datetime, timezone
import subprocess
import os
import atexit
import csv
from pathlib import Path
import shutil
import sqlite3
from http.server import BaseHTTPRequestHandler, HTTPServer
from urllib.parse import urlparse

# === OPTIONAL GUI (tkinter) ===
try:
    import tkinter as tk
    from tkinter import ttk
    TK_AVAILABLE = True
except Exception:
    TK_AVAILABLE = False

# === CONFIG ===
SERIAL_BAUD = 460800  # High-speed Bluetooth (matches ESP32 HC-05/HC-06 config)
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
PORT_8080 = 8080           # Port for development server (used by external tileserver)

# === SERIAL PORT CONFIG ===
# Optionally set COM port via environment variable (for HC-05 Bluetooth or any specific port)
N4_COM_PORT = os.environ.get('N4_COM_PORT', 'COM12').strip()

# === COMMUNICATION METHOD PREFERENCE ===
# FORCE_USB_SERIAL: True = Only use USB Serial (ignore Bluetooth)
#                   False = Auto-detect (prefer Bluetooth if available, fallback to USB)
# You can also set via environment: N4_FORCE_USB=1
FORCE_USB_SERIAL = os.environ.get('N4_FORCE_USB', '0').strip() == '1'
# FORCE_BLUETOOTH: True = Only use Bluetooth (ignore USB Serial)
#                  False = Auto-detect
# You can also set via environment: N4_FORCE_BT=1
FORCE_BLUETOOTH = os.environ.get('N4_FORCE_BT', '0').strip() == '1'

# === CONTROL & TEST FLAGS ===
# USE_GUI_CONTROL: 1 = control via Tkinter GUI; 0 = control via React app (MQTT commands)
USE_GUI_CONTROL = int(os.environ.get('N4_USE_GUI', '0'))
# SIMULATION_MODE: 1 = simulate telemetry; 0 = use real serial
SIMULATION_MODE = int(os.environ.get('N4_SIM', '0'))
SIM_RATE_HZ = int(os.environ.get('N4_SIM_RATE', '20'))

# === MAP/TILES SETUP ===
# Tiles are served externally (e.g., tileserver-gl on 8080). No internal tileserver here.

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
app_processes = []  # Deprecated: left empty for backward compatibility

"""
Tiles are served by an external process (tileserver-gl) started by start_basestation.py.
Internal Python tileserver code removed for simplicity and stability.
"""

def get_node_major_version():
    try:
        out = subprocess.check_output(["node", "-v"], text=True, timeout=3).strip()
        # v22.18.0 -> 22
        if out.startswith('v'):
            out = out[1:]
        major = int(out.split('.')[0])
        return major
    except Exception:
        return None

def docker_available():
    return False

def kill_processes_on_port(port):
    # Orchestration moved to start_basestation.py
    return

def start_app_services():
    # Orchestration moved to start_basestation.py
    logger.info("ℹ External services should be started by start_basestation.py")

def cleanup_processes():
    # No child processes are spawned by this module anymore
    return

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

    # Communication method preference logging
    if FORCE_USB_SERIAL:
        logger.info("🔌 Communication Mode: FORCED USB Serial (Bluetooth disabled)")
    elif FORCE_BLUETOOTH:
        logger.info("📡 Communication Mode: FORCED Bluetooth (USB disabled)")
    else:
        logger.info("🔄 Communication Mode: Auto-detect (Bluetooth preferred, USB fallback)")

    # 1. If explicit COM port is set, try it first (for HC-05 or any user-specified port)
    if N4_COM_PORT:
        # Skip if forced mode conflicts
        if FORCE_USB_SERIAL and any(bt_id in N4_COM_PORT.upper() for bt_id in ['BLUETOOTH', 'BT']):
            logger.warning(f"⚠️ Skipping Bluetooth port {N4_COM_PORT} (USB Serial forced)")
        elif FORCE_BLUETOOTH and not any(bt_id in N4_COM_PORT.upper() for bt_id in ['BLUETOOTH', 'BT']):
            logger.warning(f"⚠️ Skipping USB port {N4_COM_PORT} (Bluetooth forced)")
        else:
            try:
                # Try default baud for HC-05 (9600), fallback to SERIAL_BAUD
                try_bauds = [9600, SERIAL_BAUD]
                for baud in try_bauds:
                    try:
                        test_conn = serial.Serial(N4_COM_PORT, baud, timeout=0.5)
                        test_conn.close()
                        logger.info(f"✅ Using specified COM port: {N4_COM_PORT} @ {baud} bps")
                        return N4_COM_PORT
                    except Exception as e:
                        logger.debug(f"Specified COM port {N4_COM_PORT} not available at {baud}: {e}")
                # If both bauds fail, fallback to auto-detect
            except Exception as e:
                logger.debug(f"Specified COM port {N4_COM_PORT} not available: {e}")

    # 2. Auto-detect ESP32/USB/TTL adapters (skip if Bluetooth forced)
    if not FORCE_BLUETOOTH:
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

    # 3. Auto-detect Bluetooth SPP (HC-05/HC-06) (skip if USB forced)
    if not FORCE_USB_SERIAL:
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
        self.last_xbee = 0
        self.current_mode = "UNKNOWN"
        
    def update_serial(self):
        with self.lock:
            self.last_serial = time.time()
            
    def update_mqtt(self):
        with self.lock:
            self.last_mqtt = time.time()
            
    def update_xbee(self):
        with self.lock:
            self.last_xbee = time.time()
            
    def get_mode(self):
        with self.lock:
            now = time.time()
            serial_active = (now - self.last_serial) < AUTO_DETECT_INTERVAL*2
            mqtt_active = (now - self.last_mqtt) < AUTO_DETECT_INTERVAL*2
            xbee_active = (now - self.last_xbee) < AUTO_DETECT_INTERVAL*2
            
            # Build mode string from active connections
            active_modes = []
            if mqtt_active:
                active_modes.append("MQTT")
            if serial_active:
                active_modes.append("BEACON")
            if xbee_active:
                active_modes.append("XBEE")
            
            if len(active_modes) > 1:
                return "+".join(active_modes)  # e.g., "MQTT+XBEE"
            elif len(active_modes) == 1:
                return active_modes[0]
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
        ts = datetime.now(timezone.utc).isoformat(timespec='seconds').replace('+00:00', 'Z')
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
            if USE_GUI_CONTROL:
                logger.info(f"📨 MQTT command ignored (GUI control active): {cmd}")
                return
            logger.info(f"📨 MQTT command: {cmd}")
            # Handle overrides in simulation mode locally; otherwise forward to serial
            handled = False
            upper = cmd.upper()
            # Accept both legacy and new command names
            if SIMULATION_MODE:
                from datetime import datetime as _dt
                global SIM_DROGUE_ARMED, SIM_MAIN_ARMED, SIM_AUTO_FALLBACK, SIM_ARMED
                # Accept ARM_DROGUE, DISARM_DROGUE, ARM_MAIN, DISARM_MAIN
                if upper in ("ARM", "DISARM", "DROGUE_ARM", "DROGUE_DISARM", "MAIN_ARM", "MAIN_DISARM", "ARM_DROGUE", "DISARM_DROGUE", "ARM_MAIN", "DISARM_MAIN", "AUTO_ON", "AUTO_OFF"):
                    if upper in ("DROGUE_ARM", "ARM_DROGUE"):
                        SIM_DROGUE_ARMED = 1
                    elif upper in ("DROGUE_DISARM", "DISARM_DROGUE"):
                        SIM_DROGUE_ARMED = 0
                    elif upper in ("MAIN_ARM", "ARM_MAIN"):
                        SIM_MAIN_ARMED = 1
                    elif upper in ("MAIN_DISARM", "DISARM_MAIN"):
                        SIM_MAIN_ARMED = 0
                    elif upper == "ARM":
                        SIM_ARMED = 1
                    elif upper == "DISARM":
                        SIM_ARMED = 0
                    elif upper == "AUTO_ON":
                        SIM_AUTO_FALLBACK = True
                    elif upper == "AUTO_OFF":
                        SIM_AUTO_FALLBACK = False
                    logger.info(f"🧪 [SIM] Override applied: drogue={SIM_DROGUE_ARMED}, main={SIM_MAIN_ARMED}, auto={SIM_AUTO_FALLBACK}")
                    try:
                        csv_line = generate_sim_csv(0, time.time())
                        process_serial_data(csv_line)
                    except Exception:
                        pass
                    handled = True
            if not handled:
                # Map new commands to serial
                serial_cmd = upper
                # DROGUE
                if upper in ("ARM_DROGUE", "DROGUE_ARM"):
                    serial_cmd = "DROGUE_ON"
                elif upper in ("DISARM_DROGUE", "DROGUE_DISARM"):
                    serial_cmd = "DROGUE_OFF"
                # MAIN
                elif upper in ("ARM_MAIN", "MAIN_ARM"):
                    serial_cmd = "MAIN_ON"
                elif upper in ("DISARM_MAIN", "MAIN_DISARM"):
                    serial_cmd = "MAIN_OFF"
                send_command(serial_cmd, "MQTT")
            
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
                            "pyro1_state": int(fields[19]),
                            "pyro2_state": int(fields[20]),
                            "pyro1_state": int(fields[19]),
                            "pyro2_state": int(fields[20]),
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
                        # Add app-friendly flat fields
                        data["pyroDrogue"] = data["chute_state"]["drogue"]
                        data["pyroMain"] = data["chute_state"]["main"]
                        # Add app-friendly flat fields
                        data["pyroDrogue"] = data["chute_state"]["drogue"]
                        data["pyroMain"] = data["chute_state"]["main"]
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
        
        # Strip device identifier if present (e.g., |ESP32:N4_BASE_BT_1)
        if '|ESP32:' in line:
            line = line.split('|ESP32:')[0].strip()
            
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
                
                # Flatten chute state for the app (support both drogue/main and pyro1/pyro2)
                if isinstance(data.get('chute_state'), dict):
                    chute = data['chute_state']
                    # Prefer pyro1_state/pyro2_state if present, else fallback to drogue/main
                    data['pyroDrogue'] = chute.get('pyro1_state', chute.get('drogue', 0))
                    data['pyroMain'] = chute.get('pyro2_state', chute.get('main', 0))
                    data['pyro1_state'] = chute.get('pyro1_state', chute.get('drogue', 0))
                    data['pyro2_state'] = chute.get('pyro2_state', chute.get('main', 0))

                # Normalize communication mode and update connection status
                comm_mode = data.get("communication_mode", "BEACON").upper()
                data["communication_mode"] = comm_mode
                
                # Update connection status based on mode
                if comm_mode == "XBEE":
                    connection_status.update_xbee()
                elif comm_mode in ["BEACON", "AUTO"]:
                    connection_status.update_serial()
                
                update_last_telemetry(data)
                if mqtt_connected:
                    client.publish(APP_TELEMETRY_TOPIC, json.dumps(data))
                    logger.info(f"📡 Forwarded beacon JSON telemetry - Record #{data.get('record_number', 'N/A')} - Kalman Alt: {kalman_alt}, Vel: {kalman_vel}")
                else:
                    logger.debug("📡 MQTT not connected, skipping telemetry forward")
                log_to_csv(data)
            except json.JSONDecodeError:
                # Ignore debug messages from ESP32 (e.g., "[XBEE TX] Packet #...", "[BEACON TX]", "[WIFI TX]")
                if line.startswith('[') or line.startswith('STATUS:'):
                    logger.debug(f"🔧 ESP32 debug: {line}")
                else:
                    logger.warning(f"⚠️ Unrecognized serial payload format: {line}")
                
    except Exception as e:
        logger.error(f"❌ Serial data processing error: {e}")

def send_command(cmd, source):
    """Send command to flight computer with improved error handling"""
    # Map friendly names to actual commands
    command_map = {
        'MQTT': 'CMD_MQTT_MODE',
        'BEACON': 'CMD_BEACON_MODE',
        'XBEE': 'CMD_XBEE_MODE',
        'AUTO_ON': 'CMD_AUTO_FALLBACK_ON',
        'AUTO_OFF': 'CMD_AUTO_FALLBACK_OFF',
    }
    
    cmd_upper = cmd.upper().strip()
    actual_cmd = command_map.get(cmd_upper, cmd)
    
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
            serial_conn.write((actual_cmd + "\n").encode())
            serial_conn.flush()
            
            logger.info(f"📤 Sent command: {actual_cmd} (from {cmd}, source: {source})")
            
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

# === TELEMETRY SIMULATOR ===
sim_thread = None
sim_running = False
SIM_DROGUE_ARMED = 0
SIM_MAIN_ARMED = 0
SIM_AUTO_FALLBACK = False
SIM_ARMED = 0

def generate_sim_csv(record_number: int, t0: float):
    """Generate one 25-field CSV row matching firmware format."""
    import math
    # Simulate a moving path: lat/lon change over time in a curve
    t = time.time() - t0
    ax, ay, az = 0.0, 0.0, 1.0
    pitch, roll = 0.0, 0.0
    gx, gy, gz = 0.0, 0.0, 0.0
    # Start: Nairobi CBD, move east and north in a curve
    start_lat, start_lon = -1.286389, 36.817223
    # Path: spiral outwards, then stop
    radius = 0.01 + 0.0005 * t  # meters to degrees
    angle = t * 0.15  # radians, slow rotation
    lat = start_lat + radius * math.cos(angle)
    lon = start_lon + radius * math.sin(angle)
    gps_alt = 1660.0 + 10 * math.sin(t/10)
    gps_time = int(time.time())
    pressure = 85000.0
    temperature = 25.0
    alt_agl = max(0.0, 100.0 * math.sin(min(t, 60)/60.0 * math.pi))
    vel = 100.0 * (math.pi/60.0) * math.cos(min(t, 60)/60.0 * math.pi)
    # Apply overrides; if AUTO fallback, keep time-based auto as well
    if SIM_AUTO_FALLBACK:
        auto_drogue = 1 if t > 30 else 0
        auto_main = 1 if t > 40 else 0
        drogue = max(SIM_DROGUE_ARMED, auto_drogue)
        main = max(SIM_MAIN_ARMED, auto_main)
    else:
        drogue = SIM_DROGUE_ARMED
        main = SIM_MAIN_ARMED
    battery = 11.8
    rssi = int(-40 - 20*math.log10(1+t))
    kal_alt = alt_agl * 0.98
    kal_vv = vel * 0.95

    fields = [
        record_number,  # 0 record_number
        SIM_ARMED,      # 1 operation_mode (user-controlled in SIM)
        1,              # 2 state
        ax, ay, az,     # 3-5 acc
        pitch, roll,    # 6-7
        gx, gy, gz,     # 8-10 gyro
        lat, lon, gps_alt, gps_time,  # 11-14 gps
        pressure, temperature, alt_agl, vel,  # 15-18 alt data
        drogue, main,   # 19-20 chute
        battery, rssi,  # 21-22 battery & rssi
        kal_alt, kal_vv # 23-24 kalman
    ]
    return ",".join(str(x) for x in fields)

def simulation_loop():
    global sim_running
    logger.info("🧪 Simulation mode active: generating telemetry")
    sim_running = True
    rate = max(1, SIM_RATE_HZ)
    period = 1.0 / rate
    rn = 0
    t0 = time.time()
    while sim_running:
        rn += 1
        csv_line = generate_sim_csv(rn, t0)
        process_serial_data(csv_line)
        time.sleep(period)

def start_simulation():
    global sim_thread
    if sim_thread and sim_thread.is_alive():
        return
    sim_thread = threading.Thread(target=simulation_loop, daemon=True)
    sim_thread.start()

def stop_simulation():
    global sim_running
    sim_running = False

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

        # Main arm/disarm buttons
        ttk.Button(btn_frame, text="ARM MAIN", command=lambda: send_cmd("ARM_MAIN")).pack(side='left', padx=5)
        ttk.Button(btn_frame, text="DISARM MAIN", command=lambda: send_cmd("DISARM_MAIN")).pack(side='left', padx=5)
        # Drogue arm/disarm buttons
        ttk.Button(btn_frame, text="ARM DROGUE", command=lambda: send_cmd("ARM_DROGUE")).pack(side='left', padx=5)
        ttk.Button(btn_frame, text="DISARM DROGUE", command=lambda: send_cmd("DISARM_DROGUE")).pack(side='left', padx=5)
        # Legacy/status buttons
        for c in ["ARM", "DISARM", "STATUS"]:
            ttk.Button(btn_frame, text=c, command=lambda cc=c: send_cmd(cc)).pack(side='left', padx=5)

        def refresh():
            with last_telemetry_lock:
                data = last_telemetry.copy()
            if data:
                gps = data.get('gps_data', {})
                alt_data = data.get('alt_data', {})
                kalman = data.get('kalman_data', {})
                vars_map['timestamp'].set(datetime.now(timezone.utc).strftime('%H:%M:%S'))
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
    logger.info("🚀 Starting N4 Base Station Server (headless; external services assumed running)")
    try:
        # Setup CSV logging
        setup_csv_logging()

        # Setup MQTT connection after services are started (unless headless and broker unavailable)
        setup_mqtt_connection()

        # Start USB monitoring
        start_usb_monitor()

        # Simulation vs real serial
        if SIMULATION_MODE:
            logger.info("SIMULATION_MODE=1: skipping serial open and starting simulator")
            start_simulation()
        else:
            open_serial()

        # Start main loop in background
        loop_thread = threading.Thread(target=main_loop, daemon=True)
        loop_thread.start()

        # Start GUI only if enabled
        if USE_GUI_CONTROL:
            start_gui()

        # Run command interface (blocking until quit)
        command_interface()

    except KeyboardInterrupt:
        logger.info("🛑 Shutting down...")
    finally:
        # Cleanup
        stop_usb_monitor()
        stop_simulation()
        close_serial()
        try:
            client.disconnect()
        except Exception:
            pass
        logger.info("👋 Shutdown complete")