#!/usr/bin/env python3
"""
N4 Bluetooth Monitor - Continuous data receiver
Checks pairing, discovers COM port, sends handshake, and continuously prints telemetry until '2' is pressed
"""

import serial
import serial.tools.list_ports
import time
import sys
import threading
import subprocess
from pathlib import Path

# Configuration
BT_DEVICE_NAME = "N4_Base_BT_1"
LAPTOP_ID = "LAPTOP:N4_BASESTATION"
BAUD_RATE = 115200
HANDSHAKE_TIMEOUT = 5  # seconds to wait for handshake response

class BluetoothMonitor:
    def __init__(self):
        self.com_port = None
        self.running = True
        self.handshake_complete = False
        
    def check_if_paired(self):
        """Check if Bluetooth device is already paired using PowerShell"""
        try:
            print("🔍 Checking if device is already paired...")
            cmd = f'Get-PnpDevice | Where-Object {{$_.FriendlyName -like "*{BT_DEVICE_NAME}*"}}'
            result = subprocess.run(
                ["powershell", "-Command", cmd],
                capture_output=True,
                text=True,
                timeout=10
            )
            
            if BT_DEVICE_NAME in result.stdout:
                print(f"  ✓ {BT_DEVICE_NAME} appears to be paired\n")
                return True
            else:
                print(f"  ✗ {BT_DEVICE_NAME} not found in paired devices\n")
                return False
                
        except Exception as e:
            print(f"  ⚠️  Could not check pairing status: {e}")
            return None
    
    def prompt_for_pairing(self):
        """Guide user through manual pairing"""
        print("=" * 60)
        print("⚠️  BLUETOOTH PAIRING REQUIRED")
        print("=" * 60)
        print(f"\nDevice to pair: {BT_DEVICE_NAME}")
        print("\nSteps to pair:")
        print("  1. Go to Settings → Bluetooth & devices")
        print("  2. Click 'Add device' → 'Bluetooth'")
        print(f"  3. Select '{BT_DEVICE_NAME}' from the list")
        print("  4. Enter PIN: 0001 (if prompted)")
        print("  5. Wait for 'Connected' status")
        print("\n" + "=" * 60)
        input("\nPress ENTER when pairing is complete...")
        print()
    
    def discover_bluetooth_port(self):
        """Search Bluetooth COM ports and attempt handshake"""
        print("=" * 60)
        print("🎯 Discovering Bluetooth COM port...")
        print("=" * 60)
        
        # Get all COM ports
        ports = list(serial.tools.list_ports.comports())
        
        # Separate Bluetooth ports from others
        bt_ports = []
        other_ports = []
        
        for port in ports:
            desc_lower = port.description.lower()
            if 'bluetooth' in desc_lower or 'bt' in desc_lower:
                bt_ports.append(port)
            else:
                other_ports.append(port)
        
        print(f"\nFound {len(bt_ports)} Bluetooth port(s), {len(other_ports)} other port(s)")
        
        if not bt_ports:
            print("❌ No Bluetooth COM ports found!")
            print("   Make sure the device is paired and connected.")
            return None
        
        print("Testing Bluetooth ports...\n")
        
        # Try each Bluetooth port
        for i, port in enumerate(bt_ports):
            print(f"[{i+1}/{len(bt_ports)}] Testing {port.device}")
            print(f"     Description: {port.description}")
            
            result = self.test_port_handshake(port.device)
            if result:
                print(f"\n✅ Found device on {port.device}!\n")
                return port.device
            
            print()
        
        print("❌ Device not found on any Bluetooth port")
        print("   Make sure:")
        print("   - ESP32 is powered on")
        print("   - HC-05 module is connected")
        print("   - Bluetooth device is connected (not just paired)")
        return None
    
    def test_port_handshake(self, port):
        """Test a single port by attempting handshake"""
        ser = None
        try:
            print(f"  🎧 Opening {port}...")
            ser = serial.Serial(
                port=port,
                baudrate=BAUD_RATE,
                timeout=0.5  # Short timeout for non-blocking reads
            )
            print(f"     ✓ Port opened")
            
            # Wait briefly for any initial data
            time.sleep(0.3)
            
            # Clear buffer and check for ID
            id_found = False
            while ser.in_waiting > 0:
                line = ser.readline().decode('utf-8', errors='ignore').strip()
                if line.startswith("ID:"):
                    print(f"     📨 Received ID: {line[:50]}...")
                    id_found = True
            
            # Send handshake
            print(f"     📤 Sending handshake...")
            ser.write(f"{LAPTOP_ID}\n".encode('utf-8'))
            ser.flush()
            
            # Wait for ACK with timeout (max 3 seconds)
            print(f"     ⏳ Waiting for ACK (timeout: 3s)...")
            start_time = time.time()
            response_count = 0
            
            while time.time() - start_time < 3.0:
                if ser.in_waiting > 0:
                    line = ser.readline().decode('utf-8', errors='ignore').strip()
                    if line:  # Only print non-empty lines
                        response_count += 1
                        print(f"     📨 Response #{response_count}: {line[:60]}...")
                        
                        if "ACK:HANDSHAKE_OK" in line:
                            print(f"     ✅ Handshake successful!")
                            ser.close()
                            return True
                else:
                    # No data available, small sleep to prevent CPU spinning
                    time.sleep(0.05)
            
            print(f"     ✗ Timeout - No ACK received ({response_count} responses total)")
            ser.close()
            return False
            
        except serial.SerialException as e:
            print(f"     ✗ Cannot open port: {e}")
            if ser:
                try:
                    ser.close()
                except:
                    pass
            return False
        except Exception as e:
            print(f"     ✗ Error: {e}")
            if ser:
                try:
                    ser.close()
                except:
                    pass
            return False
    
    def send_handshake(self, ser):
        """Send handshake to ESP32 (initial connection already established)"""
        print(f"\n📤 Re-establishing handshake...")
        ser.write(f"{LAPTOP_ID}\n".encode('utf-8'))
        ser.flush()
        time.sleep(0.3)
    
    def listen_for_stop(self, ser):
        """Thread to listen for '2' keypress"""
        print("\n💡 Press '2' and Enter to stop transmission\n")
        while self.running:
            try:
                user_input = input()
                if user_input.strip() == "2":
                    print("\n🛑 Sending stop command...")
                    ser.write(b"2\n")
                    ser.flush()
                    self.running = False
                    break
            except:
                break
    
    def monitor(self):
        """Main monitoring loop"""
        print("=" * 60)
        print("N4 Bluetooth Monitor")
        print("=" * 60)
        print()
        
        # Step 1: Check if device is paired
        is_paired = self.check_if_paired()
        
        if is_paired == False:
            # Device not paired, guide user
            self.prompt_for_pairing()
            
            # Recheck after pairing
            is_paired = self.check_if_paired()
            if not is_paired:
                print("❌ Device still not paired. Please pair manually and try again.")
                return False
        
        # Step 2: Discover COM port by testing handshake
        self.com_port = self.discover_bluetooth_port()
        
        if not self.com_port:
            print("\n❌ Could not find Bluetooth device")
            return False
        
        # Step 3: Connect and start monitoring
        print("=" * 60)
        print("📡 STARTING CONTINUOUS MONITORING")
        print("=" * 60)
        print(f"COM Port: {self.com_port}")
        print(f"Baud Rate: {BAUD_RATE}")
        print("=" * 60)
        
        try:
            # Open serial connection
            print(f"\n🔌 Connecting to {self.com_port}...")
            ser = serial.Serial(
                port=self.com_port,
                baudrate=BAUD_RATE,
                timeout=1
            )
            print("✅ Connected!")
            
            # Send handshake again (port discovery already did this, but ESP32 needs it to start telemetry)
            self.send_handshake(ser)
            
            # Start stop listener thread
            stop_thread = threading.Thread(target=self.listen_for_stop, args=(ser,), daemon=True)
            stop_thread.start()
            
            # Main data reception loop
            print("\n" + "=" * 60)
            print("📡 RECEIVING TELEMETRY DATA")
            print("=" * 60 + "\n")
            
            packet_count = 0
            start_time = time.time()
            
            while self.running:
                if ser.in_waiting > 0:
                    line = ser.readline().decode('utf-8', errors='ignore').strip()
                    
                    if line:
                        packet_count += 1
                        
                        # Check for acknowledgments
                        if line.startswith("ACK:"):
                            print(f"\n✅ {line}\n")
                            continue
                        
                        # Print telemetry data
                        if line.startswith("{"):
                            # Parse and display JSON
                            try:
                                import json
                                data = json.loads(line)
                                
                                # Display compact summary every packet
                                print(f"[{packet_count:04d}] Record #{data.get('record_number', 'N/A'):04d} | "
                                      f"Alt: {data.get('alt_data', {}).get('AGL', 0):6.2f}m | "
                                      f"Vel: {data.get('alt_data', {}).get('kalman_vertical_velocity', 0):6.2f}m/s | "
                                      f"Mode: {'ARMED' if data.get('operation_mode') == 1 else 'SAFE':>5} | "
                                      f"Bat: {data.get('battery_voltage', 0):4.1f}V")
                                
                                # Show full JSON every 10 packets
                                if packet_count % 10 == 0:
                                    print(f"\n--- Full Packet #{packet_count} ---")
                                    print(json.dumps(data, indent=2))
                                    print("-" * 60 + "\n")
                                    
                            except json.JSONDecodeError:
                                print(f"[{packet_count:04d}] {line[:100]}...")
                        else:
                            # Non-JSON data
                            print(f"[{packet_count:04d}] {line}")
                
                time.sleep(0.01)  # Small delay to prevent CPU spinning
            
            # Cleanup
            elapsed = time.time() - start_time
            print("\n" + "=" * 60)
            print("📊 SESSION SUMMARY")
            print("=" * 60)
            print(f"Total packets received: {packet_count}")
            print(f"Session duration: {elapsed:.1f} seconds")
            print(f"Average rate: {packet_count/elapsed:.1f} packets/sec")
            print("=" * 60)
            
            ser.close()
            print("\n✅ Connection closed")
            return True
            
        except serial.SerialException as e:
            print(f"\n❌ Serial error: {e}")
            return False
        except KeyboardInterrupt:
            print("\n\n⚠️  Interrupted by user (Ctrl+C)")
            if 'ser' in locals():
                ser.close()
            return False
        except Exception as e:
            print(f"\n❌ Error: {e}")
            import traceback
            traceback.print_exc()
            return False

def main():
    """Main entry point"""
    monitor = BluetoothMonitor()
    success = monitor.monitor()
    
    sys.exit(0 if success else 1)

if __name__ == "__main__":
    main()
