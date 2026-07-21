/*
 * XBee Pro 900HP - SPI Sender (ESP32)
 * 
 * Transmits binary telemetry packets at 10Hz via SPI to XBee
 * Sends 36-byte struct containing rocket telemetry data
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
 * - ID = 7777 (Network ID)
 * - HP = 0 (Preamble ID)
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
// XBee S3B SPI Mode (Mode 0, MSB First)
// Max speed: 3.5 MHz, using 1 MHz for stability
SPISettings xbeeSPI(1000000, MSBFIRST, SPI_MODE0);

// --- DATA STRUCTURE ---
// 36 Bytes Total - must match receiver exactly
struct __attribute__((packed)) TelemetryData {
    uint32_t record_number;
    uint8_t operation_mode;
    uint8_t state;
    float ax, ay, az;
    float altitude;
    float velocity;
    float battery_voltage;
};

TelemetryData packet;
unsigned long lastTxTime = 0;

void setup() {
  Serial.begin(115200);
  
  // Setup Pins
  pinMode(CS_PIN, OUTPUT);
  digitalWrite(CS_PIN, HIGH); // Start Deselected
  pinMode(ATTN_PIN, INPUT);   // XBee Input
  
  // Start SPI with your specific pins
  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, CS_PIN);
  
  delay(1000);
  Serial.println("--- ESP32 SPI Sender Started ---");
  Serial.println("Sending packets at 10Hz (100ms interval)");
}

void loop() {
  // Send every 100ms (10Hz)
  if (millis() - lastTxTime > 100) {
    lastTxTime = millis();
    
    // 1. Update Dummy Data (simulate flight telemetry)
    packet.record_number++;
    packet.operation_mode = 1; // ARMED
    packet.state = 5;          // FLIGHT
    packet.ax = 0.05; 
    packet.ay = 9.81; 
    packet.az = 15.2;          // Simulated high-G
    packet.altitude = 100.0 + (packet.record_number * 0.5);
    packet.velocity = 50.2;
    packet.battery_voltage = 3.8;

    Serial.print("Sending Packet #"); Serial.println(packet.record_number);

    // 2. Transmit via XBee SPI
    sendXBeeFrame((uint8_t*)&packet, sizeof(TelemetryData));
  }
}

void sendXBeeFrame(uint8_t* data, int length) {
  // If XBee is busy (ATTN is Low), skip to prevent errors
  if(digitalRead(ATTN_PIN) == LOW) {
    Serial.println("Warning: XBee busy, skipping packet");
    return;
  }

  int totalLen = 14 + length;
  long checksumTotal = 0;
  
  SPI.beginTransaction(xbeeSPI);
  digitalWrite(CS_PIN, LOW);
  delayMicroseconds(50);
  
  // Header
  SPI.transfer(0x7E);        // Start Delimiter
  SPI.transfer((totalLen >> 8) & 0xFF); // Length MSB
  SPI.transfer(totalLen & 0xFF);        // Length LSB
  
  // Frame Data (Transmit Request 0x10)
  SPI.transfer(0x10); checksumTotal += 0x10;
  SPI.transfer(0x00); checksumTotal += 0x00; // Frame ID 0 (No Ack)
  
  // Destination Address (Broadcast)
  for(int i=0; i<6; i++) { SPI.transfer(0x00); checksumTotal += 0x00; }
  SPI.transfer(0xFF); checksumTotal += 0xFF;
  SPI.transfer(0xFF); checksumTotal += 0xFF;
  SPI.transfer(0xFF); checksumTotal += 0xFF;
  SPI.transfer(0xFE); checksumTotal += 0xFE;
  
  // Options
  SPI.transfer(0x00); checksumTotal += 0x00;
  SPI.transfer(0x00); checksumTotal += 0x00;
  
  // Payload (Binary Struct)
  for (int i = 0; i < length; i++) {
    SPI.transfer(data[i]);
    checksumTotal += data[i];
  }
  
  // Checksum
  SPI.transfer(0xFF - (checksumTotal & 0xFF));
  
  digitalWrite(CS_PIN, HIGH);
  SPI.endTransaction();
}
