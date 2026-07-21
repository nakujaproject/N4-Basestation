/*
 * XBee Pro 900HP - Simple UART Sender (ESP32)
 * 
 * Sends "Hello World" text messages via XBee UART
 * Good for initial XBee testing and range tests
 * 
 * XBee Configuration Required (XCTU):
 * - AP = 0 (Transparent Mode)
 * - BD = 7 (115200 baud) - Must match sketch
 * - ID = 7777 (Network ID)
 * - HP = 0 (Preamble ID)
 * - DL = FFFF (Broadcast destination)
 * 
 * Wiring Options:
 * 
 * Option A: Direct Connection (ESP32 -> XBee)
 * ESP32 GND    -> XBee Pin 10 (GND)
 * ESP32 3V3    -> XBee Pin 1  (VCC) - Only if XBee draws < 500mA
 * ESP32 GPIO 17 -> XBee Pin 3  (DIN/RX)
 * ESP32 GPIO 16 -> XBee Pin 2  (DOUT/TX)
 * 
 * Option B: Using Arduino XBee Shield V03 (Recommended)
 * 1. Set Shield switch to "XBee" position
 * 2. Connect ESP32 TX/RX to Shield's TX/RX headers
 * 3. Power XBee from Shield regulator (more stable)
 * 
 * Serial Monitor: 115200 baud
 */

#include <HardwareSerial.h>

// UART2 on ESP32: RX=GPIO16, TX=GPIO17
HardwareSerial XBeeSerial(2);

const int TX_PIN = 17;
const int RX_PIN = 16;
const int LED_PIN = 2; // Built-in LED

unsigned long lastSendTime = 0;
uint32_t messageCount = 0;

void setup() {
  Serial.begin(115200);
  XBeeSerial.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN);
  
  pinMode(LED_PIN, OUTPUT);
  
  delay(1000);
  
  Serial.println("====================================");
  Serial.println("  XBee UART Simple Sender - ESP32");
  Serial.println("====================================");
  Serial.println();
  Serial.println("Sending 'Hello World' messages at 1Hz...");
  Serial.println("XBee UART: 115200 baud on GPIO16/17");
  Serial.println();
}

void loop() {
  // Send message every 1 second (1Hz)
  if (millis() - lastSendTime >= 1000) {
    lastSendTime = millis();
    messageCount++;
    
    // Construct message
    String message = "Hello World #" + String(messageCount);
    
    // Send to XBee
    XBeeSerial.println(message);
    
    // Print to Serial Monitor
    Serial.print("["); Serial.print(millis()); Serial.print(" ms] ");
    Serial.print("Sent: "); Serial.println(message);
    
    // Blink LED
    digitalWrite(LED_PIN, HIGH);
    delay(50);
    digitalWrite(LED_PIN, LOW);
  }
  
  // Echo any incoming data from XBee to Serial Monitor
  if (XBeeSerial.available()) {
    String incoming = XBeeSerial.readStringUntil('\n');
    Serial.print("RECEIVED: ");
    Serial.println(incoming);
  }
}
