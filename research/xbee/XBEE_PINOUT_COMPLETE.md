# XBee Pro 900HP Complete Pinout Reference

## Pin Diagram Overview

```
     ┌─────────────────┐
  1  │● VCC      GND ●│ 10
  2  │● DOUT    /RTS ●│ 9
  3  │● DIN     /CTS ●│ 8
  4  │● DIO12   SLEEP●│ 7
  5  │● /RESET   PWM0●│ 6
     ├─────────────────┤
 11  │● DIO4    DIO13●│ 20
 12  │● /CTS    DIO14●│ 19
 13  │● ON/SLEEP DIO1●│ 18
 14  │● VREF     DIO0●│ 17
 15  │● Assoc   DIO3 ●│ 16
     └─────────────────┘
     
     XBee Pro 900HP S3B
     (Through-Hole Module)
```

![XBee Pro 900HP Physical Pinout](images/xbee_pinout.jpg)

*Physical pin layout of the XBee Pro 900HP S3B module. Pin 1 (VCC) is top-left; Pin 10 (GND) is top-right; UART pins DOUT (2) and DIN (3) are on the left side.*

![XBee Pro 900HP Physical Pinout](images/xbee_pinout.jpg)

*Physical pin layout of the XBee Pro 900HP S3B module. Pin 1 (VCC) is top-left; Pin 10 (GND) is top-right; UART pins DOUT (2) and DIN (3) are on the left side.*

---

## Complete Pin Reference

### Top Row (Pins 1-10)

| Pin | Name | Function | I/O | Description | Common Use |
|-----|------|----------|-----|-------------|-----------|
| **1** | **VCC** | Power Supply | Input | 3.3V power input (2.1V - 3.6V) | Connect to 3.3V regulator |
| **2** | **DOUT** | Data Out (TX) | Output | UART transmit pin | ESP32 RX pin (GPIO 34) |
| **3** | **DIN** | Data In (RX) | Input | UART receive pin | ESP32 TX pin (GPIO 32) |
| **4** | **DIO12** | Digital I/O 12 | Both | SPI_MISO when P2=1, GPIO otherwise | SPI: ESP32 GPIO 19 (MISO) |
| **5** | **/RESET** | Module Reset | Input | Active-low reset, pull low to reset | Usually left floating or pullup |
| **6** | **PWM0** | PWM Output 0 | Output | Configurable PWM output | Motor control, servo signals |
| **7** | **SLEEP** | Sleep Indicator | Output | Low when awake, high when sleeping | Power management indicator |
| **8** | **/CTS** | Clear To Send | Output | UART flow control (active low) | ESP32 GPIO (optional) |
| **9** | **/RTS** | Request To Send | Input | UART flow control (active low) | ESP32 GPIO (optional) |
| **10** | **GND** | Ground | - | 0V ground reference | Connect to ESP32/Arduino GND |

### Bottom Row (Pins 11-20)

| Pin | Name | Function | I/O | Description | Common Use |
|-----|------|----------|-----|-------------|-----------|
| **11** | **DIO4** | Digital I/O 4 | Both | SPI_MOSI when D4=1, GPIO otherwise | SPI: ESP32 GPIO 23 (MOSI) |
| **12** | **/CTS** | Clear To Send | Output | Duplicate of Pin 8 | Not used (duplicate) |
| **13** | **ON/SLEEP** | Sleep Control | Input | Pull low to force sleep mode | Usually left floating |
| **14** | **VREF** | Voltage Reference | Output | Output of internal 1.2V reference | ADC reference (optional) |
| **15** | **Assoc** | Association LED | Output | Low when associated with network | LED indicator (optional) |
| **16** | **DIO3** | Digital I/O 3 | Both | SPI_SSEL (CS) when D3=1, GPIO otherwise | SPI: ESP32 GPIO 5 (CS) |
| **17** | **DIO0** | Digital I/O 0 | Both | General purpose I/O, PWM capable | LED, relay control |
| **18** | **DIO2** | Digital I/O 2 | Both | SPI_CLK when D2=1, GPIO otherwise | SPI: ESP32 GPIO 18 (SCK) |
| **19** | **DIO1** | Digital I/O 1 | Both | SPI_ATTN when D1=5, GPIO otherwise | SPI: ESP32 GPIO 4 (ATTN) |
| **20** | **DIO13** | Digital I/O 13 | Both | DOUT duplicate when P3=1, GPIO otherwise | Usually not used |

---

## Pin Configuration Modes

### Mode 1: UART Transparent (Production - Recommended)

**Use Case:** Simple CSV/text transmission, 115200 baud, minimal wiring

**XCTU Settings:**
```
AP = 0   (Transparent Mode)
BD = 7   (115200 baud)
P3 = 1   (DOUT enabled)
P4 = 1   (DIN enabled)
D1-D4 = 0 (Disable SPI)
P2 = 0   (Disable SPI_MISO)
```

