/*
 * XBee SPI Recovery Tool
 * 
 * USE CASE: Your XBee is stuck in SPI mode and XCTU can't talk to it
 * 
 * This code re-enables the UART pins (P3/P4) so you can reconnect via USB
 * After running this, disconnect the XBee from ESP32 and connect to XCTU
 * The XBee will respond on UART (default 9600 baud or last saved rate)
 * 
 * HOW IT WORKS:
 * 1. Sends AT commands via SPI to enable UART pins
 * 2. Writes settings to flash (WR command)
 * 3. Reboots XBee (FR command) to apply changes
 * 
 * Wiring (ESP32 VSPI -> XBee):
 * ESP32 GND  -> XBee Pin 10 (GND)
 * ESP32 3V3  -> XBee Pin 1  (VCC)
 * ESP32 GPIO 5  -> XBee Pin 17 (DIO3/CS)
 * ESP32 GPIO 18 -> XBee Pin 18 (DIO2/SCK)
 * ESP32 GPIO 19 -> XBee Pin 4  (DIO12/MISO)
 * ESP32 GPIO 23 -> XBee Pin 11 (DIO4/MOSI)
 * ESP32 GPIO 4  -> XBee Pin 19 (DIO1/ATTN) - Optional
 * 
 * AFTER RUNNING:
 * 1. Open Serial Monitor (115200 baud) to see results
 * 2. Wait for "DONE" message
 * 3. Disconnect XBee from ESP32
 * 4. Connect XBee to XCTU via USB adapter
 * 5. XBee should now respond on UART
 */

#include <SPI.h>

// --- CIRCUIT CONNECTIONS (VSPI) ---
const int CS_PIN = 5;
const int SCK_PIN = 18;
const int MISO_PIN = 19;
const int MOSI_PIN = 23;
const int ATTN_PIN = 4; // Optional for this, but good to have connected

// --- SPEED SETTING ---
// We use 1 MHz. XBee S3B max is 3.5 MHz, but 1 MHz is safer for jumper wires.
SPISettings xbeeSPI(1000000, MSBFIRST, SPI_MODE0);

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("====================================");
  Serial.println("  XBee SPI -> UART Rescue Tool");
  Serial.println("====================================");
  Serial.println();
  
  // 1. Setup Pins
  pinMode(CS_PIN, OUTPUT);
  digitalWrite(CS_PIN, HIGH); // Deselect
  pinMode(ATTN_PIN, INPUT);
  
  // 2. Start SPI Bus
  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, CS_PIN);
  delay(100);

  Serial.println("[1/4] Sending AT Commands to enable UART...");
  Serial.println();

  // 3. Send Commands
  
  // Enable DOUT (P3 = 1) - UART TX Pin
  Serial.print("  -> Setting P3 (DOUT)... ");
  sendATCommand("P3", 1); 
  delay(100);
  Serial.println("OK");
  
  // Enable DIN (P4 = 1) - UART RX Pin
  Serial.print("  -> Setting P4 (DIN)... ");
  sendATCommand("P4", 1); 
  delay(100);
  Serial.println("OK");

  Serial.println();
  Serial.println("[2/4] Writing settings to flash (WR)...");
  
  // Write to Flash (WR) - Saves settings permanently
  sendATCommand("WR", -1); 
  delay(500); // Give it time to write
  Serial.println("  -> Settings saved");

  Serial.println();
  Serial.println("[3/4] Rebooting XBee (FR)...");
  
  // Soft Reset (FR) - Reboots XBee to apply changes
  sendATCommand("FR", -1); 
  delay(1000); // Wait for reboot
  Serial.println("  -> XBee rebooted");
  
  Serial.println();
  Serial.println("====================================");
  Serial.println("           ✓ DONE");
  Serial.println("====================================");
  Serial.println();
  Serial.println("[4/4] Next Steps:");
  Serial.println("  1. Disconnect XBee from ESP32");
  Serial.println("  2. Connect XBee to XCTU via USB");
  Serial.println("  3. XBee should respond on UART");
  Serial.println("     (Try 9600 baud first, or last saved rate)");
  Serial.println();
  Serial.println("If XCTU still can't find it:");
  Serial.println("  - Check power connections");
  Serial.println("  - Try different baud rates (9600, 115200)");
  Serial.println("  - Verify USB adapter is working");
}

void loop() {
  // Nothing to do here
}

/*
 * Function to construct and send an API Frame (0x08 - AT Command)
 * 
 * Parameters:
 *   cmd   - 2-character AT command (e.g., "P3", "WR", "FR")
 *   param - Parameter value (use -1 for commands with no parameter)
 */
void sendATCommand(char* cmd, int param) {
  // Calculate Length: 4 bytes standard (Type + ID + Cmd1 + Cmd2) + Optional Param
  int len = 4; 
  if(param != -1) len++; 

  long checksumTotal = 0;
  
  SPI.beginTransaction(xbeeSPI);
  digitalWrite(CS_PIN, LOW); // Select XBee
  delayMicroseconds(50);     // Tiny delay for stability

  // --- HEADER ---
  SPI.transfer(0x7E);        // Start Delimiter
  SPI.transfer(0x00);        // Length MSB
  SPI.transfer(len);         // Length LSB

  // --- FRAME DATA ---
  // Frame Type: 0x08 (AT Command)
  SPI.transfer(0x08); checksumTotal += 0x08; 
  
  // Frame ID: 0x01 (Request Ack)
  SPI.transfer(0x01); checksumTotal += 0x01; 

  // AT Command (e.g., 'P', '3')
  SPI.transfer(cmd[0]); checksumTotal += cmd[0];
  SPI.transfer(cmd[1]); checksumTotal += cmd[1];

  // Parameter (Optional)
  if(param != -1) {
    SPI.transfer(param); checksumTotal += param;
  }

  // --- CHECKSUM ---
  byte checksum = 0xFF - (checksumTotal & 0xFF);
  SPI.transfer(checksum);

  digitalWrite(CS_PIN, HIGH); // Deselect XBee
  SPI.endTransaction();
}
