# XBee SPI Rescue Programmer

## Problem Statement

When an XBee module is configured for **SPI Mode**, the UART pins (DOUT/DIN) are disabled. This creates a "lock-out" situation:

- **XCTU** connects via USB adapter which uses UART pins
- With UART disabled (`P3=0, P4=0`), XCTU cannot communicate with the module
- You cannot reconfigure the XBee back to UART mode using XCTU

**Solution:** Use SPI interface to send AT commands that re-enable UART pins.

---

## What This Tool Does

This ESP32 sketch sends AT commands via SPI to:

1. **Enable UART TX** (`ATP3 1`) - Re-enables DOUT pin
2. **Enable UART RX** (`ATP4 1`) - Re-enables DIN pin  
3. **Write to Flash** (`ATWR`) - Saves settings permanently
4. **Soft Reset** (`ATFR`) - Reboots XBee to apply changes

After running this code, you can reconnect the XBee to XCTU via USB adapter.

---

## When to Use This Tool

**Use Cases:**
- XBee configured for SPI mode and you need to change settings in XCTU
- Testing SPI communication completed, switching back to development mode
- Accidentally disabled UART pins and locked out of XCTU
- Need to reconfigure Network ID, power levels, or other settings

**Warning:** This tool is a "rescue" mechanism. Once UART is re-enabled, the XBee will no longer work in SPI mode until you reconfigure it.

---

## Wiring Diagram

### ESP32 to XBee (SPI Mode)

| ESP32 Pin | XBee Pin | Function | Notes |
|-----------|----------|----------|-------|
| **3V3** | Pin 1 | VCC | ⚠️ Use Shield regulator if available |
| **GND** | Pin 10 | GND | Common ground required |
| **GPIO 5** | Pin 17 (DIO3) | CS | Chip Select |
| **GPIO 18** | Pin 18 (DIO2) | SCK | Clock |
| **GPIO 19** | Pin 4 (DIO12) | MISO | Data from XBee |
| **GPIO 23** | Pin 11 (DIO4) | MOSI | Data to XBee |
| **GPIO 4** | Pin 19 (DIO1) | ATTN | Optional (not critical) |

**Important:** This wiring is for SPI mode. After running this tool, the XBee will revert to UART mode.

---

## Speed Specifications

### SPI Clock Frequency

**ESP32 Control:** Master sets clock speed  
**XBee Limit:** 3.5 MHz maximum (per S3B datasheet)  
**Recommended:** **1 MHz** (`1000000` Hz)

**Why 1MHz?**
- Safe for breadboard/jumper wire connections
- Fast enough for AT command transmission
- Well within XBee specifications
- Reliable across all XBee modules

**Code Setting:**
```cpp
SPISettings xbeeSPI(1000000, MSBFIRST, SPI_MODE0);
```

### UART Baud Rate (After Rescue)

**Factory Default:** 9600 baud  
**Previous Setting:** Whatever was configured before SPI mode

**After running this tool:**
- XBee returns to last saved UART baud rate (typically 9600)
- Connect to XCTU at 9600 baud initially
- Can change to 115200 baud using XCTU after connection

---

## Complete Rescue Code

