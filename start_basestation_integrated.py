#!/usr/bin/env python3
"""
N4 Base Station Integrated Startup Script
Combines Bluetooth setup, USB detection, and service management.

Features:
- Automatic Bluetooth COM port detection
- USB serial reconnection
- All services orchestration (MQTT, Vite, TileServer, Node API)
- Python telemetry server with simulation support

Usage:
    python start_basestation_integrated.py [--simulation] [--skip-bluetooth]
    
    --simulation: Start in simulation mode (no real hardware required)
    --skip-bluetooth: Skip Bluetooth detection (useful for USB-only mode)
"""

import subprocess
import sys
import os
import time
import shutil
import argparse
import serial
import serial.tools.list_ports

PROC_GROUP = []

PORTS = {
    "tiles": 8080,
    "vite": 5173,
    "mqtt": 1883,
    "api": 3000,
}

# Bluetooth Configuration
BT_DEVICE_NAME = "N4_Base_BT_1"
LISTEN_TIMEOUT = 8  # seconds
MAX_RETRIES = 2

# MBTiles Configuration
MBTILES_FILENAME = "osm-2020-02-10-v3.11_africa_kenya.mbtiles"

def print_banner():
    """Print startup banner"""
    print("\n" + "="*70)
    print("  N4 BASE STATION - INTEGRATED STARTUP")
    print("="*70)
    print("  🚀 Rocket Recovery Team - Basestation Software")
    print("  📡 Bluetooth + USB + Web Services")
    print("="*70 + "\n")

def kill_on_port(port: int):
    """Kill any processes listening on the given TCP port (Windows)."""
    try:
        cmd = f"netstat -ano | findstr :{port}"
        res = subprocess.run(cmd, shell=True, capture_output=True, text=True)
        if res.returncode == 0 and res.stdout:
            lines = [ln.strip() for ln in res.stdout.strip().splitlines() if ln.strip()]
            pids = set()
            for ln in lines:
                parts = ln.split()
                if len(parts) >= 5 and 'LISTENING' in ln:
                    pid = parts[-1]
                    if pid.isdigit():
                        pids.add(pid)
            for pid in pids:
                try:
                    print(f"  🔪 Killing PID {pid} on port {port}…")
                    subprocess.run(f"taskkill /PID {pid} /F", shell=True, capture_output=True)
                except Exception:
                    pass
    except Exception:
        pass

def _spawn(name, cmd, cwd=None):
    """Spawn a child process and store it for cleanup."""
    print(f"  ▶ Starting {name}...")
    p = subprocess.Popen(
        cmd,
        shell=True,
        cwd=cwd or os.getcwd(),
        stdout=None,
        stderr=None,
        creationflags=subprocess.CREATE_NEW_PROCESS_GROUP,
    )
    PROC_GROUP.append((name, p))
    return p

def _cleanup():
    """Terminate all spawned processes."""
    print("\n🧹 Cleaning up child processes...")
    for name, p in PROC_GROUP:
        try:
            if p.poll() is None:
                print(f"  ⏹ Stopping {name}...")
                p.terminate()
                try:
                    p.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    print(f"  ⚠ Forcing kill of {name}...")
                    p.kill()
        except Exception:
            pass

def _ensure_npm_deps():
    """Install npm dependencies if missing."""
    try:
        need_install = False
        nm = os.path.join(os.getcwd(), 'node_modules')
        sqljs = os.path.join(nm, 'sql.js')
        if not os.path.isdir(nm) or not os.path.isdir(sqljs):
            need_install = True
        if need_install:
            print("  📦 Installing npm dependencies (one-time)...")
            lockfile = os.path.join(os.getcwd(), 'package-lock.json')
            if os.path.exists(lockfile):
                subprocess.run(["cmd", "/c", "npm", "install"], check=False)
            else:
                subprocess.run(["cmd", "/c", "npm", "install", "--no-fund", "--no-audit"], check=False)
        else:
            print("  ✅ npm dependencies present")
    except Exception:
        pass

