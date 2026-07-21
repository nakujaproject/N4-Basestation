/**
 * HC-05/HC-06 AT Command Interface for ESP32
 * 
 * This sketch allows you to configure your HC-05 or HC-06 Bluetooth module
 * using AT commands directly from the Arduino Serial Monitor.
 * 
 * Hardware Setup:
 * - HC-05/HC-06 VCC → 5V or 3.3V (check your module specs)
 * - HC-05/HC-06 GND → GND
 * - HC-05/HC-06 RX  → GPIO 17 (ESP32 TX)
 * - HC-05/HC-06 TX  → GPIO 16 (ESP32 RX)
 * 
 * For HC-05 AT Mode:
 * 1. Disconnect VCC
 * 2. Press and HOLD the button on HC-05
 * 3. Connect VCC while holding button
 * 4. LED should blink slowly (once every 2 seconds)
 * 5. Release button - now in AT mode
 * 
 * For HC-06:
 * - No AT mode needed, just power on normally
 * 
 * Serial Monitor Settings:
 * - Baud rate: 115200
 * - Line ending: "Both NL & CR"
 * 
 * Common AT Commands:
 * - AT                           (test connection)
 * - AT+NAME?                     (get name - HC-05)
 * - AT+NAME=NewName              (set name)
 * - AT+UART?                     (get baud - HC-05)
 * - AT+UART=460800,0,0           (set 460800 baud - HC-05)
 * - AT+BAUD9                     (set 460800 baud - HC-06)
 * - AT+VERSION?                  (get firmware version)
 * - AT+PSWD?                     (get pairing password)
 * 
 * Tested and working with:
 * - HC-05 modules (requires AT mode)
 * - HC-06 modules (always in AT mode when unpaired)
 * - ESP32 DevKit boards
 * 
 * This code is PROVEN to work - do not modify unless necessary!
 */

#include <HardwareSerial.h>

HardwareSerial BTSerial(2);

#define BT_RX 16
#define BT_TX 17

String rxBuffer = "";  // buffer for incoming BT data

void setup() {
  Serial.begin(115200);
  delay(1000);

  // HC-05 AT mode default baud = 38400
  // HC-06 default = 9600 (try 38400 first, then 9600 if no response)
  BTSerial.begin(38400, SERIAL_8N1, BT_RX, BT_TX);

  Serial.println("=== ESP32 ↔ Bluetooth UART Monitor ===");
  Serial.println("RX: GPIO16  TX: GPIO17");
  Serial.println("BT Baud: 38400");
  Serial.println("Line Ending: Both NL & CR");
  Serial.println("=====================================");
  Serial.println();
  Serial.println("HC-05 AT Mode Setup:");
  Serial.println("1. Disconnect VCC from HC-05");
  Serial.println("2. Press and HOLD the button");
  Serial.println("3. Connect VCC while holding button");
  Serial.println("4. LED blinks slowly = AT mode ready");
  Serial.println("5. Release button");
  Serial.println();
  Serial.println("HC-06: Just power on (always in AT mode)");
  Serial.println();
  Serial.println("Type 'AT' to test connection...");
  Serial.println("=====================================");
}

void loop() {
  // ---------------------
  // Forward from PC → BT
  // ---------------------
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n'); // read a full line
    cmd.trim();                                // remove \r or whitespace

    if (cmd.length() > 0) {
      Serial.print("[TX → BT] ");
      Serial.println(cmd);                     // print nicely

      BTSerial.print(cmd);                     // send command
      BTSerial.print("\r\n");                  // ensure AT command line ending
    }
  }

  // ---------------------
  // Forward from BT → PC
  // ---------------------
  while (BTSerial.available()) {
    char c = BTSerial.read();
    rxBuffer += c;

    // Print full line once we get newline
    if (c == '\n') {
      rxBuffer.trim();                         // remove \r and \n
      if (rxBuffer.length() > 0) {
        Serial.print("[RX ← BT] ");
        Serial.println(rxBuffer);
      }
      rxBuffer = "";                           // reset buffer
    }
  }
}
