# Unified Packet Structure - All Communication Modes

## Overview
All communication modes (XBee, Beacon, MQTT/WiFi) now use **identical JSON packet structure**. The only difference is the `communication_mode` field.

## Packet Format
```json
{
  "record_number": 5452,
  "operation_mode": 0,
  "state": 0,
  "battery_voltage": 12.5,
  "wifi_rssi": -40,
  "acc_data": {
    "ax": -0.03,
    "ay": 0.11,
    "az": 0.99,
    "pitch": -2.21,
    "roll": 6.35
  },
  "gyro_data": {
    "gx": -8.99,
    "gy": -3.96,
    "gz": -3.84
  },
  "gps_data": {
    "latitude": -1.2921,
    "longitude": 36.8219,
    "gps_altitude": 1650.5,
    "time": 470741
  },
  "alt_data": {
    "pressure": 852.73,
    "temperature": 26.91,
    "AGL": -1.93,
    "velocity": 0,
    "kalman_altitude": -1.93,
    "kalman_vertical_velocity": 1.7
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
    "rssi": -40
  },
  "communication_mode": "XBee",    ← ONLY DIFFERENCE
  "timestamp": 507189,
  "packets_received": 169           ← SINGLE UNIFIED COUNTER
}|ESP32:N4_BASE_BT_1
```

## Communication Modes

### 1. XBee Mode
- **CSV Source:** XBee UART2 (900MHz radio)
- **RSSI:** Read from XBee PWM pin (GPIO 35)
- **Packet Field:** `"communication_mode": "XBee"`
- **Debug Print:** `[XBEE TX] Packet #169 | Rec#5452 | Alt=-1.9m | Vel=0.0m/s`

### 2. Beacon Mode
- **CSV Source:** ESP-NOW beacon frames (promiscuous WiFi)
- **RSSI:** From WiFi packet header
- **Packet Field:** `"communication_mode": "Beacon"`
- **Debug Print:** `[BEACON TX] Packet #169 | Rec#5452 | Alt=-1.9m | Vel=0.0m/s`

### 3. MQTT/WiFi Mode
- **CSV Source:** MQTT topic subscription (future implementation)
- **RSSI:** WiFi connection RSSI
- **Packet Field:** `"communication_mode": "MQTT"`
- **Debug Print:** `[WIFI TX] Packet #169 | Rec#5452 | Alt=-1.9m | Vel=0.0m/s`

### 4. Auto Mode
- **Behavior:** Automatically switches between modes based on first received packet
- **Packet Field:** `"communication_mode": "Auto"` (before first packet)
- **After Detection:** Changes to "XBee", "Beacon", or "MQTT" dynamically

## Transmission Channels

### USB Serial (Serial)
- **Baud Rate:** 115200
- **Purpose:** PC monitoring, debugging
- **Content:** 
  - Telemetry JSON (all modes)
  - LOG messages
  - Debug prints (`[XBEE TX]`, `[BEACON TX]`, `[WIFI TX]`)
  - STATUS heartbeat (every 10 seconds)

### Bluetooth SPP (BTSerial)
- **UART:** UART1 (GPIO 16 RX, GPIO 17 TX)
- **Baud Rate:** 115200
- **Purpose:** Wireless monitoring, Python server connection
- **Content:**
  - Telemetry JSON (all modes) - **IDENTICAL to USB Serial**
  - LOG messages
  - Heartbeat (500ms when no telemetry)
  - ❌ NO debug prints (`[XBEE TX]` etc.) - only on USB Serial

### XBee Radio (XBeeSerial)
- **UART:** UART2 (GPIO 34 RX, GPIO 32 TX)
- **Baud Rate:** 115200
- **Purpose:** Receive rocket telemetry (900MHz long-range)
- **Content:** Incoming 25-field CSV from rocket

## Packet Counter Unification

### Before (Confusing)
```json
{
  "packets_received": 169,         // Beacon counter
  "xbee_packets_received": 2506    // XBee counter
}
```

### After (Unified)
```json
{
  "packets_received": 2675,  // SINGLE counter for ALL modes
  "communication_mode": "XBee"  // Server knows mode from this field
}
```