**Wiring (4 wires):**
```
Pin 1  (VCC)  -> 3.3V
Pin 2  (DOUT) -> ESP32 GPIO 34 (RX)
Pin 3  (DIN)  -> ESP32 GPIO 32 (TX)
Pin 10 (GND)  -> GND
```

**Bandwidth:** 115200 baud = ~11.5 KB/s theoretical, ~8 KB/s practical

---

### Mode 2: SPI with API Frames (High Performance)

**Use Case:** High-speed binary data, full-duplex, 1 MHz SPI clock

**XCTU Settings:**
```
AP = 1   (API Mode - REQUIRED)
D1 = 5   (SPI_ATTN output)
D2 = 1   (SPI_CLK input)
D3 = 1   (SPI_SSEL input)
D4 = 1   (SPI_MOSI input)
P2 = 1   (SPI_MISO output)
P3 = 0   (UART DOUT disabled)
P4 = 0   (UART DIN disabled)
```

**Wiring (7 wires):**
```
Pin 1  (VCC)   -> 3.3V
Pin 4  (DIO12) -> ESP32 GPIO 19 (MISO)
Pin 10 (GND)   -> GND
Pin 11 (DIO4)  -> ESP32 GPIO 23 (MOSI)
Pin 16 (DIO3)  -> ESP32 GPIO 5  (CS)
Pin 18 (DIO2)  -> ESP32 GPIO 18 (SCK)
Pin 19 (DIO1)  -> ESP32 GPIO 4  (ATTN)
```

**Bandwidth:** 1 MHz SPI = ~125 KB/s theoretical, ~80 KB/s practical

---

### Mode 3: Arduino Pass-Through (XCTU Configuration)

**Use Case:** Program XBee using XCTU when in SPI mode (rescue mode)

**Wiring (Shield Method):**
1. XBee inserted in Arduino XBee Shield V03
2. Shield switch set to "USB" position
3. Arduino connected to PC via USB
4. XCTU communicates through Arduino's USB-Serial bridge

**Shield Switch Positions:**
- **USB:** Arduino Serial ↔ XBee (for XCTU)
- **XBee:** Arduino pins 0/1 ↔ XBee (for sketches using Serial)

---

## Power Specifications

| Parameter | Min | Typical | Max | Unit | Notes |
|-----------|-----|---------|-----|------|-------|
| **Supply Voltage** | 2.1 | 3.3 | 3.6 | V | Outside this range = damage |
| **TX Current** | - | 215 | 250 | mA | At maximum power (PL=4) |
| **RX Current** | - | 35 | 45 | mA | Listening for packets |
| **Sleep Current** | - | 15 | 25 | µA | Deep sleep mode enabled |
| **Peak Inrush** | - | 300 | 400 | mA | First 10ms after power-on |

**Critical:** ESP32 3.3V pin typically provides only ~600mA total. With XBee drawing 215mA TX + ESP32 peripherals, this leaves little margin. **Always use Arduino Shield's dedicated 3.3V regulator** (rated 800mA+) or external 3.3V supply.

---

## Pin Voltage Levels

| Pin Type | Logic Low | Logic High | Absolute Max | Damage Threshold |
|----------|-----------|------------|--------------|------------------|
| **Input** | 0V - 0.6V | 2.0V - 3.6V | -0.5V to +4.0V | >4.0V |
| **Output** | 0V - 0.4V | 2.7V - 3.3V | - | - |

**5V Tolerance:** XBee pins are **NOT 5V tolerant**. Direct connection to Arduino 5V logic (Uno, Mega) will destroy the module. Always use level shifters or Arduino XBee Shield (has built-in level shifters).

---

## Unused Pin Recommendations

| Pin | Name | Safe State | Reason |
|-----|------|-----------|---------|
| 5 | /RESET | Floating or pullup | Module won't reset accidentally |
| 6 | PWM0 | Floating | No harm if not used |
| 7 | SLEEP | Ignore (output) | Module drives this, don't connect |
| 8, 9 | /CTS, /RTS | Floating | Flow control not needed for 50Hz |
| 12 | /CTS (dup) | Floating | Duplicate of pin 8 |
| 13 | ON/SLEEP | Floating or pullup | Module stays awake |
| 14 | VREF | Floating | Internal reference, no external use |
| 15 | Assoc | Optional LED | Blinks when network joined |
| 17 | DIO0 | Floating | General I/O, safe if unused |
| 20 | DIO13 | Floating | Duplicate DOUT, not needed |

**Best Practice:** Leave all unused pins floating (not connected). Do not tie to GND or VCC unless datasheet specifies.

---

## RF Antenna (Critical)

**Antenna Type:** Integrated PCB trace antenna (on S3B module) or external connector (on some variants)

**Orientation:** For maximum range, mount XBee **vertically** with antenna pointing up. Signal strength drops 20-30% when horizontal.

**Frequency:** 902 - 928 MHz (ISM band, unlicensed in USA)

