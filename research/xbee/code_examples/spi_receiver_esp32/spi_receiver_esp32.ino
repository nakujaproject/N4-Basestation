/*
 * XBee Pro 900HP - SPI Receiver (ESP32)
 * 
 * Receives binary telemetry packets via SPI from XBee
 * Parses 36-byte struct and prints to Serial Monitor
 * 
 * XBee Configuration Required (XCTU):
 * - AP = 1 (API Mode Enabled)
 * - D1 = 5 (SPI_ATTN)
 * - D2 = 1 (SPI_CLK)
 * - D3 = 1 (SPI_SSEL)
 * - D4 = 1 (SPI_MOSI)
 * - P2 = 1 (SPI_MISO)
 * - P3 = 0 (UART DOUT Disabled)
 * - P4 = 0 (UART DIN Disabled)
 * - ID = 7777 (Network ID - must match sender)
 * - HP = 0 (Preamble ID - must match sender)
 * 
 * Wiring (ESP32 VSPI -> XBee):
 * ESP32 GND  -> XBee Pin 10 (GND)
 * ESP32 3V3  -> XBee Pin 1  (VCC) - Use Shield regulator if available
 * ESP32 GPIO 5  -> XBee Pin 17 (DIO3/CS)
 * ESP32 GPIO 18 -> XBee Pin 18 (DIO2/SCK)
 * ESP32 GPIO 19 -> XBee Pin 4  (DIO12/MISO)
 * ESP32 GPIO 23 -> XBee Pin 11 (DIO4/MOSI)
 * ESP32 GPIO 4  -> XBee Pin 19 (DIO1/ATTN)
 */

#include <SPI.h>

// --- WIRING (MATCHES YOUR CIRCUIT) ---
const int CS_PIN = 5;
const int SCK_PIN = 18;
const int MISO_PIN = 19;
const int MOSI_PIN = 23;
const int ATTN_PIN = 4;

// --- SPI SETTINGS ---
// Max speed: 3.5 MHz, using 1 MHz for stability
SPISettings xbeeSPI(1000000, MSBFIRST, SPI_MODE0);

// --- DATA STRUCTURE (Must Match Sender) ---
struct __attribute__((packed)) TelemetryData {
    uint32_t record_number;
    uint8_t operation_mode;
    uint8_t state;
    float ax, ay, az;
    float altitude;
    float velocity;
    float battery_voltage;
};

TelemetryData incomingData;
unsigned long lastPacketTime = 0;
uint32_t packetsReceived = 0;

void setup() {
  Serial.begin(115200);
  
  pinMode(CS_PIN, OUTPUT);
  digitalWrite(CS_PIN, HIGH);
  pinMode(ATTN_PIN, INPUT);
  
  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, CS_PIN);
  
  delay(1000);
  Serial.println("--- ESP32 SPI Receiver Ready ---");
  Serial.println("Waiting for XBee ATTN signal...");
}

void loop() {
  // Wait for XBee to assert ATTN (Low means data is waiting)
  if (digitalRead(ATTN_PIN) == LOW) {
    readSPIPacket();
    lastPacketTime = millis();
  }
  
  // Connection timeout detection (5 seconds)
  if (packetsReceived > 0 && (millis() - lastPacketTime > 5000)) {
    Serial.println("WARNING: No packets received for 5 seconds");
    delay(1000); // Print once per second
  }
}

void readSPIPacket() {
  SPI.beginTransaction(xbeeSPI);
  digitalWrite(CS_PIN, LOW);
  delayMicroseconds(50);

  // 1. Find Start Delimiter (0x7E)
  int timeout = 0;
  bool found = false;
  while(digitalRead(ATTN_PIN) == LOW && timeout < 500) {
    if(SPI.transfer(0x00) == 0x7E) {
      found = true;
      break;
    }
    timeout++;
  }

  if(found) {
    // 2. Read Length (2 bytes)
    byte msb = SPI.transfer(0x00);
    byte lsb = SPI.transfer(0x00);
    int len = (msb << 8) | lsb;

    // 3. Read Frame Type
    byte frameType = SPI.transfer(0x00);

    // We only care about RX Packets (0x90)
    if(frameType == 0x90) {
      // Skip 11 bytes of Overhead (Source Address + Options)
      for(int i=0; i<11; i++) SPI.transfer(0x00);

      // Payload Length = Total Frame Len - 12 bytes overhead
      int payloadLen = len - 12;

      // 4. Read Payload into Struct
      if(payloadLen == sizeof(TelemetryData)) {
        uint8_t* ptr = (uint8_t*)&incomingData;
        for(int i=0; i<payloadLen; i++) {
          ptr[i] = SPI.transfer(0x00);
        }
        packetsReceived++;
        printData();
      } else {
        // Flush incorrect size
        Serial.print("ERROR: Unexpected payload size: ");
        Serial.println(payloadLen);
        for(int i=0; i<payloadLen; i++) SPI.transfer(0x00);
      }
      
      // Consume Checksum
      SPI.transfer(0x00);
    } 
    else {
      // Not a data packet (maybe status frame 0x8B), flush it
      for(int i=0; i < len - 1; i++) SPI.transfer(0x00);
      SPI.transfer(0x00); // Checksum
    }
  } else {
    Serial.println("ERROR: Start delimiter (0x7E) not found");
  }

  digitalWrite(CS_PIN, HIGH);
  SPI.endTransaction();
}

void printData() {
  Serial.print("Rx Packet #"); Serial.print(incomingData.record_number);
  Serial.print(" | Alt: "); Serial.print(incomingData.altitude); Serial.print("m");
  Serial.print(" | Vel: "); Serial.print(incomingData.velocity); Serial.print("m/s");
  Serial.print(" | AccZ: "); Serial.print(incomingData.az); Serial.print("g");
  Serial.print(" | Battery: "); Serial.print(incomingData.battery_voltage); Serial.print("V");
  Serial.print(" | Total Rx: "); Serial.println(packetsReceived);
}