### Counter Behavior
- **Increments:** Every packet received (XBee, Beacon, or MQTT)
- **Reset:** On ESP32 reboot only
- **Usage:** Server tracks total packets, mode identified by `communication_mode` field

## CSV Input Format (25 Fields)
All modes receive **identical CSV format** from rocket:
```
record,op_mode,state,ax,ay,az,pitch,roll,gx,gy,gz,lat,lon,
gps_alt,gps_time,pressure,temp,agl,velocity,drogue,main,
battery,rssi,kalman_alt,kalman_vel
```

**Size:** ~250 bytes per packet

## Device Identification

### Heartbeat (When No Telemetry)
```json
{
  "type": "heartbeat",
  "uptime": 1234,
  "device_id": "ESP32:N4_BASE_BT_1",
  "xbee_enabled": true,
  "waiting_for_data": true
}|ESP32:N4_BASE_BT_1
```

**Frequency:** 500ms interval (fast COM port detection)

### Startup Behavior
1. Sends **3 rapid heartbeats** (100ms apart) on boot
2. Continues heartbeat every 500ms until first telemetry packet
3. Once telemetry starts, heartbeat stops (telemetry becomes heartbeat)

## Server-Side Parsing

### Python server.py
```python
# Parse JSON (all modes handled identically)
data = json.loads(line.split('|')[0])  # Extract JSON before device ID

# Identify mode
mode = data.get('communication_mode', 'Unknown')

if mode == 'XBee':
    # Handle XBee telemetry
    rssi = data['wifi_rssi']  # Actually XBee RSSI
elif mode == 'Beacon':
    # Handle Beacon telemetry
    rssi = data['wifi_rssi']  # Actually beacon RSSI
elif mode == 'MQTT':
    # Handle MQTT telemetry
    rssi = data['wifi_rssi']  # WiFi RSSI

# Debug messages ignored (starts with '[')
if line.startswith('['):
    logger.debug(f"ESP32 debug: {line}")  # [XBEE TX], [BEACON TX], etc.
```

## Benefits of Unification

### 1. Server Simplicity
- **Single JSON parser** handles all modes
- No special cases for XBee vs Beacon
- Mode identified by one field: `communication_mode`

### 2. Dashboard Compatibility
- **Same data structure** regardless of mode
- Frontend doesn't need mode-specific parsers
- Badge color changes based on `communication_mode` field

### 3. Debugging Clarity
- **Clear debug prints:** `[XBEE TX]`, `[BEACON TX]`, `[WIFI TX]`
- USB Serial shows mode explicitly
- Bluetooth gets clean JSON only (no debug clutter)

### 4. Counter Clarity
- **One counter** = total packets received
- No confusion about "which counter to use?"
- Mode determined by `communication_mode`, not counter

## Testing Checklist

- [ ] Upload code to ESP32
- [ ] Verify Bluetooth connection (HC-05/HC-06)
- [ ] Test COM port detection (should find within 1 second)
- [ ] Power on rocket with XBee transmitter
- [ ] Verify `[XBEE TX]` prints on USB Serial
- [ ] Verify JSON appears in Python server with `"communication_mode": "XBee"`
- [ ] Switch rocket to Beacon mode
- [ ] Verify `[BEACON TX]` prints on USB Serial
- [ ] Verify `"communication_mode": "Beacon"` in server
- [ ] Check dashboard badge color (purple for XBee, orange for Beacon)
- [ ] Verify single `packets_received` counter increments for both modes

## Code Reference

### Arduino Code
- **File:** `Basestation_Code_7_XBee_Fixed.ino`
- **Telemetry Function:** `sendTelemetryJSON()` (lines 245-315)
- **XBee Handler:** `handleXBeeTelemetry()` (lines 317-360)
- **Beacon Handler:** `handleBeacon()` (lines 363-425)
- **Heartbeat Function:** `sendHeartbeat()` (lines 172-201)

### Python Server
- **File:** `research/scripts/server.py`
- **JSON Parser:** Lines 840-870
- **Debug Filter:** Lines 864-869

### React Dashboard
- **File:** `src/App.jsx`
- **Mode Detection:** Lines 430-448 (JSON), 605-623 (CSV)
- **Badge Display:** Communication mode badge with color
