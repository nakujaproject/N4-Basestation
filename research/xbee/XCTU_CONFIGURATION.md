# XCTU Configuration Guide - DigiMesh 900HP

## Prerequisites

### Firmware Requirements
- **Function Set:** DigiMesh 900HP
- **DO NOT USE:** XSC firmware (legacy compatibility mode)
- **Recommended Version:** Latest stable DigiMesh 900HP firmware

### Tools
- Digi XCTU Software (latest version)
- XBee mounted on Shield or USB Explorer
- USB cable

---

## Configuration Strategy

We use **DigiMesh** protocol with **Transparent Mode (AT)** for simple point-to-point communication. This allows direct serial data transmission without API frame overhead.

### Key Concept: Matching Parameters

For successful communication:
- **ID (Network ID)** must match on Sender and Receiver
- **HP (Preamble ID)** must match on Sender and Receiver
- **BD (Baud Rate)** must match your Arduino/ESP32 code
- **AP (API Mode)** set to 0 for Transparent Mode

---

## Sender Configuration (Rocket/Transmitter)

This module broadcasts telemetry data to the network.

### Required Settings:

| Parameter | Value | Description |
|-----------|-------|-------------|
| **ID** (Network ID) | `7777` | **CRUCIAL:** Must match Receiver. Identifies your network. |
| **HP** (Preamble ID) | `0` | **CRUCIAL:** Must match Receiver. Minimizes interference. |
| **DL** (Dest. Low) | `FFFF` | Broadcast mode - all nodes receive. |
| **DH** (Dest. High) | `0` | Upper 32 bits of destination address. |
| **AP** (API Mode) | `0` | Transparent Mode - data goes straight through. |
| **BD** (Baud Rate) | `3` | 9600 baud (for Arduino) or `7` for 115200 (for ESP32). |
| **PL** (Power Level) | `0` | Lowest power (+7dBm) for bench testing. |
| **TO** (Transmit Options) | `C0` | DigiMesh delivery method with mesh header. |

### Power Level Selection:

| PL Value | Output Power | Use Case |
|----------|--------------|----------|
| `0` | +7 dBm | Bench testing, close range |
| `1` | +10 dBm | Short range flights |
| `2` | +13 dBm | Medium range |
| `3` | +16 dBm | Long range |
| `4` | +24 dBm (250mW) | Maximum range, field testing |

⚠️ **Testing Note:** Start with PL=0 for bench tests to avoid signal saturation when modules are close together.

---

## Receiver Configuration (Ground Station)

This module listens for broadcast packets.

### Required Settings:

| Parameter | Value | Description |
|-----------|-------|-------------|
| **ID** (Network ID) | `7777` | **MUST match Sender exactly.** |
| **HP** (Preamble ID) | `0` | **MUST match Sender exactly.** |
| **AP** (API Mode) | `0` | Transparent Mode - raw data out DOUT pin. |
| **BD** (Baud Rate) | `3` | 9600 baud (match your `Serial.begin()` value). |
| **PL** (Power Level) | `0` | Lowest power for testing. |
| **CE** (Node Options) | `0` | Standard Router - repeats messages, accepts broadcasts. |

---

## Step-by-Step XCTU Configuration

### 1. Connect Module to PC

**Using Shield as Pass-Through:**
1. Mount XBee on Arduino Shield
2. Jumper Arduino RESET to GND
3. Set Shield switch to **USB** position
4. Connect Arduino to PC via USB

**Using USB Explorer:**
1. Mount XBee on Explorer
2. Connect to PC via USB

### 2. Open XCTU

1. Launch XCTU
2. Click **Discover devices** button (magnifying glass icon)
3. Select COM port, click **Next**
4. XBee should appear in device list

### 3. Read Current Configuration

1. Select the XBee module
2. Click **Read** button (refresh icon)
3. Current settings will load

### 4. Update Firmware (if needed)

1. Click **Update Firmware** button
2. **Function Set:** Select "DigiMesh 900HP"
3. **Firmware Version:** Select latest version
4. Click **Update**
5. Wait for completion

### 5. Configure Parameters

#### For Sender:
1. Find **Networking** section:
   - ID = `7777`
   - HP = `0`
   - DH = `0`
   - DL = `FFFF`

2. Find **Serial Interfacing** section:
   - BD = `3` (9600) or `7` (115200)
   - AP = `0`

3. Find **RF Interfacing** section:
   - PL = `0` (testing) or `4` (field)
   - TO = `C0`

