# N4 Base Station - Bluetooth Setup Guide

## Overview

This guide documents the Bluetooth integration for the N4 Base Station, including the development journey, challenges encountered, and final implementation. The system uses an HC-05 Bluetooth module to establish wireless communication between the ESP32 rocket computer and the ground station laptop.

## Hardware Requirements

- **ESP32 Development Board** - Rocket computer
- **HC-05 Bluetooth Module** - Wireless serial communication
  - Device Name: `N4_Base_BT_1`
  - PIN: `0001`
  - Baud Rate: 115200
- **Windows Laptop** - Ground station
- **Wiring:**
  - ESP32 GPIO 17 (TX) → HC-05 RX
  - ESP32 GPIO 16 (RX) → HC-05 TX
  - HC-05 VCC → 5V
  - HC-05 GND → GND

## Quick Start

### 1. Pair the Bluetooth Module

1. Power on the ESP32 with HC-05 connected
2. On Windows, go to: Settings → Bluetooth & devices → Add device
3. Look for device named **N4_Base_BT_1**
4. Enter PIN: **0001**
5. Wait for pairing to complete

### 2. Upload ESP32 Firmware

1. Open `research/bluetooth_pairing_test.ino` in Arduino IDE
2. Select your ESP32 board and COM port
3. Upload the sketch
4. **Important:** Close Arduino IDE Serial Monitor after upload

### 3. Detect and Configure COM Port

```powershell
python bluetooth_setup.py
```

The script will:
- Check if N4_Base_BT_1 is paired
- Scan all available COM ports
- Listen for the device identifier
- Save the detected port to `.env.local`

Expected output:
```
🔍 Checking if N4_Base_BT_1 is paired...
✅ Device is paired!

🔍 Scanning for N4_Base_BT_1...
📡 Testing COM5 (Standard Serial over Bluetooth link)...
✅ Received telemetry with identifier!
✅ Device verified! Found on COM5

💾 Saved configuration:
   N4_COM_PORT=COM5
```

### 4. Run the Base Station

```powershell
python server.py
```

The server will automatically use the configured COM port from `.env.local`.

---

## Development Journey: Mistakes & Solutions

### Challenge 1: AT Command Approach ❌

**Initial Approach:**
We initially tried to identify the correct COM port by sending AT commands to the HC-05 module to query its name.

**Problem:**
- Paired HC-05 modules operate in **data mode**, not **command mode**
- Data mode doesn't respond to AT commands like `AT+NAME?`
- AT commands only work when the module is unpaired or in AT configuration mode

**Lesson Learned:**
Once a Bluetooth module is paired and connected, it acts as a transparent serial bridge. You can only send/receive application data, not AT configuration commands.

---

### Challenge 2: Slow Detection (45+ seconds) ❌

**Initial Approach:**
Used 15-second timeout per port with 3 retries, waiting for ESP32 to send an identification packet.

**Problem:**
- With 10+ COM ports on Windows, scanning took 45+ seconds
- Most ports would timeout completely before finding the right one
- User experience was poor

**Solution: ✅ Optimized Timeouts**
- Reduced timeout to **8 seconds** per port
- Reduced retries to **2 attempts**
- Worst case: 16 seconds per port, but typically finds device in 2-8 seconds
- Prioritized Bluetooth-related ports first

**Code Implementation:**
```python
LISTEN_TIMEOUT = 8  # seconds
MAX_RETRIES = 2
RETRY_DELAY = 1  # second between retries
```

---

### Challenge 3: Timing Mismatch ❌

**Initial Approach:**
ESP32 sent identification packet once on startup, then waited for laptop acknowledgment.

**Problem:**
- ESP32 sent ID packet → Laptop opened COM port → ID already missed
- Python script connected *after* the packet was already transmitted
- No way to request retransmission without bidirectional communication

**Attempted Solutions (All Failed):**
1. **Continuous ID Broadcast** - ESP32 sent ID every 2 seconds
   - Still had timing issues if laptop connected between broadcasts
   
