# XBee V03 Shield & S3B Hardware Setup

## Hardware Components

### XBee Module
- **Model:** XBee-PRO 900HP (S3B)
- **Frequency:** 902-928 MHz ISM Band
- **Firmware:** DigiMesh 900HP (NOT XSC legacy firmware)

### Shield
- **Model:** Arduino XBee Shield V03
- **Tutorial:** [Arduino XBee Shield Guide](https://docs.arduino.cc/retired/shields/arduino-xbee-shield/)

## V03 Shield Features

### The Switch (Critical Understanding)

The V03 Shield has a toggle switch that routes UART data lines:

**USB Position:**
- Connects XBee DIN/DOUT directly to USB-to-Serial chip on Arduino
- **Use Case:** Configuring XBee with XCTU
- **Requirement:** Arduino RESET pin must be jumpered to GND to bypass MCU

**XBee Position:**
- Connects XBee DIN/DOUT to Arduino Digital Pins (D0/D1)
- **Use Case:** Normal flight operation
- **Issue:** May conflict with Serial Monitor (D0/D1 are hardware serial)

![XBee Shield V03 — USB/XBee Mode Switch](images/XBEE_Shield_Module.jpeg)

*The slide switch on the V03 Shield selects between USB passthrough (for XCTU configuration) and microcontroller UART (for flight operation). Always flip back to XBee position before connecting the ESP32.*

### Power Regulation

The V03 Shield includes:
- **3.3V Voltage Regulator** for XBee
- **Robust Power Supply** capable of handling 215mA TX current
- **Power LED** indicator

⚠️ **Critical:** The shield's voltage regulator is more reliable than ESP32's 3.3V pin for powering XBee modules.

---

## Wiring for XCTU Configuration

### Method 1: Using Arduino as Pass-Through

1. **Mount Shield** on Arduino Uno
2. **Jumper Wire:** Connect Arduino **RESET** pin to **GND**
   - This bypasses the ATmega328 microcontroller
   - Makes Arduino act as pure USB-to-Serial adapter
3. **Switch Position:** Set to **USB**
4. **Power:** Connect Arduino to PC via USB
5. **XCTU:** Open XCTU, XBee will appear on Arduino's COM port

### Method 2: Direct USB Explorer (Alternative)

If you have a dedicated XBee USB Explorer board, use that instead.

---

## Wiring for SoftwareSerial (Arduino Uno Receiver)

When using the V03 Shield with routing issues, bypass it and wire directly:

### Connections:

| XBee Pin | Arduino Pin | Function |
|----------|-------------|----------|
| **Pin 1 (VCC)** | 3.3V (from Shield) | Power |
| **Pin 10 (GND)** | GND | Ground |
| **Pin 2 (DOUT)** | Digital Pin 2 | RX (Receiver) |
| **Pin 3 (DIN)** | Digital Pin 3 | TX (Transmitter) |

**Note:** Power XBee from Shield's 3.3V regulator, but use jumper wires for data (Pins 2 & 3).

![XBee Pro 900HP Pinout](images/xbee_pinout.jpg)

*XBee Pro 900HP S3B pin layout — Pin 2 (DOUT) is TX from XBee to host MCU; Pin 3 (DIN) is RX into XBee from host MCU.*

---

## Hybrid Wiring for ESP32 SPI Testing

This configuration uses:
- **Shield:** Provides robust 3.3V power
- **ESP32:** Provides SPI data control

### Power Connections:
- Shield powered via 5V USB or barrel jack
- **CRITICAL:** Connect **ESP32 GND** to **Shield GND** (common ground required)

### Data Connections (ESP32 VSPI to XBee):

| XBee Header Pin | Function | ESP32 Pin (VSPI) |
|-----------------|----------|------------------|
| **Pin 17 (DIO3)** | CS (Chip Select) | **GPIO 5** |
| **Pin 11 (DIO4)** | MOSI (Master Out) | **GPIO 23** |
| **Pin 4 (DIO12)** | MISO (Master In) | **GPIO 19** |
| **Pin 18 (DIO2)** | SCK (Clock) | **GPIO 18** |
| **Pin 19 (DIO1)** | ATTN (Attention) | **GPIO 4** |

⚠️ **3.3V Compatibility:** ESP32 GPIO pins are 3.3V tolerant, matching XBee logic levels. The shield's voltage regulator provides clean 3.3V power for the XBee module.

---

## ESP32 Power Considerations

### Why NOT Power XBee from ESP32 3.3V Pin:

1. **Current Limitation:** ESP32's 3.3V pin can supply ~600mA max
2. **XBee Requirements:** 
   - Transmit: 215mA typical
   - Peak inrush: Can spike higher
   - Shield's regulator designed specifically for XBee loads

3. **Voltage Stability:**
   - ESP32 3.3V can drop under load
   - Shield regulator maintains stable 3.3V
   - Brown-out causes XBee resets/corrupted data

### Recommended Power Strategy:

✅ **Use Shield's Voltage Regulator:**
- Connect 5V source to Shield
- Shield provides clean, stable 3.3V to XBee
- ESP32 only handles data signals (no power load)

✅ **Common Ground:**
- Always connect ESP32 GND to Shield GND
- Required for proper signal reference

---

## Switch Position Quick Reference

| Task | Switch Position | Notes |
|------|----------------|-------|
| Configure with XCTU | **USB** | Jumper RESET to GND |
| Flight mode (Arduino) | **XBee** | Remove RESET jumper |
| Bypass shield (Direct wiring) | N/A | Wire directly to pins |
| ESP32 SPI hybrid | N/A | Power from shield, data from ESP32 |

---

## Troubleshooting Power Issues

### Symptom: XBee keeps resetting
- Check Shield's 3.3V LED is ON
- Measure voltage at XBee Pin 1: Should be 3.3V ±0.1V
- Add 100µF capacitor across VCC/GND if not present

### Symptom: Inconsistent transmission
- Verify Shield's voltage regulator rating (should be ≥500mA)
- Check for voltage drop during TX burst
- Use oscilloscope if available

### Symptom: XCTU can't connect
- Verify switch is in **USB** position
- Confirm RESET-to-GND jumper installed
- Check COM port in Device Manager
- Try different USB cable/port

---

## Best Practices

1. **Always use Shield's voltage regulator** for XBee power
2. **Never forget common ground** when mixing power sources
3. **Test power stability** before SPI/UART testing
4. **Keep wires short** for SPI connections (< 6 inches ideal)
5. **Add decoupling capacitor** (100µF) near XBee VCC if not present on shield

---

**See also:**
- [XCTU Configuration Guide](XCTU_CONFIGURATION.md)
- [SPI Troubleshooting](SPI_TROUBLESHOOTING.md)
- [Code Examples](code_examples/)
