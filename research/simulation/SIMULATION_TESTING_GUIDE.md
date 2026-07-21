# N4 Simulation Testing Guide

## Overview

This guide covers the **second stage** of base station development - a complete flight simulator that helps you develop and test the entire base station software stack without needing a real rocket.

## What's Included

### 1. **Simulated_BaseStation_Code.ino** 
ESP32 sketch that simulates realistic rocket flight with Bluetooth output

**Features:**
- ✅ Complete flight physics simulation (launch → apogee → landing)
- ✅ Bluetooth serial output via HC-05/HC-06
- ✅ JSON telemetry format matching real base station
- ✅ Device identifier for COM port detection
- ✅ Interactive commands (ARM, LAUNCH, DISARM, RESET, STATUS)
- ✅ Beacon listening code ready (commented out for simulation)
- ✅ PWM configuration support
- ⚡ 10 Hz telemetry rate (100ms intervals)

### 2. **start_basestation_integrated.py**
Unified Python script combining Bluetooth setup and service orchestration

**Features:**
- ✅ Automatic Bluetooth COM port detection
- ✅ Configuration persistence (.env.local)
- ✅ All services management (MQTT, Vite, TileServer, Node API)
- ✅ Python telemetry server integration
- ✅ Simulation mode support
- ✅ USB fallback mode

## Flight Simulation Parameters

The simulator generates realistic data through all flight phases:

### Flight Phases

| Phase | Duration | Characteristics |
|-------|----------|----------------|
| **PRE_LAUNCH** | Until ARM+LAUNCH | Rocket on pad, minimal movement |
| **POWERED_ASCENT** | 0-3.5s | Motor thrust 80 m/s², high acceleration |
| **COASTING** | 3.5s-apogee | Ballistic flight, decelerating |
| **APOGEE** | Instant | Velocity crosses zero, drogue deploys |
| **DROGUE_DESCENT** | Apogee-200m | Falling with drogue chute |
| **MAIN_DESCENT** | 200m-0m | Slower descent with main chute |
| **LANDED** | Final | All motion stopped |

### Simulated Parameters

```
Motor burn time:    3.5 seconds
Motor thrust:       80 m/s² (net acceleration)
Expected apogee:    ~500-600m
Drogue deployment:  At apogee (auto)
Main deployment:    200m AGL
Landing velocity:   ~5 m/s with main chute
```

### Telemetry Data (25 fields + Kalman)

All data varies realistically based on flight phase:

- **Acceleration:** Changes from +80 m/s² (ascent) to negative (descent)
- **Gyroscope:** High rotation during powered flight, stable during chute descent
- **Altitude:** Smooth increase/decrease with realistic physics
- **Velocity:** Positive during ascent, negative during descent
- **Pressure/Temperature:** Vary with altitude (standard atmosphere model)
- **GPS:** Small drift during flight, stable on ground
- **Battery:** Gradual drain during flight
- **RSSI:** Degrades with altitude
- **Kalman Filters:** Smoothed altitude and velocity estimates

## Hardware Setup

### Required Components
- ESP32 DevKit (any variant)
- HC-05 or HC-06 Bluetooth module
- Jumper wires
- USB cable for programming

### Wiring

```
ESP32          HC-05/HC-06
-----          -----------
GPIO 17 (TX2) → RX
GPIO 16 (RX2) → TX
3.3V          → VCC
GND           → GND
```

### Bluetooth Module Setup

**For HC-05:**
- Default name: `HC-05`
- Rename to: `N4_Base_BT_1` (using AT commands)
- Default PIN: `0001` or `1234`

**For HC-06:**
- Default name: `HC-06`
- Rename to: `N4_Base_BT_1` (using AT commands)
- Default PIN: `1234`

**AT Command Reference:**
```
AT+NAME=N4_Base_BT_1
AT+PSWD=0001
AT+UART=115200,0,0
```

*Note: AT commands only work before pairing. See BLUETOOTH_SETUP.md for details.*