2. **ID + Telemetry Simultaneously** - Send both at same time
   - Didn't solve the fundamental timing problem

---

### Challenge 4: Bidirectional Handshake Failure ❌

**Attempted Approach:**
Implement a handshake protocol where:
1. ESP32 sends identification: `ID:ESP32:N4_BASE_BT_1`
2. Laptop responds: `HANDSHAKE:LAPTOP:N4_BASESTATION`
3. ESP32 confirms and starts telemetry

**Problem:**
```
⚠️ CRITICAL DISCOVERY: The laptop cannot reliably send data to the HC-05 module
```

**What We Tried:**
- Multiple handshake timing strategies
- ACK/NACK responses
- Different baud rates
- Various Python serial configurations

**Root Cause:**
Windows Bluetooth stack may not support reliable **laptop → HC-05** transmission for certain Bluetooth SPP configurations. Receiving data (ESP32 → Laptop) works perfectly, but sending data back fails or is unreliable.

**Lesson Learned:**
Don't assume bidirectional communication works just because one direction works. Test both directions early in development.

---

### Challenge 5: Final Solution ✅

**Implemented Approach: Embedded Identifier**

Instead of a separate handshake, we embed the device identifier directly in every telemetry packet:

**ESP32 Code:**
```cpp
// Always append device ID to every telemetry packet
json += "|ESP32:N4_BASE_BT_1";
json += "\n";
BTSerial.print(json);
```

**Example Packet:**
```json
{"record_number":123,"operation_mode":0,...,"packets_received":456}|ESP32:N4_BASE_BT_1
```

**Python Detection:**
```python
def listen_for_identification(self, port, baud=115200, timeout=8):
    identifier = f"|{self.expected_device_id}"
    
    while time.time() - start_time < timeout:
        line = ser.readline().decode('utf-8', errors='ignore').strip()
        
        if identifier in line:
            # Found the device!
            clean_data = line.replace(identifier, '')
            return True
```

**Why This Works:**
- ✅ **One-way communication only** - No laptop → ESP32 transmission needed
- ✅ **No timing issues** - Identifier in every packet, can't miss it
- ✅ **Instant detection** - First packet received confirms device
- ✅ **Simple & reliable** - No complex handshake protocol
- ✅ **Backward compatible** - Just strip the identifier for normal processing

---

## Implementation Details

### Python Scripts

#### `bluetooth_setup.py`
Automated COM port detection and configuration.

**Key Features:**
- Checks Bluetooth pairing status via PowerShell
- Scans all COM ports, prioritizing Bluetooth ports
- Listens for `|ESP32:N4_BASE_BT_1` in telemetry stream
- Verifies device by collecting 3 packets with identifier
- Saves detected port to `.env.local`

**Usage:**
```powershell
python bluetooth_setup.py
```

#### `bluetooth_monitor.py`
Continuous telemetry monitoring tool.

**Key Features:**
- Checks if device is paired
- Discovers Bluetooth COM port automatically
- Displays telemetry in real-time
- Press '2' or 'q' to stop
- Non-blocking input with threading

**Usage:**
```powershell
python bluetooth_monitor.py
```

### ESP32 Firmware

#### `research/bluetooth_pairing_test.ino`
Test firmware for Bluetooth port identification.

**Key Features:**
- Sends telemetry at 10 Hz (100ms interval)
- Appends `|ESP32:N4_BASE_BT_1` to every packet
- Simulates realistic sensor data with drift
- Supports commands: ARM, DISARM, STATUS, q (stop), 2 (stop)
- Prints all received Bluetooth data to Serial monitor
- No handshake required - one-way communication

**Configuration:**
```cpp
const char* DEVICE_ID = "ESP32:N4_BASE_BT_1";
const unsigned long TELEMETRY_INTERVAL = 100;  // 10 Hz

HardwareSerial BTSerial(2);  // UART2
#define BT_TX 17  // ESP32 TX to HC-05 RX
#define BT_RX 16  // ESP32 RX to HC-05 TX
```

---

## Telemetry Data Format

### Complete Telemetry Packet Structure

