# Bluetooth Research & Configuration

This folder contains all Bluetooth-related research, configuration tools, and documentation for the N4 Base Station project.

## 📁 Contents

### 1. **HC05_AT_Command_Setup.ino**
Arduino sketch for configuring HC-05/HC-06 Bluetooth modules using AT commands.

**Purpose:** Configure baud rate, name, password, and other module settings

**Features:**
- Tested and proven to work reliably
- Supports both HC-05 and HC-06 modules
- User-friendly command interface
- Clear setup instructions included
- Proper line ending handling (CR+LF)

**Usage:**
1. Upload to ESP32
2. Connect HC-05/HC-06 to GPIO 16/17
3. For HC-05: Enter AT mode (hold button during power-on)
4. Open Serial Monitor (115200 baud, "Both NL & CR")
5. Type AT commands (e.g., `AT+UART=460800,0,0`)

**Common Commands:**
```
AT                      // Test connection
AT+UART?                // Check baud rate (HC-05)
AT+UART=460800,0,0      // Set 460800 baud (HC-05)
AT+BAUD9                // Set 460800 baud (HC-06)
AT+NAME=N4_BASE         // Set module name
```

---

### 2. **HC05_BAUDRATE_CONFIG.md**
Comprehensive guide for configuring HC-05/HC-06 baud rates to improve telemetry speed.

**Purpose:** Fix slow Bluetooth data transfer issues

**Topics Covered:**
- Why default baud rates (9600/38400) are too slow
- Step-by-step AT command configuration
- HC-05 vs HC-06 differences
- Baud rate recommendations (460800 for best performance)
- Troubleshooting common issues
- AT command reference table
- Verification methods

**Key Takeaway:**
Default Bluetooth modules ship at 9600 or 38400 baud, which causes slow/choppy telemetry. Increasing to 460800 baud provides 4-12x faster data transmission for real-time rocket telemetry.

---

## 🔧 Hardware Wiring

### ESP32 ↔ HC-05/HC-06 Connection

```
ESP32         HC-05/HC-06
-----         -----------
GPIO 17  →    RX
GPIO 16  ←    TX
3.3V     →    VCC  (or 5V, check your module)
GND      →    GND
```

⚠️ **Important:** Some HC-05 modules require 5V, others work with 3.3V. Check your module specifications!

---

## 📊 Baud Rate Performance Comparison

| Baud Rate | Data Throughput | Telemetry Quality | Recommendation |
|-----------|----------------|-------------------|----------------|
| 9600      | ~0.9 KB/sec    | Slow, choppy      | ❌ Not suitable |
| 38400     | ~3.6 KB/sec    | Better but laggy  | ⚠️ Minimum acceptable |
| 115200    | ~10 KB/sec     | Good for most use | ✅ Recommended minimum |
| 460800    | ~40 KB/sec     | Excellent, smooth | ✅✅ Best performance |

**N4 Base Station Telemetry Requirements:**
- JSON packet size: ~800-1000 bytes
- Update frequency: 10 Hz (every 100ms)
- Required throughput: ~10 KB/sec minimum
- **Recommended baud rate: 460800** for smooth real-time updates

---

## 🚀 Quick Start: Configure New Module

### Option 1: Using AT Command Setup (Recommended)

1. **Upload HC05_AT_Command_Setup.ino** to ESP32
2. **Connect module** to GPIO 16/17
3. **Enter AT mode** (HC-05 only):
   - Disconnect VCC
   - Hold button on module
   - Connect VCC while holding
   - LED blinks slowly (2 sec intervals)
   - Release button
4. **Open Serial Monitor** (115200 baud, "Both NL & CR")
5. **Test connection:** Type `AT` → expect `OK`
6. **Set baud rate:**
   - HC-05: `AT+UART=460800,0,0`
   - HC-06: `AT+BAUD9` (or `AT+BAUD8` for 115200)
7. **Verify:** Response should be `OK` or `OK460800`
8. **Power cycle** the module

### Option 2: Using USB-to-TTL Adapter

See detailed instructions in [HC05_BAUDRATE_CONFIG.md](HC05_BAUDRATE_CONFIG.md)

---

## 🐛 Troubleshooting

### No Response to AT Commands

**Check 1:** Correct baud rate
- Try 38400, 9600, or 115200 in Serial Monitor
- HC-05 AT mode default: 38400
- HC-06 default: 9600 or 38400