## Step-by-Step Testing Procedure

### Stage 1: Upload Simulator to ESP32

1. **Open Arduino IDE**
   ```
   File → Open → research/Simulated_BaseStation_Code.ino
   ```

2. **Configure Board**
   - Board: "ESP32 Dev Module"
   - Upload Speed: 921600
   - Flash Frequency: 80MHz
   - Port: Select your ESP32's COM port

3. **Upload Code**
   - Click Upload (→) button
   - Wait for "Done uploading"

4. **Verify Operation**
   - Open Serial Monitor (115200 baud)
   - Should see:
     ```
     N4 SIMULATED BASE STATION - Flight Simulator
     Device ID: ESP32:N4_BASE_BT_1
     Mode: Realistic flight simulation with Bluetooth output
     ```

5. **Test Commands (Serial Monitor)**
   ```
   STATUS  → See current state
   ARM     → Arm the system
   LAUNCH  → Start flight simulation
   RESET   → Reset to pre-launch
   ```

### Stage 2: Pair Bluetooth

1. **Windows Bluetooth Pairing**
   - Settings → Bluetooth & devices → Add device
   - Select "Bluetooth"
   - Look for `N4_Base_BT_1`
   - Enter PIN: `0001` (or `1234`)

2. **⚠️ CRITICAL: Reset After Pairing**
   
   **After successful pairing, you MUST reset the Bluetooth connection:**
   
   - **Turn OFF the HC-05/HC-06 module** (disconnect VCC or power off ESP32)
   - **Wait 5 seconds**
   - **Turn ON the module again** (reconnect VCC or power on ESP32)
   
   **Why?** The Bluetooth module needs to reinitialize its connection in SPP (Serial Port Profile) mode after pairing. Without this reset, data transmission may not work properly.

3. **Connection Trial and Error**
   
   **If connection doesn't work immediately, try these combinations:**
   
   a. **Reset Bluetooth module** (power cycle as above)
   b. **Reset ESP32** (press EN button or replug USB)
   c. **Reset both together**:
      - Disconnect ESP32 USB
      - Wait 10 seconds
      - Reconnect USB
      - Check for telemetry
   
   d. **Windows Bluetooth troubleshooting**:
      - Remove device from Windows (Settings → Bluetooth & devices)
      - Re-pair from scratch
      - Power cycle module after pairing (step 2)
   
   **This is normal!** Bluetooth SPP connections can be finicky. Keep trying different reset combinations until you see data flowing.

4. **Verify Pairing**
   ```powershell
   Get-PnpDevice -Class Bluetooth | Where-Object {$_.FriendlyName -like "*N4_Base_BT_1*"}
   ```

5. **Note COM Port**
   - Check Device Manager → Ports (COM & LPT)
   - Find "Standard Serial over Bluetooth link (COMx)"
   - Example: COM7

### Stage 3: Test Bluetooth Connection

**Option A: Using bluetooth_monitor.py**
```bash
cd N4-Basestation
python bluetooth_monitor.py
```
- Should auto-detect COM port
- Display real-time telemetry
- Type 'q' to exit

**Option B: Manual test**
```python
import serial
ser = serial.Serial('COM7', 115200, timeout=1)  # Use your COM port
while True:
    line = ser.readline().decode('utf-8').strip()
    print(line)
```

### Stage 4: Run Integrated Base Station

1. **First-Time Setup**
   ```bash
   cd N4-Basestation
   npm install
   ```

2. **Start with Bluetooth Detection**
   ```bash
   python start_basestation_integrated.py
   ```
   
   This will:
   - ✅ Detect Bluetooth COM port automatically
   - ✅ Save configuration to `.env.local`
   - ✅ Start all services (MQTT, Vite, TileServer, API)
   - ✅ Connect to telemetry

3. **Start with Saved Config** (after first run)
   ```bash
   python start_basestation_integrated.py
   ```
   - Uses saved COM port from `.env.local`
   - Faster startup (no re-scanning)

