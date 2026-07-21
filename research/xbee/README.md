# Project Documentation: High-Speed Rocket Telemetry (XBee Pro 900HP)

This is a comprehensive technical documentation package covering the hardware, configuration, protocols, code, and engineering decisions made during XBee integration for rocket telemetry.

---

## 1. Hardware Overview: XBee-PRO 900HP (S3B)

The core communication module is the **Digi XBee-PRO 900HP (S3B)**. It was selected for its long-range capabilities and high data throughput required for supersonic (Mach 1) flight dynamics.

### **Specifications**

* **Frequency:** 902-928 MHz (ISM Band).
* **Maximum Range:** Up to 28 miles (45 km) Line-of-Sight with high-gain antennas.
* **Data Rate (Air):** 200 kbps (Selected for high-speed telemetry).
* **Transmit Power:** Up to 250mW (+24 dBm).
* **Supply Voltage:** 2.1 to 3.6VDC (**Critical:** 3.3V nominal).
* **Transmit Current:** ~215 mA (Requires a robust power supply).
* **Receive Current:** ~29 mA.

### **Key Pinout (THT Footprint)**

| Pin # | Name | Function | Note |
| --- | --- | --- | --- |
| **1** | **VCC** | Power Supply | Must be 3.3V. Add 100µF capacitor for stability. |
| **2** | **DOUT** | UART Data Out | TX from XBee → RX on MCU. |
| **3** | **DIN** | UART Data In | RX on XBee ← TX on MCU. |
| **4** | **SPI_MISO** | SPI Data Out | *Not used in final UART config.* |
| **5** | **RESET** | Module Reset | Pull Low to reset. Leave floating if unused. |
| **6** | **PWM0/RSSI** | Signal Strength | PWM output indicating signal quality. |
| **10** | **GND** | Ground | Common Ground is critical. |
| **11** | **SPI_MOSI** | SPI Data In | *Not used in final UART config.* |
| **17** | **SPI_SSEL** | Chip Select | *Not used in final UART config.* |
| **18** | **SPI_CLK** | Clock | *Not used in final UART config.* |
| **20** | **AD0/DIO0** | Commissioning | Button used for recovery/network formation. |

---

## 2. Configuration & XCTU Setup

All configuration is performed using **Digi XCTU** software.

### **Connecting to XCTU**

There are two ways to connect the XBee to the PC:

1. **USB Adapter:** A dedicated USB explorer board.
2. **Arduino Shield Pass-Through:** (See Section 3).

### **Required Configuration (High-Speed Transparent Mode)**

To enable 50Hz updates for the rocket, we use **Transparent Mode (AT)** at high baud rates.

* **Firmware:** XBee-PRO 900HP 200K (Must be the "200K" variant, not "10K").
* **AP (API Enable):** `0` (Transparent Mode). *Crucial for CSV text transmission.*
* **BD (Baud Rate):** `7` (115200 baud). *Ensures serial pipe is faster than radio link.*
* **PL (Power Level):** `4` (Highest).
* **ID (PAN ID):** [User Choice, e.g., 7777]. *Sender and Receiver must match.*
* **HP (Preamble ID):** [User Choice]. *Sender and Receiver must match.*

### **Disabling SPI (Conflict Resolution)**

To prevent the "floating pin" issues encountered during research, we explicitly disable SPI pins to ensure UART takes priority.

* **D1 (DIO1):** `0` (Disabled).
* **D2 (DIO2):** `0` (Disabled).
* **D3 (DIO3):** `0` (Disabled).
* **D4 (DIO4):** `0` (Disabled).
* **P2 (DIO12):** `0` (Disabled).

---

## 3. The XBee Shield & Arduino Interface

We utilized an Arduino/XBee Shield for prototyping and configuration.

### **The "Switch" (USB vs. XBee)**

The shield contains a toggle switch that routes the data lines:

* **USB Position:** Connects XBee DIN/DOUT directly to the USB-to-Serial chip on the Arduino.
  * *Usage:* Connecting to **XCTU** to configure the XBee.

* **XBee Position:** Connects XBee DIN/DOUT to the Arduino's Digital Pins (usually D2/D3 or D0/D1).
  * *Usage:* Flight mode. The Arduino talks to the XBee.

### **Technique: Arduino "Pass-Through" (Connect XBee to XCTU)**

If you do not have a USB Explorer, you can use an Arduino Uno as a bridge to XCTU:

1. **Hardware:** Mount Shield on Arduino.
2. **Jumper:** Connect **RESET** pin on Arduino to **GND**. (This bypasses the ATmega microcontroller).
3. **Switch:** Set Shield switch to **USB**.
4. **Action:** Open XCTU. The XBee will appear on the Arduino's COM port.

---

## 4. Integration: ESP32 High-Speed UART

For the flight computer, we transitioned from Arduino Uno to **ESP32** for higher processing speed. We implemented **Hardware UART** via the ESP32 GPIO Matrix.

### **Wiring Diagram (Flight Computer)**

| ESP32 Pin | XBee Pin | Function | Direction |
| --- | --- | --- | --- |
| **3V3** | **Pin 1** | Power | - |
| **GND** | **Pin 10** | Ground | - |
| **GPIO 34** | **Pin 2 (DOUT)** | RX | XBee → ESP32 |
| **GPIO 32** | **Pin 3 (DIN)** | TX | ESP32 → XBee |

*Note: GPIO 34 is Input-Only, making it ideal for RX protection.*

