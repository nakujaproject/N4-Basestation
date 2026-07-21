# SPI Troubleshooting & Development Journey

## Background

During development, we successfully implemented SPI protocol for communication between ESP32 and XBee S3B. This document records the complete debugging journey, critical breakthroughs, and final working implementation.

---

## Development Timeline

### Phase 1: Initial SPI Attempt (Failed)
**Problem:** "Hello World" test with Arduino receiver showed corrupted data in XCTU  
**Symptom:** `ello WorldHello WorldHello World...` (missing first character)  
**Root Cause:** Arduino's SoftwareSerial couldn't keep up with data rate  
**Lesson Learned:** UART limitations revealed need for hardware solution

### Phase 2: ESP32-to-ESP32 Implementation (Success)
**Solution:** Switched both sender and receiver to ESP32 with hardware SPI  
**Result:** Binary struct transmission working at 10Hz  
**Challenge:** Bluetooth forwarding not working initially (incomplete code)

### Phase 3: The "Weird Characters" Mystery (Critical Debugging)
**Problem:** Receiver showing unreadable binary garbage: `üÍÌL=ÃõA33sA@DÍÌHB33s@`  
**Diagnostic:** Data appeared when sender connected, stopped when disconnected  
**Critical Discovery:** `ATTN Pin State: HIGH (Idle)` - Pin never going LOW  
**Root Cause:** XBee still in UART Transparent Mode, not SPI Mode

### Phase 4: Configuration Fix (Breakthrough)
**Solution:** Properly configured XBee I/O pins in XCTU  
**Critical Settings:**
- **P3 (DOUT) = 0 (Disabled)** - Kills UART TX (stops garbage data)
- **P4 (DIN) = 0 (Disabled)** - Kills UART RX
- **D1 = 5 (SPI_ATTN)** - Enables Attention pin
- **D2, D3, D4, P2 = 1** - Enables SPI pins (CLK, SSEL, MOSI, MISO)

**Result:** ATTN pin started working, SPI frames detected (`7E` headers visible)

### Phase 5: The XCTU Lock-Out Problem
**New Challenge:** XBee in SPI mode can't be reconfigured via XCTU (UART disabled)  
**User Question:** "That will mean that we cant use xctu to configure the xbees again"  
**Solution Required:** SPI-based AT command programmer (rescue tool)

---

## Why SPI?

### Advantages Realized:
- **Higher Throughput:** Achieved 10Hz+ with 36-byte binary struct
- **Hardware Support:** XBee S3B SPI mode proven reliable
- **ESP32 Capability:** VSPI handles 1MHz cleanly
- **Direct Binary Transfer:** No CSV parsing overhead

### Challenges Overcome:
- **Pin Configuration:** Required strict XCTU I/O settings
- **Diagnostic Confusion:** UART vs SPI mode symptoms similar
- **Configuration Lock-Out:** Resolved with SPI rescue programmer
- **Common Ground:** Critical for reliable data transfer

---

## SPI Hardware Configuration

### ESP32 VSPI Pins

| Signal | ESP32 Pin | XBee Pin | Function |
|--------|-----------|----------|----------|
| **CS** | GPIO 5 | Pin 17 (DIO3) | Chip Select |
| **MOSI** | GPIO 23 | Pin 11 (DIO4) | Master Out, Slave In |
| **MISO** | GPIO 19 | Pin 4 (DIO12) | Master In, Slave Out |
| **SCK** | GPIO 18 | Pin 18 (DIO2) | Clock |
| **ATTN** | GPIO 4 | Pin 19 (DIO1) | Attention (Data Ready) |

### Power Configuration (Critical)

⚠️ **DO NOT power XBee from ESP32 3.3V pin directly**

**Recommended Hybrid Setup:**
1. **Power:** Use Arduino Shield's 3.3V voltage regulator
2. **Data:** Connect ESP32 GPIO pins via jumper wires
3. **Ground:** Common ground between ESP32 and Shield **REQUIRED**

**Why Hybrid?**
- Shield regulator handles 215mA TX current reliably
- ESP32 3.3V pin limited to ~600mA total
- Brown-outs cause data corruption/module resets

---

## SPI Test Code (Loopback Test)

### Purpose
Verify SPI communication is working by asking XBee for its firmware version.

### Expected Behavior:
- **Success:** XBee responds with firmware version (e.g., `20A7` in hex)
- **Failure:** Receive `0x00` (zeros) or `0xFF` (all ones) = broken connection

### Test Code (ESP32)