4. **Alternative Modes**
   ```bash
   # Simulation mode (no hardware needed)
   python start_basestation_integrated.py --simulation
   
   # Skip Bluetooth, USB only
   python start_basestation_integrated.py --skip-bluetooth
   ```

### Stage 5: Test Flight Simulation

1. **Open Web Interface**
   ```
   http://localhost:5173
   ```

2. **Verify Telemetry Display**
   - Should see real-time data updating
   - All values should be realistic (on pad)

3. **Simulate Flight Sequence**
   
   **Via Web UI (if commands supported):**
   - Click "ARM" button
   - Click "LAUNCH" button
   
   **Via Serial Monitor:**
   ```
   ARM     → System armed
   LAUNCH  → Flight starts!
   ```

4. **Observe Flight Phases**
   - **0-3.5s:** Powered ascent (acceleration visible)
   - **3.5-~15s:** Coasting to apogee
   - **~15s:** Apogee reached, drogue deploys
   - **~15-40s:** Drogue descent
   - **200m AGL:** Main chute deploys
   - **~40-80s:** Main descent
   - **Landing:** All motion stops

5. **Monitor Web Dashboard**
   - Altitude graph should show realistic flight profile
   - Velocity should go positive → zero → negative
   - Acceleration should show motor burn
   - Parachute states should trigger correctly
   - Map should show position (simulated GPS)

6. **Reset and Repeat**
   ```
   RESET  → Back to pre-launch
   ARM    → Ready again
   LAUNCH → Another flight!
   ```

## 🔧 Bluetooth Connection Troubleshooting Flowchart

If you're having trouble getting Bluetooth data to flow, follow this decision tree:

```
┌─────────────────────────────┐
│ Can you pair the device?    │
└──────────┬──────────────────┘
           │
    ┌──────┴──────┐
    │             │
   YES            NO → Check wiring, power, PIN code
    │                  HC-05/HC-06 LED blinking?
    ↓
┌───────────────────────────────┐
│ After pairing, did you        │
│ power cycle the module?       │
└──────────┬────────────────────┘
           │
    ┌──────┴──────┐
    │             │
   YES            NO → ⚡ POWER CYCLE NOW! (VCC off/on, wait 5s)
    │
    ↓
┌───────────────────────────────┐
│ Can you see the COM port in   │
│ Device Manager?               │
└──────────┬────────────────────┘
           │
    ┌──────┴──────┐
    │             │
   YES            NO → Unpair, re-pair, THEN power cycle
    │
    ↓
┌───────────────────────────────┐
│ Can you open the COM port?    │
│ (Try: python bluetooth_monitor.py)
└──────────┬────────────────────┘
           │
    ┌──────┴──────┐
    │             │
   YES            NO → Close Arduino IDE, other programs
    │                  Try different COM port?
    ↓
┌───────────────────────────────┐
│ Do you see any data at all?   │
└──────────┬────────────────────┘
           │
    ┌──────┴──────┐
    │             │
   YES            NO → ⚡ TRY THESE IN ORDER:
    │                  1. Reset ESP32 (EN button)
    │                  2. Reset Bluetooth module (power cycle)
    │                  3. Reset BOTH together
    │                  4. Close/reopen COM port
    │                  5. Repeat steps 1-4 until it works!
    ↓
┌───────────────────────────────┐
│ Is the data JSON format with  │
│ |ESP32:N4_BASE_BT_1 at end?  │
└──────────┬────────────────────┘
           │
    ┌──────┴──────┐
    │             │
   YES            NO → Wrong code uploaded? Re-upload simulator
    │
    ↓
┌───────────────────────────────┐
│ ✅ SUCCESS! You're ready!     │
│ Run: python start_basestation │
│      _integrated.py           │
└───────────────────────────────┘
```

**Remember:** Bluetooth SPP connections can be temperamental. The reset-and-retry approach WILL work - just be patient!

## Telemetry Output Format

