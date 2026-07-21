#!/usr/bin/env python3
"""
N4 Bluetooth Setup Utility
Automatically discovers, pairs, and configures HC-05/HC-06 Bluetooth module
"""

import serial
import serial.tools.list_ports
import subprocess
import time
import re
import sys

# Configuration
BT_DEVICE_NAME = "N4_Base_BT_1"
BT_PIN = "0001"  # Default HC-05 PIN
LISTEN_TIMEOUT = 8  # seconds to listen for identification (ESP32 sends ID immediately)
MAX_RETRIES = 2  # Number of times to retry each port
RETRY_DELAY = 1  # Seconds to wait between retries

class BluetoothSetup:
    def __init__(self):
        self.target_name = BT_DEVICE_NAME
        self.pin = BT_PIN
        self.com_port = None
        self.baud_rate = None
    
    def listen_for_identification(self, port, baud=115200, timeout=15):
        """Listen on a COM port for identification packets"""
        try:
            print(f"  🎧 Listening on {port} @ {baud} baud (timeout: {timeout}s)...")
            print(f"     Opening connection...")
            ser = serial.Serial(
                port=port,
                baudrate=baud,
                timeout=1  # 1 second read timeout
            )
            print(f"     ✓ Port opened, listening for data...")
            
            start_time = time.time()
            data_received = []
            last_status = start_time
            packets_shown = 0
            MAX_PACKETS_TO_SHOW = 5
            
            while (time.time() - start_time) < timeout:
                # Show periodic status updates
                if time.time() - last_status > 2:  # Update every 2 seconds
                    elapsed = int(time.time() - start_time)
                    print(f"     ... {elapsed}s elapsed, still listening...")
                    last_status = time.time()
                
                if ser.in_waiting > 0:
                    line = ser.readline().decode('utf-8', errors='ignore').strip()
                    if line:  # Only add non-empty lines
                        data_received.append(line)
                        
                        # Show first few packets
                        if packets_shown < MAX_PACKETS_TO_SHOW:
                            print(f"     📨 Data: {line[:60]}..." if len(line) > 60 else f"     📨 Data: {line}")
                            packets_shown += 1
                        
                        # Check for identification - now embedded in telemetry
                        if "|ESP32:N4_BASE_BT_1" in line:
                            print(f"\n    ✅ FOUND! Device identifier in data:")
                            print(f"       {line[:80]}...")
                            
                            # Collect a few more packets to verify
                            telemetry_count = sum(1 for d in data_received if d.startswith("{"))
                            
                            if telemetry_count >= 3:
                                print(f"\n    ✅ Device verified!")
                                print(f"    📊 Total packets: {len(data_received)}")
                                print(f"\n    Sample telemetry:")
                                telem_samples = [d for d in data_received if d.startswith("{")][:3]
                                for i, pkt in enumerate(telem_samples):
                                    # Remove the |ESP32:N4_BASE_BT_1 suffix for display
                                    clean_pkt = pkt.split("|")[0] if "|" in pkt else pkt
                                    print(f"       [{i+1}] {clean_pkt[:70]}...")
                                ser.close()
                                return True, port
                            
                            # After collecting some packets, confirm and exit
                            if len(data_received) >= 3:
                                print(f"\n    ✓ Device confirmed (receiving telemetry)")
                                print(f"    📊 Total packets received: {len(data_received)}")
                                print(f"\n    Sample data:")
                                for i, pkt in enumerate(data_received[:3]):
                                    print(f"       [{i+1}] {pkt[:80]}...")
                                ser.close()
                                return True, port
            
            ser.close()
            
            # Timeout reached
            if data_received:
                # Check if any had our identifier
                has_id = any("|ESP32:N4_BASE_BT_1" in d for d in data_received)
                if has_id:
                    print(f"    ⚠️  Found identifier but didn't collect enough packets")
                else:
                    print(f"    ⚠️  Timeout - received data but no device identifier")
                return False, "TIMEOUT_NO_ID"
            else:
                print(f"    - No data received")
                return False, None
                
        except serial.SerialException as e:
            error_str = str(e).lower()
            if "access is denied" in error_str or "permissionerror" in error_str:
                print(f"    ✗ Port busy (close Arduino IDE Serial Monitor or other programs)")
            elif "timeout" in error_str:
                print(f"    ✗ Connection timeout (device may be disconnected or pairing incomplete)")
            else:
                print(f"    ✗ Cannot open port: {e}")
            return False, None
        except Exception as e:
            print(f"    ✗ Unexpected error: {e}")
            return False, None
    
    def test_all_ports_for_device(self):
        """Test all COM ports by listening for identification packets"""
        print("\n🔍 Testing all COM ports for device identification...")
        print(f"   Looking for ID packets from {self.target_name}")
        print(f"   Timeout per port: {LISTEN_TIMEOUT}s, Max retries: {MAX_RETRIES}\n")
        
        all_ports = list(serial.tools.list_ports.comports())
        
        if not all_ports:
            print("  ❌ No COM ports found!")
            return False
        
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
        
        if bt_ports:
            print(f"Found {len(bt_ports)} Bluetooth port(s), {len(other_ports)} other port(s)")
            print(f"Testing Bluetooth ports first...\n")
        else:
            print(f"Found {len(all_ports)} COM port(s) (no Bluetooth-labeled ports)\n")
        
        # Test each port with retries
        for i, port in enumerate(ordered_ports, 1):
            print(f"[{i}/{len(ordered_ports)}] Testing {port.device}")
            print(f"     Description: {port.description}")
            
            # Try multiple times for each port
            for attempt in range(1, MAX_RETRIES + 1):
                if attempt > 1:
                    print(f"\n  🔄 Retry attempt {attempt}/{MAX_RETRIES} for {port.device}")
                    time.sleep(RETRY_DELAY)
                
                found, data = self.listen_for_identification(port.device, timeout=LISTEN_TIMEOUT)
                
                if found:
                    print(f"\n🎯 FOUND THE DEVICE!")
                    print(f"   Port: {port.device}")
                    print(f"   Device: {self.target_name}")
                    print(f"   Detection data: {data}")
                    
                    self.com_port = port.device
                    self.baud_rate = 115200
                    return True
                
                # If we got data but wrong device, don't retry
                if data == "DATA_NO_ID":
                    print(f"     ⚠️  Port has data but not our device, skipping retries")
                    break
                
                # If permission denied or port busy, suggest action
                if data is None and attempt == MAX_RETRIES:
                    print(f"     ⚠️  Could not access port after {MAX_RETRIES} attempts")
            
            print()  # Blank line between ports
        
        print("❌ Device not found on any COM port after all retries")
        return False
    
    def check_if_paired(self):
        """Check if device is already paired"""
        print("\n🔍 Checking if device is already paired...")
        
        try:
            ps_command = f"""
            Get-PnpDevice | Where-Object {{$_.FriendlyName -like '*{self.target_name}*'}} | 
            Select-Object FriendlyName, Status, InstanceId
            """
            
            result = subprocess.run(
                ["powershell", "-Command", ps_command],
                capture_output=True,
                text=True,
                timeout=10
            )
            
            if result.returncode == 0 and self.target_name in result.stdout:
                print(f"  ✓ {self.target_name} appears to be paired")
                return True
            
        except Exception as e:
            print(f"  Could not check pairing status: {e}")
        
        return False
    
    def pair_device(self):
        """Guide user through manual pairing"""
        print(f"\n⚠️  Manual pairing required for {self.target_name}")
        print("\nPlease follow these steps:")
        print("1. Open Windows Settings > Bluetooth & devices")
        print("2. Click 'Add device' > 'Bluetooth'")
        print(f"3. Select '{self.target_name}' when it appears")
        print(f"4. Enter PIN: {self.pin} (or try 0000/1234 if {self.pin} doesn't work)")
        print("5. Wait for pairing to complete")
        
        input("\nPress ENTER when you've completed the pairing steps...")
        
        is_paired = self.check_if_paired()
        if is_paired:
            print("  ✓ Pairing confirmed!")
        else:
            print("  ⚠️  Could not confirm pairing, but will proceed...")
        
        return True
    
    def save_config(self):
        """Save COM port configuration"""
        if not self.com_port:
            return False
        
        try:
            env_file = ".env.local"
            config_line = f"N4_COM_PORT={self.com_port}\n"
            
            lines = []
            try:
                with open(env_file, 'r') as f:
                    lines = f.readlines()
            except FileNotFoundError:
                pass
            
            updated = False
            for i, line in enumerate(lines):
                if line.startswith("N4_COM_PORT="):
                    lines[i] = config_line
                    updated = True
                    break
            
            if not updated:
                lines.append(config_line)
            
            with open(env_file, 'w') as f:
                f.writelines(lines)
            
            print(f"\n✅ Configuration saved to {env_file}")
            print(f"   N4_COM_PORT={self.com_port}")
            
            return True
            
        except Exception as e:
            print(f"❌ Failed to save configuration: {e}")
            return False
    
    def run(self):
        """Main setup workflow"""
        print("=" * 60)
        print("N4 Bluetooth Setup Utility - Auto-Detection Mode")
        print("=" * 60)
        print(f"\n💡 This will listen to all COM ports for identification")
        print(f"   from your {self.target_name} device.\n")
        print("📋 Requirements:")
        print(f"   1. ESP32 with test code uploaded")
        print(f"   2. Bluetooth module powered on")
        print(f"   3. Device paired with Windows (will guide if needed)\n")
        
        # Step 1: Check if paired
        if not self.check_if_paired():
            print("\n" + "=" * 60)
            if not self.pair_device():
                return False
            time.sleep(2)
        
        # Step 2: Test all ports for identification
        print("\n" + "=" * 60)
        print("🎯 Starting automatic port detection...")
        print("=" * 60)
        
        if self.test_all_ports_for_device():
            self.save_config()
            return True
        else:
            print("\n⚠️  Automatic detection failed")
            print("\nTroubleshooting:")
            print("1. Ensure ESP32 is running the bluetooth_pairing_test.ino code")
            print("2. Check that HC-05 module is powered and connected")
            print("3. Verify device is paired in Windows Bluetooth settings")
            print("4. Check Device Manager for COM port assignment")
            return False

def main():
    """Main entry point"""
    setup = BluetoothSetup()
    
    try:
        success = setup.run()
        
        if success:
            print("\n" + "=" * 60)
            print("✅ Bluetooth Setup Complete!")
            print("=" * 60)
            print(f"\nDetected COM Port: {setup.com_port}")
            print(f"\nYou can now run the base station:")
            print(f"  python start_basestation.py")
            print(f"\nOr set COM port manually:")
            print(f"  set N4_COM_PORT={setup.com_port}")
            return 0
        else:
            print("\n" + "=" * 60)
            print("❌ Bluetooth Setup Failed")
            print("=" * 60)
            return 1
            
    except KeyboardInterrupt:
        print("\n\n⚠️  Setup cancelled by user")
        return 1
    except Exception as e:
        print(f"\n❌ Unexpected error: {e}")
        import traceback
        traceback.print_exc()
        return 1

if __name__ == "__main__":
    sys.exit(main())
