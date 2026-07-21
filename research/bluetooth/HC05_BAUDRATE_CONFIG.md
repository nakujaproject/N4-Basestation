# HC-05/HC-06 Baud Rate Configuration Guide

## Problem: Slow Bluetooth Data Transfer

**Symptom:** Dashboard receives telemetry slowly, data appears delayed or choppy.

**Root Cause:** HC-05/HC-06 Bluetooth module default baud rate (9600 or 38400) is too slow for real-time telemetry. The ESP32 code now uses 460800 baud for faster transmission.

---

## Solution: Configure HC-05/HC-06 to 460800 Baud

### Required Hardware:
- HC-05 or HC-06 Bluetooth module
- USB-to-TTL adapter (or another ESP32)
- Jumper wires

### Method 1: Using USB-to-TTL Adapter

#### Step 1: Enter AT Command Mode

**For HC-05:**
1. Disconnect VCC from HC-05
2. Press and HOLD the button on HC-05
3. Connect VCC while holding button
4. LED should blink slowly (once every 2 seconds)
5. Release button - you're now in AT mode

**For HC-06:**
1. No button needed - it's always in AT mode when not connected
2. Just power on normally

#### Step 2: Wiring

```
USB-TTL Adapter    HC-05/HC-06
--------------     -----------
    VCC       →       VCC (5V or 3.3V)
    GND       →       GND
    TX        →       RX
    RX        →       TX
```

⚠️ **Important:** Some HC-05 modules need 5V, others 3.3V. Check your module specs.

#### Step 3: Open Serial Monitor

1. Open Arduino IDE Serial Monitor
2. Set baud rate to **38400** (HC-05) or **9600** (HC-06)
3. Set line ending to **"Both NL & CR"**

#### Step 4: Test AT Commands

Type: `AT`

Expected response: `OK`

If no response:
- Try different baud rates (9600, 38400, 115200)
- Check TX/RX wiring (might need to swap)
- Ensure HC-05 is in AT mode (slow blink)

#### Step 5: Check Current Baud Rate

**HC-05:**
```
AT+UART?
```
Response: `+UART:38400,0,0` (or similar)

**HC-06:**
```
AT+BAUD8
```
Response: `OK115200` (or current baud)

#### Step 6: Set to 460800 Baud

**HC-05:**
```
AT+UART=460800,0,0
```
Response: `OK`

**HC-06:**
```
AT+BAUD460800
```
Response: `OK460800`

⚠️ **HC-06 Note:** Not all HC-06 modules support 460800. Try these in order:
- `AT+BAUD9` (460800) - preferred
- `AT+BAUD8` (115200) - if 460800 fails
- `AT+BAUDA` (1382400) - some support this

If HC-06 doesn't support 460800, use 115200 and update the code accordingly.

#### Step 7: Verify Configuration

**HC-05:**
```
AT+UART?
```
Response: `+UART:460800,0,0`

**HC-06:** Just check that you got `OK460800` response.

#### Step 8: Power Cycle

1. Disconnect VCC
2. Wait 3 seconds
3. Reconnect VCC
4. Module should now operate at 460800 baud

---

### Method 2: Using Another ESP32

If you don't have a USB-to-TTL adapter, use this Arduino sketch on a spare ESP32:

```cpp
#include <HardwareSerial.h>

HardwareSerial BTSerial(2);
#define BT_TX 17
#define BT_RX 16

void setup() {
  Serial.begin(115200);
  BTSerial.begin(38400, SERIAL_8N1, BT_RX, BT_TX);  // Start at default baud
  
  Serial.println("HC-05/HC-06 AT Command Interface");
  Serial.println("Type AT commands in Serial Monitor");
  Serial.println("Set to 'Both NL & CR' line ending");
}

void loop() {
  // Forward from Serial Monitor to BT module
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    BTSerial.print(cmd);
    BTSerial.print("\r\n");  // AT commands need CR+LF
    Serial.print("SENT: ");
    Serial.println(cmd);
  }
  
  // Forward from BT module to Serial Monitor
  if (BTSerial.available()) {
    String response = BTSerial.readStringUntil('\n');
    Serial.print("RESPONSE: ");
    Serial.println(response);
  }
}
```

**Steps:**
1. Upload this sketch to a spare ESP32
2. Open Serial Monitor at 115200 baud
3. If using HC-05, put it in AT mode first (hold button during power-on)
4. Type `AT` - should see `OK` response
5. Type `AT+UART=460800,0,0` (HC-05) or `AT+BAUD9` (HC-06)
6. If no response, try changing `BTSerial.begin(38400,...)` to 9600 or 115200

---

## Update Base Station Code (If Using Different Baud Rate)

If your HC-06 only supports 115200 baud (not 460800), update the code:

**File:** `research/Basestation_Code_6_Bluetooth.ino`