def check_bluetooth_pairing():
    """Check if Bluetooth device is paired"""
    try:
        print(f"  🔍 Checking Bluetooth pairing for {BT_DEVICE_NAME}...")
        result = subprocess.run(
            ["powershell", "-Command", 
             "Get-PnpDevice -Class Bluetooth -FriendlyName '*' | Select-Object FriendlyName, Status | Format-List"],
            capture_output=True,
            text=True,
            timeout=10
        )
        
        if BT_DEVICE_NAME in result.stdout:
            print(f"  ✅ {BT_DEVICE_NAME} is paired")
            return True
        else:
            print(f"  ⚠️  {BT_DEVICE_NAME} not found in paired devices")
            print("\n  To pair the device:")
            print("  1. Open Settings > Bluetooth & devices")
            print("  2. Click 'Add device'")
            print(f"  3. Select '{BT_DEVICE_NAME}'")
            print("  4. Use PIN: 0001 if prompted\n")
            return False
    except Exception as e:
        print(f"  ⚠️  Could not check Bluetooth pairing: {e}")
        return False

def listen_for_identification(port, timeout=8):
    """Listen on a COM port for device identification"""
    try:
        print(f"    🎧 Listening on {port} (timeout: {timeout}s)...")
        ser = serial.Serial(
            port=port,
            baudrate=460800,  # High-speed Bluetooth (matches ESP32 HC-05/HC-06 config)
            timeout=1
        )
        
        start_time = time.time()
        data_received = []
        
        while (time.time() - start_time) < timeout:
            if ser.in_waiting > 0:
                line = ser.readline().decode('utf-8', errors='ignore').strip()
                if line:
                    data_received.append(line)
                    
                    # Check for device identifier embedded in telemetry
                    if "|ESP32:N4_BASE_BT_1" in line:
                        print(f"    ✅ FOUND! Device identifier detected")
                        
                        # Verify with a few more packets
                        telemetry_count = sum(1 for d in data_received if d.startswith("{"))
                        if telemetry_count >= 2:
                            print(f"    ✅ Device verified! ({telemetry_count} packets)")
                            ser.close()
                            return True
        
        ser.close()
        if data_received:
            has_id = any("|ESP32:N4_BASE_BT_1" in d for d in data_received)
            if has_id:
                print(f"    ✅ Found device identifier!")
                return True
            else:
                print(f"    ⚠️  Data received but no device identifier")
        else:
            print(f"    ⚠️  No data received")
        return False
                
    except serial.SerialException as e:
        error_str = str(e).lower()
        if "access is denied" in error_str:
            print(f"    ✗ Port busy (close other programs)")
        else:
            print(f"    ✗ Cannot open: {e}")
        return False
    except Exception as e:
        print(f"    ✗ Error: {e}")
        return False

def discover_bluetooth_port():
    """Discover Bluetooth COM port by testing all ports"""
    print("\n📡 BLUETOOTH SETUP")
    print("="*70)
    
    if not check_bluetooth_pairing():
        return None
    
    print(f"\n  🔍 Scanning COM ports for {BT_DEVICE_NAME}...")
    
    all_ports = list(serial.tools.list_ports.comports())
    if not all_ports:
        print("  ❌ No COM ports found!")
        return None
    
    # Prioritize Bluetooth ports
    bt_ports = []
    other_ports = []
    for port in all_ports:
        desc = (port.description or "").upper()
        if "BLUETOOTH" in desc or "BT" in desc:
            bt_ports.append(port)
        else:
            other_ports.append(port)
    
    ordered_ports = bt_ports + other_ports
    
    print(f"  Found {len(bt_ports)} Bluetooth port(s), {len(other_ports)} other port(s)\n")
    
    # Test each port
    for port_info in ordered_ports:
        port = port_info.device
        desc = port_info.description or "Unknown"
        
        print(f"\n  Testing {port} ({desc})...")
        
        for attempt in range(MAX_RETRIES):
            if attempt > 0:
                print(f"    Retry {attempt + 1}/{MAX_RETRIES}...")
            
            if listen_for_identification(port, LISTEN_TIMEOUT):
                print(f"\n  🎉 SUCCESS! Found device on {port}")
                save_bluetooth_config(port)
                return port
            
            if attempt < MAX_RETRIES - 1:
                time.sleep(1)
    
    print("\n  ❌ Device not found on any COM port")
    print("\n  Troubleshooting:")
    print("  - Ensure ESP32 is powered on")
    print("  - Check Bluetooth pairing")
    print("  - Upload bluetooth_pairing_test.ino to ESP32")
    print("  - Close Arduino IDE Serial Monitor\n")
    
    return None