```json
{
  "record_number": 123,
  "operation_mode": 0,
  "state": 0,
  "battery_voltage": 12.6,
  "wifi_rssi": -75,
  "acc_data": {
    "ax": -0.59,
    "ay": -0.02,
    "az": 0.69,
    "pitch": -36.0,
    "roll": -2.0
  },
  "gyro_data": {
    "gx": -5.5,
    "gy": 3.0,
    "gz": 2.8
  },
  "gps_data": {
    "latitude": 0.0,
    "longitude": 0.0,
    "gps_altitude": 0.0,
    "time": 123456
  },
  "alt_data": {
    "pressure": 858.0,
    "temperature": 26.7,
    "AGL": 0.0,
    "velocity": 0.0,
    "kalman_altitude": 0.0,
    "kalman_vertical_velocity": 0.0
  },
  "chute_state": {
    "pyro1_state": 0,
    "pyro2_state": 0
  },
  "connection_status": {
    "connected": true,
    "has_ever_connected": true,
    "packet_age_ms": 0,
    "timeout_exceeded": false,
    "rssi": -75
  },
  "communication_mode": "Bluetooth",
  "timestamp": 123456,
  "packets_received": 456
}|ESP32:N4_BASE_BT_1
```

**Note:** The `|ESP32:N4_BASE_BT_1` identifier is appended after the closing brace and stripped by the Python script for clean JSON parsing.

---

## Troubleshooting

### Device Not Found

**Symptom:** Script reports "Device not found on any COM port"

**Solutions:**
1. Check Bluetooth pairing:
   ```powershell
   Get-PnpDevice | Where-Object {$_.FriendlyName -like "*N4_Base_BT_1*"}
   ```
2. Verify ESP32 is powered on and HC-05 LED is blinking
3. Close Arduino IDE Serial Monitor (locks the COM port)
4. Check HC-05 wiring:
   - VCC → 5V (not 3.3V)
   - GND → GND
   - TX → RX, RX → TX (crossover)
5. Re-upload ESP32 firmware

### Slow Detection

**Symptom:** Takes longer than 10 seconds to find device

**Solutions:**
1. Reduce number of COM ports being scanned:
   - Close other applications using serial ports
   - Unplug unnecessary USB devices
2. Check ESP32 Serial Monitor output:
   - Should show "📡 Sending telemetry at 10 Hz..."
   - Should show periodic "📊 Record #..." messages
3. Verify telemetry is being sent:
   - Open Arduino IDE Serial Monitor (115200 baud)
   - Should see status updates every few seconds

### Access Denied / Port Locked

**Symptom:** `serial.serialutil.SerialException: could not open port`

**Solutions:**
1. Close Arduino IDE Serial Monitor
2. Close any other serial terminal programs
3. Check if another Python script is using the port:
   ```powershell
   Get-Process python | Stop-Process
   ```
4. Reboot the ESP32
5. Restart Bluetooth service:
   ```powershell
   Restart-Service bthserv
   ```

### No Data Received

**Symptom:** COM port detected but no telemetry displayed

**Solutions:**
1. Check baud rate matches (115200 on both sides)
2. Verify ESP32 is running:
   - LED should be on
   - Arduino Serial Monitor should show telemetry
3. Check Bluetooth connection status:
   - HC-05 LED should be solid (not blinking) when connected
4. Try re-pairing the Bluetooth device
5. Check `.env.local` has correct port:
   ```
   N4_COM_PORT=COM5
   ```

### Laptop Can't Send Commands to ESP32

**Symptom:** Typing 'q' or sending ARM command doesn't work

**Known Issue:**
This is expected behavior. Our testing revealed that Windows → HC-05 data transmission is unreliable. The current implementation is **one-way communication only** (ESP32 → Laptop).

**Workaround:**
- Use Arduino IDE Serial Monitor to send commands directly via USB
- Or implement WiFi-based command channel for bidirectional control

---

## Configuration Files

### `.env.local`
Created automatically by `bluetooth_setup.py`:

```env
N4_COM_PORT=COM5
```

This file is read by:
- `server.py` - Main base station server
- `bluetooth_monitor.py` - Monitoring tool

### Bluetooth Configuration
- **Device Name:** N4_Base_BT_1
- **PIN:** 0001
- **Baud Rate:** 115200
- **Data Bits:** 8
- **Parity:** None
- **Stop Bits:** 1
- **Flow Control:** None

---

## Best Practices

### 1. Always Close Serial Monitor
Before running Python scripts, close Arduino IDE Serial Monitor to release the COM port.

### 2. Verify Pairing First
Check pairing status before attempting connection:
```powershell
Get-PnpDevice | Where-Object {$_.FriendlyName -like "*Bluetooth*"}
```

### 3. Use .env.local
Don't hardcode COM ports. Always use the `.env.local` configuration file.

### 4. Test Incrementally
1. First, verify ESP32 sends data (Arduino Serial Monitor)
2. Then, test Bluetooth pairing (Windows settings)
3. Finally, run Python detection script

### 5. Monitor ESP32 Output
Keep Arduino Serial Monitor open on a second computer or use USB serial while testing Bluetooth. This helps debug issues.

---

## Key Lessons for Future Projects

### ✅ DO:
- Test both directions of communication early
- Use embedded identifiers for simple device detection
- Optimize timeouts for user experience
- Prioritize one-way communication if bidirectional isn't essential
- Document failures and solutions

### ❌ DON'T:
- Assume AT commands work in data mode
- Use long timeouts without testing user experience
- Implement complex handshakes without verifying basic send/receive first
- Trust that bidirectional serial works without testing both directions
- Give up after first approach fails - iterate!

---

## Technical References

### PowerShell Commands
```powershell
# List all Bluetooth devices
Get-PnpDevice | Where-Object {$_.Class -eq "Bluetooth"}

# Check specific device status
Get-PnpDevice | Where-Object {$_.FriendlyName -like "*N4_Base_BT_1*"}

# Restart Bluetooth service
Restart-Service bthserv

# List all COM ports
Get-WmiObject Win32_SerialPort | Select-Object Name, DeviceID
```

### Python Serial Configuration
```python
import serial

ser = serial.Serial(
    port='COM5',
    baudrate=115200,
    bytesize=serial.EIGHTBITS,
    parity=serial.PARITY_NONE,
    stopbits=serial.STOPBITS_ONE,
    timeout=1
)
```

### ESP32 Serial Configuration
```cpp
HardwareSerial BTSerial(2);
BTSerial.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN);
```

---

## Future Improvements

### Potential Enhancements:
1. **Bidirectional Communication via WiFi** - Add separate WiFi channel for commands
2. **Multiple Device Support** - Detect and manage multiple rockets
3. **Auto-reconnection** - Automatically reconnect if Bluetooth drops
4. **Signal Strength Monitoring** - Track RSSI to predict connection loss
5. **Telemetry Buffering** - Cache data if connection drops temporarily
6. **GUI Configuration Tool** - Visual interface for Bluetooth setup

### Known Limitations:
- One-way communication only (ESP32 → Laptop)
- Windows Bluetooth stack limitations
- Bluetooth range (~10-30 meters depending on environment)
- No encryption on Bluetooth link (use at launch site only)

---

## Support & Contact

For issues or questions:
1. Check this documentation first
2. Review ESP32 Serial Monitor output
3. Test with Arduino Serial Monitor directly
4. Check Windows Bluetooth device status

---

## Changelog

### Version 1.0 (Current)
- ✅ One-way Bluetooth communication
- ✅ Embedded identifier in telemetry packets
- ✅ Automated COM port detection
- ✅ 8-second timeout per port
- ✅ PowerShell pairing status checks
- ✅ Configuration saved to .env.local
- ❌ No laptop → ESP32 communication (Windows limitation)

---

**Last Updated:** January 12, 2026  
**Authors:** N4 Recovery Team  
**Device:** N4_Base_BT_1 (HC-05)  
**Status:** Production Ready ✅