### JSON Structure
```json
{
  "record_number": 1234,
  "operation_mode": 1,
  "state": 2,
  "battery_voltage": 14.5,
  "wifi_rssi": -65,
  "acc_data": {
    "ax": -0.59,
    "ay": -0.02,
    "az": 8.5,
    "pitch": 89.5,
    "roll": 2.3
  },
  "gyro_data": {
    "gx": 15.2,
    "gy": -3.4,
    "gz": 8.7
  },
  "gps_data": {
    "latitude": -1.2921,
    "longitude": 36.8219,
    "gps_altitude": 2150.0,
    "time": 123456
  },
  "alt_data": {
    "pressure": 790.5,
    "temperature": 18.3,
    "AGL": 489.5,
    "velocity": 85.2,
    "kalman_altitude": 490.1,
    "kalman_vertical_velocity": 84.8
  },
  "chute_state": {
    "pyro1_state": 1,
    "pyro2_state": 0
  },
  "connection_status": {
    "connected": true,
    "has_ever_connected": true,
    "packet_age_ms": 50,
    "timeout_exceeded": false,
    "rssi": -65
  },
  "communication_mode": "Bluetooth-Simulated",
  "timestamp": 123456789,
  "packets_received": 5678
}|ESP32:N4_BASE_BT_1
```

**Note:** The `|ESP32:N4_BASE_BT_1` identifier is always appended.

### Status Messages (Heartbeat)
Every 10 seconds, a status message is sent:
```json
STATUS:{
  "type": "status",
  "armed": true,
  "flight_active": true,
  "packets_received": 1234,
  "uptime": 123456,
  "simulation_mode": true,
  "flight": {
    "phase": 2,
    "time": 15.3,
    "altitude": 489.5,
    "velocity": 85.2,
    "max_altitude": 523.7,
    "drogue_deployed": true,
    "main_deployed": false
  },
  "pwm_config": {
    "vcc": 14.8,
    "drogue_voltage": 9.0,
    "main_voltage": 10.0,
    "drogue_duration_ms": 3000,
    "main_duration_ms": 5000
  }
}
```

## Command Reference

### Flight Commands

| Command | Description | Response |
|---------|-------------|----------|
| `ARM` | Arm the system (required before launch) | `ACK:ARMED` |
| `DISARM` | Disarm and reset simulation | `ACK:DISARMED` |
| `LAUNCH` | Start flight simulation (requires ARM) | `ACK:LAUNCH` |
| `START` | Same as LAUNCH | `ACK:LAUNCH` |
| `RESET` | Reset to pre-launch state | `ACK:RESET` |
| `RESTART` | Same as RESET | `ACK:RESET` |
| `STATUS` | Get current flight status | Status string |
| `STOP` | Pause simulation | `ACK:STOPPED` |
| `Q` | Stop telemetry | `ACK:STOPPED` |
| `2` | Stop telemetry | `ACK:STOPPED` |

### PWM Configuration Commands

```
SET_PWM:{"vcc":14.8,"drogue_v":9.0,"main_v":10.0,"drogue_time":3000,"main_time":5000}
```
Response: `PWM_CONFIG_OK:Vcc=14.8,Drogue=9.0V(3000ms),Main=10.0V(5000ms)`

## Troubleshooting

### ⚡ Quick Fix: Bluetooth Not Working After Pairing

**THE MOST COMMON ISSUE:**

After pairing your Bluetooth module, **you MUST reset it** for data to flow:

1. **Disconnect power** from HC-05/HC-06 (or unplug ESP32 USB)
2. **Wait 5 seconds**
3. **Reconnect power** (or replug ESP32)
4. **Check for data** - should now be working!

If still not working, try resetting both ESP32 and Bluetooth module together. **Keep trying different reset combinations** - this is normal for Bluetooth SPP and will eventually work!

---

### Simulator Not Starting

**Problem:** Serial Monitor shows nothing after upload

**Solutions:**
- Check USB cable (data cable, not charge-only)
- Try different USB port
- Press EN/RESET button on ESP32
- Re-upload code
- Check baud rate (should be 115200)