```cpp
#include <SPI.h>

// --- PIN DEFINITIONS ---
const int CS_PIN = 5;
const int ATTN_PIN = 4;

// SPI Settings: XBee S3B supports up to 3.5MHz. We use 1MHz for safety.
SPISettings xbeeSPI(1000000, MSBFIRST, SPI_MODE0);

// API Frame to ask for Firmware Version ("VR")
// 7E (Start) 00 04 (Len) 08 (AT Cmd) 52 (ID) 56 (V) 52 (R) 1D (Checksum)
byte cmdVR[] = {0x7E, 0x00, 0x04, 0x08, 0x52, 0x56, 0x52, 0x1D};

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n--- XBee SPI Hardware Test ---");

  // 1. Setup Pins
  pinMode(CS_PIN, OUTPUT);
  digitalWrite(CS_PIN, HIGH); // Start Deselected
  pinMode(ATTN_PIN, INPUT);

  // 2. Start SPI
  SPI.begin();

  // 3. Reset Sequence (Optional but good practice)
  Serial.println("Waiting for XBee to be ready...");
  delay(2000); // Give XBee time to boot
}

void loop() {
  Serial.println("\n[TEST] Sending 'ATVR' Request...");
  
  // --- STEP 1: SEND REQUEST ---
  sendSPIFrame(cmdVR, sizeof(cmdVR));

  // --- STEP 2: WAIT FOR RESPONSE ---
  // The XBee will pull ATTN low when the answer is ready
  unsigned long startWait = millis();
  bool responseReady = false;
  
  Serial.print("Waiting for ATTN...");
  while (millis() - startWait < 1000) {
    if (digitalRead(ATTN_PIN) == LOW) {
      responseReady = true;
      Serial.println(" OK! (Data Ready)");
      break;
    }
  }

  if (responseReady) {
    readSPIResponse();
  } else {
    Serial.println(" TIMEOUT. (Check Wiring: ATTN Pin or Power)");
  }

  delay(3000); // Run test every 3 seconds
}

// --- HELPER FUNCTIONS ---

void sendSPIFrame(byte* data, int len) {
  SPI.beginTransaction(xbeeSPI);
  digitalWrite(CS_PIN, LOW); // Select XBee
  delayMicroseconds(50);

  for (int i = 0; i < len; i++) {
    SPI.transfer(data[i]); // Send byte
    delayMicroseconds(10); // Small inter-byte delay for stability
  }

  digitalWrite(CS_PIN, HIGH); // Deselect
  SPI.endTransaction();
}

void readSPIResponse() {
  SPI.beginTransaction(xbeeSPI);
  digitalWrite(CS_PIN, LOW);
  
  Serial.print("RX FRAME: ");
  
  // Read until ATTN goes HIGH (buffer empty) or we hit a safety limit
  int safety = 0;
  while (digitalRead(ATTN_PIN) == LOW && safety < 100) {
    byte incoming = SPI.transfer(0x00); // Send dummy byte to push data out
    
    if (incoming < 0x10) Serial.print("0"); // Formatting
    Serial.print(incoming, HEX);
    Serial.print(" ");
    
    safety++;
  }
  Serial.println();
  
  digitalWrite(CS_PIN, HIGH);
  SPI.endTransaction();
}
```

---

## Critical Diagnostic: The "Weird Characters" Mystery

### Symptoms (The Breakthrough Moment)

**User Report:**
```
Ok ... we are receiving data packets but its unreadable
üÍÌL=ÃõA33sA@DÍÌHB33s@
```

**Behavior Observed:**
- Weird binary garbage appearing in Serial Monitor
- Data appeared ONLY when sender was powered on
- Data stopped when sender disconnected
- ATTN Pin diagnostic showed: `ATTN Pin State: HIGH (Idle)`

### The Smoking Gun

This behavior revealed **exactly** what was wrong:

**Diagnosis:** Receiver XBee was still in **UART (Transparent) Mode**, NOT SPI Mode.