**Range:**
- **Line of Sight:** Up to 28 miles (45 km) with clear path
- **Urban:** 1-3 km typical (buildings interfere)
- **Rocket Flight:** 2-5 km tested (moving target reduces reliability)

**Obstructions:** Metal enclosures, carbon fiber, and dense materials block 900 MHz signals. Mount XBee near non-conductive surface (plastic, fiberglass).

---

## Common Wiring Mistakes

### ❌ Wrong: ESP32 3.3V Pin Powering XBee
**Problem:** 215mA TX current causes voltage sag, brownouts, resets  
**Fix:** Use Arduino Shield regulator or external 3.3V supply (500mA+)

### ❌ Wrong: Direct Arduino 5V to XBee Pins
**Problem:** Destroys XBee input pins (not 5V tolerant)  
**Fix:** Use Arduino XBee Shield (has level shifters) or external level shifter ICs

### ❌ Wrong: No Common Ground Between ESP32 and XBee
**Problem:** Data signals reference different voltages, communication fails  
**Fix:** Always connect Pin 10 (GND) to ESP32 GND

### ❌ Wrong: Long Jumper Wires in SPI Mode (>10cm)
**Problem:** 1 MHz clock signal corrupted, MISO reads garbage  
**Fix:** Use short wires (<5cm) or switch to UART mode

### ❌ Wrong: UART and SPI Pins Enabled Simultaneously
**Problem:** XBee defaults to UART, ignores SPI commands  
**Fix:** Set P3=0, P4=0 in XCTU when using SPI mode

---

## Pin Usage Summary by Mode

| Pin | Name | UART Mode | SPI Mode | Both Modes |
|-----|------|-----------|----------|------------|
| 1 | VCC | Power ✓ | Power ✓ | Power ✓ |
| 2 | DOUT | ESP32 RX ✓ | Not used | - |
| 3 | DIN | ESP32 TX ✓ | Not used | - |
| 4 | DIO12 | Not used | MISO ✓ | - |
| 5 | /RESET | Floating | Floating | Floating |
| 6 | PWM0 | Floating | Floating | Floating |
| 7 | SLEEP | Ignore | Ignore | Ignore |
| 8 | /CTS | Optional | Not used | - |
| 9 | /RTS | Optional | Not used | - |
| 10 | GND | Ground ✓ | Ground ✓ | Ground ✓ |
| 11 | DIO4 | Not used | MOSI ✓ | - |
| 12-15 | Various | Floating | Floating | Floating |
| 16 | DIO3 | Not used | CS ✓ | - |
| 17 | DIO0 | Floating | Floating | Floating |
| 18 | DIO2 | Not used | SCK ✓ | - |
| 19 | DIO1 | Not used | ATTN ✓ | - |
| 20 | DIO13 | Floating | Floating | Floating |

**UART Mode:** 4 wires connected (VCC, GND, DOUT, DIN)  
**SPI Mode:** 7 wires connected (VCC, GND, MISO, MOSI, CS, SCK, ATTN)

---

## Quick Reference Card (Print This)

```
═══════════════════════════════════════════════════════
               XBee Pro 900HP Pinout
═══════════════════════════════════════════════════════

POWER (Always Required):
  Pin 1  (VCC)  -> 3.3V (use Shield regulator!)
  Pin 10 (GND)  -> Ground

UART MODE (Transparent - Recommended for Rockets):
  Pin 2  (DOUT) -> ESP32 GPIO 34 (RX)
  Pin 3  (DIN)  -> ESP32 GPIO 32 (TX)
  
  XCTU: AP=0, BD=7, P3=1, P4=1

SPI MODE (High Performance - Complex):
  Pin 4  (DIO12) -> ESP32 GPIO 19 (MISO)
  Pin 11 (DIO4)  -> ESP32 GPIO 23 (MOSI)
  Pin 16 (DIO3)  -> ESP32 GPIO 5  (CS)
  Pin 18 (DIO2)  -> ESP32 GPIO 18 (SCK)
  Pin 19 (DIO1)  -> ESP32 GPIO 4  (ATTN)
  
  XCTU: AP=1, D1=5, D2-D4=1, P2=1, P3=0, P4=0

CRITICAL SETTINGS (Both Modes):
  ID = 7777 (Network ID - must match)
  HP = 0    (Preamble - must match)
  DL = FFFF (Broadcast destination)

═══════════════════════════════════════════════════════
```

---

## Related Documentation

- **UART Production Code:** See `code_examples/uart_production/`
- **SPI Code Examples:** See `code_examples/spi_sender_esp32/`
- **XCTU Configuration:** See `XCTU_CONFIGURATION.md`
- **Hardware Setup:** See `HARDWARE_SETUP.md`
- **Troubleshooting:** See `SPI_TROUBLESHOOTING.md`

---

**Last Updated:** January 17, 2026  
**Module:** XBee-PRO 900HP (S3B)  
**Datasheet:** [Digi XBee-PRO 900HP Product Manual](https://www.digi.com/resources/documentation/digidocs/pdfs/90002173.pdf)