### Bluetooth Pairing Fails

**Problem:** Device not found or pairing rejected

**Solutions:**
1. Check HC-05/HC-06 LED (should be blinking fast when unpaired)
2. Verify wiring (especially TX/RX crossover)
3. Check power (3.3V for HC-05, 3.3-6V for HC-06)
4. Try default PIN: `0001` or `1234`
5. Unpair and re-pair device
6. **After successful pairing, ALWAYS power cycle the module** (see Stage 2, Step 2)

### Bluetooth Connected But No Data

**Problem:** Paired successfully but COM port shows no data

**⚠️ MOST COMMON ISSUE - Reset Required:**

1. **Power cycle Bluetooth module:**
   - Disconnect VCC wire (or unplug ESP32 USB)
   - Wait 5 seconds
   - Reconnect VCC (or replug ESP32)
   - Check for data again

2. **Try systematic reset combinations:**
   - Reset just the Bluetooth module (disconnect/reconnect VCC)
   - Reset just the ESP32 (press EN button)
   - Reset both together (full power cycle)
   - Close and reopen the serial connection
   
3. **Windows Bluetooth stack reset:**
   - Remove device from Windows Bluetooth settings
   - Power off Bluetooth module completely
   - Re-pair from scratch
   - **Immediately power cycle after pairing** (critical!)

4. **Trial and error approach:**
   - Some Bluetooth modules require specific power-up sequences
   - Try resetting ESP32 **while** the module is powered
   - Try connecting to COM port **before** resetting module
   - Try different combinations until data flows
   - **This is normal behavior for Bluetooth SPP** - keep trying!

**Why this happens:** Bluetooth SPP (Serial Port Profile) needs proper handshaking after pairing. The module must reinitialize in data transfer mode, which often requires a power cycle.

### COM Port Detection Fails

**Problem:** `start_basestation_integrated.py` can't find device

**Solutions:**
1. Close Arduino IDE Serial Monitor
2. Close any other serial terminal programs
3. Check Device Manager for COM port conflicts
4. Manually verify port: `python bluetooth_monitor.py`
5. Delete `.env.local` and re-run detection

### No Telemetry in Web UI

**Problem:** Web UI loads but shows "No Data" or stale data

**Solutions:**
1. Check terminal - is telemetry server running?
2. Check MQTT broker - is Mosquitto running?
3. Open browser console (F12) - any errors?
4. Verify MQTT connection: `mqtt://localhost:1883`
5. Restart all services

### Unrealistic Flight Data

**Problem:** Altitude/velocity values don't make sense

**Solutions:**
- Reset simulation: Send `RESET` command
- Check flight phase: Send `STATUS` command
- Verify you sent `ARM` before `LAUNCH`
- Re-upload code to ESP32

### Services Won't Start

**Problem:** Port already in use errors

**Solutions:**
```bash
# Kill processes on ports
netstat -ano | findstr :5173
taskkill /PID <PID> /F

# Or restart script (it tries to kill automatically)
python start_basestation_integrated.py
```

## Transitioning to Real Hardware

When ready to test with actual beacon/ESP-NOW hardware:

### 1. Uncomment Beacon Code

In `Simulated_BaseStation_Code.ino`, find the section:
```cpp
// ====== BEACON CODE (COMMENTED OUT FOR SIMULATION) ======
```

Uncomment all code in that section (remove `/*` and `*/`).

### 2. Configure MAC Addresses

Update these lines with your actual hardware MACs:
```cpp
uint8_t rocket_mac[] = {0x08, 0xD1, 0xF9, 0x15, 0x9C, 0x04};  // Your rocket MAC
uint8_t my_mac[] = {0x10, 0x06, 0x1c, 0xa6, 0x11, 0xf0};      // Your base MAC
```

### 3. Enable Beacon Listening

In `setup()`, uncomment:
```cpp
setupBeaconListening();
sendLogMessage("INFO", "📡 Beacon listening enabled", "System");
```