Change this line in `setup()`:
```cpp
BTSerial.begin(115200, SERIAL_8N1, BT_RX, BT_TX);  // Use 115200 if module doesn't support higher
```

---

## Verification After Configuration

### Test 1: Baud Rate Check

Upload the base station code and open Serial Monitor:
- You should see startup messages
- If you see garbled text from Bluetooth, baud rate mismatch persists

### Test 2: Data Rate Check

1. Start the base station with simulator or real beacons
2. Monitor dashboard update frequency
3. Expected: Telemetry updates every ~100ms (10 Hz)

### Test 3: Latency Check

Run this test to measure actual throughput:
```python
import serial
import time

ser = serial.Serial('COM7', 115200)  # Use your Bluetooth COM port
start = time.time()
bytes_received = 0

while time.time() - start < 10:
    if ser.in_waiting:
        data = ser.read(ser.in_waiting)
        bytes_received += len(data)

print(f"Throughput: {bytes_received / 10:.1f} bytes/sec")
print(f"Throughput: {bytes_received / 10 / 1024:.2f} KB/sec")
```

**Expected Results:**
- At 115200 baud: ~8-10 KB/sec
- At 460800 baud: ~30-40 KB/sec

---

## Alternative: Reduce Data Payload

If baud rate upgrade doesn't help enough, optimize the JSON payload:

### Option 1: Reduce Heartbeat Frequency

Change in `loop()`:
```cpp
// Heartbeat status every 30 seconds instead of 10
if (millis() - lastHeartbeat > 30000) {
```

### Option 2: Reduce JSON Size

Comment out less critical fields in `sendTelemetryJSON()`:
```cpp
// Comment these out if not needed:
// JsonObject gyro_data = doc.createNestedObject("gyro_data");
// gyro_data["gx"] = telemetry.gx;
// gyro_data["gy"] = telemetry.gy;
// gyro_data["gz"] = telemetry.gz;
```

---

## Troubleshooting

### Module Not Responding to AT Commands

**Check 1:** Ensure correct baud rate
- Try 9600, 38400, 115200 in Serial Monitor

**Check 2:** HC-05 needs AT mode
- LED should blink slowly (once every 2 seconds)
- If blinking fast, not in AT mode

**Check 3:** Line endings
- Serial Monitor must be set to "Both NL & CR"

### Module Configured But Still Slow

**Check 1:** Verify with AT+UART? command
- Ensure it's actually set to 460800

**Check 2:** Power cycle after configuration
- Module needs reset to apply new baud rate

**Check 3:** Check base station code matches
- ESP32 code must use same baud rate as module

### Module Doesn't Support 460800

Some HC-06 clones don't support high baud rates:
- Use 115200 instead (still 4x faster than 38400)
- Update code: `BTSerial.begin(115200, SERIAL_8N1, BT_RX, BT_TX);`
- 115200 should be sufficient for this telemetry rate

---

## HC-05 vs HC-06 AT Command Reference

### Common Commands

| Function | HC-05 | HC-06 |
|----------|-------|-------|
| Test | `AT` | `AT` |
| Get name | `AT+NAME?` | `AT+NAME` |
| Set name | `AT+NAME=NewName` | `AT+NAMENewName` |
| Get baud | `AT+UART?` | N/A (use set commands) |
| Set 9600 | `AT+UART=9600,0,0` | `AT+BAUD4` |
| Set 38400 | `AT+UART=38400,0,0` | `AT+BAUD6` |
| Set 115200 | `AT+UART=115200,0,0` | `AT+BAUD8` |
| Set 460800 | `AT+UART=460800,0,0` | `AT+BAUD9` |
| Reset | `AT+RESET` | N/A (power cycle) |

### HC-06 Baud Rate Codes

| Code | Baud Rate |
|------|-----------|
| AT+BAUD1 | 1200 |
| AT+BAUD2 | 2400 |
| AT+BAUD3 | 4800 |
| AT+BAUD4 | 9600 |
| AT+BAUD5 | 19200 |
| AT+BAUD6 | 38400 |
| AT+BAUD7 | 57600 |
| AT+BAUD8 | 115200 |
| AT+BAUD9 | 230400 |
| AT+BAUDA | 460800 |
| AT+BAUDB | 921600 |
| AT+BAUDC | 1382400 |

⚠️ **Note:** Not all HC-06 clones support codes above BAUD8 (115200).

---

## Summary

1. **Default HC-05/HC-06:** 9600-38400 baud (TOO SLOW)
2. **Recommended:** 460800 baud (4-12x faster)
3. **Minimum:** 115200 baud if 460800 not supported
4. **Configuration:** Use AT commands to set module baud rate
5. **Code Update:** Already updated to 460800 baud
6. **Verification:** Run throughput test to confirm

After configuring to 460800 baud, your telemetry should update smoothly in real-time!
