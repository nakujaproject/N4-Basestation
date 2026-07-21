# XBee Code Examples

This folder contains working Arduino/ESP32 sketches for XBee Pro 900HP communication testing.

## 📁 Folder Structure (Categorized by Protocol)

```
code_examples/
│
├── 📂 UART MODE (Recommended for Production)
│   ├── uart_production/              ⭐ FINAL FLIGHT-READY CODE
│   │   ├── uart_csv_sender_rocket/      # 50Hz CSV sender (ESP32)
│   │   └── uart_csv_receiver_ground/    # CSV parser & display (ESP32)
│   ├── uart_simple_sender/              # Text "Hello World" sender
│   ├── uart_simple_receiver/            # Text message receiver
│   └── arduino_uart_receiver/           # Binary struct UART receiver
│
├── 📂 SPI MODE (High Performance - Complex)
│   ├── spi_sender_esp32/                # 10Hz binary telemetry sender
│   ├── spi_receiver_esp32/              # Binary struct SPI receiver
│   └── spi_rescue_programmer/           # ⚠️ Recovery tool (restore UART)
│
└── README.md (this file)
```

---

## ⭐ PRODUCTION CODE (Use This for Flight)

### UART CSV Mode - 50Hz Telemetry

**Location:** `uart_production/`

**Sender (Rocket):** `uart_csv_sender_rocket.ino`  
**Receiver (Ground):** `uart_csv_receiver_ground.ino`

**Why This is Best:**
- ✅ Simple: 4-wire connection (VCC, GND, TX, RX)
- ✅ Reliable: Hardware UART handles all framing
- ✅ Fast: 115200 baud = 50Hz CSV updates
- ✅ Debuggable: CSV data readable in Serial Monitor
- ✅ Flight-Proven: Used in Nakuja rocket flights

**XBee Config (XCTU):**
```
AP = 0   (Transparent Mode)
BD = 7   (115200 baud)
P3 = 1   (UART DOUT enabled)
P4 = 1   (UART DIN enabled)
ID = 7777
HP = 0
```

**CSV Format:** `timestamp,state,altitude,velocity,accel_z,battery`  
**Example:** `1250,1,145.2,340.0,15.2,4.1`

---

## 🚀 Quick Start

### 1. Simple UART Test (Recommended First Test)

**Goal:** Verify basic XBee communication before attempting SPI.

**Sender:** Upload `uart_simple_sender.ino` to ESP32  
**Receiver:** Upload `uart_simple_receiver.ino` to Arduino Uno  

**XBee Config (XCTU):**
- Both modules: `ID=7777`, `HP=0`, `AP=0` (Transparent Mode)
- Sender: `BD=7` (115200 baud)
- Receiver: `BD=3` (9600 baud)

**Expected Result:** Arduino Serial Monitor shows "Hello World" messages.

---

### 2. Binary UART Telemetry (Production Testing)

**Goal:** Test high-speed binary struct transmission over UART.

**Sender:** Use SPI sender code but configure XBee for UART  
**Receiver:** Upload `arduino_uart_receiver.ino` to Arduino Uno  

**XBee Config:**
- `AP=0` (Transparent Mode - not API!)
- `BD=3` (9600 baud) - matches sketch
- `ID=7777`, `HP=0`

**Expected Result:** Arduino parses 36-byte struct and prints telemetry values.

---

### 3. SPI Telemetry (High Performance)

**Goal:** Achieve maximum throughput with ESP32 ↔ XBee SPI interface.

**Sender:** Upload `spi_sender_esp32.ino` to ESP32  
**Receiver:** Upload `spi_receiver_esp32.ino` to ESP32  