**Check 2:** HC-05 in AT mode
- LED must blink slowly (once every 2 seconds)
- Fast blinking = normal mode, not AT mode
- Re-enter AT mode (hold button during power-on)

**Check 3:** Line endings
- Serial Monitor MUST be set to "Both NL & CR"
- Some modules only respond with proper line endings

**Check 4:** Wiring
- RX/TX might be swapped
- Try reversing GPIO 16 ↔ 17 connections

### Slow Telemetry After Configuration

**Verify module baud rate:**
```
AT+UART?    // HC-05
```
Should return: `+UART:460800,0,0`

**Verify ESP32 code:**
Check that base station code matches:
```cpp
BTSerial.begin(460800, SERIAL_8N1, BT_RX, BT_TX);
```

**Power cycle required:**
- Module needs reset after configuration
- Disconnect VCC for 3 seconds, reconnect

### Module Doesn't Support 460800

Some HC-06 clones don't support high baud rates:
- Try `AT+BAUD8` (115200 baud)
- Update base station code to match: `BTSerial.begin(115200, ...)`
- 115200 is still 4x faster than default 38400

---

## 📝 HC-05 vs HC-06 Differences

| Feature | HC-05 | HC-06 |
|---------|-------|-------|
| **AT Mode Entry** | Press button during power-on | Always in AT mode (when unpaired) |
| **AT Command Format** | `AT+UART=460800,0,0` | `AT+BAUD9` |
| **Query Commands** | Supported (`AT+UART?`) | Limited (no query for baud) |
| **Default Baud** | 38400 (AT mode), 9600 (normal) | 9600 or 38400 |
| **Max Baud Rate** | Up to 1382400 | Up to 1382400 (varies by clone) |
| **LED Indicator** | Slow blink = AT mode | Same blink always |

---

## 🎯 Integration with Base Station

Both production and simulator base stations support Bluetooth:

### Production Code: `Basestation_Code_6_Bluetooth.ino`
- Receives real beacon telemetry
- Outputs to USB + Bluetooth (460800 baud)
- Device identifier: `|ESP32:N4_BASE_BT_1`

### Simulator Code: `Simulated_BaseStation_Integrated.ino`
- Generates realistic flight simulation
- Outputs to USB + Bluetooth (460800 baud)
- Same device identifier for auto-detection

**Both codes updated to use 460800 baud for optimal performance!**

---

## 📖 Additional Resources

### AT Command Quick Reference

See [HC05_BAUDRATE_CONFIG.md](HC05_BAUDRATE_CONFIG.md) for complete AT command tables.

### Bluetooth Pairing & Connection

See main documentation:
- [../BLUETOOTH_SETUP.md](../BLUETOOTH_SETUP.md) - Initial pairing guide
- [../SIMULATION_TESTING_GUIDE.md](../SIMULATION_TESTING_GUIDE.md) - Testing procedures
- [../COMMUNICATION_MODES.md](../COMMUNICATION_MODES.md) - USB vs Bluetooth switching

---

## ✅ Verification Checklist

After configuring your module:

- [ ] AT commands respond with `OK`
- [ ] Baud rate set to 460800 (or 115200 minimum)
- [ ] Module power cycled after configuration
- [ ] Base station code updated to match baud rate
- [ ] Telemetry updates smoothly at 10 Hz
- [ ] No garbled text in Serial Monitor
- [ ] Dashboard shows real-time updates without lag

---

## 📞 Support

If you encounter issues:

1. **Check this folder's documentation** - Most issues are covered
2. **Verify hardware connections** - Wrong wiring is common
3. **Test with AT command setup** - Confirms basic communication
4. **Check baud rate mismatch** - Code and module must match
5. **Try lower baud rate** - 115200 if 460800 fails

**Remember:** The AT command setup code (`HC05_AT_Command_Setup.ino`) in this folder is proven to work. If AT commands fail, the issue is likely hardware wiring or module power.

---

## 🔄 Update Log

- **2026-01-13:** Added 460800 baud configuration
- **2026-01-13:** Created bluetooth research folder
- **2026-01-13:** Added working AT command setup code
- **2026-01-13:** Comprehensive HC-05/HC-06 documentation

---

**Note:** All code and documentation in this folder has been tested and verified to work with HC-05 and HC-06 modules on ESP32 hardware.