```cpp
#include <SPI.h>

// ===== CIRCUIT CONNECTIONS (VSPI) =====
const int CS_PIN = 5;
const int SCK_PIN = 18;
const int MISO_PIN = 19;
const int MOSI_PIN = 23;
const int ATTN_PIN = 4; // Optional for this tool

// ===== SPI SPEED SETTING =====
// XBee S3B max is 3.5 MHz, we use 1 MHz for safety
SPISettings xbeeSPI(1000000, MSBFIRST, SPI_MODE0);

// ===== SETUP =====
void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("==========================================");
  Serial.println("  XBee SPI -> UART Rescue Tool");
  Serial.println("  Enables UART pins via SPI commands");
  Serial.println("==========================================\n");
  
  // Configure Pins
  pinMode(CS_PIN, OUTPUT);
  digitalWrite(CS_PIN, HIGH); // Start deselected
  pinMode(ATTN_PIN, INPUT);
  
  // Initialize SPI Bus
  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, CS_PIN);
  delay(100);

  Serial.println("[INFO] SPI Initialized");
  Serial.println("[INFO] Sending AT Commands to enable UART...\n");

  // === SEND AT COMMANDS ===
  
  // 1. Enable DOUT (Pin 3 = UART TX)
  Serial.println("→ Sending: ATP3 1 (Enable DOUT)");
  sendATCommand("P3", 1);
  delay(100);
  
  // 2. Enable DIN (Pin 4 = UART RX)
  Serial.println("→ Sending: ATP4 1 (Enable DIN)");
  sendATCommand("P4", 1);
  delay(100);

  // 3. Write to Flash (Saves settings permanently)
  Serial.println("→ Sending: ATWR (Write to Flash)");
  sendATCommand("WR", -1); // -1 = No parameter
  delay(500); // Give XBee time to write to flash

  // 4. Soft Reset (Reboots XBee to apply changes)
  Serial.println("→ Sending: ATFR (Soft Reset)");
  sendATCommand("FR", -1);
  
  Serial.println("\n==========================================");
  Serial.println("          RESCUE COMPLETE");
  Serial.println("==========================================");
  Serial.println("\n[NEXT STEPS]");
  Serial.println("1. Disconnect XBee from ESP32 circuit");
  Serial.println("2. Connect XBee to XCTU via USB adapter");
  Serial.println("3. XCTU should now detect the module");
  Serial.println("4. Default UART baud rate: 9600");
  Serial.println("5. You can now reconfigure using XCTU\n");
  
  Serial.println("[TIP] If XCTU doesn't detect:");
  Serial.println("      - Try baud rate 9600 (factory default)");
  Serial.println("      - Wait 5 seconds for XBee to fully boot");
  Serial.println("      - Check USB adapter drivers installed\n");
}

void loop() {
  // Nothing to do - rescue is one-time operation
}

// ===== SEND AT COMMAND VIA SPI =====
// Constructs and transmits API Frame (0x08 - AT Command)
void sendATCommand(char* cmd, int param) {
  // Calculate frame length
  // 4 bytes standard: Type(1) + ID(1) + Cmd(2)
  // +1 byte if parameter provided
  int len = 4;
  if(param != -1) len++;

  long checksumTotal = 0;
  
  // Begin SPI transaction
  SPI.beginTransaction(xbeeSPI);
  digitalWrite(CS_PIN, LOW); // Select XBee
  delayMicroseconds(50);     // XBee wake-up time

  // === API FRAME HEADER ===
  SPI.transfer(0x7E);        // Start Delimiter
  SPI.transfer(0x00);        // Length MSB
  SPI.transfer(len);         // Length LSB

  // === FRAME DATA ===
  // Frame Type: 0x08 (AT Command)
  SPI.transfer(0x08); 
  checksumTotal += 0x08;
  
  // Frame ID: 0x01 (Request Acknowledgment)
  SPI.transfer(0x01); 
  checksumTotal += 0x01;

  // AT Command (2 characters)
  SPI.transfer(cmd[0]); 
  checksumTotal += cmd[0];
  SPI.transfer(cmd[1]); 
  checksumTotal += cmd[1];

  // Parameter (Optional)
  if(param != -1) {
    SPI.transfer(param); 
    checksumTotal += param;
  }

  // === CHECKSUM ===
  byte checksum = 0xFF - (checksumTotal & 0xFF);
  SPI.transfer(checksum);

  // End SPI transaction
  digitalWrite(CS_PIN, HIGH); // Deselect XBee
  SPI.endTransaction();
  
  // Status output
  Serial.print("   [OK] AT");
  Serial.print(cmd[0]); 
  Serial.print(cmd[1]);
  if(param != -1) { 
    Serial.print(" "); 
    Serial.print(param); 
  }
  Serial.println(" sent");
}
```

---

## How to Use

### Step 1: Upload Code

1. Connect ESP32 to computer via USB
2. Open Arduino IDE
3. Copy the code above
4. Select board: **ESP32 Dev Module**
5. Select correct COM port
6. Click **Upload**

### Step 2: Wire Circuit

Connect ESP32 to XBee using the wiring diagram above. Ensure:
- XBee is powered (3.3V at Pin 1)
- Common ground connected
- All 5 SPI wires connected (CS, SCK, MISO, MOSI, ATTN)

### Step 3: Run Tool

1. Open Serial Monitor (115200 baud)
2. Press ESP32 reset button
3. Watch for command sequence

**Expected Output:**
```
==========================================
  XBee SPI -> UART Rescue Tool
  Enables UART pins via SPI commands
==========================================

[INFO] SPI Initialized
[INFO] Sending AT Commands to enable UART...

→ Sending: ATP3 1 (Enable DOUT)
   [OK] ATP3 1 sent
→ Sending: ATP4 1 (Enable DIN)
   [OK] ATP4 1 sent
→ Sending: ATWR (Write to Flash)
   [OK] ATWR sent
→ Sending: ATFR (Soft Reset)
   [OK] ATFR sent

==========================================
          RESCUE COMPLETE
==========================================
```

### Step 4: Reconnect to XCTU

1. **Disconnect** XBee from ESP32 circuit
2. **Insert** XBee into USB adapter (or Shield)
3. **Connect** adapter to computer
4. **Open XCTU**
5. Click **Discover** (use baud rate 9600)
6. XBee should now appear in device list

---

## Troubleshooting

### Problem: Commands sent but XCTU still can't detect