### 4. Hybrid Mode

The code will now:
- ✅ Listen for real beacons (when available)
- ✅ Override simulation with real data
- ✅ Fall back to simulation if no beacons received
- ✅ Maintain Bluetooth output

## Expected Results

### Successful Test Session

When everything works correctly, you should see:

**Terminal Output:**
```
========================================================================
  N4 BASE STATION - INTEGRATED STARTUP
========================================================================
  
📡 BLUETOOTH SETUP
  ✅ N4_Base_BT_1 is paired
  🔍 Scanning COM ports...
  Testing COM7 (Standard Serial over Bluetooth link)...
    ✅ FOUND! Device identifier detected
  🎉 SUCCESS! Found device on COM7
  💾 Saved configuration to .env.local

🚀 STARTING SERVICES
  ✅ All services running

========================================================================
  ✅ BASE STATION RUNNING
========================================================================
  Services:
  - Web UI:     http://localhost:5173
  - Bluetooth:  COM7
```

**Web Dashboard:**
- 🟢 Green connection indicator
- 📊 Real-time graphs updating smoothly
- 🗺️ Map showing position (simulated GPS)
- 🎯 All telemetry values realistic
- ⚡ Updates at 10 Hz (100ms)

**Flight Simulation:**
- 🚀 Launch sequence executes correctly
- 📈 Altitude climbs to ~500-600m
- 🎯 Apogee detected, drogue deploys
- 🪂 Main chute deploys at 200m
- ✅ Safe landing at 0m

## Performance Metrics

### Telemetry Rate
- **Target:** 10 Hz (100ms intervals)
- **Bluetooth:** ~8-10 Hz (slight USB overhead)
- **USB Serial:** 10 Hz (consistent)

### Latency
- **Bluetooth:** 50-150ms end-to-end
- **USB Serial:** 20-50ms end-to-end
- **Web UI Update:** +50-100ms (network/rendering)

### Flight Duration
- **Powered Ascent:** 3.5 seconds
- **Coast to Apogee:** ~11-15 seconds
- **Drogue Descent:** ~25-35 seconds
- **Main Descent:** ~40-50 seconds
- **Total Flight:** ~80-105 seconds

## Next Steps

After successful simulation testing:

1. **Integrate Commands** - Test ARM/DISARM from web UI
2. **Log Analysis** - Review CSV logs in `src/telemetry/`
3. **Real Beacon Test** - Uncomment beacon code, test with flight computer
4. **Range Testing** - Test Bluetooth/Beacon range outdoors
5. **Battery Life** - Measure power consumption during extended operation
6. **Field Testing** - Deploy to actual launch site

## Additional Resources

- **Main Documentation:** [README.md](../README.md)
- **Bluetooth Setup:** [BLUETOOTH_SETUP.md](../BLUETOOTH_SETUP.md)
- **Hardware Guide:** [SETUP.md](../SETUP.md)
- **Command Interface:** [research/command_interface_research.md](command_interface_research.md)

## Appendix: Flight Physics

The simulator uses simplified physics:

### Ascent Phase (Powered)
```
F_net = F_thrust - F_drag - F_gravity
F_thrust = m * 80 m/s²  (constant during burn)
F_drag = -k * v²        (increases with velocity)
F_gravity = -m * 9.81 m/s²

a(t) = 80 - 9.81 - 0.015*v²
v(t+dt) = v(t) + a(t)*dt
h(t+dt) = h(t) + v(t)*dt
```

### Coast Phase
```
a(t) = -9.81 - 0.015*v²
(No thrust, only drag and gravity)
```

### Descent Phase (Drogue)
```
a(t) = -9.81 + 0.25*v²
(Drag coefficient much higher)
```

### Descent Phase (Main)
```
a(t) = -9.81 + 1.5*v²
(Highest drag coefficient, slow descent)
```

These values are tuned to give realistic flight profiles for a small hobby rocket.

---

**Questions or Issues?**  
Check the main documentation or create an issue in the project repository.
