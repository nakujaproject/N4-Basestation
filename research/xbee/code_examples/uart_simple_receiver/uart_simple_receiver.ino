/*
 * XBee Pro 900HP - Simple UART Receiver (Arduino)
 * 
 * Receives text messages from XBee and prints to Serial Monitor
 * Good for initial XBee testing and range tests
 * 
 * XBee Configuration Required (XCTU):
 * - AP = 0 (Transparent Mode)
 * - BD = 3 (9600 baud) - Must match sketch
 * - ID = 7777 (Network ID - must match sender)
 * - HP = 0 (Preamble ID - must match sender)
 * 
 * Wiring (Arduino Uno -> XBee):
 * Arduino GND -> XBee Pin 10 (GND)
 * Arduino 5V  -> XBee Shield 5V input
 * Arduino Pin 2 (RX) -> XBee Pin 2 (DOUT/TX)
 * Arduino Pin 3 (TX) -> XBee Pin 3 (DIN/RX)
 * 
 * Serial Monitor: 9600 baud
 */

#include <SoftwareSerial.h>

// SoftwareSerial: RX=Pin 2, TX=Pin 3
SoftwareSerial xbee(2, 3);

const int LED_PIN = 13; // Built-in LED
unsigned long lastPacketTime = 0;
uint32_t messagesReceived = 0;

void setup() {
  Serial.begin(9600);
  xbee.begin(9600);  // Must match XBee BD setting
  
  pinMode(LED_PIN, OUTPUT);
  
  Serial.println("====================================");
  Serial.println(" XBee UART Simple Receiver - Arduino");
  Serial.println("====================================");
  Serial.println();
  Serial.println("Waiting for messages...");
  Serial.println("XBee UART: 9600 baud on pins 2/3");
  Serial.println();
}

void loop() {
  // Check for incoming data from XBee
  if (xbee.available()) {
    String message = xbee.readStringUntil('\n');
    message.trim(); // Remove whitespace
    
    if (message.length() > 0) {
      messagesReceived++;
      lastPacketTime = millis();
      
      // Print to Serial Monitor
      Serial.print("["); Serial.print(millis()); Serial.print(" ms] ");
      Serial.print("Message #"); Serial.print(messagesReceived);
      Serial.print(": ");
      Serial.println(message);
      
      // Blink LED to show activity
      digitalWrite(LED_PIN, HIGH);
      delay(50);
      digitalWrite(LED_PIN, LOW);
    }
  }
  
  // Connection timeout detection (10 seconds)
  if (messagesReceived > 0 && (millis() - lastPacketTime > 10000)) {
    Serial.println();
    Serial.println("WARNING: No messages for 10 seconds");
    Serial.println("Check:");
    Serial.println("  - Sender is powered on");
    Serial.println("  - XBee ID/HP settings match");
    Serial.println("  - Antennas are connected");
    Serial.println("  - Distance is within range");
    Serial.println();
    delay(5000); // Print once every 5 seconds
  }
}