**What Was Happening:**
1. Sender transmitting binary struct via SPI (correctly configured)
2. Receiver XBee receiving data BUT outputting on UART Pin (DOUT/Pin 2)
3. ESP32 listening on SPI pins (seeing nothing)
4. Serial Monitor connected to UART pins (seeing garbage binary data)
5. ATTN pin never used (UART mode doesn't assert attention signal)

**Why "Weird Characters":**
- Binary struct bytes being interpreted as ASCII text
- Floating-point values appearing as random symbols
- No frame delimiters (transparent mode passes raw bytes)

### The Fix: Proper XBee Configuration

**Critical XCTU Settings (Exact Order Matters):**

| Setting | Name | Value | Why? |
|---------|------|-------|------|
| **P3** | DIO13 / DOUT | **0 - Disabled** | **CRITICAL:** Kills UART TX (stops garbage) |
| **P4** | DIO14 / DIN | **0 - Disabled** | **CRITICAL:** Kills UART RX |
| **D1** | DIO1 / SPI_ATTN | **5 - SPI_ATTN** | Enables Attention pin |
| **D2** | DIO2 / SPI_CLK | **1 - SPI_CLK** | Enables SPI Clock |
| **D3** | DIO3 / SPI_SSEL | **1 - SPI_SSEL** | Enables Chip Select |
| **D4** | DIO4 / SPI_MOSI | **1 - SPI_MOSI** | Enables Data In |
| **P2** | DIO12 / SPI_MISO | **1 - SPI_MISO** | Enables Data Out |

**Verification After Fix:**

**Before:**
```
ATTN Pin State: HIGH (Idle)
[Weird garbage in Serial Monitor]
```

**After:**
```
ATTN Pin State: LOW (Active/Data Ready)
[!] ATTN went LOW - XBee has data!
Reading SPI Bus: 7E 00 24 ...
```

**Result:** The `7E` header appeared immediately, confirming SPI frames detected.

---

## Interpreting Test Results

### Scenario A: SUCCESS ✅

**Serial Monitor Output:**
```
[TEST] Sending 'ATVR' Request...
Waiting for ATTN... OK! (Data Ready)
RX FRAME: 7E 00 05 88 52 56 52 00 20 A7 ...
```

**Breakdown:**
- `7E` = Start Delimiter (Good!)
- `00 05` = Length
- `88` = AT Command Response frame type
- `56 52` = "VR" command echo
- `00` = Status OK
- `20 A7` = Firmware version in hex

**Conclusion:** SPI communication is working perfectly.

---

### Scenario B: ALL ZEROS ❌

**Serial Monitor Output:**
```
[TEST] Sending 'ATVR' Request...
Waiting for ATTN... OK! (Data Ready)
RX FRAME: 00 00 00 00 00 00 00 00 ...
```

**Diagnosis:** MISO line is broken or disconnected.

**Troubleshooting:**
1. Check jumper wire from ESP32 GPIO 19 to XBee Pin 4
2. Verify XBee Pin 4 is configured as SPI_MISO (check XCTU settings)
3. Test with multimeter: Should see voltage changes on MISO during transmission
4. Check for cold solder joints if pins are soldered

---

### Scenario C: ALL FFs ❌

**Serial Monitor Output:**
```
[TEST] Sending 'ATVR' Request...
Waiting for ATTN... OK! (Data Ready)
RX FRAME: FF FF FF FF FF FF FF FF ...
```

**Diagnosis:** MISO line is floating or XBee not responding.

**Troubleshooting:**
1. Verify XBee has power (check Shield LED)
2. Measure VCC at XBee Pin 1: Should be 3.3V ±0.1V
3. Check ground connection between ESP32 and Shield
4. XBee may be in wrong mode (not SPI) - check XCTU config

---

### Scenario D: TIMEOUT ❌

**Serial Monitor Output:**
```
[TEST] Sending 'ATVR' Request...
Waiting for ATTN... TIMEOUT. (Check Wiring: ATTN Pin or Power)
```

**Diagnosis:** XBee never received command OR never signaled data ready.

**Troubleshooting:**
1. **No Power:** Check Shield LED is ON
2. **MOSI Broken:** XBee can't receive command
   - Check GPIO 23 to XBee Pin 11 connection
3. **CS Broken:** XBee never "woke up"
   - Check GPIO 5 to XBee Pin 17 connection
4. **ATTN Not Connected:** ESP32 can't detect response ready
   - Check GPIO 4 to XBee Pin 19 connection
5. **Wrong Mode:** XBee configured for UART instead of SPI
   - Use XCTU to verify SPI mode enabled

---

## Common SPI Failures We Encountered

### 1. Configuration Mode Mismatch (MOST COMMON)

**Problem:** XBee remains in UART mode even when wired for SPI.

**Symptoms:**
- Garbage binary data in Serial Monitor
- ATTN pin always HIGH (never asserts)
- Data appears/disappears with sender power

**Root Cause:** XCTU I/O pins not properly configured.

**Solution:** Explicitly disable UART pins and enable SPI pins:
```
P3 (DOUT) = 0 (Disabled)  ← CRITICAL
P4 (DIN) = 0 (Disabled)   ← CRITICAL
D1, D2, D3, D4, P2 = SPI functions
```

**Verification:** ATTN pin should toggle LOW when data arrives.

### 2. SoftwareSerial Overflow (Early Testing)

**Problem:** Arduino receiver showing corrupted data (`ello World` instead of `Hello World`).

**Diagnosis:** SoftwareSerial on Arduino Uno can't keep up with binary struct transmission.

**Solution:** Switched to ESP32 with hardware Serial/SPI.

### 3. Power Brown-Outs

**Problem:** XBee resets intermittently during SPI transactions.

**Diagnosis:** ESP32 3.3V pin couldn't supply 215mA TX current.

**Solution:** Use Shield's dedicated voltage regulator for XBee power.

### 4. API Frame Complexity

**Problem:** SPI requires API mode (complex frame structure with checksums).

**Reality:** While complex, API frames work reliably once properly implemented.

**Note:** Binary struct transmission successful at 10Hz after proper configuration.

---

## Speed Specifications

### A. SPI Clock Frequency (ESP32 → XBee)

**Who Controls:** ESP32 (SPI Master)  
**XBee Limit:** 3.5 MHz (per S3B datasheet)  
**Recommended:** **1 MHz** (`1000000` in SPISettings)

**Why 1MHz?**
- Fast enough for 10Hz+ telemetry
- Safe for jumper wire connections
- Reliable on breadboards
- Well within XBee specifications

**Code:**
```cpp
SPISettings xbeeSPI(1000000, MSBFIRST, SPI_MODE0);
```

### B. UART Baud Rate (Fallback/Rescue Mode)

**Who Controls:** XBee internal setting (`BD` parameter)  
**Factory Default:** 9600 baud  
**Recommended:** 115200 baud (after configuration)

**When Used:**
- XCTU configuration via USB adapter
- SPI rescue programmer (after re-enabling UART)
- Fallback debugging mode

---

## SPI Success Criteria

### Working Configuration Checklist:

✅ **Power:**
- XBee VCC = 3.3V ±0.1V (measured at Pin 1)
- Common ground between ESP32 and XBee
- Stable power supply (>215mA capacity)

✅ **Wiring:**
- All 5 SPI wires connected (CS, MOSI, MISO, SCK, ATTN)
- No loose jumper wires
- Shield between data lines and power wires (if noisy environment)

✅ **XBee Configuration:**
- AP = 1 (API Mode Enabled)
- P3 = 0, P4 = 0 (UART Disabled)
- D1 = 5, D2/D3/D4 = 1, P2 = 1 (SPI Enabled)
- ID and HP match between sender/receiver

✅ **ESP32 Code:**
- SPISettings: 1MHz, MSBFIRST, SPI_MODE0
- CS pin starts HIGH, goes LOW during transaction
- ATTN pin monitored for data-ready signal
- Proper API frame construction (checksums calculated)

✅ **Verification:**
- ATTN pin toggles LOW when data arrives
- Serial Monitor shows `7E` start delimiters
- Binary struct received matches sender format
- No timeouts or garbage data

---

## The Final Outcome

### What Worked:
- ✅ ESP32-to-ESP32 SPI communication at 10Hz
- ✅ 36-byte binary struct transmission
- ✅ API frame protocol implemented successfully
- ✅ Proper I/O pin configuration discovered
- ✅ SPI rescue programmer developed (see rescue tool docs)

### Design Decision:
After successful SPI implementation, **UART Transparent Mode** selected for production:

**Reasons:**
1. **Simplicity:** 2 wires vs 5 wires
2. **Robustness:** No clock synchronization issues
3. **Debugging:** Readable CSV format in transparent mode
4. **Sufficient Speed:** 115200 baud handles 50Hz telemetry easily
5. **Configuration:** Can use XCTU anytime (no lock-out)

### Key Takeaway:
SPI works perfectly when properly configured, but UART's simplicity better suits rocket telemetry constraints (vibration, weight, debugging in field).

---

## Lessons Learned

1. ✅ **Power First:** Verify stable 3.3V before debugging communication
2. ✅ **Start Simple:** UART is easier to debug than SPI
3. ✅ **Minimize Wires:** Fewer wires = more reliable under vibration
4. ✅ **Common Ground:** Never skip ground connection in split-power setups
5. ✅ **Test Incrementally:** Loopback test before full sender/receiver
6. ✅ **Document Everything:** SPI failure modes help future troubleshooting

---

## When to Use SPI (Future Projects)

Consider SPI if:
- ✅ Need full-duplex bidirectional data
- ✅ Sending >100Hz updates
- ✅ Can solder connections (no jumper wires)
- ✅ Have reliable power supply (>500mA @ 3.3V)
- ✅ Comfortable with API frame protocol

Stick with UART if:
- ✅ One-way telemetry (rocket → ground)
- ✅ <100Hz update rate sufficient
- ✅ Prototyping with jumper wires
- ✅ Want simple debugging
- ✅ Transparent Mode preferred (readable text)

---

## References

- [XBee S3B Datasheet](datasheets/) - SPI interface specifications
- [UART Implementation](code_examples/uart_sender_receiver.md)
- [ESP32 VSPI Documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/spi_master.html)