**Check:**
1. Wait 5 seconds after soft reset before connecting to XCTU
2. Try baud rate 9600 in XCTU discovery settings
3. Verify USB adapter drivers installed (CH340/FTDI)
4. Check XBee LED on adapter (should blink during discovery)
5. Try pressing reset button on USB adapter

### Problem: [OK] messages don't appear

**Check:**
1. Verify SPI wiring (especially CS and MOSI)
2. Check XBee powered (measure 3.3V at Pin 1)
3. Verify common ground between ESP32 and XBee
4. Check XBee was actually in SPI mode before running tool

### Problem: XCTU detects but wrong settings

**Solution:**
- XBee retained other settings (Network ID, power level, etc.)
- Only UART pins were re-enabled
- Use XCTU to reconfigure other parameters as needed

---

## API Frame Format Explained

### AT Command Frame (0x08)

```
7E           Start Delimiter
00 04        Length (4 bytes of data)
08           Frame Type (AT Command)
01           Frame ID (0x01 = Request Response)
50 33        AT Command ("P3" in ASCII hex)
01           Parameter Value (1 = Enable)
XX           Checksum (0xFF - sum of data bytes)
```

### Example: ATP3 1

```
7E 00 05 08 01 50 33 01 XX
│  │  │  │  │  │  │  │  └─ Checksum
│  │  │  │  │  │  │  └──── Parameter (1)
│  │  │  │  │  │  └─────── Command "3"
│  │  │  │  │  └────────── Command "P"
│  │  │  │  └───────────── Frame ID
│  │  │  └──────────────── AT Command frame type
│  │  └─────────────────── Length LSB
│  └────────────────────── Length MSB
└───────────────────────── Start Delimiter
```

---

## Advanced: Reading XBee Responses

The code above is "fire and forget" - it sends commands without waiting for responses. To verify XBee acknowledged commands, add response reading:

```cpp
void readXBeeResponse() {
  delay(50); // Wait for XBee to process
  
  if(digitalRead(ATTN_PIN) == LOW) {
    Serial.println("[INFO] XBee has response data");
    
    SPI.beginTransaction(xbeeSPI);
    digitalWrite(CS_PIN, LOW);
    delayMicroseconds(50);
    
    // Look for 0x7E start
    for(int i=0; i<100; i++) {
      byte b = SPI.transfer(0x00);
      if(b == 0x7E) {
        Serial.print("Response Frame: 7E ");
        
        // Read next 16 bytes
        for(int j=0; j<16; j++) {
          byte val = SPI.transfer(0x00);
          if(val < 0x10) Serial.print("0");
          Serial.print(val, HEX);
          Serial.print(" ");
        }
        Serial.println();
        break;
      }
    }
    
    digitalWrite(CS_PIN, HIGH);
    SPI.endTransaction();
  }
}
```

Call `readXBeeResponse()` after each `sendATCommand()` to see acknowledgments.

---

## Preventing Lock-Out in Future

### Best Practices:

1. **Use UART for Configuration:**
   - Configure all XBee settings in XCTU before enabling SPI mode
   - Only switch to SPI mode when configuration is finalized

2. **Keep a Backup Module:**
   - Have spare XBee configured for UART debugging
   - Label modules clearly (SPI vs UART)

3. **Test in Stages:**
   - Verify UART communication first (transparent mode)
   - Switch to SPI only after proving UART works
   - Keep this rescue tool accessible

4. **Document Settings:**
   - Save XCTU profiles before switching to SPI
   - Record Network ID, power levels, baud rates
   - Keep printed copy with hardware

---

## Speed Comparison

| Interface | Speed | Configuration | Debugging |
|-----------|-------|---------------|-----------|
| **UART** | 115200 baud | XCTU accessible | Easy (Serial Monitor) |
| **SPI** | 1 MHz | Locked out of XCTU | Complex (API frames) |

**Recommendation:** Use UART during development, switch to SPI only for production if higher speeds required.

---

## Related Documentation

- [SPI_TROUBLESHOOTING.md](../SPI_TROUBLESHOOTING.md) - Why we switched from SPI to UART
- [XCTU_CONFIGURATION.md](../XCTU_CONFIGURATION.md) - Proper XBee configuration guide
- [HARDWARE_SETUP.md](../HARDWARE_SETUP.md) - Wiring and power considerations
- [esp32_spi_sender.md](./esp32_spi_sender.md) - Working SPI transmission code
- [esp32_spi_receiver.md](./esp32_spi_receiver.md) - Working SPI reception code

---

## Summary

**Problem:** XBee in SPI mode = UART disabled = XCTU can't connect  
**Solution:** Use SPI interface to send AT commands re-enabling UART  
**Result:** XBee accessible to XCTU again for reconfiguration

This rescue tool is essential for development workflows involving SPI mode testing.