def save_bluetooth_config(com_port):
    """Save Bluetooth COM port to .env.local"""
    try:
        env_file = os.path.join(os.getcwd(), '.env.local')
        
        # Read existing content
        existing_lines = []
        if os.path.exists(env_file):
            with open(env_file, 'r') as f:
                existing_lines = [line for line in f.readlines() if not line.startswith('N4_COM_PORT=')]
        
        # Write with new port
        with open(env_file, 'w') as f:
            f.writelines(existing_lines)
            f.write(f'N4_COM_PORT={com_port}\n')
        
        print(f"  💾 Saved configuration to .env.local")
        print(f"     N4_COM_PORT={com_port}")
    except Exception as e:
        print(f"  ⚠️  Could not save config: {e}")

def load_bluetooth_config():
    """Load Bluetooth COM port from .env.local"""
    try:
        env_file = os.path.join(os.getcwd(), '.env.local')
        if os.path.exists(env_file):
            with open(env_file, 'r') as f:
                for line in f:
                    if line.startswith('N4_COM_PORT='):
                        port = line.strip().split('=')[1]
                        print(f"  📂 Loaded from config: {port}")
                        return port
    except Exception:
        pass
    return None

def verify_mbtiles():
    """Verify MBTiles file exists"""
    mbtiles_path = os.path.join(os.getcwd(), MBTILES_FILENAME)
    print(f"\n  🗺️  Checking for map tiles: {MBTILES_FILENAME}")
    
    if os.path.exists(mbtiles_path):
        print(f"  ✅ Map tiles found")
        return mbtiles_path
    else:
        print(f"  ❌ Map tiles NOT FOUND at {mbtiles_path}")
        print("     TileServer will not start without this file")
        return None

def start_services(bluetooth_port=None, simulation_mode=False):
    """Start all base station services"""
    print("\n🚀 STARTING SERVICES")
    print("="*70)
    
    # Ensure ports are free
    print("\n  🔧 Freeing required ports...")
    for label, prt in PORTS.items():
        kill_on_port(prt)
    time.sleep(1)
    
    # Ensure npm dependencies
    print("\n  📦 Checking dependencies...")
    _ensure_npm_deps()
    
    print("\n  🔄 Launching services...\n")
    
    # 1) Mosquitto MQTT Broker
    _spawn("mosquitto", ["cmd", "/c", "mosquitto", "-c", "mosquitto.conf"])
    
    # 2) TileServer-GL
    config_path = os.path.join(os.getcwd(), "config.json")
    if shutil.which("tileserver-gl"):
        _spawn("tileserver-gl", ["cmd", "/c", "tileserver-gl", config_path])
    else:
        _spawn("tileserver-gl(npx)", ["cmd", "/c", "npx", "--yes", "tileserver-gl", config_path])
    
    # 3) Vite Frontend
    _spawn("vite", ["cmd", "/c", "npm", "run", "dev:client"])
    
    # 4) Node.js API Server
    _spawn("node api", ["cmd", "/c", "node", "server.js"])
    
    # 5) Python Telemetry Server
    print("  ▶ Starting telemetry server...")
    start_telemetry_server(bluetooth_port, simulation_mode)

def start_telemetry_server(bluetooth_port, simulation_mode):
    """Start the integrated Python telemetry server"""
    import threading
    
    # Set environment variable for Bluetooth port
    if bluetooth_port:
        os.environ['N4_COM_PORT'] = bluetooth_port
        print(f"     Bluetooth port: {bluetooth_port}")
    
    # Set communication method preference
    if hasattr(start_telemetry_server, 'force_usb'):
        os.environ['N4_FORCE_USB'] = '1'
        print(f"     Mode: FORCED USB Serial")
    elif hasattr(start_telemetry_server, 'force_bluetooth'):
        os.environ['N4_FORCE_BT'] = '1'
        print(f"     Mode: FORCED Bluetooth")
    
    if simulation_mode:
        os.environ['SIMULATION_MODE'] = '1'
        print(f"     Mode: SIMULATION")
    else:
        print(f"     Mode: LIVE")
    
    try:
        from research.scripts.server import (
            setup_csv_logging, setup_mqtt_connection, start_usb_monitor,
            open_serial, start_simulation, main_loop,
            SIMULATION_MODE as server_sim_mode
        )
        
        # Setup
        setup_csv_logging()
        setup_mqtt_connection()
        start_usb_monitor()
        
        # Start serial or simulation
        if simulation_mode or server_sim_mode:
            print("     Starting data simulator...")
            start_simulation()
        else:
            print("     Opening serial connection...")
            open_serial()
        
        # Start main loop in background thread
        loop_thread = threading.Thread(target=main_loop, daemon=True)
        loop_thread.start()
        
        print("  ✅ Telemetry server running")
        
    except ImportError as e:
        print(f"  ❌ Could not import telemetry server: {e}")
        print("     Make sure research/server.py exists")
    except Exception as e:
        print(f"  ❌ Error starting telemetry server: {e}")