### **Sender Code (Rocket)**

Transmits CSV telemetry at 50Hz (20ms intervals).

```cpp
#include <HardwareSerial.h>

#define RX_PIN 34
#define TX_PIN 32
HardwareSerial XBeeSerial(2);

void setup() {
  Serial.begin(115200);
  // High-Speed UART Setup
  XBeeSerial.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN);
}

void loop() {
  static unsigned long lastTx = 0;
  if (millis() - lastTx >= 20) { // 50Hz
    lastTx = millis();
    
    // Format: timestamp,state,altitude,velocity,accel,voltage
    String packet = String(millis()) + ",1,1200.5,340.0,15.2,4.2";
    XBeeSerial.println(packet);
  }
}
```

### **Receiver Code (Ground Station)**

Receives and parses CSV data.

```cpp
#include <HardwareSerial.h>

#define RX_PIN 34
#define TX_PIN 32
HardwareSerial XBeeSerial(2);

void setup() {
  Serial.begin(115200);
  XBeeSerial.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN);
}

void loop() {
  if (XBeeSerial.available()) {
    String data = XBeeSerial.readStringUntil('\n');
    data.trim();
    if(data.length() > 0) {
      Serial.print("RX: ");
      Serial.println(data);
    }
  }
}
```

---

## 5. Research & Troubleshooting Log (The Journey)

This section documents the engineering decisions made during development.

### **Phase 1: Speed Constraints**

* **Initial Idea:** Reduce rate to 10kbps for maximum range.
* **Pivot:** Rejected. Rocket reaches Mach 1 (~340 m/s). At 10kbps (10Hz), the rocket travels 34 meters between data points.
* **Resolution:** Switch to **200kbps firmware**. This allows ~50-60Hz updates, providing 5-6 meter resolution during ascent.

### **Phase 2: Protocol Selection (SPI vs. UART)**

* **Attempt 1 (SPI):** We attempted to use VSPI on ESP32 (Pins 5, 18, 19, 23).
* **The Failure:** The Receiver XBee produced "All Zeros" or "All FFs" on the MISO line.
  * *Cause:* The XBee S3B has a conflict where it defaults to UART if pins are not strictly managed. Additionally, SPI requires 5 synchronized wires, which proved unstable under high-G simulation (jumper wires).

* **The Fix:** We pivoted to **High-Speed UART**.
  * UART requires only 2 wires (RX/TX).
  * It is asynchronous (no clock synchronization issues).
  * We utilized the ESP32's ability to remap Hardware Serial to any pins, repurposing the soldered SPI wires (GPIO 34/32) for UART.

### **Phase 3: Data Formatting**

* **Attempt 1 (Binary Structs):** Efficient but difficult to debug without a custom parser.
* **Resolution (CSV):** Switched to Comma Separated Values (Text).
  * *Pros:* Readable by humans, Excel, and standard Serial Plotters.
  * *Cons:* Slightly higher data usage, but well within the 200kbps bandwidth limit of the XBee Pro 900HP.

---

## 6. Directory Structure

```
xbee/
├── README.md                    # This file - Complete technical documentation
├── datasheets/                  # XBee Pro 900HP datasheets and specs
├── images/                      # Wiring diagrams, configuration screenshots
├── configurations/              # XCTU profile exports (.xpro files)
└── code_examples/              # Arduino/ESP32 example sketches
```

---

## 7. Quick Reference

### **Critical Configuration Checklist**

- [ ] Firmware: XBee-PRO 900HP **200K** variant
- [ ] AP = 0 (Transparent Mode)
- [ ] BD = 7 (115200 baud)
- [ ] PL = 4 (Maximum power)
- [ ] ID = Same on Sender and Receiver
- [ ] HP = Same on Sender and Receiver
- [ ] SPI pins disabled (D1, D2, D3, D4, P2 = 0)
- [ ] 100µF capacitor on VCC
- [ ] Common ground between ESP32 and XBee

### **Typical Data Rate Achieved**

* **Update Rate:** 50 Hz (20ms intervals)
* **Packet Size:** ~40 bytes (CSV format)
* **Bandwidth Used:** 16 kbps (well within 200 kbps limit)
* **Range:** Tested up to 2 km with line-of-sight
* **Latency:** < 50ms typical

### **Troubleshooting**

**Problem:** No data received
- Check PAN ID and HP match on both modules
- Verify baud rate is 115200 on both UART and XBee config
- Ensure SPI pins are disabled
- Check power supply (3.3V, adequate current)

**Problem:** Garbled data
- Reduce baud rate to 57600 and test
- Check for loose connections
- Verify firmware is "200K" variant

**Problem:** Short range
- Increase PL (Power Level) to 4
- Check antenna connections
- Ensure clear line-of-sight

---

## 8. Next Steps

- [ ] Add wiring diagrams to `images/`
- [ ] Upload XBee Pro 900HP datasheet to `datasheets/`
- [ ] Export XCTU profiles to `configurations/`
- [ ] Create code examples folder with sender/receiver sketches
- [ ] Document power supply requirements
- [ ] Add range test results and plots

---

## 9. Related Documentation

- [Main Project README](../../README.md)
- [Research Overview](../README.md)
- [Arduino Code Examples](../arduino_code/)
- [Range Test Results](../range_tests/)

---

**Last Updated:** 2026-01-17  
**Status:** Documentation Overview Complete - Awaiting images and datasheets