4. Click **Write** button (pencil icon)

#### For Receiver:
1. Repeat steps above with Receiver module
2. Ensure ID, HP, BD match Sender
3. Click **Write** button

### 6. Verify Configuration

1. Click **Read** button again
2. Confirm all values were written correctly
3. Save profile: **Profile** → **Save**
4. Name it descriptively (e.g., "Sender_DigiMesh_9600")

---

## Configuration Profiles

Export and save your configurations:

1. **Profiles** menu → **Save Current Profile**
2. Save as `.xpro` file
3. Store in `configurations/` folder
4. To load: **Profiles** menu → **Load Profile**

### Recommended Profile Names:
- `Sender_DigiMesh_9600_PL0.xpro` - Sender, 9600 baud, low power
- `Sender_DigiMesh_115200_PL4.xpro` - Sender, high speed, max power
- `Receiver_DigiMesh_9600.xpro` - Receiver, 9600 baud

---

## Verification Testing

### Terminal Test (Built into XCTU)

1. Click **Console** button (terminal icon)
2. **Sender:** Type text, press Enter
3. **Receiver:** Open second XCTU instance, open Console
4. Text should appear on Receiver console

✅ **Success:** Characters appear correctly
❌ **Failure:** Gibberish or no data → Check ID/HP match, verify baud rate

![First XBee Transmission — XCTU Console](images/First_Xbee%20Transmission.PNG)

*Successful first transmission in the XCTU console: sent characters appearing on the receiver side confirms the radio link is working end-to-end.*

![Live Telemetry Packet Stream](images/Sent%20packet%20screenshot.PNG)

*Live CSV telemetry packets streaming from the rocket-side XBee into the ground station. Each line is one 25-field telemetry frame at 50 Hz.*

![First XBee Transmission — XCTU Console](images/First_Xbee%20Transmission.PNG)

*Successful first transmission in the XCTU console: sent characters appearing on the receiver side confirms the radio link is working end-to-end.*

![Live Telemetry Packet Stream](images/Sent%20packet%20screenshot.PNG)

*Live CSV telemetry packets streaming from the rocket-side XBee into the ground station. Each line is one 25-field telemetry frame at 50 Hz.*

---

## Troubleshooting Configuration Issues

### Problem: XCTU Can't Discover Module

**Solutions:**
- Verify switch is in USB position (if using shield)
- Check RESET-to-GND jumper installed (if using shield)
- Try different baud rates during discovery
- Check Device Manager for COM port conflicts

### Problem: Module Discovered But Can't Read/Write

**Solutions:**
- Module may be in API mode from previous config
- Try different AP modes during discovery
- Factory reset: Hold CONFIG button during power-up (if available)
- Re-flash firmware completely

### Problem: Settings Won't Stick

**Solutions:**
- Verify Write command completes without errors
- Check for firmware/parameter compatibility
- Ensure sufficient power during write operation
- Try updating firmware first

### Problem: Gibberish Data After Configuration

**Solutions:**
- Arduino baud rate doesn't match BD parameter
- Change BD to match your `Serial.begin()` value
- Common mismatch: BD=3 (9600) but code uses 115200

---

## Common Baud Rate Values

| BD Value | Baud Rate | Use Case |
|----------|-----------|----------|
| `0` | 1200 | Ultra-low speed, legacy |
| `1` | 2400 | Low speed |
| `2` | 4800 | Low speed |
| `3` | **9600** | **Standard Arduino** |
| `4` | 19200 | Medium speed |
| `5` | 38400 | Medium-high speed |
| `6` | 57600 | High speed |
| `7` | **115200** | **High-speed ESP32** |
| `8` | 230400 | Very high speed (may be unstable) |

---

## Best Practices

1. ✅ **Always save profiles** after successful configuration
2. ✅ **Label modules** (Sender/Receiver) physically with tape
3. ✅ **Start with low power** (PL=0) for bench testing
4. ✅ **Match baud rates** between XCTU config and code
5. ✅ **Document ID/HP values** for your project
6. ✅ **Keep firmware updated** to latest stable version
7. ✅ **Test with XCTU console** before deploying code

---

## Next Steps

- [Hardware Setup Guide](HARDWARE_SETUP.md)
- [Code Examples](code_examples/)
- [SPI Troubleshooting](SPI_TROUBLESHOOTING.md)
- [Range Testing Guide](RANGE_TESTING.md)