def main():
    """Main entry point"""
    parser = argparse.ArgumentParser(description='N4 Base Station Integrated Startup')
    parser.add_argument('--simulation', action='store_true', 
                       help='Start in simulation mode (no real hardware)')
    parser.add_argument('--skip-bluetooth', action='store_true',
                       help='Skip Bluetooth detection')
    parser.add_argument('--force-usb', action='store_true',
                       help='Force USB Serial communication only (disable Bluetooth)')
    parser.add_argument('--force-bluetooth', action='store_true',
                       help='Force Bluetooth communication only (disable USB Serial)')
    args = parser.parse_args()
    
    print_banner()
    
    try:
        # Change to script directory
        os.chdir(os.path.dirname(os.path.abspath(__file__)))
        
        # Verify map tiles
        verify_mbtiles()
        
        # Set communication method flags
        if args.force_usb:
            start_telemetry_server.force_usb = True
            print("\n  🔌 Communication Mode: FORCED USB Serial (Bluetooth disabled)")
        elif args.force_bluetooth:
            start_telemetry_server.force_bluetooth = True
            print("\n  📡 Communication Mode: FORCED Bluetooth (USB disabled)")
        else:
            print("\n  🔄 Communication Mode: Auto-detect (Bluetooth preferred)")
        
        # Bluetooth setup (unless skipped, forced USB, or in simulation)
        bluetooth_port = None
        if not args.skip_bluetooth and not args.simulation and not args.force_usb:
            # Try to load from config first
            bluetooth_port = load_bluetooth_config()
            
            if bluetooth_port:
                print(f"\n  🔄 Verifying saved port {bluetooth_port}...")
                if listen_for_identification(bluetooth_port, timeout=5):
                    print(f"  ✅ Port verified and working!")
                else:
                    print(f"  ⚠️  Saved port not responding, re-scanning...")
                    bluetooth_port = discover_bluetooth_port()
            else:
                # No saved config, discover
                bluetooth_port = discover_bluetooth_port()
            
            if not bluetooth_port:
                print("\n  ⚠️  Continuing without Bluetooth (USB mode only)")
        elif args.skip_bluetooth or args.force_usb:
            print("\n  ⏭️  Skipping Bluetooth setup (USB mode)")
        else:
            print("\n  🎮 Simulation mode - no Bluetooth needed")
        
        # Start all services
        start_services(bluetooth_port, args.simulation)
        
        print("\n" + "="*70)
        print("  ✅ BASE STATION RUNNING")
        print("="*70)
        print("\n  Services:")
        print("  - Web UI:     http://localhost:5173")
        print("  - API:        http://localhost:3000")
        print("  - Map Tiles:  http://localhost:8080")
        print("  - MQTT:       mqtt://localhost:1883")
        if bluetooth_port:
            print(f"  - Bluetooth:  {bluetooth_port}")
        print("\n  Press Ctrl+C to stop all services\n")
        print("="*70 + "\n")
        
        # Keep main process alive
        while True:
            time.sleep(1)
    
    except KeyboardInterrupt:
        print("\n\n🛑 Shutting down base station...")
    except Exception as e:
        print(f"\n❌ Error: {e}")
        import traceback
        traceback.print_exc()
    finally:
        _cleanup()
        print("\n👋 Base station stopped.\n")

if __name__ == "__main__":
    main()
