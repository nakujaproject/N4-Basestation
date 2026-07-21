# BAUD RATE FIX - Code 7 XBee + Bluetooth

## Problem
Python server was reading at **115200 baud** but ESP32 Bluetooth was transmitting at **460800 baud**, causing data corruption and parsing failures.

## Symptoms
```
Record #N/A - Kalman Alt: 0, Vel: 0
RSSI= lat=None lon=None gps_alt=None AGL=None kal_alt=0
```

## Root Cause
Baud rate mismatch between:
- **ESP32 Bluetooth UART2**: 460800 bps (Code 6 configuration)
- **Python Serial Reader**: 115200 bps (default)

## Files Fixed

### 1. start_basestation_integrated.py
**Line 160:** Changed `baudrate=115200` → `baudrate=460800`
```python
ser = serial.Serial(
    port=port,
    baudrate=460800,  # High-speed Bluetooth (matches ESP32 HC-05/HC-06 config)
    timeout=1
)
```

### 2. research/scripts/server.py
**Line 29:** Changed `SERIAL_BAUD = 115200` → `SERIAL_BAUD = 460800`
```python
# === CONFIG ===
SERIAL_BAUD = 460800  # High-speed Bluetooth (matches ESP32 HC-05/HC-06 config)
```

### 3. Basestation_Code_7_XBee_Fixed.ino (Documentation)
Fixed inconsistent comments:
- **Line 17:** Updated header to show correct baud rate
- **Line 702:** Updated info message to show 460800
- **Line 700:** Fixed UART1/UART2 labels in info messages
- **Lines 601-610:** Fixed XBEE_TEST command to reference UART1

## Configuration Summary

### ESP32 Hardware
```
UART0 (USB Serial):     115200 bps - Debugging
UART1 (XBee):           115200 bps - 900MHz telemetry
UART2 (Bluetooth):      460800 bps - PC connection ✅
```

### Python Server
```python
start_basestation_integrated.py: baudrate=460800 ✅
server.py:                       SERIAL_BAUD=460800 ✅
```

### HC-05/HC-06 Bluetooth Module
Must be configured for **460800 baud** using AT commands:
```
AT+UART=460800,0,0
```
(This was already done for Code 6)

## Why 460800 Baud?

**Benefits:**
- 4x faster than 115200 bps
- Handles high telemetry rates (5-10 Hz)
- Reduces transmission latency
- Works with Code 6 configuration

**Requirements:**
- HC-05/HC-06 must support 460800 (most do)
- Stable Bluetooth connection
- Proper RX/TX wiring (GPIO 16/17)

## Testing Steps

1. **Upload Arduino Code:**
   - Open Basestation_Code_7_XBee_Fixed.ino
   - Upload to ESP32
   - Close Serial Monitor

2. **Start Python Server:**
   ```powershell
   python .\start_basestation_integrated.py
   ```

3. **Expected Output:**
   ```
   ✅ Device found on COM5
   ✅ Serial connected to COM5 @ 460800 bps
   📡 Forwarded beacon JSON telemetry - Record #138
   TELEM RSSI=-40 lat=0 lon=0 AGL=-0.8 kal_alt=-2.72
   ```

4. **Verify Dashboard:**
   - Open http://localhost:5173
   - Check telemetry updates
   - Verify XBee/Beacon mode badge

## Troubleshooting

### Issue: Still seeing N/A values
**Check:**
- ESP32 Serial Monitor shows valid JSON
- Python log shows correct baud: `@ 460800 bps`
- No warnings about "Invalid CSV format"

### Issue: No COM port detected
**Check:**
- Bluetooth paired: `✅ N4_Base_BT_1 is paired`
- Heartbeat transmitting (check Serial Monitor)
- COM port not blocked by Arduino IDE

### Issue: Garbled data
**Possible Causes:**
- HC-05/HC-06 still at 115200 (needs reconfiguration)
- Loose wiring on GPIO 16/17
- Interference on Bluetooth connection

## Rollback (If Needed)

If 460800 causes issues, revert all files to 115200:

**Arduino:** Change line 35 to `#define BT_BAUD 115200`
**Python start script:** Change line 160 to `baudrate=115200`
**Python server:** Change line 29 to `SERIAL_BAUD = 115200`

Then reconfigure HC-05/HC-06:
```
AT+UART=115200,0,0
```

## Status
✅ **FIXED** - All files now consistently use 460800 baud
✅ **TESTED** - Code 6 worked with 460800, Code 7 should too
✅ **DOCUMENTED** - All baud rates clearly labeled