**XBee Config (CRITICAL):**
```
AP = 1   (API Mode - REQUIRED for SPI)
D1 = 5   (SPI_ATTN)
D2 = 1   (SPI_CLK)
D3 = 1   (SPI_SSEL)
D4 = 1   (SPI_MOSI)
P2 = 1Protocol Comparison Table

| Feature | UART CSV (Production) | UART Binary | SPI Binary |
|---------|----------------------|-------------|------------|
| **Data Type** | Text (CSV) | Binary Struct | Binary Struct |
| **Speed** | 115200 baud | 115200 baud | 1 MHz SPI |
| **Wiring** | 4 wires | 4 wires | 7 wires |
| **Complexity** | Very Simple | Medium | Complex |
| **CPU Load** | Low | Medium | Low |
| **Use Case** | **Flight Production** | Testing | High-Speed Testing |
| **Max Update Rate** | 50 Hz | 20 Hz | 100+ Hz |
| **Debugging** | Easy (CSV readable) | Hard (binary) | Very Hard (API frames) |
| **XCTU Access** | ✅ Always available | ✅ Always available | ❌ Locked out (needs rescue) |
| **Reliability** | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐ | ⭐⭐⭐ |

**Recommendation:** Use **UART CSV (Production)** for rocket flights. SPI is for testing/learning only.
### 4. SPI Recovery Tool (Troubleshooting)

**Problem:** XBee is stuck in SPI mode, XCTU can't connect via USB.

**Solution:** Upload `spi_rescue_programmer.ino` to ESP32 connected to XBee via SPI.

**Steps:**
1. Wire ESP32 → XBee with SPI connections (see sketch comments)
2. Upload and run sketch
3. Wait for "DONE" message
4. Disconnect XBee from ESP32
5. Connect XBee to XCTU via USB adapter
6. XBee should now respond on UART (try 9600 baud)

**What it does:**
- Sends `ATP3 1` (Enable UART TX)
- Sends `ATP4 1` (Enable UART RX)
- Sends `ATWR` (Write to flash)
- Sends `ATFR` (Reboot XBee)

---

## 📊 Comparison Table

| Feature | UART Simple | UART Binary | SPI Binary |
|---------|-------------|-------------|------------|
| **Data Type** | Text | Binary Struct | Binary Struct |
| **Speed** | Low (9600 baud) | Medium (115200 baud) | High (1 MHz SPI) |
| **Complexity** | Very Simple | Medium | Complex |
| **CPU Load** | Low | Medium | Low |
| **Use Case** | Testing/Range | Production Backup | Primary Telemetry |
| **Max Update Rate** | 1 Hz | 20 Hz | 100+ Hz |
| **Buffer Issues** | Rare | Possible | Rare |

---

## 🔌 Wiring Reference

### ESP32 VSPI Pins (SPI Mode)
```
ESP32 GPIO 5  -> XBee Pin 17 (DIO3/CS)
ESP32 GPIO 18 -> XBee Pin 18 (DIO2/SCK)
ESP32 GPIO 19 -> XBee Pin 4  (DIO12/MISO)
ESP32 GPIO 23 -> XBee Pin 11 (DIO4/MOSI)
ESP32 GPIO 4  -> XBee Pin 19 (DIO1/ATTN)
ESP32 GND     -> XBee Pin 10 (GND)
ESP32 3V3     -> XBee Pin 1  (VCC) - Use Shield regulator if available
```

### ESP32 UART Pins
```
ESP32 GPIO 17 (TX) -> XBee Pin 3 (DIN/RX)
ESP32 GPIO 16 (RX) -> XBee Pin 2 (DOUT/TX)
ESP32 GND          -> XBee Pin 10 (GND)
```

### Arduino Uno UART Pins (SoftwareSerial)
```
Arduino Pin 3 (TX) -> XBee Pin 3 (DIN/RX)
Arduino Pin 2 (RX) -> XBee Pin 2 (DOUT/TX)
Arduino GND        -> XBee Pin 10 (GND)
Arduino 5V         -> XBee Shield 5V input
```

---

## ⚠️ Common Issues

### 1. "Weird Characters" on Serial Monitor (UART Mode)
**Cause:** XBee is sending binary data but Serial Monitor expects text.  
**Fix:** Use the binary receiver sketch, not `Serial.println()`.

### 2. ATTN Pin Always HIGH (SPI Mode)
**Cause:** XBee is still in UART mode (`P3/P4` enabled).  
**Fix:** Set `P3=0`, `P4=0` in XCTU, or use rescue programmer.

### 3. Receiver Gets Nothing (SPI Mode)
**Checklist:**
- [ ] XBee has `AP=1` (API Mode)
- [ ] XBee has `D1=5, D2=1, D3=1, D4=1, P2=1`
- [ ] XBee has `P3=0, P4=0` (UART disabled)
- [ ] Both XBees have matching `ID` and `HP`
- [ ] Common ground connected between ESP32 and XBee
- [ ] Power supply stable (XBee needs 215mA for TX)

### 4. XCTU Can't Find XBee After SPI Mode
**Fix:** Use `spi_rescue_programmer.ino` to restore UART.

---

## 📖 Further Reading

- **Hardware Setup:** See `../HARDWARE_SETUP.md`
- **XCTU Configuration:** See `../XCTU_CONFIGURATION.md`
- **SPI Troubleshooting:** See `../SPI_TROUBLESHOOTING.md`
- **Main Documentation:** See `../README.md`

--- (Text Messages)
   └─> uart_simple_sender/receiver 
       Goal: Verify basic XBee communication
       Time: 10 minutes

2. Verify Range
   └─> Test at 1m, 10m, 100m, 1km
       Goal: Ensure antenna orientation correct
       Time: 30 minutes

3. Deploy Production Code ⭐
   └─> uart_production/uart_csv_sender_rocket
       uart_production/uart_csv_receiver_ground
       Goal: Flight-ready 50Hz CSV telemetry
       Time: Ready to fly!

4. (Optional) Test Binary UART
   └─> arduino_uart_receiver (struct over UART)
       Goal: Compare binary vs CSV performance
       Time: 20 minutes

5. (Optional) Test High-Speed SPI
   └─> spi_sender/receiver (ESP32 only)
       Goal: Learn SPI protocol, 10Hz binary
       Time: 2+ hours (complex debugging)

6. If Stuck in SPI Mode
   └─> spi_rescue_programmer
       Goal: Restore UART access for XCTU
       Time: 5 minutes
4. Optimize for Speed
   └─> spi_sender/receiver (SPI mode)

5. If Stuck in SPI
   └─> spi_rescue_programmer (restore UART)
```

---

## 📝 Notes

- All sketches include detailed comments and wiring diagrams
- Struct definitions must match **exactly** between sender/receiver
- SPI mode requires API Mode (`AP=1`) on XBee
- UART mode works with Transparent Mode (`AP=0`) or API Mode (`AP=1`)
- SoftwareSerial on Arduino Uno is limited to 57600 baud max
- Hardware serial (Serial1/Serial2) is preferred for high-speed UART

---

**Last Updated:** January 17, 2026  
**XBee Model:** XBee-PRO 900HP (S3B)  
**Firmware:** DigiMesh 900HP  
**Tested With:** ESP32 DevKit, Arduino Uno R3
